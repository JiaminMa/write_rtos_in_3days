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

void delay(uint32_t count)
{
    while(--count > 0);
}

void task1_entry(void *param)
{
    for(;;) {
        printk("task1_entry\n");
        delay(65536000);
        task_sched();
    }
}

void task2_entry(void *param)
{
    for(;;) {
        printk("task2_entry\n");
        delay(65536000);
        task_sched();
    }
}

task_t task1;
task_t task2;
task_stack_t task1_stk[1024];
task_stack_t task2_stk[1024];

int main()
{

    systick_t *systick_p = (systick_t *)SYSTICK_BASE;
    clear_bss();

    DEBUG("Hello RTOS\n");
    DEBUG("psp:0x%x\n", get_psp());
    DEBUG("msp:0x%x\n", get_msp());

    task_init(&task1, task1_entry, (void *)0x11111111, &task1_stk[1024]);
    task_init(&task2, task2_entry, (void *)0x22222222, &task2_stk[1024]);

    g_task_table[0] = &task1;
    g_task_table[1] = &task2;
    g_next_task = g_task_table[0];

    task_run_first();

    for(;;);
    return 0;
}
