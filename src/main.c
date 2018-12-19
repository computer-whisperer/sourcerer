#include <locale.h>
#include <time.h>

#include "utils.h"
#include "change_proposers.h"
#include "tasks.h"
#include "search.h"

int main(int argc, char * argv[]){
  // Seed random num
  fast_srand(time(NULL));
  // Set locale for big number commas
  setlocale(LC_NUMERIC, "");

  struct Function_T * function = build_main_four_args();
  TaskJudge_T judge;
  
  judge = task_judge_count;
  //judge = task_judge_hello_world;
  //judge = task_judge_calculator;
  //judge = task_judge_expfit;
  
  simulated_annealing_search(function, judge);
}
