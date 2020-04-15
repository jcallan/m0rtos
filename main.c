#include <stdint.h>
#include <stdio.h>
#include "stm32l031xx.h"
#include "stm32l0xx_ll_utils.h"
#include "stm32l0xx_ll_rcc.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_lpuart.h"
#include "stm32l0xx_ll_usart.h"

#include "util.h"
#include "config.h"
#include "task.h"

#define GET_LPUART_BRR_VALUE(UART_CLOCK, BAUDRATE)  (((UART_CLOCK * 16) + (BAUDRATE / 32)) / (BAUDRATE / 16))
#define GET_USART_BRR_VALUE(UART_CLOCK, BAUDRATE)   (((UART_CLOCK) + (BAUDRATE / 2)) / (BAUDRATE))

task_t task1, task2;
uint32_t task1_stack[64] __ALIGNED(8);
uint32_t task2_stack[64] __ALIGNED(8);


void task1_main(void *arg)
{
    while(1)
    {
        sleep(10);
        for (volatile unsigned i = 0; i < 100000; ++i)
        {
            /* spin */
        }
        dprintf("_");
    }
}

void task2_main(void *arg)
{
    while(1)
    {
        for (volatile unsigned i = 0; i < 100000; ++i)
        {
            /* spin */
        }
        sleep(1);
        dprintf("2");
    }
}

void init_usart2(void)
{
    //LL_USART_ClockInitTypeDef usart2_clk_init;
    //LL_USART_InitTypeDef usart2_init;

    /* Select APB1 clock for USART2 and GPIOB */
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->IOPENR |= RCC_IOPENR_IOPBEN;

    /* Set PB6 and PB7 pins for "alternate function" (USART2) */
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOB, LL_GPIO_PIN_6, LL_GPIO_AF_0);
    LL_GPIO_SetAFPin_0_7(GPIOB, LL_GPIO_PIN_7, LL_GPIO_AF_0);
    
    /* Initialise USART2 */
//    LL_USART_ClockStructInit(&usart2_clk_init);
//    usart2_clk_init.ClockOutput = LL_USART_CLOCK_DISABLE;
//    LL_USART_ClockInit(USART2, &usart2_clk_init);
//    LL_USART_StructInit(&usart2_init);
//    usart2_init.BaudRate = 115200U;
//    LL_USART_Init(USART2, &usart2_init);
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE;
    USART2->BRR = GET_USART_BRR_VALUE(32000000, 115200);
    LL_USART_Enable(USART2);
}

void init_lpusart1(void)
{
    /* Select APB1 clock for LPUART1 and GPIOA */
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);
    RCC->APB1ENR |= RCC_APB1ENR_LPUART1EN;
    RCC->IOPENR |= RCC_IOPENR_IOPAEN;

    /* Set PB6 and PB7 pins for "alternate function" (USART2) */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_3, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_6);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_3, LL_GPIO_AF_6);
    
    /* Initialise LPUART1 */
    LPUART1->CR1 = USART_CR1_TE | USART_CR1_RE;
    LPUART1->BRR = GET_LPUART_BRR_VALUE(32000000, 115200);
    LL_LPUART_Enable(LPUART1);
}

/*
 * Timer tick interrupt handler
 */
void LPTIM1_IRQHandler(void)
{
    LPTIM1->ICR = LPTIM_IER_ARRMIE;
    tick();
}

/*
 * Configure the timer to tick at 1000Hz
 * NVIC stuff (interrupt priority and enable) is done in start_rtos().
 */
void init_lptim(unsigned clocks_per_tick)
{
    LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_PCLK1);
    RCC->APB1ENR |= RCC_APB1ENR_LPTIM1EN;
    LPTIM1->IER = LPTIM_IER_ARRMIE;
    LPTIM1->CR  = LPTIM_CR_ENABLE;
    LPTIM1->ARR = (LPTIM1->ARR & 0xffff0000) | clocks_per_tick;
    LPTIM1->CR |= LPTIM_CR_CNTSTRT;
}

int outbyte(int c)
{
    while (!(LPUART1->ISR & LL_LPUART_ISR_TXE))
    {
        /* Wait */
    }
    LL_USART_TransmitData8(LPUART1, c);
    return 0;
}

void __NO_RETURN main(void)
{
    LL_UTILS_PLLInitTypeDef pll_init = {RCC_CFGR_PLLMUL4, RCC_CFGR_PLLDIV2};
    LL_UTILS_ClkInitTypeDef clk_init = {RCC_CFGR_HPRE_DIV1, RCC_CFGR_PPRE1_DIV1, RCC_CFGR_PPRE2_DIV1};
    
    /* Switch to 32 MHz clock */
    LL_PLL_ConfigSystemClock_HSI(&pll_init, &clk_init);
    
    init_lpusart1();
    //init_usart2();
    dprintf("Hello world!\n");

    add_task(task2_main, &task2, task2_stack, sizeof(task2_stack) / 4, 0);
    add_task(task1_main, &task1, task1_stack, sizeof(task1_stack) / 4, 0);
    
    init_lptim(32000);
    start_rtos(32000);
    
    while(1)
    {
        /* Spin */
    }
}
