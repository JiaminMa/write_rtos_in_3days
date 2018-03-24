/*-----------------------------------------------------------------------------
 *      Name:         cmsis_rv.c
 *      Purpose:      Driver validation test cases entry point
 *----------------------------------------------------------------------------
 *      Copyright(c) KEIL - An ARM Company
 *----------------------------------------------------------------------------*/
#include "cmsis_rv.h"
#include "RTE_Components.h"
#include "RV_Framework.h"
#include "RV_Config.h"

/*-----------------------------------------------------------------------------
 *      Prototypes
 *----------------------------------------------------------------------------*/
void DEF_IRQHandler (void);

/*-----------------------------------------------------------------------------
 *      Variables declarations
 *----------------------------------------------------------------------------*/
void (*TST_IRQHandler)(void);
uint32_t ISR_ExNum;

/*-----------------------------------------------------------------------------
 *      Default IRQ Handler
 *----------------------------------------------------------------------------*/
void DEF_IRQHandler (void) {
  if (TST_IRQHandler!=NULL) TST_IRQHandler();
}

/*-----------------------------------------------------------------------------
 *      Init test suite
 *----------------------------------------------------------------------------*/
static void TS_Init (void) {  
#ifdef RTE_RV_MEMORYPOOL
  CreateMemoryPool();
#endif 
#ifdef RTE_RV_MSGQUEUE
  CreateMessageQueue();
#endif 
#ifdef RTE_RV_MAILQUEUE
  CreateMailQueue();
#endif 
#ifdef RTE_RV_WAITFUNC
  StartCortexCycleCounter();
#endif 
}

/*-----------------------------------------------------------------------------
 *      Set Pending IRQ
 *----------------------------------------------------------------------------*/
void SetPendingIRQ(IRQn_Type IRQn) {

  NVIC_SetPendingIRQ(IRQn);
  // data synchronization, instruction synchronization and memory barriers
  __DSB();
  __ISB();
  __DMB();
}

/*-----------------------------------------------------------------------------
 *      Test cases list
 *----------------------------------------------------------------------------*/
static TEST_CASE TC_LIST[] = {
#ifdef RTE_RV_THREAD 
  TCD ( TC_ThreadCreate,                         1      ),
  TCD ( TC_ThreadMultiInstance,                  1      ),
  TCD ( TC_ThreadTerminate,                      1      ),
  TCD ( TC_ThreadRestart,                        1      ),
  TCD ( TC_ThreadGetId,                          1      ),
  TCD ( TC_ThreadPriority,                       1      ),
  TCD ( TC_ThreadPriorityExec,                   1      ),
  TCD ( TC_ThreadChainedCreate,                  1      ),
  TCD ( TC_ThreadYield,                          1      ),
  TCD ( TC_ThreadParam,                          1      ),
  TCD ( TC_ThreadInterrupts,                     1      ),
#endif                                        
#ifdef RTE_RV_GENWAIT                         
  TCD ( TC_GenWaitBasic,                         1      ),
  TCD ( TC_GenWaitInterrupts,                    1      ),
#endif                                        
#ifdef RTE_RV_TIMER                           
  TCD ( TC_TimerOneShot,                         1      ),
  TCD ( TC_TimerPeriodic,                        1      ),
  TCD ( TC_TimerParam,                           1      ),
  TCD ( TC_TimerInterrupts,                      1      ),
#endif                                        
#ifdef RTE_RV_SIGNAL                          
  TCD ( TC_SignalMainThread,                     1      ),
  TCD ( TC_SignalChildThread,                    1      ),
  TCD ( TC_SignalChildToParent,                  1      ),
  TCD ( TC_SignalChildToChild,                   1      ),
  TCD ( TC_SignalWaitTimeout,                    1      ),
  TCD ( TC_SignalCheckTimeout,                   1      ),
  TCD ( TC_SignalParam,                          1      ),
  TCD ( TC_SignalInterrupts,                     1      ),
#endif                                        
#ifdef RTE_RV_SEMAPHORE                       
  TCD ( TC_SemaphoreCreateAndDelete,             1      ),
  TCD ( TC_SemaphoreObtainCounting,              1      ),
  TCD ( TC_SemaphoreObtainBinary,                1      ),
  TCD ( TC_SemaphoreWaitForBinary,               1      ),
  TCD ( TC_SemaphoreWaitForCounting,             1      ),
  TCD ( TC_SemaphoreZeroCount,                   1      ),
  TCD ( TC_SemaphoreWaitTimeout,                 1      ),
  TCD ( TC_SemaphoreCheckTimeout,                1      ),
  TCD ( TC_SemParam,                             1      ),        
  TCD ( TC_SemInterrupts,                        1      ),   
#endif  
#ifdef RTE_RV_MUTEX                                                                       
  TCD ( TC_MutexBasic,                           1      ),
  TCD ( TC_MutexTimeout,                         1      ),
  TCD ( TC_MutexCheckTimeout,                    1      ),
  TCD ( TC_MutexNestedAcquire,                   1      ),
  TCD ( TC_MutexPriorityInversion,               1      ),
  TCD ( TC_MutexOwnership,                       1      ),
  TCD ( TC_MutexParam,                           1      ),
  TCD ( TC_MutexInterrupts,                      1      ),                                           
#endif  
#ifdef RTE_RV_MEMORYPOOL
  TCD ( TC_MemPoolAllocAndFree,                  1      ),   
  TCD ( TC_MemPoolAllocAndFreeComb,              1      ),   
  TCD ( TC_MemPoolZeroInit,                      1      ),   
  TCD ( TC_MemPoolParam,                         1      ),   
  TCD ( TC_MemPoolInterrupts,                    1      ),   
#endif  
#ifdef RTE_RV_MSGQUEUE
  TCD ( TC_MsgQBasic,                            1      ),  
  TCD ( TC_MsgQWait,                             1      ),  
  TCD ( TC_MsgQCheckTimeout,                     1      ),  
  TCD ( TC_MsgQParam,                            1      ),  
  TCD ( TC_MsgQInterrupts,                       1      ),  
  TCD ( TC_MsgFromThreadToISR,                   1      ),  
  TCD ( TC_MsgFromISRToThread,                   1      ),  
#endif 
#ifdef RTE_RV_MAILQUEUE
  TCD ( TC_MailAlloc,                            1      ),
  TCD ( TC_MailCAlloc,                           1      ),
  TCD ( TC_MailToThread,                         1      ),
  TCD ( TC_MailFromThread,                       1      ),
  TCD ( TC_MailTimeout,                          1      ),
  TCD ( TC_MailCheckTimeout,                     1      ),
  TCD ( TC_MailParam,                            1      ),
  TCD ( TC_MailInterrupts,                       1      ),
  TCD ( TC_MailFromThreadToISR,                  1      ),
  TCD ( TC_MailFromISRToThread,                  1      ),
#endif 
#ifdef RTE_RV_WAITFUNC
  TCD ( TC_MeasureOsDelayTicks,                  1      ),
  TCD ( TC_MeasureOsWaitTicks,                   1      ),
  TCD ( TC_MeasureOsSignalWaitTicks,             1      ),
  TCD ( TC_MeasureOsMutexWaitTicks,              1      ),
  TCD ( TC_MeasureOsSemaphoreWaitTicks,          1      ),
  TCD ( TC_MeasureOsMessageWaitTicks,            1      ),
  TCD ( TC_MeasureOsMailWaitTicks,               1      ),
#endif 
};                                                              

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdate-time"
#endif
/*-----------------------------------------------------------------------------
 *      Test suite description
 *----------------------------------------------------------------------------*/
TEST_SUITE ts = {
  __FILE__, __DATE__, __TIME__,
  "CMSIS-RTOS Test Suite",
  TS_Init,  
  1,
  TC_LIST,
  ARRAY_SIZE (TC_LIST),  
};  
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#endif
