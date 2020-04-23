#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32l031xx.h" 
#include "config.h"
#include "task.h"

#define INITIAL_REGISTER_VALUE  0xdeadbeef

/* Flags for the state of a task */
#define TASK_RUNNABLE   0
#define TASK_SLEEPING   1
#define TASK_BLOCKED    2

volatile uint32_t ticks;

static task_t *task_list                     = NULL;
static task_t *runnable_list[NUM_TASK_PRIOS] = {NULL};
static task_t *suspended_list                = NULL;
static task_t *running_task                  = NULL;

uint32_t enabled_irqs;
int nesting = 0;

static uint32_t idle_task_stack[48] __ALIGNED(8);
static task_t idle_task;

/*
 * Enter critical section - prevent all interrupts below realtime priority
 * Calls to this function cannot be nested
 */
__ASM void _enter_critical(void)
{
    IMPORT enabled_irqs
    
    ldr r0, =enabled_irqs
    ldr r1, =0xE000E180
    ldr r2, =~REALTIME_IRQS
    cpsid i
    ldr r3, [r1]        /* Read NVIC->ICER[0]                     */
    str r2, [r1]        /* Disable all except realtime interrupts */
    cpsie i
    ands r3, r3, r2     /* Mask out the realtime interrupts       */
    str r3, [r0]        /* Save the value in enabled_irqs2        */
    bx lr
    
    ALIGN 4
}

/*
 * Exit critical section - return interrupt mask to what it was before
 * Calls to this function cannot be nested
 */
void _exit_critical(void)
{
    NVIC->ISER[0] = enabled_irqs;
}

/*
 * Enter critical section in task context
 * Calls to this function can be nested within a task
 */
void enter_critical(void)
{
    if (nesting == 0)
    {
        _enter_critical();
    }
    ++nesting;
}

/*
 * Exit critical section in task context
 * Calls to this function can be nested within a task
 */
void exit_critical(void)
{
    --nesting;
    if (nesting == 0)
    {
        _exit_critical();
    }
}

void task_returned(void)
{
    while (1)
    {
        /* spin */
    }
}

/*
 * Set up stack contents as if the task_function just got swapped out (see task_switch)
 * stack must be 8 byte aligned
 */
static uint32_t *create_task_stack(task_function_t *task_function, uint32_t *stack, unsigned stack_words)
{
    stack[stack_words - 1] = 1u << 24;                          /* xPSR (just the Thumb bit)  */
    stack[stack_words - 2] = (uint32_t)task_function;           /* pc (return address)        */
    stack[stack_words - 3] = (uint32_t)&task_returned;          /* lr (trap in case of error) */
    for (int i = stack_words - 4; i >= stack_words - 16; --i)
    {
        stack[i] = INITIAL_REGISTER_VALUE;
    }
    return stack + stack_words - 16;
}

/*
 * Add a new task
 * Tasks are added in the runnable state.
 * stack must be 8 byte aligned
 */
int add_task(task_function_t *task_function, task_t *task, uint32_t *stack, unsigned stack_words,
             unsigned priority)
{
    task->sp             = create_task_stack(task_function, stack, stack_words);
    task->stack          = stack;
    task->stack_words    = stack_words;
    task->priority       = priority;
    task->flags          = TASK_RUNNABLE;
    task->next_suspended = NULL;
    task->next_task      = task_list;
    task->next_runnable  = runnable_list[priority];
    task_list               = task;
    runnable_list[priority] = task;
    return 0;
}

static bool wake_tasks_blocked_on_semaphore(semaphore_t *sem)
{
    task_t *task;
    
    /* See if there are some tasks to unblock */
    task = sem->blocked_list;
    while (task)
    {
        task->wait_for = NULL;
        task = task->next_blocked;
    }
    if (sem->blocked_list)
    {
        sem->blocked_list = NULL;
        return true;
    }
    return false;
}

static void block_on_semaphore(semaphore_t *sem, bool sleep, uint32_t target_ticks)
{
    unsigned p;

    /* Move this task to the blocked list and the suspended list */
    p = running_task->priority;
    runnable_list[p] = runnable_list[p]->next_runnable;
    running_task->next_blocked = sem->blocked_list;
    sem->blocked_list = running_task;
    running_task->next_suspended = suspended_list;
    suspended_list = running_task;
    if (sleep)
    {
        running_task->flags |= TASK_BLOCKED | TASK_SLEEPING;
        running_task->wait_until = target_ticks;
    }
    else
    {
        running_task->flags |= TASK_BLOCKED;
    }
    running_task->wait_for = sem;
}

/*
 * Wait for a semaphore
 * ticks_to_wait special values: zero (don't wait), negative (wait forever)
 */
bool wait_semaphore(semaphore_t *sem, int ticks_to_wait)
{
    bool got = false;
    uint32_t target_ticks;

    target_ticks = ticks + ticks_to_wait;
    
    while (true)
    {
        enter_critical();
        
        if (sem->value > 0)
        {
            --sem->value;
            /* Success */
            got = true;
            if (wake_tasks_blocked_on_semaphore(sem))
            {
                yield();
            }
            exit_critical();
            break;
        }
        else
        {
            if (ticks_to_wait == 0 || ((ticks_to_wait > 0) && (int32_t)(target_ticks - ticks) <= 0))
            {
                /* Failure: give up waiting */
                exit_critical();
                break;
            }
            block_on_semaphore(sem, ticks_to_wait > 0, target_ticks);
            yield();
        }
        
        exit_critical();
    }
    
    return got;
}

/*
 * Signal a semaphore
 * ticks_to_wait special values: zero (don't wait), negative (wait forever)
 */
bool signal_semaphore(semaphore_t *sem, int ticks_to_wait)
{
    bool put = false;
    uint32_t target_ticks;

    target_ticks = ticks + ticks_to_wait;
    
    while (true)
    {
        enter_critical();

        if (sem->value < sem->max)
        {
            ++sem->value;
            if (wake_tasks_blocked_on_semaphore(sem))
            {
                yield();
            }
            put = true;
            exit_critical();
            break;
        }
        else
        {
            if (ticks_to_wait == 0 || ((ticks_to_wait > 0) && (int32_t)(target_ticks - ticks) <= 0))
            {
                /* Failure: give up waiting */
                exit_critical();
                break;
            }
            block_on_semaphore(sem, ticks_to_wait > 0, target_ticks);
            yield();
        }
        
        exit_critical();
    }
    
    return put;
}

void sleep_until(uint32_t target_ticks)
{
    unsigned p;
    
    /* Block interrupts and move this task to the suspended list */
    enter_critical();
    running_task->flags |= TASK_SLEEPING;
    running_task->wait_until = target_ticks;
    p = running_task->priority;
    runnable_list[p] = runnable_list[p]->next_runnable;
    running_task->next_suspended = suspended_list;
    suspended_list = running_task;
    yield();
    exit_critical();
}

void sleep(uint32_t ticks_to_sleep)
{
    sleep_until(ticks + ticks_to_sleep);
}

void tick(void)
{
    bool need_yield = false;
    task_t *temp;

    ++ticks;

    /* Is there another task ready to run at this priority? */
    if (running_task->next_runnable)
    {
        need_yield = true;
    }
    else
    {
        /* Or has a task woken from sleep? */
        for (temp = suspended_list; temp; temp = temp->next_suspended)
        {
            /* Is this task ready to wake? */
            if ((temp->flags & TASK_SLEEPING) && (int32_t)(temp->wait_until - ticks) <= 0)
            {
                need_yield = true;
                break;
            }
        }
    }
                
    if (need_yield)
    {
        yield();
    }
}

void yield(void)
{
    NVIC->ISPR[0] = YIELD_BIT;
}

uint32_t *choose_next_task(uint32_t *current_sp)
{
    unsigned p;
    task_t *task, *taskb, **pprev, **pprevb, *insert_point;
    
    /* Save the outgoing task's stack pointer */
    running_task->sp = current_sp;
    
    /* Find highest priority runnable task */
    for (p = 0; p < NUM_TASK_PRIOS; ++p)
    {
        if (runnable_list[p] == NULL)
        {
            /* Check next lowest priority task list */
            continue;
        }
        if (runnable_list[p] == running_task)
        {
            /* Current task still runnable, round robin to next task if there is one */
            task = runnable_list[p];
            runnable_list[p] = task->next_runnable;
            if (runnable_list[p] == NULL)
            {
                /* No next task, keep running this one */
                runnable_list[p] = task;
            }
            else
            {
                /* Add on task at the end */
                task->next_runnable = NULL;
                insert_point = runnable_list[p];
                while (insert_point->next_runnable != NULL)
                {
                    insert_point = insert_point->next_runnable;
                }
                insert_point->next_runnable = task;
            }
        }
        running_task = runnable_list[p];
        break;
    }

    /* Check suspended list for tasks that need to wake up from sleep or being blocked */
    pprev = &suspended_list;
    for (task = suspended_list; task; task = task->next_suspended)
    {
        /* Is this task ready to wake/unblock? */
        if (  ((task->flags & TASK_SLEEPING) && ((int32_t)(task->wait_until - ticks) <= 0))
           || ((task->flags & TASK_BLOCKED)  && (task->wait_for == NULL)))
        {
            /* Remove this task from the suspended list - update previous next_suspended value */
            *pprev = task->next_suspended;
            /* Remove a timed-out task from the semaphore's list */
            if (task->wait_for != NULL)
            {
                pprevb = &task->wait_for->blocked_list;
                for (taskb = task->wait_for->blocked_list; task; task = task->next_blocked)
                {
                    if (taskb == task)
                    {
                        *pprevb = task->next_blocked;
                        break;
                    }
                    pprevb = &taskb->next_blocked;
                }
                task->wait_for = NULL;
            }
            /* Add task to the correct runnable list */
            task->flags = TASK_RUNNABLE;
            task->next_runnable = runnable_list[task->priority];
            runnable_list[task->priority] = task;
            /* Check if we should actually be running this task */
            if (task->priority <= running_task->priority)
            {
                running_task = task;
            }
        }
        else
        {
            /* This task stays on the suspended list, update pprev */
            pprev = &task->next_suspended;
        }
    }

    /* Return incoming task's stack pointer */
    return running_task->sp;
}

__ASM void Yield_IRQHandler(void)
{
    IMPORT choose_next_task
    IMPORT _exit_critical
    /* 
     * We have taken an interrupt while running the current task, so the Process Stack looks like this:
     *    ... (earlier contents of stack)
     *    xPSR
     *    Return address
     *    LR (R14)
     *    R12
     *    R3
     *    R2
     *    R1
     *    R0
     *
     * We are currently using the Main Stack, but these values will be saved/restored on the Process Stack
     * We need to add r4-r7, r8-r11, and the stack pointer must be saved in the task structure
     *    R7
     *    R6
     *    R5
     *    R4
     *    R11
     *    R10
     *    R9
     *    R8
     */
    mrs r0, psp
    subs r0, r0, #16
    stmia r0!, {r4-r7}
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    subs r0, r0, #32
    stmia r0!, {r4-r7}
    subs r0, r0, #16
    mov r4, r0
    bl _enter_critical      /* Protect task lists from IRQ access */
    mov r0, r4
    bl choose_next_task
    mov r4, r0
    bl _exit_critical
    mov r0, r4
    ldmia r0!, {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    ldmia r0!, {r4-r7}
    msr psp, r0
    ldr r0, =0xFFFFFFFD
    bx r0

    ALIGN 4
}

__NO_RETURN void idle_task_function(void *arg)
{
    running_task = &idle_task;
    __enable_irq();
    
    while (1)
    {
        /* spin */
        yield();
    }
}

__ASM void start_idle_task(uint32_t *idle_sp)
{
    PRESERVE8
    IMPORT idle_task_function

    /* Set task code to use the Process stack */
    movs r1, #2
    msr control, r1
    mov sp, r0
    bl idle_task_function
}

void __NO_RETURN start_rtos(uint32_t cpu_clocks_per_tick)
{
    __disable_irq();
    
    /* Create the idle task */
    add_task(idle_task_function, &idle_task, idle_task_stack, sizeof(idle_task_stack) / 4,
             NUM_TASK_PRIOS - 1);
    /* The idle task starts with an empty stack, as we call it directly */
    idle_task.sp = idle_task_stack + sizeof(idle_task_stack) / 4;

    /* Enable the yield interrupt, set it to the lowest priority */
    NVIC->IP[YIELD_PRIO_REG] |= YIELD_PRIO;
    NVIC->ISER[0] = YIELD_BIT;
    /* Enable the timer interrupt, set it to the lowest priority */
    NVIC->IP[TIMER_PRIO_REG] |= TIMER_PRIO;
    NVIC->ISER[0] = TIMER_BIT;
    /* Pend the interrupt that will yield to first ready task */
    yield();
    start_idle_task(idle_task.sp);
    
    /* Should never get here */
    while(1)
    {
        /* Spin */
    }
}
