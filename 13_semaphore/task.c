#include "task.h"
#include "cm3.h"
#include "os_stdio.h"
#include "lib.h"
#include "config.h"
#include "os.h"
#include "event.h"

task_t *g_current_task;
task_t *g_next_task;
list_t g_task_table[OS_PRIO_COUNT];
static task_t g_idle_task_obj;
static task_t *g_idle_task;
static task_stack_t g_idle_task_stk[1024];
static uint8_t g_sched_lock;
static bitmap_t g_task_prio_bitmap;
static list_t g_task_delay_list;

static void idle_task_entry(void *param)
{
    for(;;);
}

void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t prio, uint32_t * stack)
{
    DEBUG("%s\n", __func__);
    *(--stack) = (uint32_t) (1 << 24);          /*XPSR, Thumb Mode*/
    *(--stack) = (uint32_t) entry;              /*PC*/
    *(--stack) = (uint32_t) 14;                 /*LR*/
    *(--stack) = (uint32_t) 12;                 /*R12*/
    *(--stack) = (uint32_t) 3;                  /*R3*/
    *(--stack) = (uint32_t) 2;                  /*R2*/
    *(--stack) = (uint32_t) 1;                  /*R1*/
    *(--stack) = (uint32_t) param;              /*R0*/
    *(--stack) = (uint32_t) 11;                 /*R11*/
    *(--stack) = (uint32_t) 10;                 /*R10*/
    *(--stack) = (uint32_t) 9;                  /*R9*/
    *(--stack) = (uint32_t) 8;                  /*R8*/
    *(--stack) = (uint32_t) 7;                  /*R7*/
    *(--stack) = (uint32_t) 6;                  /*R6*/
    *(--stack) = (uint32_t) 5;                  /*R5*/
    *(--stack) = (uint32_t) 4;                  /*R4*/

    task->stack = stack;
    /*Delay list*/
    task->delay_ticks = 0;
    task->state = OS_TASK_STATE_RDY;
    list_node_init(&task->delay_node);
    /*Task table and prio bitmap*/
    task->prio = prio;
    task->slice = OS_SLICE_MAX;
    list_node_init(&task->prio_list_node);
    list_append_last(&g_task_table[prio], &(task->prio_list_node));
    bitmap_set(&g_task_prio_bitmap, prio);

    /*suspend-resume*/
    task->suspend_cnt = 0;

    task->clean = (void (*)(void *))NULL;
    task->clean_param = (void *)NULL;
    task->request_del_flag = 0;

    /*Event control block*/
    task->wait_event = (event_t *)NULL;
    task->event_msg = (void *)0;
    task->wait_event_result = NO_ERROR;

}

void task_sched()
{
    uint32_t status = task_enter_critical();
    task_t *temp_task_p;

    if (g_sched_lock > 0) {
        goto no_need_sched;
    }

    temp_task_p = task_highest_ready();
    if (temp_task_p != g_current_task) {
        g_next_task = temp_task_p;
        task_switch();
    }

no_need_sched:
    task_exit_critical(status);
    return;
}

void task_switch()
{
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
}

void task_run_first()
{
    DEBUG("%s\n", __func__);
    set_psp(0);
    MEM8(NVIC_SYSPRI2) = NVIC_PENDSV_PRI;
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
}

void task_delay(uint32_t ticks)
{
    uint32_t status = task_enter_critical();

    /*  1.Add task to delay list
     *  2.Remove the task from task list
     *  3.Clear the task bit from prioity bitmap
     * */
    task_delay_wait(g_current_task, ticks);
    task_unready(g_current_task);

    task_exit_critical(status);
    task_sched();
}

void task_delay_s(uint32_t seconds)
{
    task_delay(seconds * 100);
}

void task_system_tick_handler(void)
{
    uint32_t status = task_enter_critical();
    list_node_t *head = &(g_task_delay_list.head);
    list_node_t *temp_node = head->next;
    task_t *task = (task_t *)NULL;
    /*
     *  For each the delay list, and do:
     *  1. Self sub the node delay ticks
     * */
    while (temp_node != head) {
        task = container_of(temp_node, task_t, delay_node);
        temp_node = temp_node->next;
        if (--task->delay_ticks == 0) {

            if (task->wait_event != (event_t *)NULL) {
                event_remove_task(task, (void *)0, ERROR_TIMEOUT);
            }

            /*
             *  1.Remove the task from delay list
             *  2. Add the task to task table
             *  3.Set the prio bit to bitmap
             * */
            task_delay_wakeup(task);
            task_ready(task);
        }
    }

    /*
     *  check whether the time slice of current task exhausts
     *  if time slice is over, move the task to the prio list tail
     */
    if (--g_current_task->slice == 0) {
        if (list_count(&g_task_table[g_current_task->prio]) > 0) {
            list_remove(&g_task_table[g_current_task->prio], &(g_current_task->prio_list_node));
            list_append_last(&g_task_table[g_current_task->prio], &(g_current_task->prio_list_node));
            g_current_task->slice = OS_SLICE_MAX;
        }
    }

    task_exit_critical(status);
    task_sched();
}

void init_task_module()
{
    g_sched_lock = 0;
    uint32_t i = 0;
    bitmap_init(&g_task_prio_bitmap);
    list_init(&g_task_delay_list);
    for (i = 0; i <OS_PRIO_COUNT; i++) {
        list_init(&g_task_table[i]);
    }

    task_init(&g_idle_task_obj, idle_task_entry, (void *)0, OS_PRIO_COUNT - 1,  &g_idle_task_stk[1024]);
    g_idle_task = &g_idle_task_obj;
}

uint32_t task_enter_critical(void)
{
    uint32_t ret = get_primask();
    disable_irq();
    return ret;
}

void task_exit_critical(uint32_t status)
{
    set_primask(status);
    enable_irq();
}

void task_sched_disable(void)
{
    uint32_t status = task_enter_critical();

    if (g_sched_lock < 255) {
        g_sched_lock++;
    }
    task_exit_critical(status);
}

void task_sched_enable(void)
{
    uint32_t status = task_enter_critical();
    if (g_sched_lock > 0) {
        if (--g_sched_lock == 0) {
            task_sched();
        }
    }
    task_exit_critical(status);
}

task_t *task_highest_ready()
{
    /*
     *              Highest prio task
     *                      |
     * g_task_table[0] -> task -> task -> task;
     * ....
     * g_task_table[31] -> task -> task;
     */
    uint32_t highest_prio = bitmap_get_first_set(&g_task_prio_bitmap);
    list_node_t *node = list_head(&(g_task_table[highest_prio]));
    return container_of(node, task_t, prio_list_node);
}

void task_ready(task_t *task)
{
    list_append_last(&g_task_table[task->prio], &(task->prio_list_node));
    bitmap_set(&g_task_prio_bitmap, task->prio);
}

void task_unready(task_t *task)
{
    list_remove(&g_task_table[task->prio], &(task->prio_list_node));
    if (list_count(&g_task_table[task->prio]) == 0) {
        bitmap_clear(&g_task_prio_bitmap, task->prio);
    }
}

void task_delay_wait(task_t *task, uint32_t ticks)
{
    task->delay_ticks = ticks;
    list_insert_head(&g_task_delay_list, &(task->delay_node));
    task->state |= OS_TASK_STATE_DELAYED;
}

void task_delay_wakeup(task_t *task)
{
    list_remove(&g_task_delay_list, &(task->delay_node));
    task->state &= ~OS_TASK_STATE_DELAYED;
}

void task_suspend(task_t *task)
{
    uint32_t status = task_enter_critical();

    /*Don't suspend the task in delay state*/
    if (!(task->state & OS_TASK_STATE_DELAYED)) {
        /*If the task is first suspend*/
        if (task->suspend_cnt++ <= 0) {
            task->state |= OS_TASK_STATE_SUSPEND;

            task_unready(task);
            if (task == g_current_task) {
                task_sched();
            }
        }

    }

    task_exit_critical(status);
}

extern void task_resume(task_t *task)
{
    uint32_t status = task_enter_critical();

    if (task->state & OS_TASK_STATE_SUSPEND){
        if (--task->suspend_cnt == 0) {
            task->state &= ~OS_TASK_STATE_SUSPEND;
            task_ready(task);
            task_sched();
        }
    }
    task_exit_critical(status);
}

static void task_remove_from_prio_table(task_t *task)
{
    list_remove(&g_task_table[task->prio], &(task->prio_list_node));
    if (list_count(&g_task_table[task->prio]) == 0) {
        bitmap_clear(&g_task_prio_bitmap, task->prio);
    }
}

static void task_remove_from_delay_list(task_t *task)
{
    list_remove(&g_task_delay_list, &(task->delay_node));
}

void task_set_clean_callbk(task_t *task, void (*clean)(void *param), void *param)
{
    task->clean = clean;
    task->clean_param = param;
}

void task_force_delete(task_t *task)
{
    uint32_t status = task_enter_critical();

    if (task->state & OS_TASK_STATE_DELAYED) {
        task_remove_from_delay_list(task);
    } else if (!(task->state & OS_TASK_STATE_SUSPEND)) {
        task_remove_from_prio_table(task);
    }

    if (task->clean) {
        task->clean(task->clean_param);
    }

    if (g_current_task == task) {
        task_sched();
    }

    task_exit_critical(status);
}

void task_request_delete(task_t *task)
{
    uint32_t status = task_enter_critical();

    task->request_del_flag = 1;

    task_exit_critical(status);
}

uint8_t is_task_request_delete()
{
    uint8_t delete;

    uint32_t status = task_enter_critical();
    delete = g_current_task->request_del_flag;
    task_exit_critical(status);

    return delete;
}

void task_delete_self(void)
{
    uint8_t status = task_enter_critical();

    task_remove_from_prio_table(g_current_task);

    if (g_current_task->clean) {
        g_current_task->clean(g_current_task->clean_param);
    }

    task_sched();

    task_exit_critical(status);
}

void task_get_info(task_t *task, task_info_t *info)
{
    uint32_t status = task_enter_critical();

    info->delay_ticks = task->delay_ticks;
    info->prio = task->prio;
    info->state = task->state;
    info->slice = task->slice;
    info->suspend_cnt = task->suspend_cnt;

    task_exit_critical(status);
}

void dump_task_info(task_info_t *info)
{
    printk("Task info:\n");
    printk("delay_ticks:%d\n", info->delay_ticks);
    printk("prio:%d\n", info->prio);
    printk("slice:%d\n", info->slice);
    printk("suspend_cnt:%d\n", info->suspend_cnt);
}
