
#define YIELD_IRQ           31u
#define YIELD_BIT           (1u << (YIELD_IRQ))
#define YIELD_PRIO          ((SYS_IRQ_PRIO << (((YIELD_IRQ) & 3) * 8)))
#define YIELD_PRIO_REG      ((YIELD_IRQ) >> 2)

#define TICK_IRQ            LPTIM1_IRQn
#define TICK_BIT            (1u << (TICK_IRQ))
#define TICK_PRIO           ((SYS_IRQ_PRIO << (((TICK_IRQ) & 3) * 8)))
#define TICK_PRIO_REG       ((TICK_IRQ) >> 2)

#define SYS_IRQ_PRIO        192u
#define LOW_IRQ_PRIO        128u
#define MID_IRQ_PRIO        64u
#define HIGH_IRQ_PRIO       0u

#define SYS_IRQ_PRIORITY    3u
#define LOW_IRQ_PRIORITY    2u
#define MID_IRQ_PRIORITY    1u
#define HIGH_IRQ_PRIORITY   0u

/* Set TIM22 to be a real-time non-M0rtos interrupt */
#define REALTIME_IRQS       0x00400000

/* Set UARTs to be lower priority (IRQs 28 and 29) */
#define LOW_PRIO_IRQS       0x30000000 

#define NUM_TASK_PRIOS      4
