#include "flag_group.h"
#include "os.h"
#include "os_stdio.h"

void flag_group_init(flag_group_t *flag_group, uint32_t flags)
{
    event_init(&flag_group->event, EVENT_TYPE_FLAG_GROUP);
    flag_group->flag = flags;
}

static uint32_t flag_group_check_and_consume(flag_group_t *flag_group,
                    uint32_t type, uint32_t *flag)
{
    uint32_t src_flag = *flag;
    uint32_t is_set = type & FLAGGROUP_SET;
    uint32_t is_all = type & FLAGGROUP_ALL;
    uint32_t is_consume = type & FLAGGROUP_CONSUME;

    uint32_t cal_flag = is_set ? (flag_group->flag & src_flag) : (~flag_group->flag & src_flag);

    if (((is_all != 0) && (cal_flag == src_flag)) || ((is_all == 0) && (cal_flag != 0))){
        if (is_consume){
            if (is_set) {
                flag_group->flag &= ~src_flag;
            } else {
                flag_group->flag |= src_flag;
            }
        }

        *flag = cal_flag;
        return NO_ERROR;
    }

    *flag = cal_flag;
    return ERROR_RESOURCE_FULL;
}

uint32_t flag_group_wait(flag_group_t *flag_group, uint32_t wait_type,
            uint32_t request_flag, uint32_t *result_flag, uint32_t wait_ticks)
{
    uint32_t result;
    uint32_t flags = request_flag;
    uint32_t status = task_enter_critical();

    result = flag_group_check_and_consume(flag_group, wait_type, &flags);

    if (result != NO_ERROR) {
        g_current_task->wait_flag_type = wait_type;
        g_current_task->event_flag = request_flag;
        event_wait(&flag_group->event, g_current_task, (void *)0, EVENT_TYPE_FLAG_GROUP, wait_ticks);
        task_exit_critical(status);

        task_sched();

        *result_flag = g_current_task->event_flag;
    } else {
        *result_flag = flags;
        task_exit_critical(status);
    }

    return result;
}

uint32_t flag_group_no_wait_get(flag_group_t *flag_group, uint32_t wait_type, uint32_t request_flag,
                            uint32_t *result_flag)
{

    uint32_t flags = request_flag;
    uint32_t result;

    uint32_t status = task_enter_critical();
    result = flag_group_check_and_consume(flag_group, wait_type, &request_flag);
    task_exit_critical(status);

    *result_flag = flags;
    return result;
}

void flag_group_notify(flag_group_t *flag_group, uint8_t is_set, uint32_t flag)
{
    list_t *wait_list;
    list_node_t *node;
    list_node_t *next_node;
    uint32_t status = task_enter_critical();
    uint8_t sched = 0;
    uint32_t result;

    if (is_set) {
        flag_group->flag |= flag;
    } else {
        flag_group->flag &= ~flag;
    }

    wait_list = &flag_group->event.wait_list;
    for (node = wait_list->head.next; node != &(wait_list->head); node = next_node) {
        task_t *task = container_of(node, task_t, prio_list_node);
        uint32_t flags = task->event_flag;
        next_node = node->next;
        result = flag_group_check_and_consume(flag_group, task->wait_flag_type, &flags);
        if (result == NO_ERROR) {
            task->event_flag = flags;
            event_wakeup_task(&flag_group->event, task, (void *)0, NO_ERROR);
            sched = 1;
        }
    }

    if (sched) {
        task_sched();
    }

    task_exit_critical(status);
}

void flag_group_get_info(flag_group_t *flag_group, flag_group_info_t *info)
{
    uint32_t status = task_enter_critical();

    info->flags = flag_group->flag;
    info->task_count = event_wait_count(&flag_group->event);

    task_exit_critical(status);
}

uint32_t flag_group_destory(flag_group_t *flag_group)
{
    uint32_t status = task_enter_critical();
    uint32_t count = event_remove_all(&flag_group->event, (void *)0, ERROR_DEL); 
    task_exit_critical(status);

    if (count > 0) {
        task_sched();
    }
    return count;
}
