#include <stdint.h>
#include "stm32l031xx.h" 
#include "task.h"

task_t task1, task2;
uint32_t task1_stack[64] __ALIGNED(8);
uint32_t task2_stack[64] __ALIGNED(8);

void task1_main(void *arg)
{
    while(1)
    {
        yield_from_task();
    }
}

void task2_main(void *arg)
{
    while(1)
    {
        yield_from_task();
    }
}

void __NO_RETURN main(void)
{
    add_task(task2_main, &task2, task2_stack, sizeof(task2_stack) / 4);
    add_task(task1_main, &task1, task1_stack, sizeof(task1_stack) / 4);
    start_rtos();
    
    while(1)
    {
        /* Spin */
    }
}
