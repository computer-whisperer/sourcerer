/*
 * 
 * C code generator
 * 
 * uses a subset of the C language:
 *  -- simple assignments (a = b + c, a = foo(b, c))
 *  -- if statements
 *  -- while loops
 *  -- limitited list of system calls (printf to start with)
 *  -- int, char, ptrs only
 * 
 * */

#define SPACES_PER_INDENT 2
#define FUNCTION_ARG_COUNT 8
#define FUNCTION_NAME_LEN 32
#define VARIABLE_NAME_LEN 32
#define CODELINE_LEN 128
#define CHANGE_RELATED_VARS_LEN 4
#define CHANGE_CODELINES_LEN 4
#define PUTCHAR_BUFF_LEN 50
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <math.h>

enum BasicDataType_T {
  BASICTYPE_INT, 
  BASICTYPE_CHAR
};

struct DataType_T {
  enum BasicDataType_T basic_type;
  short pointer_level;
};

union InterpreterValue_T {
  char c;
  int i;
  void * ptr;
};

struct InterpreterVariableFrame_T {
  union InterpreterValue_T value;
  struct InterpreterVariableFrame_T * next;
};

int last_used_var_name[VARIABLE_NAME_LEN];
struct Variable_T {
  char name[VARIABLE_NAME_LEN];
  struct DataType_T data_type;
  short is_arg;
  
  int references; // Reference counter
  
  // Interpreter fields
  struct InterpreterVariableFrame_T * interpreter_top_varframe;
  int interpreter_initialized;
  
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
  CONDITION_LESS_THAN_OR_EQUAL
};

enum CodeLineType_T{
  CODELINE_TYPE_INVALID,
  CODELINE_TYPE_FUNCTION_CALL,
  CODELINE_TYPE_CONSTANT_ASSIGNMENT,
  CODELINE_TYPE_WHILE,
  CODELINE_TYPE_IF,
  CODELINE_TYPE_BLOCK_END,
  CODELINE_TYPE_RETURN
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
  FUNCTION_TYPE_CUSTOM
};

char last_used_func_name[FUNCTION_NAME_LEN];
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

struct GeneratedLine_T {
  char text[CODELINE_LEN];
  struct GeneratedLine_T* next;
};

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

void insert_type_string(struct DataType_T data_type, char* data, int* pos){
  if (data_type.basic_type == BASICTYPE_CHAR) {
    strcpy(data + *pos, "char");
    *pos += 4;
  }
  if (data_type.basic_type == BASICTYPE_INT) {
    strcpy(data + *pos, "int");
    *pos += 3;
  }
  if (data_type.pointer_level) {
    data[*pos] = ' ';
    *pos += 1;
    for (int i = 0; i < data_type.pointer_level; i++) {
      data[*pos] = '*';
      *pos += 1;
    }
  }
  data[*pos] = ' ';
  *pos += 1;
}

char * get_condition_string(enum Condition_T condition){
  switch (condition) {
    case CONDITION_EQUAL:
      return " == ";
    case CONDITION_NOT_EQUAL:
      return " != ";
    case CONDITION_GREATER_THAN:
      return " > ";
    case CONDITION_GREATER_THAN_OR_EQUAL:
      return " >= ";
    case CONDITION_LESS_THAN:
      return " < ";
    case CONDITION_LESS_THAN_OR_EQUAL:
      return " <= ";
  }
  // Default
  return " == ";
}

struct GeneratedLine_T * generate_c(struct Function_T* function){
  int indent_level = 0;
  struct GeneratedLine_T * start_line = NULL;
  struct GeneratedLine_T ** last_link = &start_line;
  struct GeneratedLine_T * new_line;

  int j;
  while (function) {
    
    if (function->type == FUNCTION_TYPE_CUSTOM) {
      new_line = malloc(sizeof(struct GeneratedLine_T));
      *last_link = new_line;
      last_link = &(new_line->next);
      
      j=0;
      // insert type string, name, and opening parenthesis
      insert_type_string(function->return_datatype, new_line->text, &j);
      strcpy(new_line->text+j, function->name);
      j += strlen(function->name);
      new_line->text[j] = '(';
      j += 1;
      // insert arguments
      for (int k = 0; k < FUNCTION_ARG_COUNT; k++) {
        if (!function->function_arguments[k])
          break;
        insert_type_string(function->function_arguments[k]->data_type, new_line->text, &j);
        strcpy(new_line->text + j, function->function_arguments[k]->name);
        j += strlen(function->function_arguments[k]->name);
        if (k < FUNCTION_ARG_COUNT - 1 && function->function_arguments[k+1]) {
          strcpy(new_line->text + j, ", ");
          j += 2;
        }
      }
      new_line->text[j] = ')';
      new_line->text[j+1] = '{';
      new_line->text[j+2] = '\0';
      indent_level++;
      // Add scope variable definitions
      struct Variable_T * variable = function->first_var;
      while (variable) {
        if (!variable->is_arg) {
          new_line = malloc(sizeof(struct GeneratedLine_T));
          *last_link = new_line;
          last_link = &(new_line->next);
          j = 0;
          while (j < indent_level*SPACES_PER_INDENT) {
            new_line->text[j] = ' ';
            j++;
          }
          // Type string;
          insert_type_string(variable->data_type, new_line->text, &j);
          // Name
          strcpy(new_line->text + j, variable->name);
          j += strlen(variable->name);
          // Semicolon
          new_line->text[j] = ';';
          new_line->text[j+1] = '\0';
        }
        variable = variable->next;
      }
      
      struct CodeLine_T * codeline = function->first_codeline;
      while (codeline) {
        new_line = malloc(sizeof(struct GeneratedLine_T));
        *last_link = new_line;
        last_link = &(new_line->next);
        
        char * cond_string;
        
        j = 0;
        switch (codeline->type) {
          case CODELINE_TYPE_FUNCTION_CALL:
            if (codeline->target_function->type == FUNCTION_TYPE_BASIC_OP) {
              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;
              strcpy(new_line->text + j, codeline->function_arguments[0]->name);
              j += strlen(codeline->function_arguments[0]->name);
              strcpy(new_line->text + j, " + ");
              j += 3;
              new_line->text[j-2] = codeline->target_function->name[0];
              strcpy(new_line->text + j, codeline->function_arguments[1]->name);
              j += strlen(codeline->function_arguments[1]->name);
              strcpy(new_line->text + j, ";\0");
            }
            else {
              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;
              strcpy(new_line->text + j, codeline->target_function->name);
              j += strlen(codeline->target_function->name);
              strcpy(new_line->text + j, "(");
              j += 1;
              for (int k = 0; k < FUNCTION_ARG_COUNT; k++) {
                if (!codeline->function_arguments[k])
                  break;
                strcpy(new_line->text + j, codeline->function_arguments[k]->name);
                j += strlen(codeline->function_arguments[k]->name);
                if (k < FUNCTION_ARG_COUNT - 1 && codeline->function_arguments[k+1]) {
                  strcpy(new_line->text + j, ", ");
                  j += 2;
                }
              }
              strcpy(new_line->text + j, ");\0");
            }
            break;
          case CODELINE_TYPE_CONSTANT_ASSIGNMENT:
            if (codeline->assigned_variable->data_type.basic_type == BASICTYPE_CHAR) {
              unsigned char value = codeline->constant.c;
              // for example: a = (char)004;
              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;

              if (value >= 32 && value < 127 && value != '\\' && value != '\'' && value != '\"' && value != '\?') {
                // Normal single-quoted constant
                new_line->text[j] = '\'';
                new_line->text[j+1] = value;
                new_line->text[j+2] = '\'';
                j += 3;
              }
              else {
                strcpy(new_line->text + j, "(char)");
                j += 6;
                new_line->text[j+0] = '0';
                new_line->text[j+1] = ((value/64) % 8) + 48;
                new_line->text[j+2] = ((value/8) % 8) + 48;
                new_line->text[j+3] = ((value/1) % 8) + 48;
                j += 4;
              }
              strcpy(new_line->text + j, ";");
            }
            if (codeline->assigned_variable->data_type.basic_type == BASICTYPE_INT) {
              // Determine sign and calculate number of chars required for the number
              int value = codeline->constant.i;
              int const_len = 0;
              char sign = '+';
              if (value < 0) {
                sign = '-';
                value = -value;
              }
              int working_value = value;
              while (working_value > 0){
                const_len++;
                working_value = working_value/10;
              }
              // for example: i = +252;

              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;
              new_line->text[j] = sign;
              j += 1;
              // Add a leading zero to make sure it actually does something when zero is the number
              if (value == 0){
                new_line->text[j] = '0';
                j++;
              }
              int k = 0;
              while (value > 0){
                new_line->text[j + const_len - k - 1] = value%10 + 48;
                value = value/10;
                k++;
              }
              j += const_len;
              strcpy(new_line->text + j, ";\0");
            }
            break;
          case CODELINE_TYPE_IF:
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "if (");
            j += 4;
            strcpy(new_line->text + j, codeline->function_arguments[0]->name);
            j += strlen(codeline->function_arguments[0]->name);
            cond_string = get_condition_string(codeline->condition);
            strcpy(new_line->text + j, cond_string);
            j += strlen(cond_string);
            strcpy(new_line->text + j, codeline->function_arguments[1]->name);
            j += strlen(codeline->function_arguments[1]->name);
            strcpy(new_line->text + j, "){");
            
            indent_level++;
            break;
          case CODELINE_TYPE_WHILE:
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "while (");
            j += 7;
            strcpy(new_line->text + j, codeline->function_arguments[0]->name);
            j += strlen(codeline->function_arguments[0]->name);
            cond_string = get_condition_string(codeline->condition);
            strcpy(new_line->text + j, cond_string);
            j += strlen(cond_string);
            strcpy(new_line->text + j, codeline->function_arguments[1]->name);
            j += strlen(codeline->function_arguments[1]->name);
            strcpy(new_line->text + j, "){");
            
            indent_level++;
            break;
          case CODELINE_TYPE_BLOCK_END:
            indent_level--;
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "}");
            break;
          case CODELINE_TYPE_RETURN:
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "return ");
            j += 7;
            strcpy(new_line->text + j, codeline->assigned_variable->name);
            j += strlen(codeline->assigned_variable->name);
            strcpy(new_line->text + j, ";");
            break;
          case CODELINE_TYPE_INVALID:
            printf("Bad boy");
        }
        codeline = codeline->next;
      }
      
      // Add trailing bracket
      indent_level--;
      new_line = malloc(sizeof(struct GeneratedLine_T));
      *last_link = new_line;
      last_link = &(new_line->next);
      new_line->text[0] = '}';
      new_line->text[1] = '\0';
      // Empty trailing line
      new_line = malloc(sizeof(struct GeneratedLine_T));
      *last_link = new_line;
      last_link = &(new_line->next);
      new_line->text[0] = '\0';
    }
    
    function = function->next; // Jump to the next function
  }
  *last_link = NULL;
  return start_line;
}


void print_generated_lines(struct GeneratedLine_T* generated_code) {
  while (generated_code) {
    printf("%s\n",generated_code->text);
    generated_code = generated_code->next;
  }
}

int interpret_codeline_test_condition(struct CodeLine_T * codeline) {
  int a, b;
  if (codeline->function_arguments[0]->data_type.basic_type == BASICTYPE_CHAR) {
    a = codeline->function_arguments[0]->interpreter_top_varframe->value.c;
    b = codeline->function_arguments[1]->interpreter_top_varframe->value.c;
  }
  else {
    a = codeline->function_arguments[0]->interpreter_top_varframe->value.i;
    b = codeline->function_arguments[1]->interpreter_top_varframe->value.i;
  }
  switch (codeline->condition) {
    case CONDITION_EQUAL:
      return (a == b);
    case CONDITION_NOT_EQUAL:
      return (a != b);
    case CONDITION_GREATER_THAN:
      return (a > b);
    case CONDITION_GREATER_THAN_OR_EQUAL:
      return (a >= b);
    case CONDITION_LESS_THAN:
      return (a < b);
    case CONDITION_LESS_THAN_OR_EQUAL:
      return (a <= b);
  }
  return 0;
}

#define PENALTY_LINE_EXECUTED 1
#define PENALTY_NO_EXIT 1000
#define PENALTY_UNINITIALIZED_VAR 3
#define MAX_LINES_EXECUTED 200

int putchar_i;
char putchar_buff[PUTCHAR_BUFF_LEN];
union InterpreterValue_T interpret_function(struct Function_T * function, union InterpreterValue_T * args, int * score){
  int s;
  if (!score)
    score = &s;
  // Add stack frames for variables in function
  union InterpreterValue_T sub_args[FUNCTION_ARG_COUNT];
  struct Variable_T * variable = function->first_var;
  while (variable) {
    struct InterpreterVariableFrame_T * newframe = malloc(sizeof(struct InterpreterVariableFrame_T));
    newframe->next = variable->interpreter_top_varframe;
    newframe->value.i = 0;
    variable->interpreter_top_varframe = newframe;
    variable->interpreter_initialized = 0;
    variable = variable->next;
  }
  // Set incoming arguments
  for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
    if (!function->function_arguments[i])
      break;
    function->function_arguments[i]->interpreter_top_varframe->value = args[i];
    function->function_arguments[i]->interpreter_initialized = 1;
  }
  union InterpreterValue_T rval = (union InterpreterValue_T){.ptr=NULL};
  // Execute commands
  struct CodeLine_T * codeline = function->first_codeline;
  struct CodeLine_T * next_codeline = NULL;
  int lines_executed = 0;
  int done = 0;
  while (codeline && !done) {
    lines_executed++;
    *score -= PENALTY_LINE_EXECUTED;
    if (lines_executed > MAX_LINES_EXECUTED) {
      *score -= PENALTY_NO_EXIT;
      break;
    }
    
    next_codeline = codeline->next;
    switch (codeline->type) {
      case CODELINE_TYPE_CONSTANT_ASSIGNMENT:
        codeline->assigned_variable->interpreter_top_varframe->value.i = codeline->constant.i;
        // Mark assigned variable as initialized
        codeline->assigned_variable->interpreter_initialized = 1;
        break;
      case CODELINE_TYPE_RETURN:
        // Penalize usage of uninitialized variables
        if (!codeline->assigned_variable->interpreter_initialized)
          *score -= PENALTY_UNINITIALIZED_VAR;
        rval = codeline->assigned_variable->interpreter_top_varframe->value;        
        done = 1;
        break;
      case CODELINE_TYPE_FUNCTION_CALL:
        // Penalize usage of uninitialized variables
        for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
          if (!codeline->target_function->function_arguments[i])
            break;
          if (!codeline->function_arguments[i]->interpreter_initialized)
            *score -= PENALTY_UNINITIALIZED_VAR;
        }
        switch (codeline->target_function->type) {
          case FUNCTION_TYPE_BASIC_OP:
            switch (codeline->target_function->name[0]) {
              // Assuming all integer arguments for now, probably a bad idea
              case '+':
                codeline->assigned_variable->interpreter_top_varframe->value.i = codeline->function_arguments[0]->interpreter_top_varframe->value.i + codeline->function_arguments[1]->interpreter_top_varframe->value.i;
                break;
              case '-':
                codeline->assigned_variable->interpreter_top_varframe->value.i = codeline->function_arguments[0]->interpreter_top_varframe->value.i - codeline->function_arguments[1]->interpreter_top_varframe->value.i;
                break;
              case '*':
                codeline->assigned_variable->interpreter_top_varframe->value.i = codeline->function_arguments[0]->interpreter_top_varframe->value.i * codeline->function_arguments[1]->interpreter_top_varframe->value.i;
                break;
              case '/':
                if (codeline->function_arguments[1]->interpreter_top_varframe->value.i == 0)
                  break;
                codeline->assigned_variable->interpreter_top_varframe->value.i = (double)codeline->function_arguments[0]->interpreter_top_varframe->value.i / (double)codeline->function_arguments[1]->interpreter_top_varframe->value.i;
                break;
            }
            break;
          case FUNCTION_TYPE_BUILTIN:
            // For now assume putchar, also a really bad idea :)
            //putchar(codeline->function_arguments[0]->interpreter_top_varframe->value.c);
            if (putchar_i < PUTCHAR_BUFF_LEN) {
              putchar_buff[putchar_i] = codeline->function_arguments[0]->interpreter_top_varframe->value.c;
              codeline->assigned_variable->interpreter_top_varframe->value.c = codeline->function_arguments[0]->interpreter_top_varframe->value.c;
              putchar_i++;
            }
            break;
          case FUNCTION_TYPE_CUSTOM:
            
            for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
              if (!codeline->function_arguments[i])
                break;
              sub_args[i] = codeline->function_arguments[i]->interpreter_top_varframe->value;
            }
            codeline->assigned_variable->interpreter_top_varframe->value = interpret_function(codeline->target_function, sub_args, score);
            break;
          // Mark assigned variable as initialized
          codeline->assigned_variable->interpreter_initialized = 1;
        }
        break;
      case CODELINE_TYPE_IF:
        // Penalize usage of uninitialized variables
        for (int i = 0; i < 2; i++)
          if (!codeline->function_arguments[i]->interpreter_initialized)
            *score -= PENALTY_UNINITIALIZED_VAR;
        if (!interpret_codeline_test_condition(codeline))
          next_codeline = codeline->block_other_end->next;
        break;
      case CODELINE_TYPE_WHILE:
        for (int i = 0; i < 2; i++)
          if (!codeline->function_arguments[i]->interpreter_initialized)
            *score -= PENALTY_UNINITIALIZED_VAR;
        if (!interpret_codeline_test_condition(codeline))
          next_codeline = codeline->block_other_end->next;
        break;
      case CODELINE_TYPE_BLOCK_END:
        if (codeline->block_other_end->type == CODELINE_TYPE_WHILE)
          if (interpret_codeline_test_condition(codeline->block_other_end))
            next_codeline = codeline->block_other_end->next; // Handle while loop return
        break;
      case CODELINE_TYPE_INVALID:
        printf("Bad boy!");
    }
    codeline = next_codeline;
  }
  // Free stack frames
  variable = function->first_var;
  while (variable) {
    struct InterpreterVariableFrame_T * frame = variable->interpreter_top_varframe;
    variable->interpreter_top_varframe = variable->interpreter_top_varframe->next;
    free(frame);
    variable = variable->next;
  }
  return rval;
}

// This will eat the datatype text, any * for pointers, and all trailing whitespace
struct DataType_T scan_datatype(char * text, int * pos) {
  struct DataType_T data_type = {.basic_type=BASICTYPE_INT, .pointer_level=0};

  if (text[*pos] == 'i')
    data_type.basic_type = BASICTYPE_INT;
  if (text[*pos] == 'c')
    data_type.basic_type = BASICTYPE_CHAR;
  
  while(text[*pos] != ' ' && text[*pos] != '\0')
    (*pos)++;
  while(text[*pos] == ' ')
    (*pos)++;
  while(text[*pos] == '*') {
    data_type.pointer_level++;
    (*pos)++;
    while(text[*pos] == ' ')
      (*pos)++;
  }
  return data_type;
}

struct Variable_T * variable_search(struct Variable_T * var, char * name) {
  while (var) {
    if (!strcmp(name, var->name)) 
      break;
    var = var->next;
  }
  return var;
}

struct Function_T * function_search(struct Function_T * func, char * name) {
  while (func) {
    if (!strcmp(name, func->name)) 
      break;
    func = func->next;
  }
  return func;
}

int parse_word(char * text, int * pos, char * buffer, int buffer_len) {
  int i = 0;
  for (i = 0; i < buffer_len-1; i++) {
    char c = text[*pos];
    // Only count numbers, letters, and underscores
    if (!(48 <= c && c <= 57) && !(65 <= c && c <= 90) && !(97 <= c && c <= 122) && c != '_')
      break;
    buffer[i] = c;
    (*pos)++;
  }
  buffer[i] = '\0';
  return i;
}

enum Condition_T parse_condition(char * text, int * pos) {
  int i;
  char buff[3];
  for (i = 0; i < 2; i++) {
    buff[i] = text[*pos];
    if (buff[i] != '<' && buff[i] != '>' && buff[i] != '=' && buff[i] != '!')
      break;
    (*pos)++;
  }
  buff[i] = '\n';
  if (i == 1) {
    if (buff[0] == '<')
      return CONDITION_LESS_THAN;
    else
      return CONDITION_GREATER_THAN;
  }
  else {
    if (buff[0] == '=')
      return CONDITION_EQUAL;
    if (buff[0] == '!')
      return CONDITION_NOT_EQUAL;
    if (buff[1] == '=') {
      if (buff[0] == '<')
        return CONDITION_LESS_THAN_OR_EQUAL;
      if (buff[0] == '>')
        return CONDITION_GREATER_THAN_OR_EQUAL;
    }
  }
  return CONDITION_EQUAL;
}

// This a greedy parser, it will tolerate bad formatting etc.
struct Function_T * parse_source_text(char * source_text, struct Function_T * context) {
  int i = 0;
  int arg_index = 0;
  struct Function_T * first_function = NULL;
  struct Function_T ** next_function_link_target = &first_function;
  struct Function_T * prev_function = NULL;
  struct Function_T * function = malloc(sizeof(struct Function_T));
  function->type = FUNCTION_TYPE_CUSTOM;
  function->prev = prev_function;
  function->next = NULL;
  function->first_codeline = NULL;
  function->first_var = NULL;
  
  int count;
  
  while (source_text[i] != '\0') {
    // Initialize new linked lists for both variables and codelines
    struct Variable_T ** next_var_link_target = &(function->first_var);
    struct Variable_T * prev_variable = NULL;
    struct Variable_T * variable = malloc(sizeof(struct Variable_T));
    variable->parent_function = function;
    variable->prev = prev_variable;
    variable->next = NULL;
    
    struct CodeLine_T ** next_codeline_link_target = &(function->first_codeline);
    struct CodeLine_T * prev_codeline = NULL;
    struct CodeLine_T * codeline = malloc(sizeof(struct CodeLine_T));
    codeline->parent_function = function;
    codeline->prev = prev_codeline;
    codeline->next = NULL;
    codeline->type = CODELINE_TYPE_INVALID;
    
    int nest_level = 0;
    struct CodeLine_T * codeline_nest_list[100];
    
    /* If we see an EOF at any point in this while, all subloops will 
     * just short-circuit and execution will flow to the bottom of the 
     * while, where we can clean up any messes
     * */
    
    
    // Hunting for a function start
    while (source_text[i] != '\0') {
      while(source_text[i] == ' ' || source_text[i] == '\n') {i++;}
      function->return_datatype = scan_datatype(source_text, &i);
      count = parse_word(source_text, &i, function->name, FUNCTION_NAME_LEN);
    
      if (count > 0 || source_text[i] == '\0')
        break;
        
      // If we failed to get a name, jump forward a character and try again.
      i++;
    }

    // Have the name, scan to argument list start
    while(source_text[i] != '(' && source_text[i] != '\0') {i++;}
    if (source_text[i] != '\0')
      i++;

    // Scan over argument list
    for (arg_index = 0; arg_index < FUNCTION_ARG_COUNT; arg_index++) {
      function->function_arguments[arg_index] = NULL;
      // Eat whitespace
      while (source_text[i] == ' '){i++;}
      if (source_text[i] == '\0' || source_text[i] == ')')
        break;
      // Must be a datatype then variable name next
      variable->data_type = scan_datatype(source_text, &i);
      count = parse_word(source_text, &i, variable->name, VARIABLE_NAME_LEN);

      if (count > 0) { // if j is 0, variable read failure
        // Read variable successfully, save and start new variable
        variable->is_arg = 1;
        *next_var_link_target = variable;
        next_var_link_target = &(variable->next);
        // Save link for argument
        function->function_arguments[arg_index] = variable;
        // New variable
        prev_variable = variable;
        variable = malloc(sizeof(struct Variable_T));
        variable->parent_function = function;
        variable->prev = prev_variable;
        variable->next = NULL;
      }
      else {
        // Retry for this arg_index
        arg_index--;
      }
      
      // Eat whitespace
      while (source_text[i] == ' '){i++;}
      if (source_text[i] == ',') {
        i++;
      }
    }

    // We should be pointing at the end parenthesis of the argument list now
    while(source_text[i] != '{' && source_text[i] != '\0') {i++;}
    if (source_text[i] != '\0')
      i++;
    
    // Iterate over codelines
    while (1) {
      while (source_text[i] == ' ' || source_text[i] == '\n'){i++;}
      if (source_text[i] == '\0')
        break;
      if (source_text[i] == '}' && nest_level == 0)
        break;
      // We are now pointing at an interesting line
      // Test if this is a declaration
      if (!strncmp(source_text + i, "int ", 4) || !strncmp(source_text + i, "char ", 5)) {
        // Handle a variable declaration
        variable->data_type = scan_datatype(source_text, &i);
        count = parse_word(source_text, &i, variable->name, VARIABLE_NAME_LEN);
        
        if (count > 0) { // if j is 0, variable read failure
          // Read variable successfully, save and start new variable
          variable->is_arg = 0;
          *next_var_link_target = variable;
          next_var_link_target = &(variable->next);
          // New variable
          prev_variable = variable;
          variable = malloc(sizeof(struct Variable_T));
          variable->parent_function = function;
          variable->prev = prev_variable;
          variable->next = NULL;
        }
      }
      else {
        char name_buffer[CODELINE_LEN];
        // Line is not a declaration -- so it could be an assignment, if statement, or while loop.
        if (!strncmp(source_text + i, "if", 2)){
          // need to get a variable, a condition, then a variable
          i += 2;
          while(source_text[i] == ' ') {i++;}
          // Should be pointing at opening parenthesis
          if (source_text[i] == '(') i++;
          while(source_text[i] == ' ') {i++;}
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[0] = variable_search(function->first_var, name_buffer);
          while(source_text[i] == ' ') {i++;}
          // Should now be looking at the condition operation, grab it
          codeline->condition = parse_condition(source_text, &i);
          while(source_text[i] == ' ') {i++;}
          // Get second variable
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[1] = variable_search(function->first_var, name_buffer);
          codeline->function_arguments[2] = NULL;
          // Step over parenthesis and open bracket
          while(source_text[i] == ' ') {i++;}
          if (source_text[i] == ')') i++;
          while(source_text[i] == ' ' || source_text[i] == '\n') {i++;}
          if (source_text[i] == '{') i++;
          
          // Should be success, verify
          if (codeline->function_arguments[0] && codeline->function_arguments[1]) {
            codeline->type = CODELINE_TYPE_IF;
            codeline_nest_list[nest_level] = codeline; // Save reference to codeline for linking up to block end later
            nest_level++;
          }
        }
        else if (!strncmp(source_text + i, "while", 5)) {
          // need to get a variable, a condition, then a variable
          i += 5;
          while(source_text[i] == ' ') {i++;}
          // Should be pointing at opening parenthesis
          if (source_text[i] == '(') i++;
          while(source_text[i] == ' ') {i++;}
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[0] = variable_search(function->first_var, name_buffer);
          while(source_text[i] == ' ') {i++;}
          // Should now be looking at the condition operation, grab it
          codeline->condition = parse_condition(source_text, &i);
          while(source_text[i] == ' ') {i++;}
          // Get second variable
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[1] = variable_search(function->first_var, name_buffer);
          codeline->function_arguments[2] = NULL;
          // Step over parenthesis and open bracket
          while(source_text[i] == ' ') {i++;}
          if (source_text[i] == ')') i++;
          while(source_text[i] == ' ' || source_text[i] == '\n') {i++;}
          if (source_text[i] == '{') i++;
          
          // Should be success, verify
          if (codeline->function_arguments[0] && codeline->function_arguments[1]) {
            codeline->type = CODELINE_TYPE_WHILE;
            codeline_nest_list[nest_level] = codeline; // Save reference to codeline for linking up to block end later
            nest_level++;
          }
        }
        else if (source_text[i] == '}') {
          // End of for or while loop
          i++;
          if (nest_level > 0) {
            nest_level--;
            codeline->block_other_end = codeline_nest_list[nest_level];
            codeline_nest_list[nest_level]->block_other_end = codeline;
            codeline->type = CODELINE_TYPE_BLOCK_END;
          }
        }
        else {
          // load it as an assignment
          count = parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->assigned_variable = variable_search(function->first_var, name_buffer);
          if (!codeline->assigned_variable) {
            printf("Did not recognize assigning variable %s", name_buffer);
            while (source_text[i] != '\n' && source_text[i] != '\0') i++;
          }

          while(source_text[i] == ' ') {i++;}
          // This should be an equal sign
          if (source_text[i] == '=')
            i++;
          while(source_text[i] == ' ') {i++;}
          // We should now be pointing at the start of the expression, test for constant expressions
          if (source_text[i] == '+' || source_text[i] == '-' || (48 <= source_text[i] && source_text[i] <= 57)){
            // Must be an integer constant, parse accordingly
            int sign = 1;
            if (source_text[i] == '+'){
              i++;
            }
            if (source_text[i] == '-'){
              sign = -1;
              i++;
            }
            while(source_text[i] == ' ') {i++;}
            int value = 0;
            while(48 <= source_text[i] && source_text[i] <= 57) {
              value = value*10 + (source_text[i] - 48);
              i++;
            }
            value *= sign;
            codeline->constant.i = value;
            // Success, int constant
            codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
          }
          else if (source_text[i] == '(') {
            // Assuming this is a char constant
            while (source_text[i] != ')' && source_text[i] != '\0'){i++;} // Fast-forward past )
            if (source_text[i] == ')') i++;
            while (source_text[i] == ' '){i++;}
            int value = 0;
            while(48 <= source_text[i] && source_text[i] <= 57) {
              value = value*10 + (source_text[i] - 48);
              i++;
            }
            codeline->constant.c = (char)value;
            codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
            // Success, char constant
          }
          else {
            count = parse_word(source_text, &i, name_buffer, CODELINE_LEN);
            while (source_text[i] == ' '){i++;}
            
            if (source_text[i] == '(') { // If next interesting character is (, then it is either a function call or a char constant
              // Must be a function call
                
              codeline->target_function =  function_search(context, name_buffer);
              i++; // Step past opening parenthesis
              // Parse through arguments
              
              for (arg_index = 0; arg_index < FUNCTION_ARG_COUNT; arg_index++) {
                while (source_text[i] == ' '){i++;}
                if (source_text[i] == ';' || source_text[i] == ')' || source_text[i] == '\0')
                  break;
                
                parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
                codeline->function_arguments[arg_index] = variable_search(function->first_var, name_buffer);
                if (!codeline->function_arguments[arg_index])
                  arg_index--; // Failed to recognize that one, skip it
                while (source_text[i] == ' '){i++;}
                if (source_text[i] == ',')
                  i++;
              } 
              if (source_text[i] == ')') i++;
              if (codeline->target_function) {
                // Success
                codeline->type = CODELINE_TYPE_FUNCTION_CALL;
              }
            }
            else {
              // Must be a basic arithmetic operation
              char op = source_text[i]; // I points to op character
              codeline->function_arguments[0] = variable_search(function->first_var, name_buffer);
              name_buffer[0] = op;
              name_buffer[1] = '\0';
              codeline->target_function = function_search(context, name_buffer);
              // Scan in second argument
              i++;
              while (source_text[i] == ' '){i++;}
              
              count = parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);

              codeline->function_arguments[1] = variable_search(function->first_var, name_buffer);
              codeline->function_arguments[2] = NULL;
              
              if (!codeline->function_arguments[0] || !codeline->function_arguments[1] || !codeline->target_function) {
                printf("Sum ting wong...");
              }
              else
                codeline->type = CODELINE_TYPE_FUNCTION_CALL;
            }
            
          }
        }
      }
      
      if (codeline->type != CODELINE_TYPE_INVALID) {
        // Save the codeline
        *next_codeline_link_target = codeline;
        next_codeline_link_target = &(codeline->next);
        // New variable
        prev_codeline = codeline;
        codeline = malloc(sizeof(struct CodeLine_T));
        codeline->parent_function = function;
        codeline->prev = prev_codeline;
        codeline->next = NULL;
        codeline->type = CODELINE_TYPE_INVALID;
      }
      
      // Eat through any trailing semicolon
      while (source_text[i] == ' '){i++;}
      if (source_text[i] == ';'){i++;}
      
    }
    
    // If we are looking at an end bracket, the function read was successful.
    if (source_text[i] == '}') {
      i++;// eat }
      
      // Save this function
      function->last_var = prev_variable;
      function->last_codeline = prev_codeline;
      *next_function_link_target = function;
      next_function_link_target = &(function->next);
      // New function
      prev_function = function;
      function = malloc(sizeof(struct Function_T));
      function->type = FUNCTION_TYPE_CUSTOM;
      function->prev = prev_function;
      function->next = NULL;
      function->first_codeline = NULL;
      function->first_var = NULL;
      // Save variables and codelines used in this function from getting freed
      variable->prev = NULL;
      codeline->prev = NULL;
    }
    
    // Free any dangling linked-list segments and clean up for the next iteration
    
    while(variable){
      struct Variable_T * to_free_v = variable;
      variable = variable->prev;
      free(to_free_v);
    }
    
    while(codeline){
      struct CodeLine_T * to_free_c = codeline;
      codeline = codeline->prev;
      free(to_free_c);
    }
    
  }
  
  // Free the working function
  free(function);
  
  // We should be done!
  return first_function;
}

struct Function_T * build_example_tree() {
  struct DataType_T dt_i = {BASICTYPE_INT, 0};
  struct DataType_T dt_c = {BASICTYPE_CHAR, 0};
  struct Variable_T * vars = malloc(sizeof(struct Variable_T)*5);
  vars[0] = (struct Variable_T){.name="a", .is_arg=0, .data_type=dt_i, .next=&(vars[1]), .prev=NULL};
  vars[1] = (struct Variable_T){.name="b", .is_arg=1, .data_type=dt_i, .next=&(vars[2]), .prev=&(vars[0])};
  vars[2] = (struct Variable_T){.name="c", .is_arg=1, .data_type=dt_i, .next=&(vars[3]), .prev=&(vars[1])};
  vars[3] = (struct Variable_T){.name="d", .is_arg=0, .data_type=dt_c, .next=&(vars[4]), .prev=&(vars[2])};
  vars[4] = (struct Variable_T){.name="e", .is_arg=0, .data_type=dt_i, .next=NULL      , .prev=&(vars[3])};
  struct Function_T * funcs = malloc(sizeof(struct Function_T)*3);
  funcs[0] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="+", .next=&(funcs[1]), .prev=NULL,};
  funcs[1] = (struct Function_T){.type=FUNCTION_TYPE_BUILTIN, .name="putchar", .next=&(funcs[2]), .prev=&(funcs[0])};
  funcs[2] = (struct Function_T){.type=FUNCTION_TYPE_CUSTOM, .name="main", .return_datatype=dt_i, .next=NULL, .prev=&(funcs[1]), .first_var=&(vars[0]), .last_var=&(vars[4]), .function_arguments={&(vars[1]), &(vars[2])}};
  int codeline_count = 9;
  struct CodeLine_T * codelines = malloc(sizeof(struct CodeLine_T)*codeline_count);
  codelines[0] = (struct CodeLine_T){.type=CODELINE_TYPE_CONSTANT_ASSIGNMENT, .assigned_variable=&(vars[4]), .constant={.i=48}, .parent_function=&(funcs[2])};
  codelines[1] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[0]), .target_function=&(funcs[0]), .function_arguments={&(vars[1]), &(vars[2])}, .parent_function=&(funcs[2])};
  codelines[2] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[0]), .target_function=&(funcs[0]), .function_arguments={&(vars[0]), &(vars[4])}, .parent_function=&(funcs[2])};
  codelines[3] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[3]), .target_function=&(funcs[1]), .function_arguments={&(vars[0]), NULL}, .parent_function=&(funcs[2])};
  codelines[4] = (struct CodeLine_T){.type=CODELINE_TYPE_IF, .function_arguments={&(vars[1]), &(vars[2])}, .block_other_end=&(codelines[8]), .condition=CONDITION_GREATER_THAN, .parent_function=&(funcs[2])};
  codelines[5] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[3]), .target_function=&(funcs[1]), .function_arguments={&(vars[0]), NULL}, .parent_function=&(funcs[2])};
  codelines[6] = (struct CodeLine_T){.type=CODELINE_TYPE_CONSTANT_ASSIGNMENT, .assigned_variable=&(vars[3]), .constant={.c='h'}, .parent_function=&(funcs[2])};
  codelines[7] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[3]), .target_function=&(funcs[1]), .function_arguments={&(vars[3]), NULL}, .parent_function=&(funcs[2])};
  codelines[8] = (struct CodeLine_T){.type=CODELINE_TYPE_BLOCK_END, .prev=&(codelines[3]), .next=NULL, .parent_function=&(funcs[2]), .block_other_end=&(codelines[4])};
  for (int i = 0; i < codeline_count; i++) {
    codelines[i].next = codelines + i + 1;
    codelines[i].prev = codelines + i - 1;
  }
  codelines[0].prev = NULL;
  codelines[codeline_count-1].next = NULL;

  funcs[2].first_codeline = &(codelines[0]);
  funcs[2].last_codeline = &(codelines[codeline_count-1]);
  return &(funcs[0]);
}

int is_same_datatype(struct DataType_T a, struct DataType_T b) {
  return a.basic_type == b.basic_type && a.pointer_level == b.pointer_level;
}


// fast random number generated yanked from the internet

unsigned int g_seed;

// Used to seed the generator.           
void fast_srand(int seed) {
    g_seed = seed;
}

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFF;
}



struct Variable_T * find_random_variable(struct Variable_T * first_variable, struct DataType_T * type, struct Variable_T ** related_vars) {
  // Count number of compatible variables
  int variable_count = 0;
  struct Variable_T * variable = first_variable;
  while (variable) {
    if (!type || is_same_datatype(variable->data_type, *type))
      variable_count++;
    variable = variable->next;
  }
  // Select from these variables and the possibility of a new one
  int selected_var = fast_rand() % (variable_count + 1);
  if (selected_var < variable_count) {
    // Var selected from existing range
    variable = first_variable;
    selected_var++;
    while (variable && selected_var){
      if (!type || is_same_datatype(variable->data_type, *type))
        selected_var--;
      if (selected_var == 0)
        break;
      variable = variable->next;
    }
    return variable;
  }
  else {
    // Make a new var
    variable = malloc(sizeof(struct Variable_T));
    variable->is_arg = 0;
    variable->references = 0;
    // Wipe name
    for (int i = 0; i < VARIABLE_NAME_LEN; i++) {
      variable->name[i] = '\0';
    }
    // Find unused name
    while(1) {
      // Increment name
      for (int i = 0; i < VARIABLE_NAME_LEN; i++) {
        if (variable->name[i] == '\0') {
          variable->name[i] = 'a';
          break;
        }
        if (variable->name[i] < 'z'){
          variable->name[i]++;
          break;
        }
        variable->name[i] = 'a';
      }
      // Search for another like it
      int valid = 1;
      
      if (!variable_search(first_variable, variable->name)) {
        // None like it in the normal vars, now check the related vars
        for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
          if (!related_vars[i])
            break;
          if (!strcmp(variable->name, related_vars[i]->name)){
            valid = 0;
            break;
          }
        }
      }
      else
        valid = 0;
        
      // None like it, good find!
      if (valid)
        break;
    }
    // Insert variable into array of new vars
    for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
      if (!related_vars[i]) {
        related_vars[i] = variable;
        break;
      }
    }

    if (type)
      variable->data_type = *type;
    // Handle request for random type
    if (!type) {
      if (fast_rand() % 2)
        variable->data_type = (struct DataType_T){.basic_type=BASICTYPE_CHAR, .pointer_level=0};
      else
        variable->data_type = (struct DataType_T){.basic_type=BASICTYPE_INT, .pointer_level=0};
    }
    
    return variable;
  }
}

struct Change_T * find_new_change(struct Function_T * function, struct Function_T * context) {
  
  struct Change_T * forward_change = malloc(sizeof(struct Change_T));
  struct Change_T * reverse_change = malloc(sizeof(struct Change_T));
  forward_change->applied = 0;
  forward_change->inverse = reverse_change;
  forward_change->specific_var = NULL;
  reverse_change->applied = 1;
  reverse_change->inverse = forward_change;
  reverse_change->specific_var = NULL;
  for (int i = 0; i < CHANGE_CODELINES_LEN; i++) {
    forward_change->codelines[i] = NULL;
    reverse_change->codelines[i] = NULL;
  }
  
  for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++)
    forward_change->related_vars[i] = NULL;
  
  // Count the number of codelines;
  int codeline_count = 0;
  struct CodeLine_T * codeline = function->first_codeline;
  while (codeline){
    codeline_count++;
    codeline = codeline->next;
  }
  
  // Count the number of functions in context
  int function_count = 0;
  struct Function_T * cfunction = context;
  while (cfunction) {
    function_count++;
    cfunction = cfunction->next;
  }
  
  // Select a random codeline
  int selected_line = 0;
  if (codeline_count) // codeline_count could be 0
    selected_line = fast_rand() % codeline_count;
  codeline = function->first_codeline;
  while(selected_line > 0){
    codeline = codeline->next;
    selected_line--;
  }
  
  // Found a random codeline, do something with it
  int r = fast_rand() % 100;
  
  if (r < 40 && codeline && codeline->type != CODELINE_TYPE_RETURN) {
    if (codeline->type == CODELINE_TYPE_CONSTANT_ASSIGNMENT) {
      // Modify the constant
      int value = (fast_rand() % 20) - 10;
      forward_change->type = CHANGE_TYPE_ALTER_CONSTANT;
      forward_change->value = value;
      forward_change->codelines[0] = codeline;
      reverse_change->type = CHANGE_TYPE_ALTER_CONSTANT;
      reverse_change->value = -value;
      reverse_change->codelines[0] = codeline;
    }
    else {
      // Modify an argument
      
      // If we are pointing at a block end, just use the head
      if (codeline->type == CODELINE_TYPE_BLOCK_END)
        codeline = codeline->block_other_end;
        
      forward_change->type = CHANGE_TYPE_SWITCH_ARGUMENT;
      forward_change->codelines[0] = codeline;
      reverse_change->type = CHANGE_TYPE_SWITCH_ARGUMENT;
      reverse_change->codelines[0] = codeline;
      
      int arg_count;
      for (arg_count = 0; arg_count < FUNCTION_ARG_COUNT; arg_count++)
        if (!codeline->function_arguments[arg_count])
          break;
      
      int arg_selected = fast_rand() % arg_count;
      forward_change->value = arg_selected;
      reverse_change->value = arg_selected;
      
      // Figure out the required data type
      struct DataType_T data_type = codeline->function_arguments[arg_selected]->data_type;

      
      forward_change->specific_var = find_random_variable(function->first_var, &data_type, forward_change->related_vars);
      reverse_change->specific_var = codeline->function_arguments[arg_selected]; 
      
      // Check if we need to remove the variable because we deleted the last reference
      if (codeline->function_arguments[arg_selected]->references == 1 && codeline->function_arguments[arg_selected] != forward_change->specific_var) {
        for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
          if (!forward_change->related_vars[i]) {
            forward_change->related_vars[i] = codeline->function_arguments[arg_selected];
            break;
          }
        }
      }
    }
  }
  else if (r < 65 || !codeline) {
    // Insert new line before the current line
    struct CodeLine_T * new_codeline = malloc(sizeof(struct CodeLine_T));
    forward_change->type = CHANGE_TYPE_INSERT;
    forward_change->codelines[0] = new_codeline;
    reverse_change->type = CHANGE_TYPE_REMOVE;
    reverse_change->codelines[0] = new_codeline;
    
    new_codeline->assigned_variable = NULL;
    new_codeline->function_arguments[0] = NULL;
    new_codeline->next = codeline;
    
    // Select type
    r = fast_rand() % 100;

    if (r < 40) {
      // constant assignment
      new_codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
      new_codeline->assigned_variable = find_random_variable(function->first_var, NULL, forward_change->related_vars);
      if (new_codeline->assigned_variable->data_type.basic_type == BASICTYPE_CHAR)
        new_codeline->constant.c = fast_rand() % 128; // Any char
      else
        new_codeline->constant.i = fast_rand() % 40 - 20; // Any int from -20 to +20
    }
    else if (r < 45) {
      // return
      new_codeline->type = CODELINE_TYPE_RETURN;
      new_codeline->assigned_variable = find_random_variable(function->first_var, &function->return_datatype, forward_change->related_vars);
    }
    else if (r < 80) {
      // function call
      new_codeline->type = CODELINE_TYPE_FUNCTION_CALL;
      
      // Find function
      int function_id = fast_rand() % function_count;
      cfunction = context;
      while (function_id > 0) {
        cfunction = cfunction->next;
        function_id--;
      }
      new_codeline->target_function = cfunction;
      
      // Find assigning variable
      new_codeline->assigned_variable = find_random_variable(function->first_var, &cfunction->return_datatype, forward_change->related_vars);
      
      // Find arguments
      for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
        if (!cfunction->function_arguments[i]) {
          new_codeline->function_arguments[i] = NULL;
          break;
        }
        new_codeline->function_arguments[i] = find_random_variable(function->first_var, &cfunction->function_arguments[i]->data_type, forward_change->related_vars);
      }
      
        
    }
    else {
      // The statement we are going to add needs a block end, lets find it now
      struct CodeLine_T * second_new_codeline = malloc(sizeof(struct CodeLine_T));
      second_new_codeline->type = CODELINE_TYPE_BLOCK_END;
      second_new_codeline->assigned_variable = NULL;
      second_new_codeline->function_arguments[0] = NULL;
      struct CodeLine_T * second_codeline_ptr = codeline;
      
      while(second_codeline_ptr) {
        while (second_codeline_ptr->type == CODELINE_TYPE_IF || second_codeline_ptr->type == CODELINE_TYPE_WHILE){
          second_codeline_ptr = second_codeline_ptr->block_other_end->next; //  Jump over the if or while block
          if (!second_codeline_ptr)
            break;
        }
        if (!second_codeline_ptr)
          break;
        if (second_codeline_ptr->type == CODELINE_TYPE_BLOCK_END) {
          break; // If we run into a block end, put the thing here
        }
        second_codeline_ptr = second_codeline_ptr->next;
        if (!(fast_rand() % 6))
          break;
      }
      
      second_new_codeline->next = second_codeline_ptr;
      new_codeline->block_other_end = second_new_codeline;
      second_new_codeline->block_other_end = new_codeline;
      forward_change->codelines[1] = second_new_codeline;
      reverse_change->codelines[1] = second_new_codeline;
      
      if (r < 90) {
        // if statement
        new_codeline->type = CODELINE_TYPE_IF;
        new_codeline->condition = fast_rand() % 6; // 6 possible conditions
        new_codeline->function_arguments[0] = find_random_variable(function->first_var, NULL, forward_change->related_vars);
        new_codeline->function_arguments[1] = find_random_variable(function->first_var, &new_codeline->function_arguments[0]->data_type, forward_change->related_vars); // Copy the argument type from the first argument
        new_codeline->function_arguments[2] = NULL;
        
      }
      else{
        // while statement
        new_codeline->type = CODELINE_TYPE_WHILE;
        new_codeline->condition = fast_rand() % 6; // 6 possible conditions
        new_codeline->function_arguments[0] = find_random_variable(function->first_var, NULL, forward_change->related_vars);
        new_codeline->function_arguments[1] = find_random_variable(function->first_var, &new_codeline->function_arguments[0]->data_type, forward_change->related_vars); // Copy the argument type from the first argument
        new_codeline->function_arguments[2] = NULL;
      }
    }
  }
  else {
    // Remove this line
    forward_change->type = CHANGE_TYPE_REMOVE;
    reverse_change->type = CHANGE_TYPE_INSERT;
    
    // If this is a blockend, just use the other end of it
    if (codeline->type == CODELINE_TYPE_BLOCK_END)
      codeline = codeline->block_other_end;
    
    forward_change->codelines[0] = codeline;
    reverse_change->codelines[0] = codeline;
    
    // If this is a for or while loop, include the other end
    if (codeline->type == CODELINE_TYPE_IF || codeline->type == CODELINE_TYPE_WHILE) {
      forward_change->codelines[1] = codeline->block_other_end;
      reverse_change->codelines[1] = codeline->block_other_end;
    }
    
    // Scan for any orphaned variables to include in the change
    if (codeline->assigned_variable && codeline->assigned_variable->references == 1) {
      for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
        if (!forward_change->related_vars[i]) {
          forward_change->related_vars[i] = codeline->assigned_variable;
          break;
        }
      }
    }
    
    for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
      if (!codeline->function_arguments[i])
        break;
      if (codeline->function_arguments[i]->references == 1) {
        for (int j = 0; j < CHANGE_RELATED_VARS_LEN; j++) {
          if (!forward_change->related_vars[j]) {
            forward_change->related_vars[j] = codeline->function_arguments[i];
            break;
          }
        }
      }
    }
  }
  // Copy related vars from forward change to reverse change (because we were too lazy to put them in both places before)
  for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++)
    reverse_change->related_vars[i] = forward_change->related_vars[i];
  return forward_change;
}

void remove_var_from_function(struct Function_T * function, struct Variable_T * variable) {
  if (variable->prev)
    variable->prev->next = variable->next;
  else
    function->first_var = variable->next;
  if (variable->next)
    variable->next->prev = variable->prev;
  else
    function->last_var = variable->prev;
  variable->next = NULL;
  variable->prev = NULL;
  variable->parent_function = NULL;
}

void insert_var_into_function(struct Function_T * function, struct Variable_T * variable) {
  variable->next = NULL;
  variable->prev = function->last_var;
  if (function->last_var)
    function->last_var->next = variable;
  else
    function->first_var = variable;
  function->last_var = variable;
  variable->parent_function = function;
}

void apply_change(struct Function_T * function, struct Change_T * change){
  if (change->applied)
    return;
  int i = 0;
  struct Variable_T * old_var;
  switch (change->type) {
    case CHANGE_TYPE_INSERT:
      // Insert codelines
      for(i = 0; i < CHANGE_CODELINES_LEN; i++) {
        if (!change->codelines[i])
          continue;
        // Increment references
        if (change->codelines[i]->assigned_variable)
          change->codelines[i]->assigned_variable->references++;
        for (int j = 0; j < FUNCTION_ARG_COUNT; j++) {
          if (!change->codelines[i]->function_arguments[j])
            break;
          change->codelines[i]->function_arguments[j]->references++;
        }
        
        if (change->codelines[i]->next){
          change->codelines[i]->prev = change->codelines[i]->next->prev;
          change->codelines[i]->next->prev = change->codelines[i];
        }
        else {
          // Insert as last line
          change->codelines[i]->prev = function->last_codeline;
          function->last_codeline = change->codelines[i];
        }
        
        if (change->codelines[i]->prev){
          change->codelines[i]->prev->next = change->codelines[i];
        }
        else {
          function->first_codeline = change->codelines[i];
        }
      }
      for(int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
        // Add each related var to the var list
        if (!change->related_vars[i])
          break;
        insert_var_into_function(function, change->related_vars[i]);
      }
      break;
    case CHANGE_TYPE_REMOVE:
      // Remove codelines
      for(i = CHANGE_CODELINES_LEN-1; i >= 0; i--) {
        if (!change->codelines[i])
          continue;
        // Decrement references
        if (change->codelines[i]->assigned_variable)
          change->codelines[i]->assigned_variable->references--;
        for (int j = 0; j < FUNCTION_ARG_COUNT; j++) {
          if (!change->codelines[i]->function_arguments[j])
            break;
          change->codelines[i]->function_arguments[j]->references--;
        }
        
        if (change->codelines[i]->next) {
          change->codelines[i]->next->prev = change->codelines[i]->prev;
        }
        else {
          function->last_codeline = change->codelines[i]->prev;
        }
        if (change->codelines[i]->prev) {
          change->codelines[i]->prev->next = change->codelines[i]->next;
        }
        else {
          function->first_codeline = change->codelines[i]->next;
        }
      }
      for(int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
        // Remove each related variable from the var linked list
        if (!change->related_vars[i])
          break;
        remove_var_from_function(function, change->related_vars[i]);
      }
      break;
    case CHANGE_TYPE_ALTER_CONSTANT:
      if (change->codelines[0]->assigned_variable->data_type.basic_type == BASICTYPE_CHAR)
        change->codelines[0]->constant.c = (change->codelines[0]->constant.c + change->value)%256;
      else
        change->codelines[0]->constant.i += change->value;
      break;
    case CHANGE_TYPE_SWITCH_ARGUMENT:
      
      old_var = change->codelines[0]->function_arguments[change->value];
      old_var->references--;
      change->codelines[0]->function_arguments[change->value] = change->specific_var;
      change->specific_var->references++;
      
      for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
        if (!change->related_vars[i])
          break;
        if (change->related_vars[i] == old_var) // If the old var is mentioned in related_vars, we must remove it
          remove_var_from_function(function, change->related_vars[i]);
        if (change->related_vars[i] == change->specific_var)
          insert_var_into_function(function, change->related_vars[i]);
      }
      break;
  }
  change->applied = 1;
  change->inverse->applied = 0;
}

void free_change(struct Change_T * change) {
  if (change->type == CHANGE_TYPE_INSERT) {
    for (int i = 0; i < CHANGE_CODELINES_LEN; i++) {
      if (!change->codelines[i])
        break;
      free(change->codelines[i]);
    }
    for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
      if (!change->related_vars[i])
        break;
      free(change->related_vars[i]);
    }
  }
  if (change->type == CHANGE_TYPE_SWITCH_ARGUMENT) {
    for (int i = 0; i < CHANGE_RELATED_VARS_LEN; i++) {
      if (!change->related_vars[i])
        break;
      if (change->related_vars[i] == change->specific_var) // If the specific var is in the related vars, it needs to be removed
        free(change->related_vars[i]);
    }
  }
  free(change->inverse);
  free(change);
}


char * concat_generated_lines(struct GeneratedLine_T * generated_lines, int max_len) {
  // Figure out the total size before allocating the buffer
  int buffer_size_needed = 0;
  struct GeneratedLine_T * pointer = generated_lines;
  while(pointer) {
    buffer_size_needed += strlen(pointer->text) + 1; // text and a newline
    if (max_len == 0)
      break;
    max_len--;
    pointer = pointer->next;
  }
  buffer_size_needed += 1; // Add a null char to the end
  
  char * buffer = malloc(buffer_size_needed);
  pointer = generated_lines;
  int i = 0;
  while(pointer) {
    strcpy(buffer+i, pointer->text);
    i += strlen(pointer->text);
    buffer[i] = '\n';
    i++;
    if (i+1 == buffer_size_needed)
      break;
    pointer = pointer->next;
  }
  buffer[i] = '\0';
  return buffer;
}

void parser_loopback_test() {
  struct Function_T * start_func = build_example_tree();
  struct GeneratedLine_T * generated_start_func = generate_c(start_func);
  print_generated_lines(generated_start_func);
  char * start_func_text = concat_generated_lines(generated_start_func, -1);
  printf("Starting parse\n");
  struct Function_T * funcs = malloc(sizeof(struct Function_T)*3);
  funcs[0] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="+", .next=&(funcs[1]), .prev=NULL,};
  funcs[1] = (struct Function_T){.type=FUNCTION_TYPE_BUILTIN, .name="putchar", .next=NULL, .prev=&(funcs[0])};
  struct Function_T * second_func = parse_source_text(start_func_text, &(funcs[0]));
  printf("Finished parse\n");
  struct GeneratedLine_T * generated_second_func = generate_c(second_func);
  char * second_func_text = concat_generated_lines(generated_second_func, -1);
  print_generated_lines(generated_second_func);
  if (strcmp(start_func_text, second_func_text))
    printf("\n\nEPIC FAIL!\n\n");
  else
    printf("\n\nSUCCESS!\n\n");
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

void basic_interpreter_test() {
  struct Function_T * first_func = build_example_tree();
  struct GeneratedLine_T * generated_code = generate_c(first_func);
  print_generated_lines(generated_code);
  union InterpreterValue_T interpret_args[FUNCTION_ARG_COUNT] = {{.i=2}, {.i=4}};
  //for (int i = 0; i < 100000000; i++){
    interpret_function(first_func->next->next, interpret_args, NULL);
  //}
  printf("Did it!");
}

void parse_interpret_test(char * fname) {
  FILE * f = fopen(fname, "r");
  char buffer[1000];
  int i = 0;
  while (buffer[i] != EOF) {
    i++;
    buffer[i] = fgetc(f);
  }
  struct Function_T * context = build_context();
  struct Function_T * main_func = parse_source_text(buffer, context);
  main_func->prev = context->next;
  context->next->next = main_func;
  print_generated_lines(generate_c(main_func));
  union InterpreterValue_T interpret_args[FUNCTION_ARG_COUNT] = {{.i=2}, {.i=4}};
  interpret_function(main_func, interpret_args, NULL);
  printf("\n");
}

void print_function(struct Function_T * function) {
  struct GeneratedLine_T * generated = generate_c(function);
  struct GeneratedLine_T * tofree;
  char * text = concat_generated_lines(generated, -1);
  printf("%s", text);
  free(text);
  while (generated) {
    tofree = generated;
    generated = generated->next;
    free(tofree);
  }
}

void print_function_limited(struct Function_T * function) {
  struct GeneratedLine_T * generated = generate_c(function);
  struct GeneratedLine_T * tofree;
  char * text = concat_generated_lines(generated, 80);
  printf("%s", text);
  free(text);
  while (generated) {
    tofree = generated;
    generated = generated->next;
    free(tofree);
  }
}

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

struct Function_T * global_context;

struct Function_T * load_from_file(char * fname) {
  FILE * f = fopen(fname, "r");
  char buffer[1000];
  int i = 0;
  while (!i || buffer[i-1] != EOF) {
    buffer[i] = fgetc(f);
    i++;
  }
  return parse_source_text(buffer, global_context);
}

struct Function_T * build_empty_main() {
  struct Function_T * func = malloc(sizeof(struct Function_T));
  (*func) =  (struct Function_T){.type=FUNCTION_TYPE_CUSTOM, .name="main", .return_datatype={BASICTYPE_INT, 0}, .next=NULL, .prev=NULL, .first_var=NULL, .last_var=NULL, .function_arguments={NULL}};
  return func;
}

struct Function_T * build_main_two_args() {
  struct Function_T * func = malloc(sizeof(struct Function_T));
  struct DataType_T dt_i = {BASICTYPE_INT, 0};
  struct DataType_T dt_c = {BASICTYPE_CHAR, 0};
  struct Variable_T * vars = malloc(sizeof(struct Variable_T)*4);
  vars[0] = (struct Variable_T){.name="a", .is_arg=1, .references=1, .data_type=dt_i, .next=&(vars[1]), .prev=NULL};
  vars[1] = (struct Variable_T){.name="b", .is_arg=1, .references=1, .data_type=dt_i, .next=&(vars[2]), .prev=&(vars[0])};
  vars[2] = (struct Variable_T){.name="c", .is_arg=1, .references=1, .data_type=dt_i, .next=&(vars[3]), .prev=&(vars[1])};
  vars[3] = (struct Variable_T){.name="d", .is_arg=1, .references=1, .data_type=dt_c, .next=NULL, .prev=&(vars[2])};
  (*func) =  (struct Function_T){.type=FUNCTION_TYPE_CUSTOM, .name="main", .return_datatype={BASICTYPE_INT, 0}, .next=NULL, .prev=NULL, .first_var=&(vars[0]), .last_var=&(vars[3]), .function_arguments={&(vars[0]), &(vars[1]), &(vars[2]), &(vars[3]), NULL}};
  return func;
}

void test_code_changer() {
  global_context = build_context();
  struct Function_T * func = load_from_file("test.c");
  struct GeneratedLine_T * gen_one = generate_c(func);
  char * buff_one = concat_generated_lines(gen_one, -1);
  print_function(func);
  struct Change_T * change = find_new_change(func, global_context);
  apply_change(func, change);
  print_function(func);
  apply_change(func, change->inverse);
  struct GeneratedLine_T * gen_two = generate_c(func);
  char * buff_two = concat_generated_lines(gen_two, -1);
  print_function(func);
  if (strcmp(buff_one, buff_two))
    printf("\n\nEPIC FAIL!\n\n");
  else
    printf("\n\nSUCCESS!\n\n");
  
}

int get_fitness_expfit(struct Function_T * function){

  int score = 100;
  union InterpreterValue_T args[4];
  union InterpreterValue_T result;
  
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 1; j++) {
      args[0].i = i;
      args[1].i = j*10;
      result = interpret_function(function, args, &score);
      long goal = exp(args[0].i);
      long error = goal - result.i;
      if (error < 0)
        error = -error;
      if (error > 100)
        error = 100;
      score -= error*10;
    }
  }
  
  int code_len = 0;
  struct CodeLine_T * codeline = function->first_codeline;
  while (codeline) {
    code_len++;
    codeline = codeline->next;
  }
  score -= code_len;
  return score;
}

int get_fitness_calculator(struct Function_T * function){

  int score = 100;
  union InterpreterValue_T args[4];
  union InterpreterValue_T result;
  
  int ops[4] = {-10, -2, 2, 10};
  
  for (int i = 0; i < 20; i += 4) {
    for (int j = 1; j < 20; j += 4) {
      for (int k = 0; k < 4; k++) {
        args[0].i = i;
        args[1].i = j;
        args[2].i = ops[k];
        result = interpret_function(function, args, &score);
        long goal = 0;
        switch (k) {
          case 0:
            goal = i + j;
            break;
          case 1:
            goal = i - j;
            break;
          case 2:
            goal = i * j;
            break;
          case 3:
            goal = i / j;
            break;
        }
        long error = goal - result.i;
        if (error < 0)
          error = -error;
        //error = error*error;
        if (error > 100)
          error = 100;
        score -= error*10;
      }
    }
  }
  
  int code_len = 0;
  struct CodeLine_T * codeline = function->first_codeline;
  while (codeline) {
    code_len++;
    codeline = codeline->next;
  }
  score -= code_len;
  return score;
}

int get_fitness_count(struct Function_T * function){


  int score = 0;
  union InterpreterValue_T args[4];
  
  for (int i = 0; i < 30; i++) {
    for (int j = 0; j < 3; j++) {
      args[0].i = i;
      args[3].c = j + 97;
      
      for (int k = 0; k < PUTCHAR_BUFF_LEN; k++)
        putchar_buff[k] = '\0';
      putchar_i = 0;

      interpret_function(function, args, &score);

      for (int k = 0; k < i; k++)
        if (putchar_buff[k] != args[3].c)
          score -= 500;
      if (putchar_buff[i] != '\0')
        score -= 500;
    }
  }

  score = score/90;
  
  int code_len = 0;
  struct CodeLine_T * codeline = function->first_codeline;
  while (codeline) {
    code_len++;
    codeline = codeline->next;
  }
  score -= code_len;
  
  return score + 200 ;
}


int get_fitness_hello_world(struct Function_T * function){
  // Reset putchar buff
  for (int i = 0; i < PUTCHAR_BUFF_LEN; i++)
    putchar_buff[i] = 1;
  putchar_i = 0;
  // Execute!
  int score = 100;
  union InterpreterValue_T args[2];
  args[0].i = 0;
  args[1].i = 0;
  args[2].i = 0;
  args[3].c = '\0';
  interpret_function(function, args, &score);
  // Score

  
  char * goal = "Hello world!";
  int len = strlen(goal) + 1;
  for (int i = 0; i < len; i++) {
    if (goal[i] != putchar_buff[i])
      score -= 1000;
    int error = goal[i] - putchar_buff[i];
    if (error < 0)
      error = -error;
    //error = error * error;
    if (error > 50)
      error = 50;
    score -= error*100;
  }

  return score;
}

int get_fitness(struct Function_T * function){
  //return get_fitness_expfit(function);
  //return get_fitness_calculator(function);
  return get_fitness_count(function);
  //return get_fitness_hello_world(function);
}

int does_pass(float new_score, float old_score, float temperature) {
  if (new_score > old_score)
    return 1;
    
  float p = exp(-(old_score - new_score)/(temperature*10));
  return fast_rand() < 32767.0 * p;
}

double getUnixTime(void)
{
    struct timespec tv;

    if(clock_gettime(CLOCK_REALTIME, &tv) != 0) return 0;

    return (tv.tv_sec + (tv.tv_nsec / 1000000000.0));
}

void train_code(long iterations) {
  global_context = build_context();
  struct Function_T * function = build_main_two_args();
  int score = get_fitness(function);
  double last_update = getUnixTime();
  long i;
  short change_history[100];
  for (i = 0; i < iterations; i++) {
    struct Change_T * change = find_new_change(function, global_context);
    apply_change(function, change);
    //check_references(function);
    int new_score = get_fitness(function);
    if (does_pass(new_score, score, (float)(iterations - i)/(iterations))) {
      // Keep the new code
      change_history[i%100] = 1;
      score = new_score;
      free_change(change->inverse);
      //print_function(function);
    }
    else {
      // Revert to the old code
      change_history[i%100] = 0;
      apply_change(function, change->inverse);
      //check_references(function);
      free_change(change);
    }
    if (!(i % 100) && getUnixTime()-last_update > 0.5) {
      last_update += 0.5;
      int change_count;
      for (int j = 0; j < 100; j++)
        change_count += change_history[j];
      score = get_fitness(function);
      printf("\e[1;1H\e[2J");
      printf("\n\n\n");
      printf("%s\n", putchar_buff);
      printf("We got a score of %i in %'i / %'i iterations.\n\n", score, i, iterations);
      print_function_limited(function);
      setlocale(LC_NUMERIC, "");
    }
  }
  score = get_fitness(function);
  print_function_limited(function);
  setlocale(LC_NUMERIC, "");
  printf("\e[1;1H\e[2J");
  printf("\n\n\n");
  printf("%s\n", putchar_buff);
  printf("We got a final score of %i in %'i iterations.\n\n", score, i);
  print_function(function);
}

#define CHANGE_HISTORY_LEN 10000
void train_code_v2() {
  global_context = build_context();
  struct Function_T * function = build_main_two_args();
  int score = get_fitness(function);
  
  double start_time = getUnixTime();
  double last_update = getUnixTime();
  
  int last_batch_score = 0;
  long last_batch_iterations = 0;
  int batch_num = 0;
  
  short change_history[CHANGE_HISTORY_LEN];
  
  long max_iterations = 100000;
  long total_iterations = 0;
  while (1) {
    long i;
    for (i = 0; i < max_iterations; i++) {
      struct Change_T * change = find_new_change(function, global_context);
      apply_change(function, change);
      int new_score = get_fitness(function);
      if (does_pass(new_score, score, (float)(max_iterations - i)/(float)(max_iterations))) {
        // Keep the new code
        change_history[i%CHANGE_HISTORY_LEN] = 1;
        score = new_score;
        free_change(change->inverse);
      }
      else {
        // Revert to the old code
        change_history[i%CHANGE_HISTORY_LEN] = 0;
        apply_change(function, change->inverse);
        free_change(change);
      }
      if (!(i % 100) && getUnixTime()-last_update > 0.5) {
        last_update += 0.5;
        int change_count = 0;
        for (int j = 0; j < CHANGE_HISTORY_LEN; j++)
          change_count += change_history[j];
        score = get_fitness(function);
        printf("\e[1;1H\e[2J"); // Clear screen
        printf("\nLast cycle: #%i \n Score = %i \n Iterations = %'i\n", batch_num-1, last_batch_score, last_batch_iterations);
        printf("\nCurrent cycle: #%i \n Output = %s \n Score = %i \n Mystery number: %f \n Changes Accepted = %%%i \n Iterations = %'i / %'i\n\n", batch_num, putchar_buff, score, (float)(max_iterations - i)/(float)(max_iterations), change_count/(CHANGE_HISTORY_LEN/100), i, max_iterations);
        print_function_limited(function);
        
      }
      total_iterations++;
    }
    score = get_fitness(function);
    if (score > 0) {
      score = get_fitness(function);
      printf("\e[1;1H\e[2J"); // Clear screen
      printf("\nLast cycle: #%i \n Max Score = %i \n Iterations = %'i\n", batch_num-1, last_batch_score, last_batch_iterations);
      printf("\nCurrent cycle: \n Output = %s \n Score = %i \n Iterations = %'i / %'i\n\n", putchar_buff, score, i, max_iterations);
      print_function_limited(function);
      printf("Goal achieved, keep going? (Y/N)");
      char buff[10];
      gets(buff);
      if (buff[0] != 'Y' && buff[0] != 'y')
        break;
      last_update = getUnixTime();
    }
    last_batch_score = score;
    last_batch_iterations = max_iterations;
    max_iterations = max_iterations * 2;
    batch_num++;
  }
  score = get_fitness(function);
  print_function_limited(function);
  printf("\e[1;1H\e[2J");
  printf("\n\n\n");
  printf("%s\n", putchar_buff);
  printf("I got a final score of %i in %'i iterations over %i cycles.\n\n", score, total_iterations, batch_num);
  print_function(function);
}

int main(int argc, char * argv[]){
  // Seed random num
  fast_srand(time(NULL));
  setlocale(LC_NUMERIC, "");
  //parse_interpret_test("test.c");
  //test_code_changer();
  train_code_v2();
}
