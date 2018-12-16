#include "c_types.h"

#ifndef EXECUTOR_H
#define EXECUTOR_H

#define PUTCHAR_BUFF_LEN 50

union ExecutorValue_T {
  char c;
  int i;
  void * ptr;
};

struct ExecutorVariableFrame_T {
  union ExecutorValue_T value;
  struct ExecutorVariableFrame_T * next;
};

struct ExecutorPerformanceReport_T {
  int lines_executed;
  int did_return;
  int uninitialized_vars_referenced;
};

int putchar_i;
char putchar_buff[PUTCHAR_BUFF_LEN];

union ExecutorValue_T execute_function(struct Function_T * function, union ExecutorValue_T * args, struct ExecutorPerformanceReport_T * report, int execution_limit);

#endif // EXECUTOR_H
