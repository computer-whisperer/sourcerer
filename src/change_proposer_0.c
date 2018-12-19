#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "c_types.h"
#include "change_proposers.h"

struct Variable_T * find_random_variable(struct Variable_T * first_variable, struct DataType_T * type) {
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
    variable->parent_function = NULL;
    variable->next = NULL;
    variable->prev = NULL;
    variable->is_arg = 0;
    variable->references = 0;
    // Wipe name
    for (int i = 0; i < VARIABLE_NAME_LEN; i++) {
      variable->name[i] = '\0';
    }
    // Name will get properly assigned when the change is applied
    
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
          if (!change_ptr->codeline->function_arguments[j])
            break;
          i = search_for_var(change_ptr->codeline->function_arguments[j], variables, &vars_found);
          variables[i] = change_ptr->codeline->function_arguments[j];
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
          if (!change_ptr->codeline->function_arguments[j])
            break;
          i = search_for_var(change_ptr->codeline->function_arguments[j], variables, &vars_found);
          variables[i] = change_ptr->codeline->function_arguments[j];
          ref_delta[i]--;
        }
        break;
        
      case CHANGE_TYPE_ALTER_ARGUMENT:
        // New variable
        i = search_for_var(change_ptr->variable, variables, &vars_found);
        variables[i] = change_ptr->variable;
        ref_delta[i]++;
        // Old variable
        i = search_for_var(change_ptr->codeline->function_arguments[change_ptr->argument_index], variables, &vars_found);
        variables[i] = change_ptr->codeline->function_arguments[change_ptr->argument_index];
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
  struct Function_T * cfunction = get_first_function(function);
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
  
  if (r < 40 && codeline && codeline->type != CODELINE_TYPE_RETURN) {
    if (codeline->type == CODELINE_TYPE_CONSTANT_ASSIGNMENT) {
      // Modify the constant
      union ConstantValue_T new_value;
      new_value.i = 0;
      if (codeline->assigned_variable->data_type.basic_type == BASICTYPE_CHAR)
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
        if (!codeline->function_arguments[arg_count])
          break;
      
      int arg_selected = fast_rand() % arg_count;
      change->argument_index = arg_selected;
      
      // Figure out the required data type
      struct DataType_T data_type = codeline->function_arguments[arg_selected]->data_type;
      
      // Build change
      change->type = CHANGE_TYPE_ALTER_ARGUMENT;
      change->variable = find_random_variable(function->first_var, &data_type);
      change->codeline = codeline;
    }
  }
  else if (r < 65 || !codeline) {
    // Insert new line before the current line
    struct CodeLine_T * new_codeline = malloc(sizeof(struct CodeLine_T));
    new_codeline->assigned_variable = NULL;
    new_codeline->function_arguments[0] = NULL;
    new_codeline->next = NULL;
    new_codeline->prev = NULL;
    new_codeline->parent_function = NULL;
    
    
    change->type = CHANGE_TYPE_INSERT_CODELINE;
    change->codeline = new_codeline;
    

    // Select type
    r = fast_rand() % 100;

    if (r < 40) {
      // constant assignment
      new_codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
      new_codeline->assigned_variable = find_random_variable(function->first_var, NULL);
      if (new_codeline->assigned_variable->data_type.basic_type == BASICTYPE_CHAR)
        new_codeline->constant.c = fast_rand() % 128; // Any char
      else
        new_codeline->constant.i = fast_rand() % 10 - 5; // Any int from -5 to +5
    }
    else if (r < 45) {
      // return
      new_codeline->type = CODELINE_TYPE_RETURN;
      new_codeline->assigned_variable = find_random_variable(function->first_var, &function->return_datatype);
    }
    else if (r < 80) {
      // function call
      new_codeline->type = CODELINE_TYPE_FUNCTION_CALL;
      
      // Find function
      int function_id = (fast_rand() % function_count) + 1;
      cfunction = get_first_function(function);
      while (cfunction && function_id) {
        if (cfunction != function)
          function_id--;
        if (function_id == 0)
          break;
        cfunction = cfunction->next;
      }
      new_codeline->target_function = cfunction;
      
      // Find assigning variable
      new_codeline->assigned_variable = find_random_variable(function->first_var, &cfunction->return_datatype);
      
      // Find arguments
      for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
        if (!cfunction->function_arguments[i]) {
          new_codeline->function_arguments[i] = NULL;
          break;
        }
        new_codeline->function_arguments[i] = find_random_variable(function->first_var, &cfunction->function_arguments[i]->data_type);
      }
      
        
    }
    else {
      // The statement we are going to add needs a block end, lets find it now
      struct CodeLine_T * second_new_codeline = malloc(sizeof(struct CodeLine_T));
      second_new_codeline->type = CODELINE_TYPE_BLOCK_END;
      second_new_codeline->assigned_variable = NULL;
      second_new_codeline->function_arguments[0] = NULL;
      second_new_codeline->next = NULL;
      second_new_codeline->prev = NULL;
      second_new_codeline->parent_function = NULL;
      
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

      
      if (r < 90) {
        // if statement
        new_codeline->type = CODELINE_TYPE_IF;
        new_codeline->condition = fast_rand() % 6; // 6 possible conditions
        new_codeline->function_arguments[0] = find_random_variable(function->first_var, NULL);
        new_codeline->function_arguments[1] = find_random_variable(function->first_var, &new_codeline->function_arguments[0]->data_type); // Copy the argument type from the first argument
        new_codeline->function_arguments[2] = NULL;
        
      }
      else{
        // while statement
        new_codeline->type = CODELINE_TYPE_WHILE;
        new_codeline->condition = fast_rand() % 6; // 6 possible conditions
        new_codeline->function_arguments[0] = find_random_variable(function->first_var, NULL);
        new_codeline->function_arguments[1] = find_random_variable(function->first_var, &new_codeline->function_arguments[0]->data_type); // Copy the argument type from the first argument
        new_codeline->function_arguments[2] = NULL;
      }
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
