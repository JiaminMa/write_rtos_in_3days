#ifndef TIMER_H
#define TIMER_H
#include "event.h"
#include "lib.h"

typedef enum timer_state{
    TIMER_CREATED,
    TIMER_STARTED,
    TIMER_RUNNING,
    TIMER_STOPPED,
    TIMER_DESTORYED,
}timer_state_e;

typedef struct timer_tag {

    list_node_t link_node;
    uint32_t duration_ticks;
    uint32_t start_delay_ticks;
    uint32_t delay_ticks;
    void (*timer_func)(void *arg);
    void *arg;
    uint32_t config;

    timer_state_e state;
}timer_t;

#define TIMER_CONFIG_TYPE_HARD      (1 << 0)
#define TIMER_CONFIG_TYPE_SOFT      (0 << 0)

extern void timer_init(timer_t *timer, uint32_t delay_ticks, uint32_t duration_ticks,
        void(*timer_func)(void *arg), void *arg, uint32_t config);
extern void timer_module_init(void);
extern void timer_start(timer_t *timer);
extern void timer_stop(timer_t *timer);
extern void timer_module_tick_notify(void);
extern void timer_destory(timer_t *timer);

#endif /*TIMER_H*/
