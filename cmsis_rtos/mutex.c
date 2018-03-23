#include "mutex.h"
#include "os_stdio.h"
#include "os.h"

void mutex_init(mutex_t *mutex)
{
    event_init(&mutex->event, EVENT_TYPE_MUTEX);
    mutex->owner = (task_t *)NULL;
    mutex->owner_original_prio = OS_PRIO_COUNT;
    mutex->lock_count = 0;
}

uint32_t mutex_lock(mutex_t *mutex, uint32_t wait_ticks)
{
    uint32_t status = task_enter_critical();
    task_t *owner;

    if (mutex->lock_count <= 0) {
        /*If the mutex is locked by a task*/
        mutex->owner = g_current_task;
        mutex->owner_original_prio = g_current_task->prio;
        mutex->lock_count++;
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        if (mutex->owner == g_current_task) {
            mutex->lock_count++;
            task_exit_critical(status);
            return NO_ERROR;
        } else {
            if (g_current_task->prio < mutex->owner->prio) {
                owner = mutex->owner;
                if (owner->state == OS_TASK_STATE_RDY) {
                    /*Move the task from low prio list to high prio list*/
                    task_unready(owner);
                    owner->prio = g_current_task->prio;
                    task_ready(owner);
                } else {
                    owner->prio = g_current_task->prio;
                }
            }
            event_wait(&mutex->event, g_current_task, (void *)0, EVENT_TYPE_MUTEX, wait_ticks);
            task_exit_critical(status);
            task_sched();
            return g_current_task->wait_event_result;
        }
    }
}

uint32_t mutex_lock_no_wait(mutex_t *mutex)
{
    uint32_t status = task_enter_critical();

    if (mutex->lock_count <= 0) {
        /*If the mutex is locked by a task*/
        mutex->owner = g_current_task;
        mutex->owner_original_prio = g_current_task->prio;
        mutex->lock_count++;
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        if (mutex->owner == g_current_task) {
            mutex->lock_count++;
            task_exit_critical(status);
            return NO_ERROR;
        }
        task_exit_critical(status);
        return ERROR_RESOURCE_FULL;
    }
}

uint32_t mutex_unlock(mutex_t *mutex)
{
    uint32_t status = task_enter_critical();

    if (mutex->lock_count <= 0) {
        task_exit_critical(status);
        return NO_ERROR;
    }

    if (mutex->owner != g_current_task) {
        task_exit_critical(status);
        return ERROR_NOT_OWNER;
    }

    if (--mutex->lock_count > 0) {
        task_exit_critical(status);
        return NO_ERROR;
    }

    if (mutex->owner_original_prio != mutex->owner->prio) {
        if (mutex->owner->state == OS_TASK_STATE_RDY) {
            task_unready(mutex->owner);
            g_current_task->prio = mutex->owner_original_prio;
            task_ready(mutex->owner);
        } else {
            g_current_task->prio = mutex->owner_original_prio;
        }
    }

    if (event_wait_count(&mutex->event) > 0) {
        task_t *task = event_wakeup(&mutex->event, (void *)0, NO_ERROR);
        mutex->owner = task;
        mutex->owner_original_prio = task->prio;
        mutex->lock_count++;
        if (task->prio < g_current_task->prio) {
            task_sched();
        }
    }

    task_exit_critical(status);
    return NO_ERROR;
}

uint32_t mutex_destory(mutex_t *mutex)
{
    uint32_t count = 0;
    uint32_t status = task_enter_critical();

    if (mutex->lock_count > 0) {
        if (mutex->owner_original_prio != mutex->owner->prio) {
            if (mutex->owner->state == OS_TASK_STATE_RDY) {
                task_unready(mutex->owner);
                mutex->owner->prio = mutex->owner_original_prio;
                task_ready(mutex->owner);
            } else {
                mutex->owner->prio = mutex->owner_original_prio;
            }
        }
    }

    count = event_remove_all(&mutex->event, (void *)0, ERROR_DEL);
    if (count > 0) {
        task_sched();
    }

    task_exit_critical(status);
    return NO_ERROR;
}

void mutex_get_info(mutex_t *mutex, mutex_info_t *info)
{
    uint32_t status = task_enter_critical();

    info->task_count = event_wait_count(&mutex->event);
    info->owner_prio = mutex->owner_original_prio;
    if (mutex->owner != (task_t *)NULL) {
        info->inhe_prio = mutex->owner->prio;
    } else {
        info->inhe_prio = OS_PRIO_COUNT;
    }

    info->owner = mutex->owner;
    info->locked_count = mutex->lock_count;
    task_exit_critical(status);
}
