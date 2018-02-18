#ifndef CM3_H
#define CM3_H
#include <stdint.h>

#define SCS_BASE            (0xE000E000)                 /*System Control Space Base Address */
#define SYSTICK_BASE        (SCS_BASE +  0x0010)         /*SysTick Base Address*/
#define SCB_BASE            (SCS_BASE +  0x0D00)
#define HSI_CLK             12000000UL
#define SYSTICK_PRIO_REG    (0xE000ED23)

typedef struct systick_tag {
    volatile uint32_t ctrl;
    volatile uint32_t load;
    volatile uint32_t val;
    volatile uint32_t calrb;
}systick_t;

extern uint32_t get_psp(void);
extern uint32_t get_msp(void);
extern uint32_t get_control_reg(void);

extern void init_systick(void);
#endif /*CM3_H*/
