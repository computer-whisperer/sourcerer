#include "executor.h"

#ifndef TASKS_H
#define TASKS_H

struct TaskInputs_T {
  int run_index;
  int a;
  int b;
  int c;
  char d;
};

struct TaskOutputs_T {
  char * putchar_text;
  int putchar_text_len;
  int ret;
};

typedef int (*TaskJudge_T)(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs);

int task_judge_hello_world(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs);

int task_judge_flip_number(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs);

int task_judge_count(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs);

int task_judge_calculator(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs);

int task_judge_expfit(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs);


#endif /* TASKS_H */
