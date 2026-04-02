#include "scheduler.h"

static TaskFn task_table[MAX_TASKS];
static int    task_count = 0;

void scheduler_init() {
    task_count = 0;
    for (int i = 0; i < MAX_TASKS; i++) task_table[i] = nullptr;
}

bool scheduler_register(TaskFn fn) {
    if (!fn) return false;
    if (task_count >= MAX_TASKS) return false;
    task_table[task_count++] = fn;
    return true;
}

void scheduler_run() {
    for (int i = 0; i < task_count; i++) {
        if (task_table[i]) task_table[i]();
    }
}
