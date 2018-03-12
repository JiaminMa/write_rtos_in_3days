#include "memblock.h"


void mem_block_init(mem_block_t *mem_block, uint8_t *start, uint32_t block_size, uint32_t block_cnt)
{
    uint8_t *mem_block_start = (uint8_t *)start;
    uint8_t *mem_block_end = mem_block_start + block_size * block_cnt;

    if (block_size < sizeof(list_node_t)) {
        goto cleanup;
    }

    event_init(&mem_block->event, EVENT_TYPE_MEM_BLOCK);

    mem_block->start = start;
    mem_block->block_size = block_size;
    mem_block->max_count = block_cnt;
    list_init(&mem_block->block_list);


    while (mem_block_start < mem_block_end) {
        list_node_init((list_node_t *)mem_block_start);
        list_append_last(&mem_block->block_list, (list_node_t *)mem_block_start);
        mem_block_start += block_size;
    }
cleanup:
    return;

}

uint32_t mem_block_alloc(mem_block_t *mem_block, uint8_t **mem, uint32_t wait_ticks)
{
    uint32_t status = task_enter_critical();

    if (list_count(&mem_block->block_list) > 0) {
        *mem = (uint8_t *)list_remove_first(&mem_block->block_list);
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        event_wait(&mem_block->event, g_current_task, (void *)0, EVENT_TYPE_MEM_BLOCK, wait_ticks);
        task_exit_critical(status);
        task_sched();
        *mem = g_current_task->event_msg;
        return g_current_task->wait_event_result;
    }
}

uint32_t mem_block_alloc_no_wait(mem_block_t *mem_block, uint8_t **mem)
{
    uint32_t status = task_enter_critical();

    if (list_count(&mem_block->block_list) > 0) {
        *mem = (uint8_t *)list_remove_first(&mem_block->block_list);
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        task_exit_critical(status);
        return ERROR_RESOURCE_FULL;
    }

}

void mem_block_free(mem_block_t *mem_block, uint8_t *mem)
{
    uint32_t status = task_enter_critical();
    if (event_wait_count(&mem_block->event) > 0) {
        task_t *task = event_wakeup(&mem_block->event, (void *)mem, NO_ERROR);
        if (task->prio > g_current_task->prio) {
            task_sched();
        }
    } else {
        list_append_last(&mem_block->block_list, (list_node_t *)mem);
    }

    task_exit_critical(status);
}

void mem_block_get_info(mem_block_t *mem_block, mem_block_info_t *info)
{
    uint32_t status = task_enter_critical();

    info->count = list_count(&mem_block->block_list);
    info->max_count = mem_block->max_count;
    info->block_size = mem_block->block_size;
    info->task_count = event_wait_count(&mem_block->event);
    task_exit_critical(status);
}
uint32_t mem_block_destory(mem_block_t *mem_block)
{
    uint32_t status = task_enter_critical();

    uint32_t count = event_remove_all(&mem_block->event, (void *)0, ERROR_DEL);

    task_exit_critical(status);

    if (count > 0) {
        task_sched();
    }
    return count;
}

