#pragma once

#define MAX_TASKS 16
typedef void (*TaskFn)();

void scheduler_init();
bool scheduler_register(TaskFn fn);
void scheduler_run();
