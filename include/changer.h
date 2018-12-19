#include "c_types.h"

#ifndef CHANGER_H
#define CHANGER_H

enum ChangeType_T {
  CHANGE_TYPE_INSERT_CODELINE,
  CHANGE_TYPE_REMOVE_CODELINE,
  CHANGE_TYPE_INSERT_VARIABLE,
  CHANGE_TYPE_REMOVE_VARIABLE,
  CHANGE_TYPE_ALTER_CONSTANT,
  CHANGE_TYPE_ALTER_ARGUMENT,
  CHANGE_TYPE_ALTER_ASSIGNING
};

struct Change_T {
  enum ChangeType_T type;
  
  struct CodeLine_T * codeline;
  struct Variable_T * variable;
  
  struct CodeLine_T * next_codeline; // Used to position inserts
  
  struct Change_T * next;
};

#endif /* CHANGER_H */
