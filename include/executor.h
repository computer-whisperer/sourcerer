#include "c_types.h"

#ifndef EXECUTOR_H
#define EXECUTOR_H

#define PUTCHAR_BUFF_LEN 50

struct MemoryDMZ_T {
  void * data_start;
  void * data_end;
  char writable;
  struct MemoryDMZ_T * next;
};

int putchar_i;
char putchar_buff[PUTCHAR_BUFF_LEN];

int execute_function(struct Function_T * function, struct MemoryDMZ_T * dmz, void * arguments_data[], size_t arguments_size[], void * return_data, size_t return_size, int execution_limit);

#endif // EXECUTOR_H
