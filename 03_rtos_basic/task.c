#include "task.h"
#include "cm3.h"
#include "os_stdio.h"

task_t *g_current_task;
task_t *g_next_task;
task_t *g_task_table[2];

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
}

void task_sched()
{
    if (g_current_task == g_task_table[0]) {
        g_next_task = g_task_table[1];
    } else {
        g_next_task = g_task_table[0];
    }

    task_switch();
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
