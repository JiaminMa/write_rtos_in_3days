#include "os_stdio.h"
#include <stdint.h>
#include "cm3.h"
#include "task.h"
#include "event.h"

extern uint32_t _bss;
extern uint32_t _ebss;

static inline void clear_bss(void)
{
    uint8_t *start = (uint8_t *)_bss;
    while ((uint32_t)start < _ebss) {
        *start = 0;
        start++;
    }
}

task_t task1;
task_t task2;
task_t task3;
task_t task4;
task_stack_t task1_stk[1024];
task_stack_t task2_stk[1024];
task_stack_t task3_stk[1024];
task_stack_t task4_stk[1024];

event_t event_wait_timeout;
event_t event_wait_normal;

void task1_cleanup_func(void *param)
{
    printk("%s\n", __func__);
}

void task1_entry(void *param)
{
    init_systick(10);
    event_init(&event_wait_timeout, EVENT_TYPE_UNKNOWN);

    for(;;) {
        printk("%s\n", __func__);
        uint32_t wakeup_count = event_remove_all(&event_wait_normal, (void *)0, 0);
        if (wakeup_count > 0) {
            task_sched();
            printk("wakeup_count:%d\n", wakeup_count);
            printk("count:%d\n", event_wait_count(&event_wait_normal));
        }
        task_sched();
        task_delay_s(1);
    }
}

void delay(uint32_t delay)
{
    while(delay--);
}

void task2_entry(void *param)
{
    for(;;) {

        event_wait(&event_wait_normal, g_current_task, (void *)0, 0, 0);
        task_sched();
        printk("%s\n", __func__);
        task_delay_s(1);
    }
}

void task3_entry(void *param)
{
    event_init(&event_wait_normal, EVENT_TYPE_UNKNOWN);
    for(;;) {
        event_wait(&event_wait_normal, g_current_task, (void *)0, 0, 0);
        task_sched();
        printk("%s\n", __func__);
        task_delay_s(1);
    }
}

void task4_entry(void *param)
{
    for(;;) {
        printk("%s\n", __func__);
        event_wakeup(&event_wait_normal, (void *)0, 0);
        task_delay_s(1);
        task_sched();
    }
}

int main()
{

    clear_bss();

    DEBUG("Hello RTOS C03_Delay_List\n");

    DEBUG("psp:0x%x\n", get_psp());
    DEBUG("msp:0x%x\n", get_msp());

    init_task_module();

    task_init(&task1, task1_entry, (void *)0x11111111, 0, &task1_stk[1024]);
    task_init(&task2, task2_entry, (void *)0x22222222, 1, &task2_stk[1024]);
    task_init(&task3, task3_entry, (void *)0x33333333, 0, &task3_stk[1024]);
    //task_init(&task4, task4_entry, (void *)0x44444444, 1, &task4_stk[1024]);
    g_next_task = task_highest_ready();
    task_run_first();

    for(;;);
    return 0;
}
