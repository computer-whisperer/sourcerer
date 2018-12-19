#ifndef C_TYPES_H
#define C_TYPES_H

#define FUNCTION_ARG_COUNT 8
#define FUNCTION_NAME_LEN 32
#define VARIABLE_NAME_LEN 32
#define CODELINE_LEN 128

enum BasicDataType_T {
  BASICTYPE_INT, 
  BASICTYPE_CHAR,
  BASICTYPE_COUNT,
};

struct DataType_T {
  enum BasicDataType_T basic_type;
  short pointer_level;
};

struct Variable_T {
  char name[VARIABLE_NAME_LEN];
  struct DataType_T data_type;
  short is_arg;
  
  int references; // Reference counter
  
  // Interpreter fields
  struct ExecutorVariableFrame_T * executor_top_varframe;
  int executor_initialized;
  
  // Linked list vars
  struct Function_T * parent_function;
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
  CODELINE_TYPE_COUNT
};

struct CodeLine_T {
  enum CodeLineType_T type;
  struct Variable_T * assigned_variable;
  union ConstantValue_T constant;
  struct Function_T * target_function;
  struct Variable_T * function_arguments[FUNCTION_ARG_COUNT];
  
  // For ifs and whiles
  struct CodeLine_T * block_other_end;
  enum Condition_T condition; // Condition is between first two arguments
  
  // Linked list vars
  struct Function_T * parent_function;
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
  char name[FUNCTION_NAME_LEN];
  
  // Links to argument variables
  struct Variable_T * function_arguments[FUNCTION_ARG_COUNT];
  
  struct DataType_T return_datatype;
  
  // Codeline linked list
  struct CodeLine_T * first_codeline;
  struct CodeLine_T * last_codeline;
  
  // Variable linked list
  struct Variable_T * first_var;
  struct Variable_T * last_var;
  
  // Previous and next functions
  struct Function_T * prev;
  struct Function_T * next;
};

enum ChangeType_T {
  CHANGE_TYPE_INSERT_CODELINE,
  CHANGE_TYPE_REMOVE_CODELINE,
  CHANGE_TYPE_INSERT_VARIABLE,
  CHANGE_TYPE_REMOVE_VARIABLE,
  CHANGE_TYPE_ALTER_CONSTANT,
  CHANGE_TYPE_ALTER_ARGUMENT,
  CHANGE_TYPE_ALTER_ASSIGNED_VARIABLE,
};

struct Change_T {
  enum ChangeType_T type;
  
  struct Function_T * function;
  
  struct CodeLine_T * codeline;
  struct Variable_T * variable;
  union ConstantValue_T constant;
  int argument_index;
  
  struct CodeLine_T * next_codeline; // Used to position inserts
  
  struct Change_T * next;
};

struct Function_T * get_first_function(struct Function_T * function);
struct Function_T * get_last_function(struct Function_T * function);

struct Function_T * build_main_four_args();

struct Variable_T * variable_name_search(struct Variable_T * var, char * name);

struct Function_T * build_context();

int is_same_datatype(struct DataType_T a, struct DataType_T b);

int assert_full_structure_integrity(struct Function_T * first_function);

struct Change_T * apply_change(struct Change_T * change);
void free_change(struct Change_T * change);

#endif  // C_TYPES_H
