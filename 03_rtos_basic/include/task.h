#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef uint32_t task_stack_t;
/*Task Control block*/
typedef struct task_tag {

    task_stack_t *stack;
}task_t;

extern task_t *g_current_task;
extern task_t *g_next_task;
extern task_t *g_task_table[2];

extern void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t * stack);
extern void task_sched(void);
extern void task_switch(void);
extern void task_run_first(void);

#endif /*TASK_H*/
