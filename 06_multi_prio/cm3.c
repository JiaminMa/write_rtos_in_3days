#include "cm3.h"
#include "os_stdio.h"
#include "task.h"

void init_systick(uint32_t ms)
{
    systick_t *systick_p = (systick_t *)SYSTICK_BASE;
    uint8_t *sys_prio_p = (uint8_t *)SYSTICK_PRIO_REG;
    *sys_prio_p = 0xf0;
    systick_p->load = ms * (SystemCoreClock / 1000) - 1;
    systick_p->val = 0;
    systick_p->ctrl = 0x7;
}

void systick_handler(void)
{
    /*DEBUG("systick_handler\n");*/
    task_system_tick_handler();
}

void trigger_pend_sv(void)
{
    MEM8(NVIC_SYSPRI2) = NVIC_PENDSV_PRI; /*Set PENDSVC loweset priority*/
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET; /*Trigger PendSV*/
}
