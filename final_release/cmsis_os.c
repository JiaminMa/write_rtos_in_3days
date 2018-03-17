#include "task.h"
#include "timer.h"
#include "cm3.h"

#define osCMSIS                             0x10002
#define osCMSIS_KERNEL                      0x10000
#define osKernelSystemId                    "KERNEL V1.00"
#define osFeature_SysTick                   1UL
#define osFeature_MainThread                0UL
#define osKernelSysTickFrequency            SystemCoreClock
#define osKernelSysTickMicroSec (microsec)  (((uint64_t)microsec * (osKernelSysTickFrequency)) / 1000000)
#define osKernelSysTickMs                   1UL

typedef enum{
    osOK = 0,
    osEventSignal           = 0x08,
    osEventMessage          = 0x10,
    osEventMail             = 0x20,
    osEventTimeout          = 0x40,
    osErrorParameter        = 0x80,
    osErrorResource         = 0x81,
    osErrorTimeoutResource  = 0xC1,
    osErrorISR              = 0x82,
    osErrorISRRecursive     = 0x83,
    osErrorPriority         = 0x84,
    osErrorNoMemory         = 0x85,
    osErrorValue            = 0x86,
    osErrorOS               = 0xFF,
    os_status_reserved      = 0x7FFFFFFF
}osStatus;

static int32_t l_is_os_running = 0;
extern uint32_t g_os_systick;

osStatus osKernelInitialize()
{
    init_task_module();
    timer_module_init();
    return osOK;
}

osStatus osKernelStart()
{
    if (l_is_os_running == 0) {
        g_next_task = task_highest_ready();
        task_run_first();
        l_is_os_running = 1;
    }
    return osOK;
}

int32_t osKernelRunning()
{
    return l_is_os_running;
}

uint32_t osKernelSysTick()
{
    return g_os_systick;
}
