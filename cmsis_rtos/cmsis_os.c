#include "timer.h"
#include "cm3.h"
#include "cmsis_os.h"

static int32_t l_is_os_running = 0;
extern uint32_t g_os_systick;

typedef struct mem {              /* << Memory Pool management struct >>     */
    struct mem *next;               /* Next Memory Block in the list           */
    uint32_t len;                /* Length of data block                    */
} MEMP;

uint32_t g_heap[HEAP_MEM_POOL];

/*Kernel information and control*/
static uint32_t rt_init_mem (void *pool, uint32_t size) {

    MEMP *ptr;

    if ((pool == NULL) || (size < sizeof(MEMP))) { return (1U); }

    ptr = (MEMP *)pool;
    ptr->next = (MEMP *)((uint32_t)pool + size - sizeof(MEMP *));
    ptr->next->next = NULL;
    ptr->len = 0U;

    return (0U);
}

osStatus osKernelInitialize()
{
    init_task_module();
    timer_module_init();
    rt_init_mem(&g_heap, HEAP_MEM_POOL);
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

/*Thread Management*/
/*  rt_init_mem
 *  rt_alloc_mem
 *  rt_free_mem are copyied form RTX.
 *  Here is only for stack allocation.
 * */

static void *rt_alloc_mem (void *pool, uint32_t size) {
    MEMP *p, *p_search, *p_new;
    uint32_t   hole_size;

    if ((pool == NULL) || (size == 0U)) { return NULL; }

    /* Add header offset to 'size' */
    size += sizeof(MEMP);
    /* Make sure that block is 4-byte aligned  */
    size = (size + 3U) & ~(uint32_t)3U;

    p_search = (MEMP *)pool;
    while (1) {
        hole_size  = (uint32_t)p_search->next - (uint32_t)p_search;
        hole_size -= p_search->len;
        /* Check if hole size is big enough */
        if (hole_size >= size) { break; }
        p_search = p_search->next;
        if (p_search->next == NULL) {
            /* Failed, we are at the end of the list */
            return NULL;
        }
    }

    if (p_search->len == 0U) {
        /* No block is allocated, set the Length of the first element */
        p_search->len = size;
        p = (MEMP *)(((uint32_t)p_search) + sizeof(MEMP));
    } else {
        /* Insert new list element into the memory list */
        p_new       = (MEMP *)((uint32_t)p_search + p_search->len);
        p_new->next = p_search->next;
        p_new->len  = size;
        p_search->next = p_new;
        p = (MEMP *)(((uint32_t)p_new) + sizeof(MEMP));
    }

    return (p);
}

static uint32_t rt_free_mem (void *pool, void *mem) {
    MEMP *p_search, *p_prev, *p_return;

    if ((pool == NULL) || (mem == NULL)) { return (1U); }

    p_return = (MEMP *)((uint32_t)mem - sizeof(MEMP));

    /* Set list header */
    p_prev = NULL;
    p_search = (MEMP *)pool;
    while (p_search != p_return) {
        p_prev   = p_search;
        p_search = p_search->next;
        if (p_search == NULL) {
            /* Valid Memory block not found */
            return (1U);
        }
    }

    if (p_prev == NULL) {
        /* First block to be released, only set length to 0 */
        p_search->len = 0U;
    } else {
        /* Discard block from chain list */
        p_prev->next = p_search->next;
    }

    return (0U);
}

task_t g_task_pool[MAX_TASK_NUM];
task_stack_t stack[1024];
osThreadId osThreadCreate(const osThreadDef_t * thread_def, void *argument)
{
    uint32_t i = 0;
    osThreadId p_task = (osThreadId) NULL;
    task_stack_t *p_task_stk;

    /*1. Search an available task*/
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if (0 == g_task_pool[i].is_used) {
            p_task = &g_task_pool[i];
            break;
        }
    }

    /*Return NULL if there's no available task*/
    if (p_task == (osThreadId)NULL) {
        DEBUG("%s:no available task\n", __func__);
        return (osThreadId)NULL;
    }

    /*2.Allocate a stack for the task*/
    p_task_stk = rt_alloc_mem(&g_heap, thread_def->stacksize);
    if (p_task_stk == (task_stack_t)NULL) {
        /*No stack memory for task*/
        DEBUG("%s:No stack memory for task\n", __func__);
        return (osThreadId)NULL;
    }

    /*3.Init the task and return */
    task_init(p_task,
              thread_def->pthread,
              argument,
              thread_def->tpriority,
              &(p_task_stk[thread_def->stacksize]),
              &(p_task_stk[0]),
              thread_def->stacksize);

    return p_task;
}
