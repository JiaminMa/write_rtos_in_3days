#ifndef SEM_H
#define SEM_H

#include "event.h"
#include "config.h"

typedef struct sem_tag {

    event_t event;
    uint32_t count;
    uint32_t max_count;
}sem_t;

typedef struct sem_info_tag {
    uint32_t count;
    uint32_t max_count;
    uint32_t task_count;
}sem_info_t;

extern void sem_init(sem_t *sem, uint32_t count, uint32_t max_count);
extern uint32_t sem_acquire(sem_t *sem, uint32_t wait_ticks);
extern uint32_t sem_acquire_no_wait(sem_t *sem);
extern void sem_release(sem_t *sem);
extern void sem_get_info(sem_t *sem, sem_info_t *info);
extern uint32_t sem_destory(sem_t *sem);

#endif /*SEM_H*/
