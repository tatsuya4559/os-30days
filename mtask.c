#include "nasmfunc.h"
#include "dsctbl.h"
#include "mtask.h"

#define  AR_TSS32   0x0089
#define  MAX_TASKS  1000
#define  TASK_GDT0  3 // From which GDT index the tasks start

typedef struct {
  int32_t num_running_tasks;
  int32_t current_task_index;
  Task *tasks[MAX_TASKS];
  Task tasks0[MAX_TASKS];
} TaskController;

TaskController *taskctl;
Timer *task_timer;

Task *
task_alloc(void)
{
  for (int32_t i = 0; i < MAX_TASKS; i++) {
    if (taskctl->tasks0[i].status == TASK_UNINITIALIZED) {
      Task *task = &taskctl->tasks0[i];
      task->status = TASK_ALLOCATED;
  /* int32_t task_b_esp = memman_alloc_4k(mem_manager, 64 * 1024) + 64 * 1024 - 8; */
      /* task->tss.eip = (int32_t) &task_b_main; */
      /* task->tss.esp = task_b_esp; */
      task->tss.eflags = 0x00000202; // IF = 1
      task->tss.eax = 0;
      task->tss.ecx = 0;
      task->tss.edx = 0;
      task->tss.ebx = 0;
      task->tss.ebp = 0;
      task->tss.esi = 0;
      task->tss.edi = 0;
      task->tss.es = 0;
      task->tss.cs = 0;
      task->tss.ss = 0;
      task->tss.ds = 0;
      task->tss.fs = 0;
      task->tss.gs = 0;
      task->tss.ldtr = 0;
      task->tss.iomap = 0x40000000;
      return task;
    }
  }
  return NULL;
}

Task *
task_init(MemoryManager *mem_manager)
{
  SegmentDescriptor *gdt = (SegmentDescriptor *) ADR_GDT;
  taskctl = (TaskController *) memman_alloc_4k(mem_manager, sizeof(TaskController));
  for (int32_t i = 0; i < MAX_TASKS; i++) {
    taskctl->tasks0[i].status = TASK_UNINITIALIZED;
    // We need to multiply by 8; this is Intel's specification.
    taskctl->tasks0[i].selector = (TASK_GDT0 + i) * 8;
    set_segmdesc(gdt + TASK_GDT0 + i, 103, (int32_t) &taskctl->tasks0[i].tss, AR_TSS32);
  }
  Task *task = task_alloc();
  task->status = TASK_RUNNING;
  taskctl->num_running_tasks = 1;
  taskctl->current_task_index = 0;
  taskctl->tasks[0] = task;
  _load_tr(task->selector);

  task_timer = timer_alloc();
  timer_set_timeout(task_timer, 2);

  return task;
}

void
task_run(Task *task)
{
  task->status = TASK_RUNNING;
  taskctl->tasks[taskctl->num_running_tasks] = task;
  taskctl->num_running_tasks++;
}

void
task_switch(void)
{
  timer_set_timeout(task_timer, 2);
  if (taskctl->num_running_tasks <= 1) {
    return;
  }
  // Round-robin
  taskctl->current_task_index++;
  if (taskctl->current_task_index == taskctl->num_running_tasks) {
    taskctl->current_task_index = 0;
  }
  _farjmp(0, taskctl->tasks[taskctl->current_task_index]->selector);
}
