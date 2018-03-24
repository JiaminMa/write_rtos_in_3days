#ifndef CMSIS_OS_H
#define CMSIS_OS_H

#include "task.h"
#include "os_stdio.h"
#include "os.h"
#include "cm3.h"
/*Kernel information and control*/
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

extern osStatus osKernelInitialize(void);
extern osStatus osKernelStart(void);
extern int32_t osKernelRunning(void);
extern uint32_t osKernelSysTick(void);

/*Thread Management*/
#define HEAP_MEM_POOL       (1024UL * 32UL)
#define MAX_TASK_NUM        5

#if 0
typedef enum {
    osPriorityIdle = -3,
    osPriorityLow = -2,
    osPriorityBelowNormal = -1,
    osPriorityNormal = 0,
    osPriorityAboveNormal = +1,
    osPriorityHigh = +2,
    osPriorityRealtime = +3,
    osPriorityError = 0x84
}osPriority;
#endif
typedef uint32_t osPriority;

typedef task_t * osThreadId;

typedef void(* os_pthread)(void const *argument);

typedef struct {
    os_pthread pthread;
    osPriority tpriority;
    uint32_t instances;
    uint32_t stacksize;
}osThreadDef_t;

#define osThreadDef(name, priority, instances, stacksz) \
const osThreadDef_t os_thread_def_##name = \
{(name), (priority), (instances), (stacksz)}

#define osThread(name)   &os_thread_def_##name

extern osThreadId osThreadCreate(const osThreadDef_t * thread_def, void *argument);
extern osStatus osThreadTerminate(osThreadId thread_id);
extern osStatus osThreadStop(osThreadId thread_id);
extern osPriority osThreadGetPriority(osThreadId thread_id);
extern osThreadId osThreadGetId(void);
extern osStatus osThreadSetPriority(osThreadId thread_id, osPriority priority);
extern osStatus osThreadYield(void);

/*Generic Wait Functions*/
extern osStatus osDelay(uint32_t millisec);

#endif /*CMSIS_OS_H*/
