#include <stdlib.h>
#include <stdio.h>

#include "c_types.h"
#include "executor.h"

int executor_evaluate_condition(struct CodeLine_T * codeline) {
  int a, b;
  if (codeline->function_arguments[0]->data_type.basic_type == BASICTYPE_CHAR) {
    a = codeline->function_arguments[0]->executor_top_varframe->value.c;
    b = codeline->function_arguments[1]->executor_top_varframe->value.c;
  }
  else {
    a = codeline->function_arguments[0]->executor_top_varframe->value.i;
    b = codeline->function_arguments[1]->executor_top_varframe->value.i;
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

union ExecutorValue_T execute_function(struct Function_T * function, union ExecutorValue_T * args, struct ExecutorPerformanceReport_T * report, int execution_limit){
  
  // Setup report
  struct ExecutorPerformanceReport_T r;
  if (!report)
    report = &r;
  report->lines_executed = 0;
  report->did_return = 0;
  report->uninitialized_vars_referenced = 0;
  report->total_lines = 0;
  
  struct CodeLine_T * codeline = function->first_codeline;
  while (codeline){
    report->total_lines++;
    codeline = codeline->next;
  }
    
  
  // Allocate memory for per-variable stack frames
  union ExecutorValue_T sub_args[FUNCTION_ARG_COUNT];
  struct Variable_T * variable = function->first_var;
  while (variable) {
    struct ExecutorVariableFrame_T * newframe = malloc(sizeof(struct ExecutorVariableFrame_T));
    newframe->next = variable->executor_top_varframe;
    newframe->value.i = 0;
    variable->executor_top_varframe = newframe;
    variable->executor_initialized = 0;
    variable = variable->next;
  }

  // Set incoming arguments
  for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
    if (!function->function_arguments[i])
      break;
    function->function_arguments[i]->executor_top_varframe->value = args[i];
    function->function_arguments[i]->executor_initialized = 1;
  }

  union ExecutorValue_T rval = (union ExecutorValue_T){.ptr=NULL};
  
  // Execute codelines
  codeline = function->first_codeline;
  struct CodeLine_T * next_codeline = NULL;
  int lines_executed = 0;
  int returned = 0;
  while (1) {
    if (!codeline || returned) {
      report->did_return = 1;
      break;
    }
    
    lines_executed++;
    report->lines_executed++;
    if (execution_limit && lines_executed > execution_limit) {
      report->did_return = 0;
      break;
    }
    
    next_codeline = codeline->next;
    switch (codeline->type) {
      
      case CODELINE_TYPE_CONSTANT_ASSIGNMENT:
        codeline->assigned_variable->executor_top_varframe->value.i = codeline->constant.i;
        // Mark assigned variable as initialized
        codeline->assigned_variable->executor_initialized = 1;
        break;
        
      case CODELINE_TYPE_RETURN:
        // Penalize usage of uninitialized variables
        if (!codeline->assigned_variable->executor_initialized)
          report->uninitialized_vars_referenced++;
        rval = codeline->assigned_variable->executor_top_varframe->value;        
        returned = 1;
        break;
        
      case CODELINE_TYPE_FUNCTION_CALL:
        // Penalize usage of uninitialized variables
        for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
          if (!codeline->target_function->function_arguments[i])
            break;
          if (!codeline->function_arguments[i]->executor_initialized)
            report->uninitialized_vars_referenced++;
        }
        switch (codeline->target_function->type) {
          
          case FUNCTION_TYPE_BASIC_OP:
            switch (codeline->target_function->name[0]) {
              // Assuming all integer arguments for now, probably a bad idea
              case '+':
                codeline->assigned_variable->executor_top_varframe->value.i = codeline->function_arguments[0]->executor_top_varframe->value.i + codeline->function_arguments[1]->executor_top_varframe->value.i;
                break;
              case '-':
                codeline->assigned_variable->executor_top_varframe->value.i = codeline->function_arguments[0]->executor_top_varframe->value.i - codeline->function_arguments[1]->executor_top_varframe->value.i;
                break;
              case '*':
                codeline->assigned_variable->executor_top_varframe->value.i = codeline->function_arguments[0]->executor_top_varframe->value.i * codeline->function_arguments[1]->executor_top_varframe->value.i;
                break;
              case '/':
                if (codeline->function_arguments[1]->executor_top_varframe->value.i == 0)
                  break;
                codeline->assigned_variable->executor_top_varframe->value.i = (double)codeline->function_arguments[0]->executor_top_varframe->value.i / (double)codeline->function_arguments[1]->executor_top_varframe->value.i;
                break;
            }
            break;
            
          case FUNCTION_TYPE_BUILTIN:
            // For now assume putchar, also a really bad idea :)
            //putchar(codeline->function_arguments[0]->executor_top_varframe->value.c);
            if (putchar_i < PUTCHAR_BUFF_LEN) {
              putchar_buff[putchar_i] = codeline->function_arguments[0]->executor_top_varframe->value.c;
              codeline->assigned_variable->executor_top_varframe->value.c = codeline->function_arguments[0]->executor_top_varframe->value.c;
              putchar_i++;
            }
            break;
            
          case FUNCTION_TYPE_CUSTOM:
            
            for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
              if (!codeline->function_arguments[i])
                break;
              sub_args[i] = codeline->function_arguments[i]->executor_top_varframe->value;
            }
            codeline->assigned_variable->executor_top_varframe->value = execute_function(codeline->target_function, sub_args, report, execution_limit);
            break;
            
          // Mark assigned variable as initialized
          codeline->assigned_variable->executor_initialized = 1;
        }
        break;
        
      case CODELINE_TYPE_IF:
        // Penalize usage of uninitialized variables
        for (int i = 0; i < 2; i++)
          if (!codeline->function_arguments[i]->executor_initialized)
            report->uninitialized_vars_referenced++;
        if (!executor_evaluate_condition(codeline))
          next_codeline = codeline->block_other_end->next;
        break;
        
      case CODELINE_TYPE_WHILE:
        // Penalize usage of uninitialized variables
        for (int i = 0; i < 2; i++)
          if (!codeline->function_arguments[i]->executor_initialized)
            report->uninitialized_vars_referenced++;
        if (!executor_evaluate_condition(codeline))
          next_codeline = codeline->block_other_end->next;
        break;
        
      case CODELINE_TYPE_BLOCK_END:
        if (codeline->block_other_end->type == CODELINE_TYPE_WHILE) {
          // Penalize usage of uninitialized variables
          for (int i = 0; i < 2; i++)
            if (!codeline->block_other_end->function_arguments[i]->executor_initialized)
              report->uninitialized_vars_referenced++;
            
          if (executor_evaluate_condition(codeline->block_other_end))
            next_codeline = codeline->block_other_end->next; // Handle while loop return
        }
        break;
        
    }
    codeline = next_codeline;
  }
  // Free stack frames
  variable = function->first_var;
  while (variable) {
    struct ExecutorVariableFrame_T * frame = variable->executor_top_varframe;
    variable->executor_top_varframe = variable->executor_top_varframe->next;
    free(frame);
    variable = variable->next;
  }
  return rval;
}
