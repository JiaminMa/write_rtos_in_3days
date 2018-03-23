#include <stdint.h>
#include "cmsis_rv.h"
#include "RV_Framework.h"
#include "RV_Report.h"
#include "RV_Typedefs.h"

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

int main()
{

    clear_bss();

    for(;;);
    return 0;
}
