#include "mailbox.h"
#include "task.h"
#include "os.h"

void mbox_init(mbox_t *mbox, void **msg_buffer, uint32_t max_count)
{
    event_init(&mbox->event, EVENT_TYPE_MAILBOX);
    mbox->msg_buffer = msg_buffer;
    mbox->max_count = max_count;
    mbox->read = 0;
    mbox->write = 0;
    mbox->count = 0;
}

uint32_t mbox_get(mbox_t *mbox, void **msg, uint32_t wait_ticks)
{
    uint32_t status = task_enter_critical();

    if (mbox->count > 0) {

        mbox->count--;
        *msg = mbox->msg_buffer[mbox->read++];
        if (mbox->read >= mbox->max_count) {
            mbox->read = 0;
        }
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        event_wait(&mbox->event, g_current_task, (void *)0, EVENT_TYPE_MAILBOX, wait_ticks);
        task_exit_critical(status);
        task_sched();

        *msg = g_current_task->event_msg;
        return g_current_task->wait_event_result;
    }
}

uint32_t mbox_get_no_wait(mbox_t *mbox, void **msg)
{
    uint32_t status = task_enter_critical();

    if (mbox->count > 0) {

        mbox->count--;
        *msg = mbox->msg_buffer[mbox->read++];
        if (mbox->read >= mbox->max_count) {
            mbox->read = 0;
        }
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        task_exit_critical(status);
        return g_current_task->wait_event_result;
    }
}

uint32_t mbox_send(mbox_t *mbox, void *msg, uint32_t notify_opition)
{
    uint32_t status = task_enter_critical();
    task_t *task = (task_t *)NULL;

    if (event_wait_count(&mbox->event) > 0) {
        task = event_wakeup(&mbox->event, (void *)msg, NO_ERROR);
        if (task->prio < g_current_task->prio) {
            task_sched();
        }
    } else {
        if (mbox->count >= mbox->max_count) {
            task_exit_critical(status);
            return ERROR_RESOURCE_FULL;
        }

        if (notify_opition & MBOX_SEND_FRONT) {
            if (mbox->read <= 0) {
                mbox->read = mbox->max_count - 1;
            } else {
                mbox->read--;
            }
            mbox->msg_buffer[mbox->read] = msg;
        } else {
            mbox->msg_buffer[mbox->write++] = msg;
            if (mbox->write >= mbox->max_count) {
                mbox->write = 0;
            }
        }

        mbox->count++;
    }
    task_exit_critical(status);
    return NO_ERROR;
}

void mbox_flush(mbox_t *mbox)
{
    uint32_t status = task_enter_critical();

    if (event_wait_count(&mbox->event) == 0) {
        mbox->read = 0;
        mbox->write = 0;
        mbox->count = 0;
    }

    task_exit_critical(status);
}

uint32_t mbox_destory(mbox_t *mbox)
{
    uint32_t status = task_enter_critical();

    uint32_t count = event_remove_all(&mbox->event, (void *)0, ERROR_DEL);

    task_exit_critical(status);
    if (count > 0) {
        task_sched();
    }

    return count;
}

void mbox_get_info(mbox_t *mbox, mbox_info_t *info)
{
    uint32_t status = task_enter_critical();
    info->count = mbox->count;
    info->max_count = mbox->max_count;
    info->task_count = event_wait_count(&mbox->event);
    task_exit_critical(status);
}
