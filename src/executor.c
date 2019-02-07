#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "membox.h"
#include "c_types.h"
#include "executor.h"

int executor_evaluate_condition(struct CodeLine_T * codeline) {
  int a, b;
  if (codeline->args[0]->data_type == codeline->function->environment->char_datatype) {
    a = *(char *)codeline->args[0]->executor_top_varframe->data_start;
    b = *(char *)codeline->args[1]->executor_top_varframe->data_start;
  }
  else {
    a = *(int *)codeline->args[0]->executor_top_varframe->data_start;
    b = *(int *)codeline->args[1]->executor_top_varframe->data_start;
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

int validate_address(struct MemoryDMZ_T * dmz, void * ptr) {
  while (dmz) {
    if (dmz->data_start <= ptr && ptr < dmz->data_end)
      return 1;
    dmz = dmz->next;
  }
  return 0;
}

int execute_function(struct Function_T * function, struct MemoryDMZ_T * dmz, void * arguments_data[], size_t arguments_size[], void * return_data, size_t return_size, int execution_limit){

  // Add membox to dmz
  struct MemoryDMZ_T my_dmz = {.data_start=function->environment->membox->data_start, .writable=1, .data_end=function->environment->membox->data_end, .next=dmz};
  dmz = &my_dmz;

  // Allocate memory for per-variable stack frames
  struct Variable_T * variable = function->first_variable;
  while (variable) {
    struct ExecutorVariableFrame_T * newframe = malloc(sizeof(struct ExecutorVariableFrame_T));
    newframe->next = variable->executor_top_varframe;
    newframe->data_start = membox_malloc(function->environment->membox, variable->data_type->size);
    if (!newframe->data_start) {
      printf("membox malloc failed!\n");
      exit(1);
    }
    variable->executor_top_varframe = newframe;
    variable = variable->next;
  }
  
  // Load argument_data into the stack frames
  if (arguments_data) {
    for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
      if (!function->args[i] || !arguments_data[i] || arguments_size[i] <= 0)
        break;
      if (function->args[i]->data_type->size < arguments_size[i])
        arguments_size[i] = function->args[i]->data_type->size;
      memcpy(function->args[i]->executor_top_varframe->data_start, arguments_data[i], arguments_size[i]);
    }
  }
  
  void * sub_arguments_data[FUNCTION_ARG_COUNT];
  int sub_arguments_size[FUNCTION_ARG_COUNT];
  
  long a, b, c;
  
  void * src_ptr;
  void * dest_ptr;
  
  int offset;
  
  struct DataType_T * data_type;
  
  // Execute codelines
  struct CodeLine_T * codeline = function->first_codeline;
  struct CodeLine_T * next_codeline = NULL;
  int lines_executed = 0;
  int returned = 0;
  while (1) {
    if (!codeline || returned) {
      break;
    }
    
    lines_executed++;
    function->executor_report.lines_executed++;
    if (execution_limit && lines_executed > execution_limit) {
      function->executor_report.times_not_returned++;
      break;
    }
    
    next_codeline = codeline->next;
    switch (codeline->type) {
      
      case CODELINE_TYPE_CONSTANT_ASSIGNMENT:
        if (codeline->assigned_variable->data_type == function->environment->char_datatype)
          *(char *)codeline->assigned_variable->executor_top_varframe->data_start = codeline->constant.c;
        else if (codeline->assigned_variable->data_type == function->environment->int_datatype)
          *(int *)codeline->assigned_variable->executor_top_varframe->data_start = codeline->constant.i;
        break;
      
      case CODELINE_TYPE_POINTER_ASSIGNMENT:
        offset = 0;
        if (codeline->args[1])
          offset = *(int *)codeline->args[1]->executor_top_varframe->data_start;
      
        src_ptr = codeline->args[0]->executor_top_varframe->data_start;
        for (int i = 0; i < codeline->arg0_reference_count; i++) {
          if (!validate_address(dmz, src_ptr)) {
            returned = 1;
            break;
          }
          src_ptr = *(void **)src_ptr;
        }
        
        if (codeline->arg0_reference_count < 0){
          // Copy address of arg0 to the assigned variable

          if (returned)
            function->executor_report.segfaults_attempted++;
          *(void **)codeline->assigned_variable->executor_top_varframe->data_start = src_ptr + offset;
        }
        else {
          // Copy actual bytes
          void * dest_ptr = codeline->assigned_variable->executor_top_varframe->data_start;
          for (int i = 0; i < codeline->assigned_variable_reference_count; i++) {
            if (!validate_address(dmz, dest_ptr)) {
              returned = 1;
              break;
            }
            dest_ptr = *(void **)dest_ptr;
          }
          
          if (!validate_address(dmz, src_ptr))
            returned = 1;
          if (!validate_address(dmz, dest_ptr))
            returned = 1;
          
          if (returned) {
            function->executor_report.segfaults_attempted++;
            break;
          }
          
          data_type = datatype_pointer_jump(codeline->assigned_variable->data_type, codeline->assigned_variable_reference_count);
          if (data_type->type == DATATYPETYPE_POINTER)
            *(void **)dest_ptr = *(void **)src_ptr + offset*data_type->pointing_to->size;
          else
            memcpy(dest_ptr, src_ptr, datatype_pointer_jump(codeline->assigned_variable->data_type, codeline->assigned_variable_reference_count)->size);
            
        }
        
        break;
        
      case CODELINE_TYPE_RETURN:
        if (return_size > codeline->assigned_variable->data_type->size)
          return_size = codeline->assigned_variable->data_type->size;
        if (return_data && return_size > 0)
          memcpy(return_data, codeline->assigned_variable->executor_top_varframe->data_start, return_size);
        returned = 1;
        break;
        
      case CODELINE_TYPE_FUNCTION_CALL:
        switch (codeline->target_function->type) {
          
          case FUNCTION_TYPE_BASIC_OP:
            a = *(int*)codeline->args[0]->executor_top_varframe->data_start;
            b = *(int*)codeline->args[1]->executor_top_varframe->data_start;
            c = 0;
            switch (codeline->target_function->name[0]) {
              case '+':
                c = a + b;
                break;
              case '-':
                c = a - b;
                break;
              case '*':
                c = a * b;
                break;
              case '/':
                if (b == 0)
                  break;
                c = a / b;
                break;
              case '%':
                if (b <= 0)
                  break;
                c = a % b;
                break;
            }
            *(int*)codeline->assigned_variable->executor_top_varframe->data_start = c;
            break;
            
          case FUNCTION_TYPE_BUILTIN:
            // For now assume putchar, also a really bad idea :)
            //putchar(codeline->args[0]->executor_top_varframe->value.c);
            if (putchar_i < PUTCHAR_BUFF_LEN) {
              putchar_buff[putchar_i] = *(char *)codeline->args[0]->executor_top_varframe->data_start;
              *(char *)codeline->assigned_variable->executor_top_varframe->data_start = *(char *)codeline->args[0]->executor_top_varframe->data_start;
              putchar_i++;
            }
            break;
            
          case FUNCTION_TYPE_CUSTOM:
            for (int i = 0; i < FUNCTION_ARG_COUNT; i++) {
              if (!codeline->args[i]) {
                sub_arguments_data[i] = NULL;
                sub_arguments_size[i] = 0;
                break;
              }
              sub_arguments_data[i] = codeline->args[i]->executor_top_varframe->data_start;
              sub_arguments_size[i] = codeline->args[i]->data_type->size;
            }
            //lines_executed += execute_function(codeline->target_function, NULL, sub_arguments_data, sub_arguments_size, codeline->assigned_variable->executor_top_varframe->data_start, codeline->assigned_variable->data_type->size, execution_limit - lines_executed);
            break;
        }
        break;
        
      case CODELINE_TYPE_IF:
        if (!executor_evaluate_condition(codeline))
          next_codeline = codeline->block_other_end->next;
        break;
        
      case CODELINE_TYPE_WHILE:
        if (!executor_evaluate_condition(codeline))
          next_codeline = codeline->block_other_end->next;
        break;
        
      case CODELINE_TYPE_BLOCK_END:
        if (codeline->block_other_end->type == CODELINE_TYPE_WHILE) {
          if (executor_evaluate_condition(codeline->block_other_end)) {
            next_codeline = codeline->block_other_end->next; // Handle while loop return
          }
          else{
        	  ;
          }
        }
        break;
      
      default:
        break;
        
    }
    codeline = next_codeline;
  }
  // Free stack frames
  variable = function->first_variable;
  while (variable) {
    struct ExecutorVariableFrame_T * frame = variable->executor_top_varframe;
    variable->executor_top_varframe = variable->executor_top_varframe->next;
    membox_free(function->environment->membox, frame->data_start);
    free(frame);
    variable = variable->next;
  }
  
  // For now, reset the memory now
  reset_membox(function->environment->membox);
  return lines_executed;
}
