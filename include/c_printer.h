#include "c_types.h"

#ifndef C_PRINTER_H
#define C_PRINTER_H

#define SPACES_PER_INDENT 2

struct GeneratedLine_T {
  char text[CODELINE_LEN];
  struct GeneratedLine_T* next;
};



void print_function(struct Function_T * function);
char * print_function_to_buffer(struct Function_T * function);
void print_function_limited(struct Function_T * function);

#endif // C_PRINTER_H
