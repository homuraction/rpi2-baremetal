#ifndef __TASK_H__
#define __TASK_H__

#include "stdint.h"
#include "cpu.h"

typedef struct context {
        uint32_t r[12]; // work register
        uint32_t sp;    // stack pointer
        uint32_t lr;    // return address
        uint32_t pc;    // program counter
        uint32_t psr;   // program status register
} context_t;

typedef enum {
        TASK_STATE_READY     = 1,
        TASK_STATE_RUNNING   = 2,
        TASK_STATE_WAITING   = 4,
        TASK_STATE_FINISHED  = 8
} task_state_t;

#define INIT_MAP            0
#define RUNNING_MAP         1
#define WAIT_INIT_MAP       2
#define RESERVED_MAP(cpuid) (WAIT_INIT_MAP + (cpuid))

typedef struct task {
        task_state_t  state;
        uint32_t      pri;
        uint32_t      mapid;
        uint32_t      ttba;
        uint32_t      asid;
        struct task  *prev;
        struct task  *next;
        context_t    *context;
} TCB;

typedef struct task_map {
        uint32_t  state;
        TCB      *task;
} task_map_t;

#define CHECK_ID(id, limit) (if ((id) >= (limit)) return -1)

void create_task(TCB **task_head, void *func, uint32_t pri, uint32_t *lock);
void start_task(TCB *task, uint32_t *lock, task_map_t *tsk_alc_map);
void end_task(TCB *task, uint32_t *lock, task_map_t *tsk_alc_map);
void destroy_task(TCB *task, uint32_t *lock, task_map_t *tsk_alc_map);
void destroy_core_tasks(void);

void move_task_spaces(void);
void move_task_space(task_map_t *task_map);

#endif /* __TASK_H__ */
