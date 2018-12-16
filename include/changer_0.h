#include "c_types.h"

#ifndef CHANGER_0_H
#define CHANGER_0_H

#define CHANGE_RELATED_VARS_LEN 4
#define CHANGE_CODELINES_LEN 4

enum ChangeType_T {
  CHANGE_TYPE_INSERT,
  CHANGE_TYPE_REMOVE,
  CHANGE_TYPE_ALTER_CONSTANT,
  CHANGE_TYPE_SWITCH_ARGUMENT,
};

struct Change_T {
  enum ChangeType_T type;
  struct CodeLine_T * codelines[CHANGE_CODELINES_LEN];
  struct Change_T * inverse;
  struct Variable_T * related_vars[CHANGE_RELATED_VARS_LEN];
  struct Variable_T * specific_var;
  int value;
  int applied;
};

void train_code_v2();

#endif // CHANGER_0_H
