
#ifndef _TASK_H
#define _TASK_H

#include <stdint.h>

struct task_s
{
    struct task_s *next_task;
    struct task_s *next_runnable;
    struct task_s *next_suspended;
    uint32_t *stack;
    uint32_t *sp;
    unsigned stack_words;
    unsigned priority;
    unsigned flags;
    uint32_t wait_until;
    void *wait_for;
};

typedef struct task_s task_t;

typedef void (task_function_t)(void *);


extern volatile uint32_t ticks;

extern void enter_critical(void);
extern void exit_critical(void);

extern void sleep(uint32_t ticks_to_sleep);
extern int add_task(task_function_t *task_function, task_t *task, uint32_t *stack,
                    unsigned stack_words, unsigned priority);
extern __NO_RETURN void start_rtos(uint32_t cpu_clocks_per_tick);
extern void yield(void);
extern void tick(void);

#endif
