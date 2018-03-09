#include "os_stdio.h"
#include <stdint.h>
#include "cm3.h"
#include "task.h"
#include "os.h"
#include "memblock.h"
#include "timer.h"

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

timer_t timer1;
timer_t timer2;
timer_t timer3;

static void timer_func1(void *arg)
{
    printk("timer_func1\n");
}

static void timer_func2(void *arg)
{
    printk("timer_func2\n");
}

static void timer_func3(void *arg)
{
    printk("timer_func3\n");
}

/*20 block of size 100 bytes*/
uint8_t mem1[20][100];
mem_block_t mem_block1;
typedef uint8_t (*block_t)[100];

void task1_entry(void *param)
{
    uint32_t stopped = 0;
    init_systick(10);

    timer_init(&timer1, 100, 100, timer_func1, (void *)0, TIMER_CONFIG_TYPE_HARD);
    timer_start(&timer1);
    timer_init(&timer2, 200, 200, timer_func2, (void *)0, TIMER_CONFIG_TYPE_SOFT);
    timer_start(&timer2);

    timer_init(&timer3, 300, 0, timer_func3, (void *)0, TIMER_CONFIG_TYPE_HARD);
    timer_start(&timer3);

    for(;;) {
        if (stopped == 0) {
            task_delay(1000);
            timer_stop(&timer2);
            task_delay(1000);
            timer_start(&timer2);
            timer_destory(&timer1);
            task_delay(1000);
            timer_destory(&timer2);
            stopped = 1;
        }
    }
}

void task2_entry(void *param)
{
    for(;;) {
        block_t block;
        mem_block_alloc(&mem_block1, (uint8_t **)&block, 0);
        printk("%s:%x\n", __func__, block);
    }
}

void task3_entry(void *param)
{
    for(;;) {
        printk("%s\n", __func__);
        task_delay_s(1);
    }

}

void task4_entry(void *param)
{
    for(;;) {
        printk("%s\n", __func__);
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
    timer_module_init();

    task_init(&task1, task1_entry, (void *)0x11111111, 0, &task1_stk[1024]);
#if 0
    task_init(&task2, task2_entry, (void *)0x22222222, 1, &task2_stk[1024]);
    task_init(&task3, task3_entry, (void *)0x33333333, 0, &task3_stk[1024]);
    task_init(&task4, task4_entry, (void *)0x44444444, 1, &task4_stk[1024]);
#endif
    g_next_task = task_highest_ready();
    task_run_first();

    for(;;);
    return 0;
}
