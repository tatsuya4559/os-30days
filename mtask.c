#include "nasmfunc.h"
#include "dsctbl.h"
#include "mtask.h"

#define  AR_TSS32   0x0089
#define  MAX_TASKS  1000
#define  MAX_TASKS_IN_A_LEVEL  100
#define  MAX_TASK_LEVELS  10
#define  TASK_GDT0  3 // From which GDT index the tasks start

typedef struct {
  int32_t num_running_tasks;
  int32_t current_task_index;
  Task* running_tasks[MAX_TASKS_IN_A_LEVEL];
} TaskLevel;

typedef struct {
  int32_t current_level;
  bool_t should_revise_level;
  TaskLevel levels[MAX_TASK_LEVELS];
  Task tasks0[MAX_TASKS];
} TaskController;

TaskController *taskctl;
Timer *task_timer;

Task *
task_now(void)
{
  TaskLevel *level = &taskctl->levels[taskctl->current_level];
  return level->running_tasks[level->current_task_index];
}

static
void
task_add(Task *task)
{
  TaskLevel *level = &taskctl->levels[task->level];
  if (level->num_running_tasks >= MAX_TASKS_IN_A_LEVEL) {
    // The level is full.
    return;
  }
  level->running_tasks[level->num_running_tasks] = task;
  level->num_running_tasks++;
  task->status = TASK_RUNNING;
}

static
void
task_remove(Task *task)
{
  TaskLevel *level = &taskctl->levels[task->level];
  // Find the task in the list of running tasks.
  int32_t i;
  for (i = 0; i < level->num_running_tasks; i++) {
    if (level->running_tasks[i] == task) {
      break;
    }
  }

  if (i == level->num_running_tasks) {
    // The task is not found.
    return;
  }
  level->num_running_tasks--;
  // Remove the task from the list of running tasks.
  if (i < level->current_task_index) {
    level->current_task_index--;
  }
  for (; i < level->num_running_tasks; i++) {
    level->running_tasks[i] = level->running_tasks[i + 1];
  }

  // Sleep the task.
  task->status = TASK_ALLOCATED;
}

static
void
determine_next_task_level(void)
{
  int32_t i;
  for (i = 0; i < MAX_TASK_LEVELS; i++) {
    if (taskctl->levels[i].num_running_tasks > 0) {
      break;
    }
  }
  taskctl->current_level = i;
  taskctl->should_revise_level = FALSE;
}

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

static
void
task_idle(void)
{
  for (;;) {
    _io_hlt();
  }
}

Task *
task_init(MemoryManager *mem_manager)
{
  SegmentDescriptor *gdt = (SegmentDescriptor *) ADR_GDT;
  taskctl = (TaskController *) memman_alloc_4k(mem_manager, sizeof(TaskController));

  // Initialize tasks
  for (int32_t i = 0; i < MAX_TASKS; i++) {
    taskctl->tasks0[i].status = TASK_UNINITIALIZED;
    // We need to multiply by 8; this is Intel's specification.
    taskctl->tasks0[i].selector = (TASK_GDT0 + i) * 8;
    set_segmdesc(gdt + TASK_GDT0 + i, 103, (int32_t) &taskctl->tasks0[i].tss, AR_TSS32);
  }

  // Initialize task levels
  for (int32_t i = 0; i < MAX_TASK_LEVELS; i++) {
    taskctl->levels[i].num_running_tasks = 0;
    taskctl->levels[i].current_task_index = 0;
  }

  // Initialize main task
  Task *task = task_alloc();
  task->status = TASK_RUNNING;
  task->priority = 2;
  task->level = 0; // Highest level
  task_add(task);
  determine_next_task_level();
  _load_tr(task->selector);
  task_timer = timer_alloc();
  // do not call timer_init because we don't need to set bus and data.
  timer_set_timeout(task_timer, task->priority);

  // Add an idle task as a sentinel.
  // Even if all other tasks get asleep, the idle task keeps the OS from hungging.
  Task *idle = task_alloc();
  idle->tss.esp = memman_alloc_4k(mem_manager, 64 * 1024) + 64 * 1024;
  idle->tss.eip = (int32_t) &task_idle;
  idle->tss.es = 1 * 8;
  idle->tss.cs = 2 * 8;
  idle->tss.ss = 1 * 8;
  idle->tss.ds = 1 * 8;
  idle->tss.fs = 1 * 8;
  idle->tss.gs = 1 * 8;
  task_run(idle, MAX_TASK_LEVELS - 1, 1);

  return task;
}

void
task_run(Task *task, int32_t level, int32_t priority)
{
  // Do not change the level
  if (level < 0) {
    level = task->level;
  }
  // Do not change the priority
  if (priority > 0) {
    task->priority = priority;
  }

  // Change the level of a running task
  if (task->status == TASK_RUNNING && task->level != level) {
    // First, remove the task from the current level.
    // And fall through to add the task to the new level.
    task_remove(task);
  }

  if (task->status != TASK_RUNNING) {
    task->level = level;
    task_add(task);
  }

  taskctl->should_revise_level = TRUE;
}

void
task_switch(void)
{
  TaskLevel *level = &taskctl->levels[taskctl->current_level];
  Task *current = task_now();

  // Round-robin
  level->current_task_index++;
  if (level->current_task_index == level->num_running_tasks) {
    level->current_task_index = 0;
  }

  if (taskctl->should_revise_level) {
    determine_next_task_level();
    level = &taskctl->levels[taskctl->current_level];
  }

  Task *next = task_now();
  // Translate priority to interval.
  // We just use priority as interval(centisecond) for now.
  // i.e. 1 priority runs 10ms per task switch.
  //      2 priority runs 20ms per task switch.
  timer_set_timeout(task_timer, next->priority);
  if (next != current) {
    _farjmp(0, next->selector);
  }
}

void
task_sleep(Task *task)
{
  if (task->status != TASK_RUNNING) {
    return;
  }
  Task *current = task_now();
  task_remove(task);

  // If the current task is the one to be slept, switch the task.
  if (task == current) {
    determine_next_task_level();
    _farjmp(0, task_now()->selector);
  }
}
