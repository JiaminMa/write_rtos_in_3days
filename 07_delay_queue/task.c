#include "task.h"
#include "cm3.h"
#include "os_stdio.h"
#include "lib.h"
#include "config.h"

task_t *g_current_task;
task_t *g_next_task;
task_t *g_task_table[OS_PRIO_COUNT];
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
    *(--stack) = (uint32_t) (1 << 24);          //XPSR, Thumb Mode
    *(--stack) = (uint32_t) entry;              //PC
    *(--stack) = (uint32_t) 0x14;               //LR
    *(--stack) = (uint32_t) 0x12;               //R12
    *(--stack) = (uint32_t) 0x3;                //R3
    *(--stack) = (uint32_t) 0x2;                //R2
    *(--stack) = (uint32_t) 0x1;                //R1
    *(--stack) = (uint32_t) param;              //R0
    *(--stack) = (uint32_t) 0x11;               //R11
    *(--stack) = (uint32_t) 0x10;               //R10
    *(--stack) = (uint32_t) 0x9;                //R9
    *(--stack) = (uint32_t) 0x8;                //R8
    *(--stack) = (uint32_t) 0x7;                //R7
    *(--stack) = (uint32_t) 0x6;                //R6
    *(--stack) = (uint32_t) 0x5;                //R5
    *(--stack) = (uint32_t) 0x4;                //R4

    task->stack = stack;
    task->delay_ticks = 0;
    task->prio = prio;

    task->state = OS_TASK_STATE_RDY;
    list_node_init(&task->delay_node);

    g_task_table[prio] = task;
    bitmap_set(&g_task_prio_bitmap, prio);
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
            /*
             *  1.Remove the task from delay list
             *  2. Add the task to task table
             *  3.Set the prio bit to bitmap
             * */
            task_delay_wakeup(task);
            task_ready(task);
        }
    }

    task_exit_critical(status);
    task_sched();
}

void init_task_module()
{
    task_init(&g_idle_task_obj, idle_task_entry, (void *)0, OS_PRIO_COUNT - 1,  &g_idle_task_stk[1024]);
    g_idle_task = &g_idle_task_obj;

    g_sched_lock = 0;
    list_init(&g_task_delay_list);

    g_next_task = task_highest_ready();
    task_run_first();
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
    uint32_t highest_prio = bitmap_get_first_set(&g_task_prio_bitmap);
    return g_task_table[highest_prio];
}

void task_ready(task_t *task)
{
    g_task_table[task->prio] = task;
    bitmap_set(&g_task_prio_bitmap, task->prio);
}

void task_unready(task_t *task)
{
    g_task_table[task->prio] = (task_t *)NULL;
    bitmap_clear(&g_task_prio_bitmap, task->prio);
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
