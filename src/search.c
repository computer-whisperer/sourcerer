#include <stdio.h>
#include <math.h>

#include "c_types.h"
#include "utils.h"
#include "tasks.h"
#include "c_printer.h"
#include "executor.h"
#include "change_proposers.h"
#include "changes.h"

struct Change_T * propose_change(struct Function_T * function) {
  return propose_random_change(function);
}

int get_fitness(struct Function_T * function, TaskJudge_T task_judge) {
  struct TaskInputs_T next_inputs = {.run_index = 0};
  struct TaskInputs_T last_inputs = {.run_index = -1};
  struct TaskOutputs_T last_outputs = {.putchar_text_len = 0};
  // Get first inputs
  task_judge(&last_inputs, &last_outputs, &next_inputs);
  
  void * function_args_data[4] = {&(next_inputs.a), &(next_inputs.b), &(next_inputs.c), &(next_inputs.d)};
  size_t function_args_size[4] = {sizeof(int), sizeof(int), sizeof(int), sizeof(char)};
  void * return_data = &last_outputs.ret;
  size_t return_size = sizeof(last_outputs.ret);
  
  int run_count = 0;
  int score = 0;
  
  while (next_inputs.run_index >= 0) {
    
    function->executor_report.lines_executed = 0;
    function->executor_report.times_not_returned = 0;
    function->executor_report.segfaults_attempted = 0;
    
    putchar_i = 0;
    execute_function(function, NULL, function_args_data, function_args_size, return_data, return_size, 200);
    last_inputs = next_inputs;
    last_outputs.putchar_text = putchar_buff;
    last_outputs.putchar_text_len = putchar_i;
    
    next_inputs.run_index++;
    
    score += 100;
    score += task_judge(&last_inputs, &last_outputs, &next_inputs);
    
    score -= function->executor_report.times_not_returned*1000;
    score -= function->executor_report.segfaults_attempted*500;
    score -= function->executor_report.lines_executed;
    score -= function->codeline_count;
    
    run_count++;
  }
  return score/run_count;
}

int best_score;
void report_progress(struct Function_T * function, TaskJudge_T task_judge, int current_cycle,  int current_iteration, int total_iterations) {
  int current_score = get_fitness(function, task_judge);
  printf("\e[1;1H\e[2J\n\n"); // Clear screen
  if (current_cycle >= 0)
    printf("Cycle: #%i \n" , current_cycle);
  putchar_buff[putchar_i] = '\0';
  printf("Output = %s \n", putchar_buff);
  printf("Current Score = %i \n", current_score);
  printf("Best Score = %i \n", best_score);
  printf("Iterations = %'i / %'i\n\n", current_iteration, total_iterations);
  print_function_limited(function);
}

void simulated_annealing_search(struct Function_T * function, TaskJudge_T task_judge) {
    
  int score = get_fitness(function, task_judge);
  best_score = score;

  double last_update = getUnixTime();
  
  int last_batch_score = 0;
  long last_batch_iterations = 0;
  int batch_num = 0;
  
  long max_iterations = 1000000;
  long total_iterations = 0;
  while (1) {
    long i;
    for (i = 0; i < max_iterations; i++) {
      struct Change_T * change = propose_change(function);
      struct Change_T * inverse_change = apply_change(change);
      
      // Score run
      int new_score = get_fitness(function, task_judge);

      // Simulated annealing to determine if this passes
      int does_pass = 0;
      if (new_score > score)
        does_pass = 1;
      else {
        float p = exp(-(score - new_score)/(((float)(max_iterations - i)/(float)(max_iterations))*10.0));
        does_pass = fast_rand() < 32767.0 * p;
      }
      
      if (does_pass) {
        if (new_score > best_score)
          best_score = new_score;
        // Keep the new code
        score = new_score;
        free_change(inverse_change);
      }
      else {
        // Revert to the old code
        change = apply_change(inverse_change);

        free_change(change);
      }
      if (!(i % 100) && getUnixTime()-last_update > 0.5) {
        last_update += 0.5;
        report_progress(function, task_judge, batch_num, i, max_iterations);
      }
      total_iterations++;
    }
    if (score > 0) {
      report_progress(function, task_judge, batch_num, i, max_iterations);
      printf("Goal achieved, keep going? (Y/N)");
      char buff[10];
      gets(buff);
      if (buff[0] != 'Y' && buff[0] != 'y')
        break;
      last_update = getUnixTime();
    }
    last_batch_score = score;
    last_batch_iterations = max_iterations;
    max_iterations = max_iterations * 1.5;
    batch_num++;
  }
}


