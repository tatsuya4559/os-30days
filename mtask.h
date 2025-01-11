#pragma once

#include "memory.h"
#include "timer.h"

typedef struct {
  int32_t backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
  // 32bit registers
  int32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  // 16bit registers
  int32_t es, cs, ss, ds, fs, gs;
  int32_t ldtr, iomap;
} TaskStatusSegment;

typedef enum {
  TASK_UNINITIALIZED,
  TASK_ALLOCATED,
  TASK_RUNNING,
} TaskStatus;

typedef struct {
  int32_t selector; // GDT index
  int32_t level; // level represents a priority group
  int32_t priority;
  TaskStatus status;
  FIFO fifo;
  TaskStatusSegment tss;
} Task;

Task *task_init(MemoryManager *mem_manager);
Task *task_alloc(void);
/**
 * Get the current running task.
 */
Task *task_now(void);
/**
 * Run the task with the given priority.
 * If the given task is already running, only its priority is updated.
 *
 * @param task The task to run.
 * @param priority The priority of the task. If the priority is 0 or less, the priority is not updated.
 */
void task_run(Task *task, int32_t level, int32_t priority);
void task_switch(void);
void task_sleep(Task *task);

extern Timer *task_timer;
