#include <locale.h>
#include <time.h>

#include "c_types.h"
#include "c_printer.h"
#include "utils.h"
#include "changes.h"
#include "change_proposers.h"
#include "tasks.h"
#include "search.h"

int main(int argc, char * argv[]){
  // Seed random num
  fast_srand(time(NULL));
  // Set locale for big number commas
  setlocale(LC_NUMERIC, "");


  // Build the base environment
  struct Environment_T * environment = build_new_environment(10000);

  TaskJudge_T judge;
  
  //judge = task_judge_count;
  //judge = task_judge_flip_number;
  //judge = task_judge_hello_world;
  judge = task_judge_calculator;
  //judge = task_judge_expfit;
  
  
  
  simulated_annealing_search(environment->main, judge);
}
