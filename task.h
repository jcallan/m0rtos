
#ifndef _TASK_H
#define _TASK_H

#include <stdint.h>
#include <stdbool.h>

struct semaphore_s;

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
    struct semaphore_s *wait_for;
};

struct semaphore_s
{
    int value;
    int max;
    struct task_s *blocked_list;
};

typedef struct task_s task_t;
typedef struct semaphore_s semaphore_t;
typedef void (task_function_t)(void *);

extern volatile uint32_t ticks;

extern void enter_critical(void);
extern void exit_critical(void);

extern bool wait_semaphore(semaphore_t *sem, unsigned amount, int ticks_to_wait);
extern bool signal_semaphore(semaphore_t *sem, unsigned amount, int ticks_to_wait);

extern void sleep(uint32_t ticks_to_sleep);
extern void sleep_until(uint32_t target_ticks);

extern int add_task(task_function_t *task_function, task_t *task, uint32_t *stack,
                    unsigned stack_words, unsigned priority);
extern __NO_RETURN void start_rtos(uint32_t cpu_clocks_per_tick);
extern void yield(void);
extern void tick(void);

#endif
