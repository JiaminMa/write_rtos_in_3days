#ifndef MEM_BLOCK_H
#define MEM_BLOCK_H

#include "event.h"
#include "os.h"
#include "task.h"
#include "config.h"
#include <stdint.h>
#include "lib.h"

typedef struct mem_block_tag {
    event_t event;
    void *start;
    uint32_t block_size;
    uint32_t max_count;
    list_t block_list;
}mem_block_t;

typedef struct mem_block_info_tag {
    uint32_t count;
    uint32_t max_count;
    uint32_t block_size;
    uint32_t task_count;
}mem_block_info_t;

extern void mem_block_init(mem_block_t *mem_block, uint8_t *start, uint32_t size, uint32_t block_cnt);
extern uint32_t mem_block_alloc(mem_block_t *mem_block, uint8_t **mem, uint32_t wait_ticks);
extern uint32_t mem_block_alloc_no_wait(mem_block_t *mem_block, uint8_t **mem);
extern void mem_block_free(mem_block_t *mem_block, uint8_t *mem);
extern void mem_block_get_info(mem_block_t *mem_block, mem_block_info_t *info);
extern uint32_t mem_block_destory(mem_block_t *mem_block);

#endif /*MEM_BLOCK_H*/
