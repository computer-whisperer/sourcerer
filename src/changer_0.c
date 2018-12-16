#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "changer_0.h"
#include "executor.h"
#include "c_types.h"
#include "c_printer.h"

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

int get_fitness_expfit(struct Function_T * function){

  int score = 100;
  union ExecutorValue_T args[4];
  union ExecutorValue_T result;
  
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 1; j++) {
      args[0].i = i;
      args[1].i = j*10;
      result = execute_function(function, args, NULL, 100);
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
  union ExecutorValue_T args[4];
  union ExecutorValue_T result;
  
  int ops[4] = {-10, -2, 2, 10};
  
  for (int i = 0; i < 20; i += 4) {
    for (int j = 1; j < 20; j += 4) {
      for (int k = 0; k < 4; k++) {
        args[0].i = i;
        args[1].i = j;
        args[2].i = ops[k];
        result = execute_function(function, args, NULL, 100);
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
  union ExecutorValue_T args[4];
  
  for (int i = 0; i < 30; i++) {
    for (int j = 0; j < 3; j++) {
      args[0].i = i;
      args[3].c = j + 97;
      
      for (int k = 0; k < PUTCHAR_BUFF_LEN; k++)
        putchar_buff[k] = '\0';
      putchar_i = 0;

      execute_function(function, args, NULL, 100);

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
  union ExecutorValue_T args[2];
  args[0].i = 0;
  args[1].i = 0;
  args[2].i = 0;
  args[3].c = '\0';
  
  struct ExecutorPerformanceReport_T report;
  execute_function(function, args, &report, 100);
  
  score -= report.lines_executed;
  score -= report.uninitialized_vars_referenced*3;
  if (!report.did_return)
    score -= 1000;
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
  //return get_fitness_count(function);
  return get_fitness_hello_world(function);
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

struct Variable_T * variable_search(struct Variable_T * var, char * name) {
  while (var) {
    if (!strcmp(name, var->name)) 
      break;
    var = var->next;
  }
  return var;
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
        new_codeline->constant.i = fast_rand() % 10 - 5; // Any int from -5 to +5
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

int does_pass(float new_score, float old_score, float temperature) {
  if (new_score > old_score)
    return 1;
    
  float p = exp(-(old_score - new_score)/(temperature*50.0));
  return fast_rand() < 32767.0 * p;
}

double getUnixTime(void)
{
    struct timespec tv;

    if(clock_gettime(CLOCK_REALTIME, &tv) != 0) return 0;

    return (tv.tv_sec + (tv.tv_nsec / 1000000000.0));
}

#define CHANGE_HISTORY_LEN 10000
void train_code_v2() {
  fast_srand(time(NULL));
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
