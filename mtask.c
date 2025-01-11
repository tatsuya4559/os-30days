#include "nasmfunc.h"
#include "dsctbl.h"
#include "mtask.h"

#define  AR_TSS32   0x0089
#define  MAX_TASKS  1000
#define  TASK_GDT0  3 // From which GDT index the tasks start
#define  TASK_SWITCH_INTERVAL 2 // 20ms

typedef struct {
  int32_t num_running_tasks;
  int32_t current_task_index;
  Task *running_tasks[MAX_TASKS];
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
  task->priority = 2;
  taskctl->num_running_tasks = 1;
  taskctl->current_task_index = 0;
  taskctl->running_tasks[0] = task;
  _load_tr(task->selector);

  task_timer = timer_alloc();
  // do not call timer_init because we don't need to set bus and data.
  timer_set_timeout(task_timer, TASK_SWITCH_INTERVAL);

  return task;
}

void
task_run(Task *task, int32_t priority)
{
  if (priority > 0) {
    task->priority = priority;
  }
  if (task->status == TASK_RUNNING) {
    return;
  }
  task->status = TASK_RUNNING;
  taskctl->running_tasks[taskctl->num_running_tasks] = task;
  taskctl->num_running_tasks++;
}

static
uint32_t
priority_to_interval(int32_t priority)
{
  // Translate priority to interval.
  // We just use priority as interval(centisecond) for now.
  // i.e. 1 priority runs 10ms per task switch.
  //      2 priority runs 20ms per task switch.
  return (uint32_t) priority;
}

void
task_switch(void)
{
  // Round-robin
  taskctl->current_task_index++;
  if (taskctl->current_task_index == taskctl->num_running_tasks) {
    taskctl->current_task_index = 0;
  }
  Task *next = taskctl->running_tasks[taskctl->current_task_index];
  timer_set_timeout(task_timer, priority_to_interval(next->priority));
  if (taskctl->num_running_tasks <= 1) {
    return;
  }
  _farjmp(0, next->selector);
}

void
task_sleep(Task *task)
{
  if (task->status != TASK_RUNNING) {
    return;
  }
  if (taskctl->num_running_tasks <= 1) {
    return;
  }

  // Find the task in the list of running tasks.
  int32_t i;
  for (i = 0; i < taskctl->num_running_tasks; i++) {
    if (taskctl->running_tasks[i] == task) {
      break;
    }
  }

  // Remove the task from the list of running tasks.
  if (i < taskctl->current_task_index) {
    taskctl->current_task_index--;
  }
  taskctl->num_running_tasks--;
  for (; i < taskctl->num_running_tasks; i++) {
    taskctl->running_tasks[i] = taskctl->running_tasks[i + 1];
  }

  // Sleep the task.
  task->status = TASK_ALLOCATED;

  // If the current task is the one to be slept, switch the task.
  Task *current = taskctl->running_tasks[taskctl->current_task_index];
  if (task == current) {
    // task_switch won't switch if there is only one running task.
    // So, we need to call _farjmp directly.
    // task_switch();
    if (taskctl->current_task_index >= taskctl->num_running_tasks) {
      taskctl->current_task_index = 0;
    }
    _farjmp(0, taskctl->running_tasks[taskctl->current_task_index]->selector);
  }
}
