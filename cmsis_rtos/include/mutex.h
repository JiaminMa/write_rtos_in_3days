#ifndef MUTEX_H
#define MUTEX_H

#include "event.h"
#include "task.h"

typedef struct mutex_tag {

    event_t event;
    uint32_t lock_count;
    task_t *owner;
    uint32_t owner_original_prio;
}mutex_t;


typedef struct mutex_info_tag {
    uint32_t task_count;
    uint32_t owner_prio;
    uint32_t inhe_prio;
    task_t *owner;
    uint32_t locked_count;
}mutex_info_t;

extern void mutex_init(mutex_t *mutex);
extern uint32_t mutex_lock_no_wait(mutex_t *mutex);
extern uint32_t mutex_lock(mutex_t *mutex, uint32_t wait_ticks);
extern uint32_t mutex_unlock(mutex_t *mutex);
extern uint32_t mutex_destory(mutex_t *mutex);
extern void mutex_get_info(mutex_t *mutex, mutex_info_t *info);
#endif /*MUTEX_H*/
