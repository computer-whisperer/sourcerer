#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "c_types.h"

struct Function_T * get_first_function(struct Function_T * function) {
  if (function)
    while (function->prev)
      function = function->prev;
  return function;
}

struct Function_T * get_last_function(struct Function_T * function) {
  if (function)
    while (function->next)
      function = function->next;
  return function;
}

int is_same_datatype(struct DataType_T a, struct DataType_T b) {
  return a.basic_type == b.basic_type && a.pointer_level == b.pointer_level;
}

int get_reference_count(struct Variable_T * var, struct Function_T * function) {
  struct CodeLine_T * codeline = function->first_codeline;
  int count = var->is_arg; // Argument counts as a reference
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

struct Variable_T * variable_name_search(struct Variable_T * var, char * name) {
  while (var) {
    if (!strcmp(name, var->name)) 
      break;
    var = var->next;
  }
  return var;
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

struct Function_T * build_main_four_args() {
  struct Function_T * func = malloc(sizeof(struct Function_T));
  struct DataType_T dt_i = {BASICTYPE_INT, 0};
  struct DataType_T dt_c = {BASICTYPE_CHAR, 0};
  int var_count = 4;
  struct Variable_T * vars = malloc(sizeof(struct Variable_T)*var_count);
  vars[0] = (struct Variable_T){.name="a", .is_arg=1, .references=1, .data_type=dt_i};
  vars[1] = (struct Variable_T){.name="b", .is_arg=1, .references=1, .data_type=dt_i};
  vars[2] = (struct Variable_T){.name="c", .is_arg=1, .references=1, .data_type=dt_i};
  vars[3] = (struct Variable_T){.name="d", .is_arg=1, .references=1, .data_type=dt_c};
  (*func) =  (struct Function_T){.type=FUNCTION_TYPE_CUSTOM, .name="main", .return_datatype={BASICTYPE_INT, 0}, .next=build_context(), .prev=NULL, .first_var=&(vars[0]), .last_var=&(vars[var_count-1]), .function_arguments={&(vars[0]), &(vars[1]), &(vars[2]), &(vars[3]), NULL}};
  func->next->prev = func;
  for (int i = 0; i < var_count; i++) {
    vars[i].parent_function = func;
    vars[i].next = vars + i + 1;
    vars[i].prev = vars + i - 1;
  }
  vars[0].prev = NULL;
  vars[var_count-1].next = NULL;
  return func;
}

int is_function_valid(struct Function_T * first_function, struct Function_T * function) {
  struct Function_T * function_ptr = first_function;
  while (function_ptr) {
    if (function_ptr == function)
      return 1;
    function_ptr = function_ptr->next;
  }
  return 0;
}

int is_variable_valid(struct Function_T * function, struct Variable_T * variable) {
  struct Variable_T * variable_ptr = function->first_var;
  while (variable_ptr) {
    if (variable_ptr == variable)
      return 1;
    variable_ptr = variable_ptr->next;
  }
  return 0;
}

// Returns 0 for success, -1 for failure
int assert_full_structure_integrity(struct Function_T * first_function) {
  struct Function_T * function = first_function;
  int i;
  if (function->prev){
    printf("Function list not linked correctly.\n");
    return -1;
  }
  while (function) {
    
    // Assert linked list integrity
    if (function->next && function != function->next->prev) {
      printf("Function list not linked correctly.\n");
      return -1;
    }
    
    // Otherwise skip checks for builtin functions
    if (function->type == FUNCTION_TYPE_CUSTOM) {
    
      // Assert variable list integrity
      struct Variable_T * variable = function->first_var;
      if (variable->prev) {
        printf("Variable list not linked correctly.\n");
        return -1;
      }
      while (variable) {
        // Check function link
        if (variable->parent_function != function) {
          printf("Variable not linked back to function.\n");
          return -1;
        }
        // Check linked list
        if (variable->next && variable->next->prev != variable) {
          printf("Variable list not linked correctly.\n");
          return -1;
        }
        // Check if is_arg is valid
        if (variable->is_arg) {
          int found = 0;
          for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
            if (!function->function_arguments[i])
              break;
            if (function->function_arguments[i] == variable) {
              found = 1;
              break;
            }
          }
          if (!found) {
            printf("Variable has is_arg set but is not in the argument list.\n");
            return -1;
          }
        }
        // Check reference count
        if (get_reference_count(variable, function) != variable->references) {
          printf("Variable has incorrect reference count.\n");
          return -1;
        }
        // Check name uniqueness
        if (variable_name_search(function->first_var, variable->name) != variable) {
          // If a name is repeated in a function, then it will get caught when the name_search is performed on the second one.
          printf("Two variables in the same function have the same name.\n");
          return -1;
        }
        
        // Check last var
        if (!function->next && variable != function->last_var) {
          printf("Function last_var is not the last variable in the linked list.\n");
          return -1;
        }
        
        variable = variable->next;
      }
      
      // Verify argument list
      for (i = 0; i < FUNCTION_ARG_COUNT; i++) {
        variable = function->function_arguments[i];
        if (!variable)
          break;
        if (!is_variable_valid(function, variable)) {
          printf("Variable is in function argument list but not listed in the function variables.\n");
          return -1;
        }
        if (!variable->is_arg) {
          printf("Variable is in function argument list but is_arg is 0.\n");
          return -1;
        }
      }
      
      if (function->type == FUNCTION_TYPE_CUSTOM) {
        struct CodeLine_T * codeline = function->first_codeline;
        if (codeline && codeline->prev) {
          printf("Codeline list not linked correctly.\n");
          return -1;
        }
        
        while (codeline) {
          if (codeline->next && codeline->next->prev != codeline) {
            printf("Codeline list not linked correctly.\n");
            return -1;
          }
          if (codeline->parent_function != function) {
            printf("Codeline not linked back to function.\n");
            return -1;
          }
          if (codeline->type >= CODELINE_TYPE_COUNT) {
            printf("Codeline has invalid type.\n");
            return -1;
          }
          
          switch (codeline->type) {
            
            case CODELINE_TYPE_CONSTANT_ASSIGNMENT:
              if (!is_variable_valid(function, codeline->assigned_variable)) {
                printf("Codeline references invalid assigned_variable.\n");
                return -1;
              }
              break;
              
            case CODELINE_TYPE_FUNCTION_CALL:
              if (!is_variable_valid(function, codeline->assigned_variable)) {
                printf("Codeline references invalid assigned_variable.\n");
                return -1;
              }
              if (!is_function_valid(first_function, codeline->target_function)) {
                printf("Codeline references invalid target_function.\n");
                return -1;
              }
              for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
                variable = codeline->function_arguments[i];
                if (!variable)
                  break;
                if (!is_variable_valid(function, variable)) {
                  printf("Codeline references an invalid variable as function argument.\n");
                  return -1;
                }
                if (!codeline->target_function->function_arguments[i]) {
                  printf("Codeline has more arguments than target function.\n");
                  return -1;
                }
                if (!is_same_datatype(codeline->target_function->function_arguments[i]->data_type, variable->data_type)) {
                  printf("Codeline has argument that does not match the type of the argument in the target_function.\n");
                  return -1;
                }
              }
              if (i < FUNCTION_ARG_COUNT && codeline->target_function->function_arguments[i]) {
                printf("Codeline has fewer arguments than target function.\n");
                return -1;
              }
              break;
            
            case CODELINE_TYPE_RETURN:
              if (!is_variable_valid(function, codeline->assigned_variable)) {
                printf("Codeline references invalid assigned_variable.\n");
                return -1;
              }
              if (!is_same_datatype(codeline->assigned_variable->data_type, function->return_datatype)) {
                printf("Codeline is trying to return a variable of the wrong type.\n");
                return -1;
              }
              break;
            
            case CODELINE_TYPE_IF:
            case CODELINE_TYPE_WHILE:
              if (!is_variable_valid(function, codeline->function_arguments[0])) {
                printf("Codeline references invalid function argument.\n");
                return -1;
              }
              if (!is_variable_valid(function, codeline->function_arguments[1])) {
                printf("Codeline references invalid function argument.\n");
                return -1;
              }
              if (codeline->condition >= CONDITION_COUNT) {
                printf("Codeline has invalid condition.\n");
                return -1;
              }
              int found = 0;
              int nest_level = 0;
              struct CodeLine_T * codeline_ptr = codeline->next;
              while (codeline_ptr) {
                if (codeline_ptr == codeline->block_other_end) {
                  found = 1;
                  break;
                }
                switch (codeline_ptr->type) {
                  case CODELINE_TYPE_IF:
                  case CODELINE_TYPE_WHILE:
                    nest_level++;
                    break;
                  case CODELINE_TYPE_BLOCK_END:
                    nest_level--;
                    break;
                  default:
                    break;
                }
                codeline_ptr = codeline_ptr->next;
              }
              if (!found || codeline->block_other_end->type != CODELINE_TYPE_BLOCK_END) {
                printf("Codeline has invalid block_other_end.\n");
                return -1;
              }
              if (nest_level != 0) {
                printf("Codeline if or while block is lopsided (another block contains one end).\n");
                return -1;
              }
              break;
            
            case CODELINE_TYPE_BLOCK_END:
              found = 0;
              codeline_ptr = codeline->prev;
              while (codeline_ptr) {
                if (codeline_ptr == codeline->block_other_end) {
                  found = 1;
                  break;
                }
                codeline_ptr = codeline_ptr->prev;
              }
              if (!found || (codeline->block_other_end->type != CODELINE_TYPE_IF && codeline->block_other_end->type != CODELINE_TYPE_WHILE)) {
                printf("Codeline has invalid block_other_end.\n");
                return -1;
              }
              break;
          }
          if (!codeline->next && function->last_codeline != codeline) {
            printf("Function last codeline is not the last codeline.\n");
            return -1;
          }
          codeline = codeline->next;
        }
      }
    }
    function = function->next;
  }
  return 0;
}

struct Change_T * apply_change(struct Change_T * change) {
  
  struct Change_T * inverse_change = NULL;
  struct Function_T * function = change->function;
  
  while (change) {
    struct CodeLine_T * codeline = change->codeline;
    struct Variable_T * variable = change->variable;
    
    struct Change_T * next_change = change->next;
    
    struct Variable_T * old_var;
    union ConstantValue_T old_cons;
    
    // In each case, perform change and morph the change object to its inverse
    switch (change->type) {
      case CHANGE_TYPE_INSERT_CODELINE:
        // Modify linked lists
        codeline->next = change->next_codeline;
        if (codeline->next) {
          codeline->prev = change->next_codeline->prev;
          change->next_codeline->prev = codeline;
        }
        else {
          codeline->prev = function->last_codeline;
          function->last_codeline = codeline;
        }
          
        if (codeline->prev) {
          codeline->prev->next = codeline;
        }
        else {
          function->first_codeline = codeline;
        }
        codeline->parent_function = function;
        
        // Update reference counts
        if (codeline->assigned_variable)
          codeline->assigned_variable->references++;
        
        for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
          if (!codeline->function_arguments[i])
            break;
          codeline->function_arguments[i]->references++;
        }
        
        // Invert change object
        change->type = CHANGE_TYPE_REMOVE_CODELINE;
        break;
        
        
      case CHANGE_TYPE_REMOVE_CODELINE:
        // Invert change object
        change->type = CHANGE_TYPE_INSERT_CODELINE;
        change->next_codeline = codeline->next;
      
        // Modify linked lists
        if (codeline->next)
          codeline->next->prev = codeline->prev;
        else
          function->last_codeline = codeline->prev;
        
        if (codeline->prev)
          codeline->prev->next = codeline->next;
        else
          function->first_codeline = codeline->next;
        
        codeline->next = NULL;
        codeline->prev = NULL;
        codeline->parent_function = NULL;
        
        // Update reference counts
        if (codeline->assigned_variable)
          codeline->assigned_variable->references--;
        
        for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
          if (!codeline->function_arguments[i])
            break;
          codeline->function_arguments[i]->references--;
        }
        
        break;
      
      
      case CHANGE_TYPE_INSERT_VARIABLE:
      
        if (!variable->name[0]) {
          // Find unused name if this variable had none previously
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
            if (!variable_name_search(function->first_var, variable->name))
              break; // Good find!
          }
        }

        // Update linked lists
        variable->next = NULL;
        variable->prev = function->last_var;
        
        if (variable->prev)
          variable->prev->next = variable;
        else
          function->first_var = variable;
        
        function->last_var = variable;
        variable->parent_function = function;
        variable->references = 0;
        
        // Invert change object
        change->type = CHANGE_TYPE_REMOVE_VARIABLE;
        
        break;
      
      
      case CHANGE_TYPE_REMOVE_VARIABLE:
        if (variable->prev)
          variable->prev->next = variable->next;
        else
          function->first_var = variable->next;
        
        if (variable->next)
          variable->next->prev = variable->prev;
        else
          function->last_var = variable->prev;
        
        variable->prev = NULL;
        variable->next = NULL;
        variable->parent_function = NULL;
        variable->references = 0;
        
        // Invert change object
        change->type = CHANGE_TYPE_INSERT_VARIABLE;
        break;
      
      
      case CHANGE_TYPE_ALTER_CONSTANT:
        old_cons = codeline->constant;
        codeline->constant = change->constant;
        
        // Invert change object
        change->constant = old_cons;
        break;
      
      
      case CHANGE_TYPE_ALTER_ASSIGNED_VARIABLE:
        old_var = codeline->assigned_variable;
        codeline->assigned_variable = variable;
        
        // Update reference counts
        variable->references++;
        old_var->references--;
        
        // Invert change object
        change->variable = old_var;
        break;
      
      case CHANGE_TYPE_ALTER_ARGUMENT:
        old_var = codeline->function_arguments[change->argument_index];
        codeline->function_arguments[change->argument_index] = variable;
        
        // Update reference counts
        variable->references++;
        old_var->references--;
        
        // Invert change object
        change->variable = old_var;
        break;
      
      default:
        printf("Invalid change!\n");
    }
    
    change->next = inverse_change;
    inverse_change = change;
    change = next_change;
  }
  
    
  if (assert_full_structure_integrity(get_first_function(function)))
    exit(1);
  
  return inverse_change;
}

void free_change(struct Change_T * change) {
  while (change) {
    
    switch (change->type) {
      case CHANGE_TYPE_INSERT_CODELINE:
        free(change->codeline);
        break;
      case CHANGE_TYPE_INSERT_VARIABLE:
        free(change->variable);
        break;
      
      // No actions required for these
      case CHANGE_TYPE_REMOVE_CODELINE:
      case CHANGE_TYPE_REMOVE_VARIABLE:
      case CHANGE_TYPE_ALTER_ARGUMENT:
      case CHANGE_TYPE_ALTER_ASSIGNED_VARIABLE:
      case CHANGE_TYPE_ALTER_CONSTANT:
        break;
    }  
    
    // Free the world from the perils of change!
    struct Change_T * change_to_free = change;
    change = change->next;
    free(change_to_free);
  }
}
