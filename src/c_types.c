#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "membox.h"
#include "c_types.h"
#include "changes.h"
#include "change_proposers.h"


int get_reference_count(struct Variable_T * var, struct Function_T * function) {
  struct CodeLine_T * codeline = function->first_codeline;
  int count = var->is_arg; // Argument counts as a reference
  while (codeline) {
    if (codeline->assigned_variable && codeline->assigned_variable == var)
      count++;
    for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
      if (!codeline->args[i])
        break;
      if (codeline->args[i] == var)
        count++;
    }
    codeline = codeline->next;
  }
  return count;
}

int get_codeline_count(struct Function_T * function) {
  struct CodeLine_T * codeline = function->first_codeline;
  int count = 0;
  while (codeline) {
    count++;
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

struct DataType_T * add_new_datatype(struct Environment_T * environment, struct DataType_T start) {
  struct DataType_T * base_data_type = malloc(sizeof(struct DataType_T));
  *base_data_type = start;
  base_data_type->environment = environment;
  base_data_type->pointer_degree = 0;
  base_data_type->pointing_to = NULL;
  base_data_type->pointed_from = NULL;
  
  // Add to linked list
  base_data_type->prev = environment->last_datatype;
  base_data_type->next = NULL;
  if (base_data_type->prev)
    base_data_type->prev->next = base_data_type;
  else
    environment->first_datatype = base_data_type;
  environment->last_datatype = base_data_type;
  
  struct DataType_T * last_type = base_data_type;
  for (int i = 1; i < MAX_POINTER_DEPTH; i++) {
    struct DataType_T * pointer_data_type = malloc(sizeof(struct DataType_T));
    pointer_data_type->type = DATATYPETYPE_POINTER;
    pointer_data_type->environment = environment;
    pointer_data_type->pointer_degree = last_type->pointer_degree + 1;
    pointer_data_type->size = sizeof(void *);
    
    // Set name
    strcpy(pointer_data_type->name, last_type->name);
    
    pointer_data_type->name[strlen(last_type->name)] = '*';
    pointer_data_type->name[strlen(last_type->name)+1] = '\0';
    
    // Link to other pointer types
    pointer_data_type->pointing_to = last_type;
    pointer_data_type->pointing_to->pointed_from = pointer_data_type;
    pointer_data_type->pointed_from = NULL;
    
    // Add to linked list
    pointer_data_type->prev = environment->last_datatype;
    pointer_data_type->next = NULL;
    if (pointer_data_type->prev)
      pointer_data_type->prev->next = pointer_data_type;
    else
      environment->first_datatype = pointer_data_type;
    environment->last_datatype = pointer_data_type;
    
    last_type = pointer_data_type;
  }
  
  return base_data_type;
}


struct Function_T * add_new_function(struct Environment_T * environment, char name[], struct DataType_T * return_datatype, struct DataType_T * argument_types[], enum FunctionType_T type) {
  struct Function_T * function = malloc(sizeof(struct Function_T));
  function->type = type;
  function->environment = environment;
  strcpy(function->name, name);
  function->return_datatype = return_datatype;
  
  function->codeline_count = 0;
  
  function->first_codeline = NULL;
  function->last_codeline = NULL;
  
  function->first_variable = NULL;
  function->last_variable = NULL;
  
  // Add to linked list
  function->next = NULL;
  function->prev = environment->last_function;
  environment->last_function = function;
  if (function->prev)
    function->prev->next = function;
  else
    environment->first_function = function;
  
  // Build variables for arguments
  for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
    if (!argument_types[i]) {
      function->args[i] = NULL;
      continue;
    }
    // New variable
    struct Variable_T * variable = malloc(sizeof(struct Variable_T));
    variable->name[0] = i + 97;
    variable->name[1] = '\0';
    variable->is_arg = 1;
    variable->function = function;
    variable->data_type = argument_types[i];
    variable->references = 1;
    
    // Add to arg list
    function->args[i] = variable;
    
    // Add to linked list
    variable->next = NULL;
    variable->prev = function->last_variable;
    function->last_variable = variable;
    if (variable->prev)
      variable->prev->next = variable;
    else
      function->first_variable = variable;
  }
  return function;
}

void free_function(struct Function_T * function) {
	struct Variable_T * variable = function->first_variable;
	struct Variable_T * var_to_free;
	while (variable) {
		var_to_free = variable;
		variable = variable->next;
		free(var_to_free);
	}

	struct CodeLine_T * codeline = function->first_codeline;
	struct CodeLine_T * codeline_to_free;
	while (codeline){
		codeline_to_free = codeline;
		codeline = codeline->next;
		free(codeline_to_free);
	}

	if (function->prev)
		function->prev->next = function->next;
	else
		function->environment->first_function = function->next;

	if (function->next)
		function->next->prev = function->prev;
	else
		function->environment->last_function = function->prev;
	free(function);
}

void randomly_populate_function(struct Function_T * function, int codeline_count) {
  while (function->codeline_count < codeline_count) {
    struct Change_T * change = propose_random_change(function);
    if (change->type != CHANGE_TYPE_REMOVE_CODELINE)
      free_change(apply_change(change));
    else
      free_change(change);
  }
}

struct Environment_T * build_new_environment(size_t membox_size) {
  struct Environment_T * environment = malloc(sizeof(struct Environment_T));
  environment->first_datatype = NULL;
  environment->last_datatype = NULL;
  environment->first_function = NULL;
  environment->last_function = NULL;
  
  // Initialize basic datatypes
  environment->int_datatype = add_new_datatype(environment, (struct DataType_T){.type=DATATYPETYPE_PRIMITIVE, .name="int", .size=sizeof(int)});
  environment->char_datatype = add_new_datatype(environment, (struct DataType_T){.type=DATATYPETYPE_PRIMITIVE, .name="char", .size=sizeof(char)});
  
  // Add available functions
  struct DataType_T * basic_int_args[] = {environment->int_datatype, environment->int_datatype, NULL, NULL, NULL, NULL, NULL, NULL};
  add_new_function(environment, "+", environment->int_datatype, basic_int_args, FUNCTION_TYPE_BASIC_OP);
  add_new_function(environment, "-", environment->int_datatype, basic_int_args, FUNCTION_TYPE_BASIC_OP);
  add_new_function(environment, "*", environment->int_datatype, basic_int_args, FUNCTION_TYPE_BASIC_OP);
  add_new_function(environment, "/", environment->int_datatype, basic_int_args, FUNCTION_TYPE_BASIC_OP);
  //add_new_function(environment, "%", environment->int_datatype, basic_int_args, FUNCTION_TYPE_BASIC_OP);
  
  //struct DataType_T * fancy_int_args[] = {environment->char_datatype, environment->int_datatype, NULL};
  //add_new_function(environment, "+", environment->char_datatype, fancy_int_args, FUNCTION_TYPE_BASIC_OP);
  
  struct DataType_T * putchar_args[] = {environment->char_datatype, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
  add_new_function(environment, "putchar", environment->char_datatype, putchar_args, FUNCTION_TYPE_BUILTIN);
  //add_new_function(environment, "farticus", environment->char_datatype, putchar_args, FUNCTION_TYPE_BUILTIN);
  
  struct DataType_T * main_args[] = {environment->int_datatype, environment->int_datatype, environment->int_datatype, environment->char_datatype, NULL, NULL, NULL, NULL};
  environment->main = add_new_function(environment, "main", environment->int_datatype, main_args, FUNCTION_TYPE_CUSTOM);
  
  // Init memory box
  environment->membox = build_membox(membox_size);
  return environment;
}

int is_datatype_in_environment(struct Environment_T * environment, struct DataType_T * data_type) {
  struct DataType_T * data_type_ptr = environment->first_datatype;
  while (data_type_ptr) {
    if (data_type_ptr == data_type)
      return 1;
    data_type_ptr = data_type_ptr->next;
  }
  return 0;
}

int is_function_in_environment(struct Environment_T * environment, struct Function_T * function) {
  struct Function_T * function_ptr = environment->first_function;
  while (function_ptr) {
    if (function_ptr == function)
      return 1;
    function_ptr = function_ptr->next;
  }
  return 0;
}

struct DataType_T * datatype_pointer_jump(struct DataType_T * origin, int reference_count) {
  if (reference_count == 0)
    return origin;
  if (!origin)
    return origin;
  
  if (reference_count > 0)
    return datatype_pointer_jump(origin->pointing_to, reference_count-1);
  else
    return datatype_pointer_jump(origin->pointed_from, reference_count+1);
}

int is_variable_in_function(struct Function_T * function, struct Variable_T * variable) {
  struct Variable_T * variable_ptr = function->first_variable;
  while (variable_ptr) {
    if (variable_ptr == variable)
      return 1;
    variable_ptr = variable_ptr->next;
  }
  return 0;
}

int get_pointer_degree(struct DataType_T * data_type) {
  int degree = 0;
  while (data_type && data_type->type == DATATYPETYPE_POINTER) {
    degree++;
    data_type = data_type->pointing_to;
  }
  return degree;
}

// Returns 0 for success, -1 for failure
int assert_environment_integrity(struct Environment_T * environment) {
  
  // Validate datatypes
  struct DataType_T * data_type = environment->first_datatype;
  if ((data_type && data_type->prev) || (!data_type && environment->last_datatype)) {
    printf("Data type list not linked correctly. \n");
    return -1;
  }
  while (data_type) {
    if (data_type->next && data_type != data_type->next->prev) {
      printf("Data type list not linked correctly. \n");
      return -1;
    }
    
    if (data_type->environment != environment) {
      printf("Data type not linked to environment. \n");
      return -1;
    }
    
    if (data_type->type == DATATYPETYPE_POINTER) {
      if (!data_type->pointing_to) {
        printf("Data type with type pointer does not have a pointing_to value. \n");
        return -1;
      }
      if (data_type->pointer_degree != get_pointer_degree(data_type)) {
        printf("Data type pointer has incorrect pointer degree.\n");
        return -1;
      }
    }
    
    if (data_type->pointing_to && !is_datatype_in_environment(environment, data_type->pointing_to)) {
      printf("Data type has invalid pointing_to link. \n");
      return -1;
    }
    
    if (data_type->pointed_from && !is_datatype_in_environment(environment, data_type->pointed_from)) {
      printf("Data type has invalid pointed_from link. \n");
      return -1;
    }
    
    if (!data_type->next && data_type != environment->last_datatype) {
      printf("Data type list not linked correctly. \n");
      return -1;
    }
    
    data_type = data_type->next;
      
  }
  
  // Validate functions
  struct Function_T * function = environment->first_function;
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
    
    if (function->environment != environment) {
      printf("Function not linked to environment.\n");
      return -1;
    }
    
    if (function->codeline_count != get_codeline_count(function)) {
      printf("Function codeline count is incorrect.\n");
      return -1;
    }
    
    // Otherwise skip checks for builtin functions
    if (function->type == FUNCTION_TYPE_CUSTOM) {
    
      // Assert variable list integrity
      struct Variable_T * variable = function->first_variable;
      if (variable->prev) {
        printf("Variable list not linked correctly.\n");
        return -1;
      }
      while (variable) {
        // Check function link
        if (variable->function != function) {
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
            if (!function->args[i])
              break;
            if (function->args[i] == variable) {
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
        if (variable_name_search(function->first_variable, variable->name) != variable) {
          // If a name is repeated in a function, then it will get caught when the name_search is performed on the second one.
          printf("Two variables in the same function have the same name.\n");
          return -1;
        }
        
        // Check last var
        if (!variable->next && variable != function->last_variable) {
          printf("Function last_variable is not the last variable in the linked list.\n");
          return -1;
        }
        
        variable = variable->next;
      }
      
      // Verify argument list
      for (i = 0; i < FUNCTION_ARG_COUNT; i++) {
        variable = function->args[i];
        if (!variable)
          break;
        if (!is_variable_in_function(function, variable)) {
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
          if (codeline->function != function) {
            printf("Codeline not linked back to function.\n");
            return -1;
          }
          if (codeline->type >= CODELINE_TYPE_COUNT) {
            printf("Codeline has invalid type.\n");
            return -1;
          }
          
          switch (codeline->type) {
            
            case CODELINE_TYPE_CONSTANT_ASSIGNMENT:
              if (!is_variable_in_function(function, codeline->assigned_variable)) {
                printf("Codeline references invalid assigned_variable.\n");
                return -1;
              }
              if (codeline->assigned_variable->data_type->type == DATATYPETYPE_POINTER) {
                printf("Codeline is trying to assign a constant to a pointer.\n");
                return -1;
              }
              break;
              
            case CODELINE_TYPE_FUNCTION_CALL:
              if (!is_variable_in_function(function, codeline->assigned_variable)) {
                printf("Codeline references invalid assigned_variable.\n");
                return -1;
              }
              if (!is_function_in_environment(environment, codeline->target_function)) {
                printf("Codeline references invalid target_function.\n");
                return -1;
              }
              for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
                variable = codeline->args[i];
                if (!variable)
                  break;
                if (!is_variable_in_function(function, variable)) {
                  printf("Codeline references an invalid variable as function argument.\n");
                  return -1;
                }
                if (!codeline->target_function->args[i]) {
                  printf("Codeline has more arguments than target function.\n");
                  return -1;
                }
                if (codeline->target_function->args[i]->data_type != variable->data_type) {
                  printf("Codeline has argument that does not match the type of the argument in the target_function.\n");
                  return -1;
                }
              }
              if (i < FUNCTION_ARG_COUNT && codeline->target_function->args[i]) {
                printf("Codeline has fewer arguments than target function.\n");
                return -1;
              }
              break;
            
            case CODELINE_TYPE_RETURN:
              if (!is_variable_in_function(function, codeline->assigned_variable)) {
                printf("Codeline references invalid assigned_variable.\n");
                return -1;
              }
              if (codeline->assigned_variable->data_type != function->return_datatype) {
                printf("Codeline is trying to return a variable of the wrong type.\n");
                return -1;
              }
              break;
            
            case CODELINE_TYPE_IF:
            case CODELINE_TYPE_WHILE:
              if (!is_variable_in_function(function, codeline->args[0])) {
                printf("Codeline references invalid function argument.\n");
                return -1;
              }
              if (!is_variable_in_function(function, codeline->args[1])) {
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
            
            case CODELINE_TYPE_POINTER_ASSIGNMENT:
              if (!is_variable_in_function(function, codeline->assigned_variable)) {
                printf("Codeline references invalid assigned_variable.\n");
                return -1;
              }
              if (!is_variable_in_function(function, codeline->args[0])) {
                printf("Codeline references invalid argument 0.\n");
                return -1;
              }
              // Check that pointer types line up
              data_type = datatype_pointer_jump(codeline->assigned_variable->data_type, codeline->assigned_variable_reference_count);
              if (!data_type || data_type != datatype_pointer_jump(codeline->args[0]->data_type, codeline->arg0_reference_count)) {
                printf("Codeline pointer reference data types do not line up.\n");
                return -1;
              }
              // Verify that second argument only occurs if pointer types all around
              if (codeline->args[1] && data_type->type != DATATYPETYPE_POINTER){
            	  printf("Pointer assignment has integer addition but is not assigning to a pointer type!\n");
            	  return -1;
              }
              
              break;
          }
          if (!codeline->next && function->last_codeline != codeline) {
            printf("Function last_codeline is not the last codeline.\n");
            return -1;
          }
          codeline = codeline->next;
        }
      }
    }
    if (!function->next && function != environment->last_function) {
      printf("Environment last_function is not the last function.\n");
      return -1;
    }
    function = function->next;
  }
  return 0;
}


