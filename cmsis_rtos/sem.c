#include "sem.h"
#include "task.h"
#include "os.h"

void sem_init(sem_t *sem, uint32_t count, uint32_t max_count)
{
    event_init(&sem->event, EVENT_TYPE_SEM);

    if (max_count == 0) {
        sem->count = count;
    } else {
        sem->count = count > max_count ? max_count : count;
    }
}

uint32_t sem_acquire(sem_t *sem, uint32_t wait_ticks)
{
    uint32_t status = task_enter_critical();

    if (sem->count > 0) {
        --sem->count;
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        event_wait(&sem->event, g_current_task, (void *)0, EVENT_TYPE_SEM, wait_ticks);
        task_exit_critical(status);
        task_sched();
        return g_current_task->wait_event_result;
    }

}

uint32_t sem_acquire_no_wait(sem_t *sem)
{
    uint32_t status = task_enter_critical();

    if (sem->count > 0) {
        --sem->count;
        task_exit_critical(status);
        return NO_ERROR;
    } else {
        task_exit_critical(status);
        return g_current_task->wait_event_result;
    }
}

void sem_release(sem_t *sem)
{
    uint32_t status = task_enter_critical();
    if (event_wait_count(&sem->event) > 0) {
        task_t *task = event_wakeup(&sem->event, (void *)0, NO_ERROR);
        if (task->prio < g_current_task->prio) {
            task_sched();
        }
    } else {
        sem->count++;
        if ((sem->max_count != 0) && (sem->count > sem->max_count)) {
            sem->count = sem->max_count;
        }
    }
    task_exit_critical(status);
}

void sem_get_info(sem_t *sem, sem_info_t *info)
{
    uint32_t status = task_enter_critical();
    info->count = sem->count;
    info->max_count = sem->max_count;
    info->task_count = event_wait_count(&sem->event);
    task_exit_critical(status);
}

uint32_t sem_destory(sem_t *sem)
{
    uint32_t status = task_enter_critical();

    uint32_t count = event_remove_all(&sem->event, (void *)0, ERROR_DEL);
    sem->count = 0;
    task_exit_critical(status);

    if (count > 0) {
        task_sched();
    }
    return count;
}
