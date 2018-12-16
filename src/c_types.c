#include <stdlib.h>
#include <stdio.h>

#include "c_types.h"

int get_reference_count(struct Variable_T * var, struct Function_T * function) {
  struct CodeLine_T * codeline = function->first_codeline;
  int count = 0;
  while (codeline) {
    if (codeline->assigned_variable && codeline->assigned_variable == var)
      count++;
    for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
      if (!codeline->function_arguments[i])
        break;
      if (codeline->function_arguments[i] == var)
        count++;
    }
    codeline = codeline->next;
  }
  return count;
}

void check_references(struct Function_T * function){
  struct Variable_T * variable = function->first_var;
  while (variable) {
    int real_ref_count = get_reference_count(variable, function);
    if (real_ref_count != variable->references) {
      printf("This is a problem.\n");
    }
    variable = variable->next;
  }
}

struct Function_T * build_context() {
  struct DataType_T dt_i = {BASICTYPE_INT, 0};
  struct DataType_T dt_c = {BASICTYPE_CHAR, 0};
  struct Variable_T * int_var = malloc(sizeof(struct Variable_T));
  struct Variable_T * char_var = malloc(sizeof(struct Variable_T));
  int_var->data_type = dt_i;
  char_var->data_type = dt_c;
  int func_count = 5;
  struct Function_T * funcs = malloc(sizeof(struct Function_T)*func_count);
  funcs[0] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="+", .return_datatype=dt_i, .function_arguments={int_var, int_var, NULL}};
  funcs[1] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="-", .return_datatype=dt_i, .function_arguments={int_var, int_var, NULL}};
  funcs[2] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="*", .return_datatype=dt_i, .function_arguments={int_var, int_var, NULL}};
  funcs[3] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="/", .return_datatype=dt_i, .function_arguments={int_var, int_var, NULL}};
  funcs[4] = (struct Function_T){.type=FUNCTION_TYPE_BUILTIN, .name="putchar", .return_datatype=dt_c, .function_arguments={char_var, NULL}};
  for (int i = 0; i < func_count; i++) {
    funcs[i].next = funcs + i + 1;
    funcs[i].prev = funcs + i - 1;
  }
  funcs[0].prev = NULL;
  funcs[func_count-1].next = NULL;
  return funcs;
}
