#include <stdint.h>
#include "stm32l031xx.h"
#include "stm32l0xx_ll_utils.h"

#include "task.h"

task_t task1, task2;
uint32_t task1_stack[64] __ALIGNED(8);
uint32_t task2_stack[64] __ALIGNED(8);

void task1_main(void *arg)
{
    while(1)
    {
        enter_critical();
        exit_critical();
        for (volatile unsigned i = 0; i < 1000; ++i)
        {
            /* spin */
        }
        yield_from_task();
    }
}

void task2_main(void *arg)
{
    while(1)
    {
        for (volatile unsigned i = 0; i < 1000; ++i)
        {
            /* spin */
        }
        yield_from_task();
    }
}

void __NO_RETURN main(void)
{
    LL_UTILS_PLLInitTypeDef pll_init = {RCC_CFGR_PLLMUL4, RCC_CFGR_PLLDIV2};
    LL_UTILS_ClkInitTypeDef clk_init = {RCC_CFGR_HPRE_DIV1, RCC_CFGR_PPRE1_DIV1, RCC_CFGR_PPRE2_DIV1};
    LL_PLL_ConfigSystemClock_HSI(&pll_init, &clk_init);

    add_task(task2_main, &task2, task2_stack, sizeof(task2_stack) / 4);
    add_task(task1_main, &task1, task1_stack, sizeof(task1_stack) / 4);
    start_rtos(32000);
    
    while(1)
    {
        /* Spin */
    }
}
