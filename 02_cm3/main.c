#include "os_stdio.h"
#include <stdint.h>
#include "cm3.h"

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

void systick_handler(void)
{
    DEBUG("systick_handler\n");
}

int main()
{

    systick_t *systick_p = (systick_t *)SYSTICK_BASE;
    clear_bss();

    DEBUG("Hello RTOS\n");
    DEBUG("psp:0x%x\n", get_psp());
    DEBUG("msp:0x%x\n", get_msp());

    init_systick();
    while(1) {
    }
    return 0;
}
