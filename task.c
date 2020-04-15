#include <stddef.h>
#include <stdint.h>
#include "stm32l031xx.h" 
#include "config.h"
#include "task.h"

#define INITIAL_REGISTER_VALUE  0xdeadbeef

volatile uint32_t ticks;

static task_t *task_list      = NULL;
static task_t *running_list   = NULL;
static task_t *suspended_list = NULL;
static task_t *running_task   = NULL;

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
 * Tasks are added in the running state.
 * stack must be 8 byte aligned
 */
int add_task(task_function_t *task_function, task_t *task, uint32_t *stack, unsigned stack_words)
{
    task->sp = create_task_stack(task_function, stack, stack_words);
    task->stack = stack;
    task->stack_words = stack_words;
    task->next_suspended = NULL;
    task->next_running = running_list;
    running_list = task;
    task->next_task = task_list;
    task_list = task;
    return 0;
}

void sleep(uint32_t ticks_to_sleep)
{
    task_t *this;

    /* Block interrupts and move this task to the suspended list */
    enter_critical();
    this = running_task;
    this->wait_until = ticks + ticks_to_sleep;
    running_list = running_list->next_running;
    this->next_suspended = suspended_list;
    suspended_list = this;
    yield_from_task();
    exit_critical();
}
    
void yield_from_task(void)
{
    NVIC->ISPR[0] = YIELD_BIT;
}

uint32_t *choose_next_task(uint32_t *current_sp)
{
    task_t *temp, *prev, *insert_point;
    
    /* Save the outgoing task's stack pointer */
    running_task->sp = current_sp;
    
    /* round robin to next task */
    temp = running_list;
    running_list = temp->next_running;
    if (running_list == NULL)
    {
        running_list = temp;
    }
    else
    {
        /* Add on temp at the end */
        temp->next_running = NULL;
        insert_point = running_list;
        while (insert_point->next_running != NULL)
        {
            insert_point = insert_point->next_running;
        }
        insert_point->next_running = temp;
    }

    /* Check suspended list for tasks that need to wake up from sleep */
    if (suspended_list != NULL)
    {
        prev = NULL;
        for (temp = suspended_list; temp; temp = temp->next_suspended, prev = temp)
        {
            /* Is this task ready to wake? */
            if ((int32_t)(temp->wait_until - ticks) <= 0)
            {
                /* Remove from suspended list */
                if (prev == NULL)
                {
                    suspended_list = suspended_list->next_suspended;
                }
                else
                {
                    prev->next_suspended = temp->next_suspended;
                    temp->next_suspended = NULL;                        // TODO: Bug - ends for loop
                }
                /* Add to running list */
                temp->next_running = running_list;
                running_list = temp;
            }
        }
    }

    /* Return incoming task's stack pointer */
    running_task = running_list;
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
        yield_from_task();
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
    add_task(idle_task_function, &idle_task, idle_task_stack, sizeof(idle_task_stack) / 4);
    /* The idle task starts with an empty stack, as we call it directly */
    idle_task.sp = idle_task_stack + sizeof(idle_task_stack) / 4;

    /* Enable the yield interrupt, set it to the lowest priority */
    NVIC->IP[YIELD_PRIO_REG] |= YIELD_PRIO;
    NVIC->ISER[0] = YIELD_BIT;
    /* Enable the timer interrupt, set it to the lowest priority */
    NVIC->IP[TIMER_PRIO_REG] |= TIMER_PRIO;
    NVIC->ISER[0] = TIMER_BIT;
    /* Pend the interrupt that will yield to first ready task */
    NVIC->ISPR[0] = YIELD_BIT;
    start_idle_task(idle_task.sp);
    
    /* Should never get here */
    while(1)
    {
        /* Spin */
    }
}
