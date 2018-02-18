#include "cm3.h"

void init_systick()
{
    systick_t *systick_p = (systick_t *)SYSTICK_BASE;
    uint8_t *sys_prio_p = (uint8_t *)SYSTICK_PRIO_REG;
    *sys_prio_p = 0xf0;
    systick_p->load = (HSI_CLK & 0xffffffUL) - 1;
    systick_p->val = 0;
    systick_p->ctrl = 0x7;
}

