#include <stdint.h>
#include <stdio.h>
#include "stm32l031xx.h"
#include "stm32l0xx_ll_utils.h"
#include "stm32l0xx_ll_rcc.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_lpuart.h"
#include "stm32l0xx_ll_usart.h"

#include "m0rtos.h"
#include "float32.h"
#include "util.h"

#define GET_LPUART_BRR_VALUE(UART_CLOCK, BAUDRATE)  (((UART_CLOCK * 16) + (BAUDRATE / 32)) / (BAUDRATE / 16))
#define GET_USART_BRR_VALUE(UART_CLOCK, BAUDRATE)   (((UART_CLOCK) + (BAUDRATE / 2)) / (BAUDRATE))

#define TICKS_PER_SECOND            100

task_t task1, task2, task3, task4;
uint32_t task1_stack[128] __ALIGNED(8);
uint32_t task2_stack[128] __ALIGNED(8);
uint32_t task3_stack[128] __ALIGNED(8);
uint32_t task4_stack[128] __ALIGNED(8);

DECLARE_QUEUE(queue1, 6);
DECLARE_QUEUE(lpuart_outq, 101);

volatile bool safe_to_stop = true;

void task1_main(void *arg)
{
    uint32_t tick_target;
    unsigned i;
    const uint8_t my_data[2] = {'a', 'b'};

#ifdef BENCHMARK
    uint32_t ticks1 = ticks;
    while (ticks1 == ticks);
    ticks1 = ticks;
    for (volatile unsigned i = 0; i < 10000000; ++i);
    uint32_t ticks2 = ticks;
    dprintf("\nLoop took %u ticks\n", ticks2 - ticks1);
#endif
    
    dprintf("\nHello world!\n");
    f32_test();

    tick_target = ticks;
    while(1)
    {
        tick_target += 1000;
        sleep_until(tick_target);
        for (i = 0; i < 4; ++i)
        {
            if (write_queue(&queue1, my_data, 2, 1))
            {
                dprintf("_");
            }
            else
            {
                dprintf("-");
            }
        }
    }
}

void task2_main(void *arg)
{
    bool got;
    unsigned i;
    uint8_t my_data;
    
    while(1)
    {
        for (volatile unsigned i = 0; i < 100000; ++i)
        {
            /* spin */
        }
        for (i = 0; i < 3; ++i)
        {
            sleep(5);
            got = read_queue(&queue1, &my_data, 1, 275);
            dprintf("%c", got ? my_data : 'X');
        }
    }
}

void task3_main(void *arg)
{
    while(1)
    {
        for (volatile unsigned i = 0; i < 50000; ++i)
        {
            /* spin */
        }
        sleep(1);
//        dprintf(":");
    }
}

void task4_main(void *arg)
{
    while(1)
    {
        for (volatile unsigned i = 0; i < 50000; ++i)
        {
            /* spin */
        }
//        dprintf(";");
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

    /* Set PB6 and PB7 pins for "alternate function 0" (USART2) */
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
    
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_SetPriority(USART2_IRQn, LOW_IRQ_PRIORITY);
}

void LPUART1_IRQHandler(void)
{
    bool got_data;
    uint8_t c;
    
    if (LPUART1->ISR & USART_ISR_TXE)
    {
        got_data = read_queue_irq(&lpuart_outq, &c, 1);
        if (got_data)
        {
            LPUART1->TDR = c;
        }
        else
        {
            LPUART1->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

void init_lpuart1(void)
{
    /* Select APB1 clock for LPUART1 and GPIOA */
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);
    RCC->APB1ENR |= RCC_APB1ENR_LPUART1EN;
    RCC->IOPENR |= RCC_IOPENR_IOPAEN;

    /* Set PA2 and PA3 pins for "alternate function 6" (LPUART1) */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_3, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_6);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_3, LL_GPIO_AF_6);
    
    /* Initialise LPUART1 */
    LPUART1->CR1 = USART_CR1_TE | USART_CR1_RE;
    LPUART1->BRR = GET_LPUART_BRR_VALUE(32000000, 115200);
    LL_LPUART_Enable(LPUART1);
    
    NVIC_EnableIRQ(LPUART1_IRQn);
    NVIC_SetPriority(LPUART1_IRQn, LOW_IRQ_PRIORITY);
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
 * Configure the tick timer
 * NVIC stuff (interrupt priority and enable) is done in start_rtos().
 */
void init_lptim(unsigned clocks_per_tick)
{
#ifndef NDEBUG
    /* Stop timer when debugger stops */
    RCC->APB2ENR |= RCC_APB2ENR_DBGEN;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_LPTIMER_STOP;
#endif
    
    LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSI);
    RCC->APB1ENR |= RCC_APB1ENR_LPTIM1EN;
    LPTIM1->IER = LPTIM_IER_ARRMIE;
    LPTIM1->CR  = LPTIM_CR_ENABLE;
    LPTIM1->ARR = (LPTIM1->ARR & 0xffff0000) | clocks_per_tick;
    LPTIM1->CR |= LPTIM_CR_CNTSTRT;
}

void init_low_power(void)
{
    /* Enable the clock to the power controller */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    
    /*
     * We also set LPDSR here, despite the Errata ("STM32L031x4/6 silicon limitations") 
     * because DBGMCU->IDCODE shows we have (at least) revision X parts.
     */
    PWR->CR |= PWR_CR_LPSDSR;
    
    /* Wake from Stop using HSI clock */
    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
}

void idle_low_power_hook(void)
{
    /* First a quick check without masking interrupts, so we don't upset the realtime stuff */
    if (safe_to_stop)
    {
        __disable_irq();
        /* Now re-check with interrupts masked, before we finally decide to enter Stop mode */
        if (safe_to_stop)
        {
            /* Disable the ADC */
            //ADC1->CR |= ADC_CR_ADDIS;
            /* Set the M0 to go into Deep Sleep */
            SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
            /* Disable everything we can, and wake up fast */
            PWR->CR |= PWR_CR_ULP | PWR_CR_FWU;
            __DSB();
            __WFI();
            PWR->CR &= ~(PWR_CR_ULP | PWR_CR_FWU);
            SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
            /* Enable the ADC */
            //ADC1->CR |= ADC_CR_ADEN;
            
#if SYS_CLOCK_HZ == 32000000            
            /* Re-configure and enable the PLL at 32MHz (HSI x 4 / 2) */
            LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, RCC_CFGR_PLLMUL4, RCC_CFGR_PLLDIV2);
            LL_RCC_PLL_Enable();
            while (!LL_RCC_PLL_IsReady())
            {
                /* Wait for PLL to be ready */
            }
            /* Switch SysClk over to the 32MHz PLL output */
            LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
            while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
            {
                /* Wait for system clock to switch to PLL */
            }
#endif
        }
        __enable_irq();
    }
    else
    {
        __WFI();
    }
}

int outbyte(int c)
{
    uint8_t data = (uint8_t)c;
    
    write_queue(&lpuart_outq, &data, 1, 0);
    LPUART1->CR1 |= USART_CR1_TXEIE;        /* Should be safe despite lack of critical section */
    return 0;
}

void __NO_RETURN main(void)
{
    LL_UTILS_PLLInitTypeDef pll_init = {RCC_CFGR_PLLMUL4, RCC_CFGR_PLLDIV2};
    LL_UTILS_ClkInitTypeDef clk_init = {RCC_CFGR_HPRE_DIV1, RCC_CFGR_PPRE1_DIV1, RCC_CFGR_PPRE2_DIV1};
    
    /* Switch to 32 MHz clock */
    LL_PLL_ConfigSystemClock_HSI(&pll_init, &clk_init);

    /* Enable prefetch (but not pre-read unless you're doing lots of queueing) */
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    init_low_power();
    init_lpuart1();
    //init_usart2();

    add_task(task4_main, &task4, task4_stack, sizeof(task4_stack) / 4, 2);
    add_task(task3_main, &task3, task3_stack, sizeof(task3_stack) / 4, 2);
    add_task(task2_main, &task2, task2_stack, sizeof(task2_stack) / 4, 1);
    add_task(task1_main, &task1, task1_stack, sizeof(task1_stack) / 4, 0);
    
    init_lptim(37000 / TICKS_PER_SECOND);
    start_rtos();
    
    while(1)
    {
        /* Spin */
    }
}
