#include <stdint.h>
#include "cmsis_os.h"

osThreadId thread_id0;
osThreadId thread_id1;

void thread_0(const void *arg)
{
    init_systick(1);
    uint32_t i = 0;
    osThreadId thread_id;
    for (;;) {
        thread_id = osThreadGetId();
        printk("%s: thread_id0:%x, thread_id:%x, prio:%d\n", __func__, thread_id0,  thread_id, osThreadGetPriority(thread_id));
        i++;
        osThreadSetPriority(thread_id0, i);
        task_delay(1000);
        if (i == 5) {
            osThreadTerminate(thread_id0);
        }
    }
}

void thread_1(const void *arg)
{
    for (;;) {
        printk("%s\n", __func__);
        task_delay(1000);
    }
}

osThreadDef(thread_0, 0, 1, 1024);
osThreadDef(thread_1, 1, 1, 1024);

int main()
{
    osKernelInitialize();

    thread_id0 = osThreadCreate(osThread(thread_0), (void *)NULL);
    thread_id1 = osThreadCreate(osThread(thread_1), (void *)NULL);

    osKernelStart();

    for(;;);
    return 0;
}
