#include <stddef.h>
#include <stdint.h>
#include "stm32l031xx.h" 
#include "config.h"
#include "task.h"

#define INITIAL_REGISTER_VALUE  0xdeadbeef

static task_t *task_list      = NULL;
static task_t *running_list   = NULL;
//static task_t *suspended_list = NULL;

static uint32_t enabled_irqs;
static int nesting = 0;
void enter_critical(void)
{
    
    if (nesting == 0)
    {
        /* Remember which IRQs were enabled */
        enabled_irqs = NVIC->ICER[0];
        /* Disable all interrupts except the realtime ones */
        NVIC->ICER[0] = ~REALTIME_IRQS;
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    }
    ++nesting;
}

void exit_critical(void)
{
    --nesting;
    if (nesting == 0)
    {
        NVIC->ISER[0] |= enabled_irqs;
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    }
}
/* TODO: probably need the non-nesting IRQ version of the above too */

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
    task->next_suspended = NULL;
    task->next_running = running_list;
    running_list = task;
    task->next = task_list;
    task_list = task;
    return 0;
}

void yield_from_task(void)
{
    NVIC->ISPR[0] = YIELD_BIT;
}

uint32_t *choose_next_task(uint32_t *current_sp)
{
    task_t *temp, *insert_point;
    
    /* Save the outgoing tasks stack pointer so we can restore it later */
    running_list->sp = current_sp;
    
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
    
    /* Return saved stack pointer */
    return running_list->sp;
}
    
    
__ASM void Yield_IRQHandler(void)
{
    IMPORT choose_next_task
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
    bl choose_next_task
    ldmia r0!, {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    ldmia r0!, {r4-r7}
    msr psp, r0
    ldr r0, =0xFFFFFFFD
    bx r0

    align 4
}

static uint32_t idle_task_stack[48] __ALIGNED(8);
static task_t idle_task;
__NO_RETURN void idle_task_function(void *arg)
{
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

void __NO_RETURN start_rtos(void)
{
    __disable_irq();
    
    /* Create the idle task */
    add_task(idle_task_function, &idle_task, idle_task_stack, sizeof(idle_task_stack) / 4);
    /* The idle task starts with an empty stack, as we call it directly */
    idle_task.sp = idle_task_stack + sizeof(idle_task_stack) / 4;
    /* Enable the yield interrupt, set it to the lowest priority */
    NVIC->IP[YIELD_PRIO_REG] |= YIELD_PRIO;
    NVIC->ISER[0] = YIELD_BIT;
    /* Enable the SysTick interrupt, set it to the lowest priority */
    SCB->SHP[1] |= SYS_IRQ_PRIO << 24;
    NVIC->IP[YIELD_PRIO_REG] = YIELD_PRIO;

    /* Pend the interrupt that will yield to first ready task */
    NVIC->ISPR[0] = YIELD_BIT;
    start_idle_task(idle_task.sp);
    
    /* Should never get here */
    while(1)
    {
        /* Spin */
    }
}
