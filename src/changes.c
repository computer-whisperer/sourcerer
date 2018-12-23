#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "changes.h"

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
        // Increment codeline count
        function->codeline_count++;
        
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
        codeline->function = function;
        
        // Update reference counts
        if (codeline->assigned_variable)
          codeline->assigned_variable->references++;
        
        for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
          if (!codeline->args[i])
            break;
          codeline->args[i]->references++;
        }
        
        // Invert change object
        change->type = CHANGE_TYPE_REMOVE_CODELINE;
        break;
        
        
      case CHANGE_TYPE_REMOVE_CODELINE:
        // Decrement codeline count
        function->codeline_count--;
      
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
        codeline->function = NULL;
        
        // Update reference counts
        if (codeline->assigned_variable)
          codeline->assigned_variable->references--;
        
        for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
          if (!codeline->args[i])
            break;
          codeline->args[i]->references--;
        }
        
        break;
      
      
      case CHANGE_TYPE_INSERT_VARIABLE:
      
        if (!variable->name[0]) {
          // Find unused name if this variable had none previously
          while(1) {
            // Increment name
            for (int i = 0; i < NAME_LEN; i++) {
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
            if (!variable_name_search(function->first_variable, variable->name))
              break; // Good find!
          }
        }

        // Update linked lists
        variable->next = NULL;
        variable->prev = function->last_variable;
        
        if (variable->prev)
          variable->prev->next = variable;
        else
          function->first_variable = variable;
        
        function->last_variable = variable;
        variable->function = function;
        variable->references = 0;
        
        // Invert change object
        change->type = CHANGE_TYPE_REMOVE_VARIABLE;
        
        break;
      
      
      case CHANGE_TYPE_REMOVE_VARIABLE:
        if (variable->prev)
          variable->prev->next = variable->next;
        else
          function->first_variable = variable->next;
        
        if (variable->next)
          variable->next->prev = variable->prev;
        else
          function->last_variable = variable->prev;
        
        variable->prev = NULL;
        variable->next = NULL;
        variable->function = NULL;
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
        old_var = codeline->args[change->argument_index];
        codeline->args[change->argument_index] = variable;
        
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
  
    
  if (fast_rand() % 1000 && assert_environment_integrity(function->environment))
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
