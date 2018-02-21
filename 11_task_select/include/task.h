#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "config.h"
#include "lib.h"

#define OS_TASK_STATE_RDY                   0
#define OS_TASK_STATE_DELAYED               (1 << 1)
#define OS_TASK_STATE_SUSPEND               (1 << 2)
typedef uint32_t task_stack_t;
/*Task Control block*/
typedef struct task_tag {

    task_stack_t *stack;
    uint32_t delay_ticks;
    uint32_t prio;

    /*Delay list*/
    list_node_t delay_node;
    uint32_t state;

    /*Same prioity slice scheduling*/
    list_node_t prio_list_node;
    uint32_t slice;

    /*Suspend resume*/
    uint32_t suspend_cnt;

    /*Task delete*/
    void (*clean)(void *param);
    void *clean_param;
    uint8_t request_del_flag;
}task_t;

typedef struct task_info_tag {

    uint32_t delay_ticks;
    uint32_t prio;
    uint32_t state;
    uint32_t slice;
    uint32_t suspend_cnt;
}task_info_t;

extern task_t *g_current_task;
extern task_t *g_next_task;
extern list_t g_task_table[OS_PRIO_COUNT];

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
extern void task_ready(task_t *task);
extern void task_unready(task_t *task);
extern void task_delay_wait(task_t *task, uint32_t ticks);
extern void task_delay_wakeup(task_t *task);
extern void task_suspend(task_t *task);
extern void task_resume(task_t *task);
extern void task_set_clean_callbk(task_t *task, void (*clean)(void *param), void *param);
extern void task_force_delete(task_t *task);
extern void task_request_delete(task_t *task);
extern uint8_t is_task_request_delete(void);
extern void task_delete_self(void);
extern void task_get_info(task_t *task, task_info_t *info);
extern void dump_task_info(task_info_t *info);

#endif /*TASK_H*/
