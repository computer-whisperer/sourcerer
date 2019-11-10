#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "c_types.h"
#include "change_proposers.h"
#include "changes.h"

struct DataType_T * select_random_datatype(struct Environment_T * environment, int allow_pointers) {
  struct DataType_T * data_type = environment->first_datatype;
  int data_type_count = 0;
  while (data_type) {
    if (allow_pointers || data_type->type != DATATYPETYPE_POINTER)
      data_type_count++;
    data_type = data_type->next;
  }
  
  data_type = environment->first_datatype;
  int selected = (fast_rand() % data_type_count) + 1;
  while (data_type) {
    if (allow_pointers || data_type->type != DATATYPETYPE_POINTER)
      selected--;
    if (!selected)
      break;
    data_type = data_type->next;
  }
  return data_type;
}

struct Variable_T * find_random_variable(struct Function_T * function, struct DataType_T * type, int allow_pointers) {
  // Count number of compatible variables
  int variable_count = 0;
  struct Variable_T * variable = function->first_variable;
  while (variable) {
    if ((!type || variable->data_type == type) && (allow_pointers || variable->data_type->type != DATATYPETYPE_POINTER))
      variable_count++;
    variable = variable->next;
  }
  // Select from these variables and the possibility of a new one
  int selected_var = fast_rand() % (variable_count + 1);
  if (selected_var < variable_count) {
    // Var selected from existing range
    variable = function->first_variable;
    selected_var++;
    while (variable && selected_var){
      if ((!type || variable->data_type == type) && (allow_pointers || variable->data_type->type != DATATYPETYPE_POINTER))
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
    variable->function = NULL;
    variable->next = NULL;
    variable->prev = NULL;
    variable->is_arg = 0;
    variable->references = 0;
    // Wipe name
    for (int i = 0; i < NAME_LEN; i++) {
      variable->name[i] = '\0';
    }
    // Name will get properly assigned when the change is applied
    
    if (type)
      variable->data_type = type;
    // Handle request for random type
    if (!type)
      variable->data_type = select_random_datatype(function->environment, allow_pointers);
    
    return variable;
  }
}

// Private helper function
int search_for_var(struct Variable_T * var, struct Variable_T ** vars, int * vars_found) {
  int i;
  for (i = 0; i < *vars_found; i++)
    if (var == vars[i])
      return i;
  (*vars_found)++;
  return i;
}

#define ADD_VARIABLE_CHANGES_BUFF_SIZE 20
struct Change_T * add_variable_changes(struct Change_T * change) {
  struct Function_T * function = change->function;
  struct Variable_T * variables[ADD_VARIABLE_CHANGES_BUFF_SIZE];
  int ref_delta[ADD_VARIABLE_CHANGES_BUFF_SIZE];
  int vars_found = 0;
  int i, j;
  
  // Find last change
  struct Change_T * last_change = change;
  if (last_change)
    while (last_change->next)
      last_change = last_change->next;
  
  for (i = 0; i < ADD_VARIABLE_CHANGES_BUFF_SIZE; i++)
    ref_delta[i] = 0;
  
  // Scan over all variables modified by the provided change.
  struct Change_T * change_ptr = change;
  while (change_ptr) {
    switch (change_ptr->type) {
      case CHANGE_TYPE_INSERT_CODELINE:
        if (change_ptr->codeline->assigned_variable) {
          i = search_for_var(change_ptr->codeline->assigned_variable, variables, &vars_found);
          variables[i] = change_ptr->codeline->assigned_variable;
          ref_delta[i]++;
        }
        for (j = 0; j < FUNCTION_ARG_COUNT; j++) {
          if (!change_ptr->codeline->args[j])
            break;
          i = search_for_var(change_ptr->codeline->args[j], variables, &vars_found);
          variables[i] = change_ptr->codeline->args[j];
          ref_delta[i]++;
        }
        break;
      
      case CHANGE_TYPE_REMOVE_CODELINE:
        if (change_ptr->codeline->assigned_variable) {
          i = search_for_var(change_ptr->codeline->assigned_variable, variables, &vars_found);
          variables[i] = change_ptr->codeline->assigned_variable;
          ref_delta[i]--;
        }
        for (j = 0; j < FUNCTION_ARG_COUNT; j++) {
          if (!change_ptr->codeline->args[j])
            break;
          i = search_for_var(change_ptr->codeline->args[j], variables, &vars_found);
          variables[i] = change_ptr->codeline->args[j];
          ref_delta[i]--;
        }
        break;
        
      case CHANGE_TYPE_ALTER_ARGUMENT:
        // New variable
        i = search_for_var(change_ptr->variable, variables, &vars_found);
        variables[i] = change_ptr->variable;
        ref_delta[i]++;
        // Old variable
        i = search_for_var(change_ptr->codeline->args[change_ptr->argument_index], variables, &vars_found);
        variables[i] = change_ptr->codeline->args[change_ptr->argument_index];
        ref_delta[i]--;
        break;
      
      case CHANGE_TYPE_ALTER_ASSIGNED_VARIABLE:
        // New variable
        i = search_for_var(change_ptr->variable, variables, &vars_found);
        variables[i] = change_ptr->variable;
        ref_delta[i]++;
        // Old variable
        i = search_for_var(change_ptr->codeline->assigned_variable, variables, &vars_found);
        variables[i] = change_ptr->codeline->assigned_variable;
        ref_delta[i]--;
        break;
      
      case CHANGE_TYPE_INSERT_VARIABLE:
      case CHANGE_TYPE_REMOVE_VARIABLE:
      case CHANGE_TYPE_ALTER_CONSTANT:
        // For now, ignore these
        break;
    }
    
    change_ptr = change_ptr->next;
  }
  
  // Add remove and insert changes as neccessary
  
  for (i = 0; i < vars_found; i++) {
    int final_ref_count = variables[i]->references + ref_delta[i];
    if (final_ref_count > 0) {
      if ( variables[i]->references == 0) {
        // New variable, add change to insert it
        struct Change_T * new_change = malloc(sizeof(struct Change_T));
        new_change->type = CHANGE_TYPE_INSERT_VARIABLE;
        new_change->variable = variables[i];
        new_change->next = change;
        new_change->function = function;
        change = new_change;
      }
    }
    else {
      // All references removed, add change to remove variable
      // It has to be at the end of the change list also!
      struct Change_T * new_change = malloc(sizeof(struct Change_T));
      new_change->type = CHANGE_TYPE_REMOVE_VARIABLE;
      new_change->variable = variables[i];
      new_change->function = function;
      new_change->next = NULL;
      if (last_change)
        last_change->next = new_change;
      last_change = new_change;
    }
  }
  return change;
}

struct Change_T * propose_random_change(struct Function_T * function) {   
  struct Change_T * change = malloc(sizeof(struct Change_T));
  change->next = NULL;
  change->function = function;
  
  // Count the number of codelines;
  int codeline_count = 0;
  struct CodeLine_T * codeline = function->first_codeline;
  while (codeline){
    codeline_count++;
    codeline = codeline->next;
  }
  
  // Count the number of functions
  int function_count = -1; // Don't count this function
  struct Function_T * cfunction = function->environment->first_function;
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
  
  change->next_codeline = codeline;
  
  // Found a random codeline, do something with it
  int r = fast_rand() % 100;
  
  if (r < 10 && codeline && codeline->type != CODELINE_TYPE_RETURN) {
    if (codeline->type == CODELINE_TYPE_CONSTANT_ASSIGNMENT) {
      // Modify the constant
      union ConstantValue_T new_value;
      new_value.i = 0;
      if (codeline->assigned_variable->data_type == function->environment->char_datatype)
        new_value.c = (codeline->constant.c + (fast_rand() % 20) - 10) % 256;
      else // int
        new_value.i = codeline->constant.i + (fast_rand() % 20) - 10;
      
      // Build change
      change->type = CHANGE_TYPE_ALTER_CONSTANT;
      change->constant = new_value;
      change->codeline = codeline;
    }
    else {
      // Modify an argument
      
      // If we are pointing at a block end, just use the head
      if (codeline->type == CODELINE_TYPE_BLOCK_END)
        codeline = codeline->block_other_end;
      
      int arg_count;
      for (arg_count = 0; arg_count < FUNCTION_ARG_COUNT; arg_count++)
        if (!codeline->args[arg_count])
          break;
      
      int arg_selected = fast_rand() % arg_count;
      change->argument_index = arg_selected;
      
      // Figure out the required data type
      struct DataType_T * data_type = codeline->args[arg_selected]->data_type;
      
      // Build change
      change->type = CHANGE_TYPE_ALTER_ARGUMENT;
      change->variable = find_random_variable(function, data_type, 1);
      change->codeline = codeline;
    }
  }
  else if (r < 50 || !codeline) {
    // Insert new line before the current line
    struct CodeLine_T * new_codeline = malloc(sizeof(struct CodeLine_T));
    new_codeline->assigned_variable = NULL;
    new_codeline->assigned_variable_reference_count = 0;
    for (int i = 0; i < FUNCTION_ARG_COUNT; i++)
      new_codeline->args[i] = NULL;
    new_codeline->arg0_reference_count = 0;
    new_codeline->next = NULL;
    new_codeline->prev = NULL;
    new_codeline->function = NULL;
    
    
    change->type = CHANGE_TYPE_INSERT_CODELINE;
    change->codeline = new_codeline;
    

    // Select type
    r = fast_rand() % 100;

    if (r < 20) {
      // constant assignment
      new_codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
      new_codeline->assigned_variable = find_random_variable(function, NULL, 0);
      if (new_codeline->assigned_variable->data_type == function->environment->char_datatype)
        new_codeline->constant.c = fast_rand() % 128; // Any char
      else
        new_codeline->constant.i = fast_rand() % 10 - 5; // Any int from -5 to +5
    }
    else if (r < 30) {
      // return
      new_codeline->type = CODELINE_TYPE_RETURN;
      new_codeline->assigned_variable = find_random_variable(function, function->return_datatype, 1);
    }
    else if (0) {
      // Pointer assignment
      new_codeline->type = CODELINE_TYPE_POINTER_ASSIGNMENT;
      
      // This loop will exit randomly, more likely on a good decision.
      int done = 0;
      while (!done) {
    	new_codeline->args[1] = NULL;
        new_codeline->assigned_variable_reference_count = 0;
        new_codeline->arg0_reference_count = 0;
        
        // If we already ran once, we need to free any variables we accidentally created.
        if (new_codeline->assigned_variable && !(new_codeline->assigned_variable->name[0])) {
          free(new_codeline->assigned_variable);
          new_codeline->assigned_variable = NULL;
        }
        if (new_codeline->args[0] && !(new_codeline->args[0]->name[0])) {
          free(new_codeline->args[0]);
          new_codeline->args[0] = NULL;
        }
        
        // Find literally anything as the assigned variable
        new_codeline->assigned_variable = find_random_variable(function, NULL, 1);
        
        // Pick reference count (number of times to * it)
        while (new_codeline->assigned_variable->data_type->pointer_degree > new_codeline->assigned_variable_reference_count) {
          if (fast_rand() % 2)
            break;
          new_codeline->assigned_variable_reference_count++;
        }
        
        struct DataType_T * goal_datatype = datatype_pointer_jump(new_codeline->assigned_variable->data_type, new_codeline->assigned_variable_reference_count);
        
        struct DataType_T * current_target_datatype = goal_datatype;
        // If the goal_datatype is a type of pointer, then sometimes try a one-lower pointer degree (stick a & before the second variable)
        if (goal_datatype->pointer_degree && fast_rand() % 3 == 0) {
          current_target_datatype = goal_datatype->pointing_to;
          new_codeline->arg0_reference_count = -1;
        }
        
        // Try to find a matching variable
        while (current_target_datatype) {
          new_codeline->args[0] = find_random_variable(function, current_target_datatype, 1);
          if (fast_rand() % 4 == 0) {
            // Sometimes, just go for it.
            done = 1;
            break;
          }
          if (new_codeline->args[0]->name[0] && fast_rand() % 2 == 0) {
            // More likely to accept this if we are using an existing variable
            done = 1;
            break;
          }
          current_target_datatype = current_target_datatype->pointed_from;
          if (current_target_datatype)
            new_codeline->arg0_reference_count++;
        }
        // Add an int to the result if it is compatible
        if (datatype_pointer_jump(new_codeline->assigned_variable->data_type, new_codeline->assigned_variable_reference_count)->type == DATATYPETYPE_POINTER) {
          new_codeline->args[1] = find_random_variable(function, function->environment->int_datatype, 0);
        }
        
      }
    }
    else if (r < 70) {
      // function call
      new_codeline->type = CODELINE_TYPE_FUNCTION_CALL;
      
      // Find function
      int function_id = (fast_rand() % function_count) + 1;
      cfunction = function->environment->first_function;
      while (cfunction && function_id) {
        if (cfunction != function)
          function_id--;
        if (function_id == 0)
          break;
        cfunction = cfunction->next;
      }
      new_codeline->target_function = cfunction;
      
      // Find assigning variable
      new_codeline->assigned_variable = find_random_variable(function, cfunction->return_datatype, 1);
      
      // Find arguments
      for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
        if (!cfunction->args[i]) {
          new_codeline->args[i] = NULL;
          break;
        }
        new_codeline->args[i] = find_random_variable(function, cfunction->args[i]->data_type, 1);
      }
      
        
    }
    else {
      // The statement we are going to add needs a block end, lets find it now
      struct CodeLine_T * second_new_codeline = malloc(sizeof(struct CodeLine_T));
      second_new_codeline->type = CODELINE_TYPE_BLOCK_END;
      second_new_codeline->assigned_variable = NULL;
      second_new_codeline->args[0] = NULL;
      second_new_codeline->next = NULL;
      second_new_codeline->prev = NULL;
      second_new_codeline->function = NULL;
      
      // Link to first end
      new_codeline->block_other_end = second_new_codeline;
      second_new_codeline->block_other_end = new_codeline;
      
      // Setup second change
      struct Change_T * second_change = malloc(sizeof(struct Change_T));
      second_change->type = CHANGE_TYPE_INSERT_CODELINE;
      second_change->function = function;
      second_change->codeline = second_new_codeline;
      second_change->next = NULL;
      change->next = second_change;
      
      // Pick a location
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
      
      // Found where the block end goes
      second_change->next_codeline = second_codeline_ptr;

      new_codeline->condition = fast_rand() % 6; // 6 possible conditions
      new_codeline->args[0] = find_random_variable(function, NULL, 1);
      new_codeline->args[1] = find_random_variable(function, new_codeline->args[0]->data_type, 1); // Copy the argument type from the first argument
      new_codeline->args[2] = NULL;
      // If the args are the same, set condition to ==
      if (new_codeline->args[0] == new_codeline->args[1])
      	new_codeline->condition = CONDITION_EQUAL;
      if (r < 90)
        // if statement
        new_codeline->type = CODELINE_TYPE_IF;
      else
        // while statement
        new_codeline->type = CODELINE_TYPE_WHILE;
    }
  }
  else {
    // Remove this line
    change->type = CHANGE_TYPE_REMOVE_CODELINE;
    
    // If this is a blockend, just use the other end of it
    if (codeline->type == CODELINE_TYPE_BLOCK_END)
      codeline = codeline->block_other_end;
    
    change->codeline = codeline;
    
    // If this is a for or while loop, include the other end
    if (codeline->type == CODELINE_TYPE_IF || codeline->type == CODELINE_TYPE_WHILE) {
      struct Change_T * second_change = malloc(sizeof(struct Change_T));
      second_change->type = CHANGE_TYPE_REMOVE_CODELINE;
      second_change->function = function;
      second_change->codeline = codeline->block_other_end;
      second_change->next = NULL;
      
      change->next = second_change;
    }
  }
  return add_variable_changes(change);
}
