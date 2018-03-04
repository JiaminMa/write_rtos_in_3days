#include "os_stdio.h"
#include <stdint.h>
#include "cm3.h"
#include "task.h"
#include "mailbox.h"
#include "os.h"

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

task_t task1;
task_t task2;
task_t task3;
task_t task4;
task_stack_t task1_stk[1024];
task_stack_t task2_stk[1024];
task_stack_t task3_stk[1024];
task_stack_t task4_stk[1024];

mbox_t mbox1;
mbox_t mbox2;
void *mbox1_msg_buffer[20];
void *mbox2_msg_buffer[20];
uint32_t msg[20];

void task1_entry(void *param)
{
    uint32_t i = 0;
    init_systick(10);

    mbox_init(&mbox1, mbox1_msg_buffer, 20);
    for(;;) {
        printk("%s:send mbox\n", __func__);
        for (i = 0; i < 20; i++) {
            msg[i] = i;
            mbox_send(&mbox1, &msg[i], MBOX_SEND_NORMAL);
        }
        task_delay_s(20);

        printk("%s:send mbox front\n", __func__);
        for (i = 0; i < 20; i++) {
            msg[i] = i;
            mbox_send(&mbox1, &msg[i], MBOX_SEND_FRONT);
        }
        task_delay_s(20);
        task_delay_s(1);
    }
}

void task2_entry(void *param)
{
    void *msg;
    uint32_t err = 0;
    for(;;) {
        err = mbox_get(&mbox1, &msg, 10);
        if (err == NO_ERROR) {
            uint32_t value = *(uint32_t *)msg;
            printk("%s:value:%d\n", __func__, value);
//            task_delay_s(1);
        }
    }
}

void task3_entry(void *param)
{
    uint32_t msg;
    init_systick(10);
    mbox_init(&mbox2, mbox2_msg_buffer, 20);
    mbox_get(&mbox2, (void *)&msg, 0);
    for(;;) {
        void *msg;
        printk("%s\n", __func__);
        task_delay_s(1);
    }

}

void task4_entry(void *param)
{
    int destory_mbox2 = 0;
    for(;;) {
        printk("%s\n", __func__);
        task_delay_s(1);
        if (destory_mbox2 == 0) {
            mbox_destory(&mbox2);
            destory_mbox2 = 1;
        }
    }
}

int main()
{

    clear_bss();

    DEBUG("Hello RTOS C03_Delay_List\n");

    DEBUG("psp:0x%x\n", get_psp());
    DEBUG("msp:0x%x\n", get_msp());

    init_task_module();

    task_init(&task1, task1_entry, (void *)0x11111111, 0, &task1_stk[1024]);
    task_init(&task2, task2_entry, (void *)0x22222222, 1, &task2_stk[1024]);
#if 0
    task_init(&task3, task3_entry, (void *)0x33333333, 0, &task3_stk[1024]);
    task_init(&task4, task4_entry, (void *)0x44444444, 1, &task4_stk[1024]);
#endif
    g_next_task = task_highest_ready();
    task_run_first();

    for(;;);
    return 0;
}
