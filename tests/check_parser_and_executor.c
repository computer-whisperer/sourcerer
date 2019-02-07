#include "c_types.h"
#include "c_parser.h"
#include "c_printer.h"
#include "changes.h"
#include "change_proposers.h"

int main() {

  // Parse it into a new environment
  struct Environment_T * env = build_new_environment(10000);
  
  load_from_file(env, "tests/interpreter_test.c");

  // Print the new function
  print_function(env->last_function);

  char * second_source = print_function_to_buffer(env->last_function);
  execute_function(env->last_function, NULL, NULL, NULL, NULL, 0, 200);
  printf(putchar_buff);
}
