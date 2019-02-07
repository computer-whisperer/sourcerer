#include <stdio.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "tasks.h"

int task_judge_expfit(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs){

  long goal = exp(last_task_inputs->a);
  long error = goal - last_task_outputs->ret;
  if (error < 0)
    error = -error;
  if (error > 100)
    error = 100;
  int score = -error*100;
  
  
  if (next_task_inputs->run_index == 10)
    next_task_inputs->run_index = -1;
  else {
    next_task_inputs->a = next_task_inputs->run_index;
  }
  
  return score;
}

int task_judge_calculator(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs){

  int score = 0;
  
  long goal = 0;
  switch(last_task_inputs->c) {
    case 0:
      goal = last_task_inputs->a + last_task_inputs->b;
      break;
    case 1:
      goal = last_task_inputs->a - last_task_inputs->b;
      break;
    case 2:
      goal = last_task_inputs->a * last_task_inputs->b;
      break;
    case 3:
      if (last_task_inputs->b)
        goal = last_task_inputs->a / last_task_inputs->b;
      break;
  }        
  
  long error = goal - last_task_outputs->ret;
  if (error < 0)
    error = -error;
  if (error > 100)
    error = 100;
  score -= error*500;
  
  if (next_task_inputs->run_index == 10)
    next_task_inputs->run_index = -1;
  else {
    next_task_inputs->a = fast_rand_seeded(next_task_inputs->run_index) % 20;
    next_task_inputs->b = fast_rand_seeded(next_task_inputs->run_index+50) % 20;
    next_task_inputs->c = fast_rand_seeded(next_task_inputs->run_index+192) % 4;
  }

  return score;
}



int task_judge_count(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs){

  int score = 0;
  for (int i = 0; i < last_task_inputs->a; i++) {
    if (last_task_outputs->putchar_text_len <= i) {
      score -= 1000*(last_task_inputs->a - i);
      break;
    }
    if (putchar_buff[i] != last_task_inputs->d)
      score -= 1000;
  }
  if (last_task_outputs->putchar_text_len > last_task_inputs->a)
    score -= (last_task_outputs->putchar_text_len - last_task_inputs->a)*1000;
    
  if (next_task_inputs->run_index == 10)
    next_task_inputs->run_index = -1;
  else if (next_task_inputs->run_index == 0) {
    next_task_inputs->a = 0;
    next_task_inputs->d = 97;
  }
  else {
    next_task_inputs->a = (last_task_inputs->a + 1)%30;
    next_task_inputs->d = 97 + ((last_task_inputs->d-97) + 1)%30;
  }
  
  return score;
}


int task_judge_flip_number(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs){
  long score = 0;
  
  int input_number = last_task_inputs->a;
  int target_number = 0;
  while (input_number > 0) {
    target_number *= 10;
    target_number += input_number % 10;
    input_number = input_number / 10;
  }
  
  long error = target_number - last_task_outputs->ret;
  if (error < 0)
    error = -error;
  
  score -= error * 100;
    
  if (next_task_inputs->run_index == 8)
    next_task_inputs->run_index = -1;

  next_task_inputs->a = fast_rand_seeded(next_task_inputs->run_index + 1)%1000;
  
  if (score > 0 || score < -10000)
    score = -10000;
  
  return score;
}


int task_judge_hello_world(struct TaskInputs_T * last_task_inputs, struct TaskOutputs_T * last_task_outputs, struct TaskInputs_T * next_task_inputs){

  int score = 0;

  char * goal = "Hello world!";

  for (int i = 0; i < strlen(goal); i++) {
    if (last_task_outputs->putchar_text_len <= i) {
      score -= 200*(strlen(goal) - i);
      break;
    }
    int error = goal[i] - last_task_outputs->putchar_text[i];
    if (error < 0)
      error = -error;
    if (error > 50)
      error = 50;
    score -= error*20;
  }
  
  if (next_task_inputs->run_index)
    next_task_inputs->run_index = -1;

  return score;
}
