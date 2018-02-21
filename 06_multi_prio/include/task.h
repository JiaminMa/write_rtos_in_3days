#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "config.h"

typedef uint32_t task_stack_t;
/*Task Control block*/
typedef struct task_tag {

    task_stack_t *stack;
    uint32_t delay_ticks;
    uint32_t prio;
}task_t;

extern task_t *g_current_task;
extern task_t *g_next_task;
extern task_t *g_task_table[OS_PRIO_COUNT];

extern void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t prio, uint32_t * stack);
extern void task_sched(void);
extern void task_switch(void);
extern void task_run_first(void);
extern void task_delay(uint32_t ticks);
extern void task_delay_s(uint32_t seconds);
extern void task_system_tick_handler(void);
extern void init_task_module(void);
extern uint32_t task_enter_critical(void);
extern void task_exit_critical(uint32_t status);
extern void task_sched_disable(void);
extern void task_sched_enable(void);
extern task_t *task_highest_ready(void);

#endif /*TASK_H*/
