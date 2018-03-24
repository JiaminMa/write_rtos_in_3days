#include <stdint.h>
#include "cmsis_os.h"

/* Because CMSIS RTOS use CamelNaming, so the app
 * use the same naming type
 * */
osThreadId threadId0;
osThreadId threadId1;

void thread0(const void *arg)
{
    init_systick(1);

    for (;;) {
        printk("%s\n", __func__);
        osDelay(1000);
    }
}

void thread1(const void *arg)
{
    for (;;) {
        printk("%s\n", __func__);
        osDelay(1000);
    }
}

osThreadDef(thread0, 0, 1, 1024);
osThreadDef(thread1, 1, 1, 1024);

int main()
{
    osKernelInitialize();

    threadId0 = osThreadCreate(osThread(thread0), (void *)NULL);
    threadId1 = osThreadCreate(osThread(thread1), (void *)NULL);

    osKernelStart();

    for(;;);
    return 0;
}
