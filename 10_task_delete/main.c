#include "os_stdio.h"
#include <stdint.h>
#include "cm3.h"
#include "task.h"

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

void task1_cleanup_func(void *param)
{
    printk("%s\n", __func__);
}

void task1_entry(void *param)
{
    init_systick(10);

    task_set_clean_callbk(g_current_task, task1_cleanup_func, (void *)0);
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);
    }
}

void delay(uint32_t delay)
{
    while(delay--);
}

void task2_entry(void *param)
{
    uint32_t task_del = 0;
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);

        if (!task_del) {
            task_force_delete(&task1);
            task_del = 1;
        }
    }
}

void task3_entry(void *param)
{
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);
        if (is_task_request_delete()) {
            task_delete_self();
        }

        task_delay_s(1);
    }
}

void task4_entry(void *param)
{
    uint32_t task3_del = 0;
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);
        if (!task3_del) {
            task_request_delete(&task3);
            task3_del = 1;
        }
        task_delay_s(1);
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
    task_init(&task4, task4_entry, (void *)0x44444444, 1, &task4_stk[1024]);

    g_next_task = task_highest_ready();
    task_run_first();

    for(;;);
    return 0;
}
