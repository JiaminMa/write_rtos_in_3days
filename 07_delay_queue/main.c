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


void task1_entry(void *param)
{
    init_systick(10);
    for(;;) {
        printk("%s\n", __func__);
        task_delay_s(1);
    }
}

void task2_entry(void *param)
{

    for(;;) {
        printk("%s\n", __func__);
        task_delay_s(2);
    }
}

task_t task1;
task_t task2;
task_stack_t task1_stk[1024];
task_stack_t task2_stk[1024];

int main()
{

    clear_bss();

    DEBUG("Hello RTOS C03_Delay_List\n");

    DEBUG("psp:0x%x\n", get_psp());
    DEBUG("msp:0x%x\n", get_msp());

    task_init(&task1, task1_entry, (void *)0x11111111, 0, &task1_stk[1024]);
    task_init(&task2, task2_entry, (void *)0x22222222, 1, &task2_stk[1024]);

    init_task_module();

    for(;;);
    return 0;
}
