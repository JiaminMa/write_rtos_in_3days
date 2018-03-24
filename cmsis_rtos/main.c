#include <stdint.h>
#include "cmsis_os.h"

void thread_0(const void *arg)
{
    init_systick(1);
    for (;;) {
        printk("%s\n", __func__);
        task_delay(1000);
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
osThreadId thread_id0;
osThreadId thread_id1;

int main()
{
    osKernelInitialize();

    thread_id0 = osThreadCreate(osThread(thread_0), (void *)NULL);
    thread_id1 = osThreadCreate(osThread(thread_1), (void *)NULL);

    osKernelStart();

    for(;;);
    return 0;
}
