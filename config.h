
#define YIELD_IRQ           31u
#define YIELD_BIT           (1u << (YIELD_IRQ))
#define YIELD_PRIO          ((SYS_IRQ_PRIO << (((YIELD_IRQ) & 3) << 3)))
#define YIELD_PRIO_REG      ((YIELD_IRQ) >> 2)

#define TIMER_IRQ           LPTIM1_IRQn
#define TIMER_BIT           (1u << (TIMER_IRQ))
#define TIMER_PRIO          ((SYS_IRQ_PRIO << (((TIMER_IRQ) & 3) << 3)))
#define TIMER_PRIO_REG      ((TIMER_IRQ) >> 2)

#define SYS_IRQ_PRIO        192u
#define LOW_IRQ_PRIO        128u
#define MID_IRQ_PRIO        64u
#define HIGH_IRQ_PRIO       0u

#define REALTIME_IRQS       0x04000000
