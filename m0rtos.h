
#ifndef _TASK_H
#define _TASK_H

#include <stdint.h>
#include <stdbool.h>
#include <cmsis_armcc.h>
#include "m0rtos_config.h"

struct queue_s;

struct task_s
{
    struct task_s *next_task;
    struct task_s *next_runnable;
    struct task_s *next_suspended;
    struct task_s *next_blocked;
    uint32_t *stack;
    uint32_t *sp;
    unsigned stack_words;
    unsigned priority;
    unsigned flags;
    uint32_t wait_until;
    struct queue_s *wait_for;
};

struct queue_s
{
    unsigned in, out, max;
    uint8_t *data;
    struct task_s *blocked_list;
};

typedef struct task_s task_t;
typedef struct queue_s queue_t;
typedef void (task_function_t)(void *);

#define DECLARE_QUEUE(queue_name, length_plus_one)  \
static uint8_t queue_name##_data_[length_plus_one]; \
queue_t queue_name = {0, 0, length_plus_one, queue_name##_data_, NULL}

extern volatile uint32_t ticks;

extern void enter_critical(void);
extern void exit_critical(void);

extern bool read_queue(queue_t *q, uint8_t *buf, unsigned amount, int ticks_to_wait);
extern bool write_queue(queue_t *q, const uint8_t *buf, unsigned amount, int ticks_to_wait);
extern bool read_queue_irq(queue_t *q, uint8_t *buf, unsigned amount);
extern bool write_queue_irq(queue_t *q, const uint8_t *buf, unsigned amount);


extern void sleep(uint32_t ticks_to_sleep);
extern void sleep_until(uint32_t target_ticks);

extern int add_task(task_function_t *task_function, task_t *task, uint32_t *stack,
                    unsigned stack_words, unsigned priority);
extern __NO_RETURN void start_rtos(uint32_t cpu_clocks_per_tick);
extern void yield(void);
extern void wake_task_realtime(task_t *task);
extern void tick(void);

#endif
