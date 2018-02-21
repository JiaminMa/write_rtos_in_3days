#include "task.h"
#include "cm3.h"
#include "os_stdio.h"

task_t *g_current_task;
task_t *g_next_task;
task_t *g_task_table[2];
static task_t g_idle_task_obj;
static task_t *g_idle_task;
static task_stack_t g_idle_task_stk[1024];

static void idle_task_entry(void *param)
{
    for(;;);
}

void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t * stack)
{
    DEBUG("%s\n", __func__);
    *(--stack) = (uint32_t) (1 << 24);          //XPSR, Thumb Mode
    *(--stack) = (uint32_t) entry;              //PC
    *(--stack) = (uint32_t) 0x14;               //LR
    *(--stack) = (uint32_t) 0x12;               //R12
    *(--stack) = (uint32_t) 0x3;                //R3
    *(--stack) = (uint32_t) 0x2;                //R2
    *(--stack) = (uint32_t) 0x1;                //R1
    *(--stack) = (uint32_t) param;              //R0
    *(--stack) = (uint32_t) 0x11;               //R11
    *(--stack) = (uint32_t) 0x10;               //R10
    *(--stack) = (uint32_t) 0x9;                //R9
    *(--stack) = (uint32_t) 0x8;                //R8
    *(--stack) = (uint32_t) 0x7;                //R7
    *(--stack) = (uint32_t) 0x6;                //R6
    *(--stack) = (uint32_t) 0x5;                //R5
    *(--stack) = (uint32_t) 0x4;                //R4

    task->stack = stack;
    task->delay_ticks = 0;
}

void task_sched()
{

    if (g_current_task == g_idle_task) {
        if (g_task_table[0]->delay_ticks == 0) {
            g_next_task = g_task_table[0];
        } else if (g_task_table[1]->delay_ticks == 0) {
            g_next_task = g_task_table[1];
        } else {
            goto no_need_sched;
        }
    } else {
        if (g_current_task == g_task_table[0]) {
            if (g_task_table[1]->delay_ticks == 0) {
                g_next_task = g_task_table[1];
            } else if (g_current_task->delay_ticks != 0) {
                g_next_task = g_idle_task;
            } else {
                goto no_need_sched;
            }
        } else if (g_current_task == g_task_table[1]) {
            if (g_task_table[0]->delay_ticks == 0) {
                g_next_task = g_task_table[0];
            } else if (g_current_task->delay_ticks != 0) {
                g_next_task = g_idle_task;
            } else {
                goto no_need_sched;
            }
        }
    }


    task_switch();

no_need_sched:
    return;
}

void task_switch()
{
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
}

void task_run_first()
{
    DEBUG("%s\n", __func__);
    set_psp(0);
    MEM8(NVIC_SYSPRI2) = NVIC_PENDSV_PRI;
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
}

void task_delay(uint32_t ticks)
{
    g_current_task->delay_ticks = ticks;
    task_sched();
}

void task_system_tick_handler(void)
{
    uint32_t i;
    for (i = 0; i < 2; i++) {
        if (g_task_table[i]->delay_ticks > 0) {
            g_task_table[i]->delay_ticks--;
        }
    }

    task_sched();
}

void init_task_module()
{
    task_init(&g_idle_task_obj, idle_task_entry, (void *)0, &g_idle_task_stk[1024]);
    g_idle_task = &g_idle_task_obj;

}


uint32_t task_enter_critical(void)
{
    uint32_t ret = get_primask();
    disable_irq();
    return ret;
}

void task_exit_critical(uint32_t status)
{
    set_primask(status);
    enable_irq();
}
