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

uint32_t test_sync_val = 0;

void task1_entry(void *param)
{
    uint32_t status = 0;
    init_systick(1000);
    for(;;) {
        printk("%s\n", __func__);
        status = task_enter_critical();
        task_delay(1);
        test_sync_val++;
        task_exit_critical(status);
        printk("task1:test_sync_val:%d\n", test_sync_val);
    }
}

void task2_entry(void *param)
{
    uint32_t counter = test_sync_val;
    uint32_t status = 0;

    for(;;) {
        printk("%s\n", __func__);

        status = task_enter_critical();
        counter = test_sync_val;
        task_delay(5);
        test_sync_val = counter +1;
        task_exit_critical(status);

        printk("task2:test_sync_val:%d\n", test_sync_val);
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

    DEBUG("Hello RTOS C02.2\n");
    DEBUG("psp:0x%x\n", get_psp());
    DEBUG("msp:0x%x\n", get_msp());

    task_init(&task1, task1_entry, (void *)0x11111111, &task1_stk[1024]);
    task_init(&task2, task2_entry, (void *)0x22222222, &task2_stk[1024]);

    g_task_table[0] = &task1;
    g_task_table[1] = &task2;
    g_next_task = g_task_table[0];
    init_task_module();

    task_run_first();

    for(;;);
    return 0;
}
