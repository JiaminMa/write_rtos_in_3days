#include "event.h"
#include "os.h"

void event_init(event_t *event, event_type_e type)
{
    event->type = type;
    list_init(&event->wait_list);
}

void event_wait(event_t *event, task_t *task, void *msg, uint32_t state, uint32_t timeout)
{
    uint32_t status = task_enter_critical();

    task->state |= state;
    task->wait_event = event;
    task->wait_event_result = NO_ERROR;

    task_unready(task);
    list_append_last(&event->wait_list, &task->prio_list_node);
    if (timeout != 0) {
        task_delay_wait(task, timeout);
    }

    task_exit_critical(status);
}

task_t *event_wakeup(event_t *event, void *msg, uint32_t result)
{
    list_node_t *node = (list_node_t *)NULL;
    task_t *task = (task_t *)NULL;

    uint32_t status = task_enter_critical();

    if ((node = list_remove_first(&event->wait_list)) != (list_node_t *)NULL) {
        task = (task_t *)container_of(node, task_t, prio_list_node);
        task->wait_event = (event_t *)NULL;
        task->event_msg = msg;
        task->state &= ~OS_TASK_WAIT_MASK;

        if (task->delay_ticks != 0) {
            task_delay_wakeup(task);
        }
        task_ready(task);
    }

    task_exit_critical(status);
    return task;
}

void event_remove_task(task_t *task, void *msg, uint32_t result)
{
    uint32_t status = task_enter_critical();

    list_remove(&task->wait_event->wait_list, &task->prio_list_node);
    task->wait_event = (event_t *)NULL;
    task->event_msg = msg;
    task->wait_event_result = result;
    task->state &= ~OS_TASK_WAIT_MASK;

    task_exit_critical(status);
}

uint32_t event_remove_all(event_t *event, void *msg, uint32_t result)
{
    list_node_t *node = (list_node_t *)NULL;
    uint32_t count = 0;
    uint32_t status = task_enter_critical();

    DEBUG("%s:\n", __func__);
    count = list_count(&event->wait_list);
    while ((node = list_remove_first(&event->wait_list)) != (list_node_t *)NULL) {
        DEBUG("########\n");
        task_t *task = (task_t *)container_of(node, task_t, prio_list_node);
        task->wait_event = (event_t *)NULL;
        task->event_msg = msg;
        task->wait_event_result = result;
        task->state &= ~OS_TASK_WAIT_MASK;
        if (task->delay_ticks != 0) {
            task_delay_wakeup(task);
        }
        task_ready(task);
    }

    task_exit_critical(status);
    return count;
}

uint32_t event_wait_count(event_t *event)
{
    uint32_t count = 0;
    uint32_t status = task_enter_critical();

    count = list_count(&event->wait_list);

    task_exit_critical(status);
    return count;
}
