
#define YIELD_IRQ           31u
#define YIELD_BIT           0x80000000u
#define YIELD_PRIO          ((SYS_IRQ_PRIO << (((YIELD_IRQ) & 3) << 3)))
#define YIELD_PRIO_REG      ((YIELD_IRQ) >> 2)
#define SYS_IRQ_PRIO        192u
#define LOW_IRQ_PRIO        128u
#define MID_IRQ_PRIO        64u
#define HIGH_IRQ_PRIO       0u

#define REALTIME_IRQS       0x04000000
