
#ifndef _TASK_H
#define _TASK_H

#include <stdint.h>

struct task_s
{
    struct task_s *next;
    struct task_s *next_running;
    struct task_s *next_suspended;
    uint32_t *stack, *sp;
};

typedef struct task_s task_t;

typedef void (task_function_t)(void *);

extern int add_task(task_function_t *task_function, task_t *task, uint32_t *stack, unsigned stack_words);
extern __NO_RETURN void start_rtos(void);
extern void yield_from_task(void);

#endif
