#include "timer.h"
#include "config.h"
#include "sem.h"

static list_t g_timer_hard_list;
static list_t g_timer_soft_list;
static sem_t g_timer_protect_sem;
static sem_t g_timer_tick_sem;

void timer_init(timer_t *timer, uint32_t delay_ticks, uint32_t duration_ticks,
        void(*timer_func)(void *arg), void *arg, uint32_t config)
{
    list_node_init(&timer->link_node);
    timer->start_delay_ticks = delay_ticks;
    timer->duration_ticks  = duration_ticks;
    timer->timer_func = timer_func;
    timer->arg = arg;
    timer->config = config;

    if (delay_ticks == 0) {
        timer->delay_ticks = duration_ticks;
    } else {
        timer->delay_ticks = timer->start_delay_ticks;
    }

    timer->state = TIMER_CREATED;
}

static task_t g_timer_task;
static task_stack_t g_timer_task_stack[OS_TIMERTASK_STACK_SIZE];


void timer_start(timer_t *timer)
{
    switch(timer->state) {
    case TIMER_CREATED:
    case TIMER_STOPPED:
        timer->delay_ticks = timer->start_delay_ticks ? timer->start_delay_ticks : timer->duration_ticks;
        timer->state = TIMER_STARTED;

        if (timer->config & TIMER_CONFIG_TYPE_HARD) {
            uint32_t status = task_enter_critical();
            list_append_last(&g_timer_hard_list, &timer->link_node);
            task_exit_critical(status);
        } else {
            sem_acquire(&g_timer_protect_sem, 0);
            list_append_last(&g_timer_soft_list, &timer->link_node);
            sem_release(&g_timer_protect_sem);
        }
        break;
    default:
        break;
    }
}

void timer_stop(timer_t *timer)
{
    switch(timer->state) {
    case TIMER_STARTED:
    case TIMER_RUNNING:
        if (timer->config & TIMER_CONFIG_TYPE_HARD) {
            uint32_t status = task_enter_critical();
            list_remove(&g_timer_hard_list, &timer->link_node);
            task_exit_critical(status);
        } else {
            sem_acquire(&g_timer_protect_sem, 0);
            list_remove(&g_timer_soft_list, &timer->link_node);
            sem_release(&g_timer_protect_sem);
        }
        timer->state = TIMER_STOPPED;
        break;
    default:
        break;
    }
}

static void timer_call_func_list(list_t *timer_list)
{
    list_node_t *node;
    timer_t *timer;

    for (node = timer_list->head.next; node != &(timer_list->head); node = node->next) {
        timer = container_of(node, timer_t, link_node);
        if ((timer->delay_ticks == 0) || (--timer->delay_ticks == 0)) {
            timer->state = TIMER_RUNNING;
            timer->timer_func(timer->arg);
            timer->state = TIMER_STARTED;
            if (timer->duration_ticks > 0) {
                timer->delay_ticks = timer->duration_ticks;
            } else {
                timer_stop(timer);
            }
        }
    }
}

void timer_destory(timer_t *timer)
{
    timer_stop(timer);
    timer->state = TIMER_DESTORYED;
}

static void timer_soft_task(const void *param)
{
    for(;;) {
        sem_acquire(&g_timer_tick_sem, 0);
        sem_acquire(&g_timer_protect_sem, 0);

        timer_call_func_list(&g_timer_soft_list);
        sem_release(&g_timer_protect_sem);
    }
}

void timer_module_tick_notify(void)
{
    uint32_t status = task_enter_critical();

    timer_call_func_list(&g_timer_hard_list);
    task_exit_critical(status);
    sem_release(&g_timer_tick_sem);
}

void timer_module_init()
{
    list_init(&g_timer_hard_list);
    list_init(&g_timer_soft_list);
    sem_init(&g_timer_protect_sem, 1, 1);
    sem_init(&g_timer_tick_sem, 0, 0);

    task_init(&g_timer_task, &timer_soft_task, (void *)0,
            OS_TIMERTASK_PRIO, &g_timer_task_stack[OS_TIMERTASK_STACK_SIZE], &g_timer_task_stack[0],
            OS_TIMERTASK_STACK_SIZE);

}
