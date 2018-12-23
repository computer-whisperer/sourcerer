#include "c_types.h"

#ifndef CHANGES_H
#define CHANGES_H

enum ChangeType_T {
  CHANGE_TYPE_INSERT_CODELINE,
  CHANGE_TYPE_REMOVE_CODELINE,
  CHANGE_TYPE_INSERT_VARIABLE,
  CHANGE_TYPE_REMOVE_VARIABLE,
  CHANGE_TYPE_ALTER_CONSTANT,
  CHANGE_TYPE_ALTER_ARGUMENT,
  CHANGE_TYPE_ALTER_ASSIGNED_VARIABLE,
  CHANGE_TYPE_INVALID,
};

struct Change_T {
  enum ChangeType_T type;
  
  struct Function_T * function;
  
  struct CodeLine_T * codeline;
  struct Variable_T * variable;
  struct DataType_T * data_type;
  
  union ConstantValue_T constant;
  int argument_index;
  
  struct CodeLine_T * next_codeline; // Used to position inserts
  
  struct Change_T * next;
};

struct Change_T * apply_change(struct Change_T * change);
void free_change(struct Change_T * change);

#endif /* CHANGES_H */
