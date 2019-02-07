#include "membox.h"

#ifndef C_TYPES_H
#define C_TYPES_H

#define FUNCTION_ARG_COUNT 8
#define NAME_LEN 32
#define STRUCT_MEMBER_LEN 8
#define CODELINE_LEN 128

#define MAX_POINTER_DEPTH 3


struct ExecutorVariableFrame_T {
  void * data_start;
  struct ExecutorVariableFrame_T * next;
};

struct ExecutorPerformanceReport_T {
  int lines_executed;
  int segfaults_attempted;
  int times_not_returned;
};


struct StructDataTypeMember_T {
  char name[NAME_LEN];
  struct DataType_T * data_type;
};

enum DataTypeType_T {
  DATATYPETYPE_PRIMITIVE,
  DATATYPETYPE_POINTER,
  DATATYPETYPE_STRUCT,
};

struct DataType_T {
  enum DataTypeType_T type;
  char name[NAME_LEN];
  size_t size;
  struct StructDataTypeMember_T struct_members[STRUCT_MEMBER_LEN];
  
  // Pointer links
  struct DataType_T * pointed_from;
  struct DataType_T * pointing_to;
  int pointer_degree;
  
  // Linked list
  struct Environment_T * environment;
  struct DataType_T * next;
  struct DataType_T * prev;
};

struct Variable_T {
  char name[NAME_LEN];
  struct DataType_T * data_type;
  short is_arg;
  
  int references; // Reference counter
  
  // Interpreter fields
  struct ExecutorVariableFrame_T * executor_top_varframe;
  
  // Linked list vars
  struct Function_T * function;
  struct Variable_T * next;
  struct Variable_T * prev;
};

union ConstantValue_T {
  char c;
  int i;
};

enum Condition_T{
  CONDITION_EQUAL,
  CONDITION_NOT_EQUAL,
  CONDITION_GREATER_THAN,
  CONDITION_GREATER_THAN_OR_EQUAL,
  CONDITION_LESS_THAN,
  CONDITION_LESS_THAN_OR_EQUAL,
  CONDITION_COUNT,
};

enum CodeLineType_T{
  CODELINE_TYPE_FUNCTION_CALL,
  CODELINE_TYPE_CONSTANT_ASSIGNMENT,
  CODELINE_TYPE_WHILE,
  CODELINE_TYPE_IF,
  CODELINE_TYPE_BLOCK_END,
  CODELINE_TYPE_RETURN,
  CODELINE_TYPE_POINTER_ASSIGNMENT,
  CODELINE_TYPE_COUNT
};

struct CodeLine_T {
  enum CodeLineType_T type;
  
  int assigned_variable_reference_count; // Used with pointer assignment type
  struct Variable_T * assigned_variable;
  union ConstantValue_T constant;
  struct Function_T * target_function;
  
  int arg0_reference_count; // Used with pointer assignment type
  struct Variable_T * args[FUNCTION_ARG_COUNT];
  
  // For ifs and whiles
  struct CodeLine_T * block_other_end;
  enum Condition_T condition; // Condition is between first two arguments
  
  // Linked list vars
  struct Function_T * function;
  struct CodeLine_T * next;
  struct CodeLine_T * prev;
};

enum FunctionType_T {
  FUNCTION_TYPE_BUILTIN,
  FUNCTION_TYPE_BASIC_OP,
  FUNCTION_TYPE_CUSTOM,
  FUNCTION_TYPE_COUNT
};

struct Function_T {
  enum FunctionType_T type;
  char name[NAME_LEN];
  
  // Links to argument variables
  struct Variable_T * args[FUNCTION_ARG_COUNT];
  
  struct DataType_T * return_datatype;
  
  // Codeline linked list
  struct CodeLine_T * first_codeline;
  struct CodeLine_T * last_codeline;
  
  int codeline_count;
  struct ExecutorPerformanceReport_T executor_report;
  
  // Data for source replication
  int original_source_offset;
  
  // Variable linked list
  struct Variable_T * first_variable;
  struct Variable_T * last_variable;
  
  // Previous and next functions
  struct Environment_T * environment;
  struct Function_T * prev;
  struct Function_T * next;
};

struct Environment_T {
  char name[NAME_LEN];
  struct Function_T * first_function;
  struct Function_T * last_function;
  struct Function_T * main;
  
  struct DataType_T * first_datatype;
  struct DataType_T * last_datatype;
  
  // All the code sourcerer shouldn't change
  char * protected_code_text;
  int protected_code_len;
  
  // Environment memory space
  struct Membox_T * membox;
  
  // Fixed datatypes for executor
  struct DataType_T * char_datatype;
  struct DataType_T * int_datatype;
};

void free_function(struct Function_T * function);

struct DataType_T * datatype_pointer_jump(struct DataType_T * origin, int reference_count);

struct Variable_T * variable_name_search(struct Variable_T * var, char * name);

struct Environment_T * build_new_environment(size_t membox_size);

void randomly_populate_function(struct Function_T * function, int codeline_count);

int assert_environment_integrity(struct Environment_T * environment);

#endif  // C_TYPES_H
