#include "c_types.h"
#include "c_parser.h"
#include "c_printer.h"
#include "changes.h"
#include "change_proposers.h"

int main() {

  // Generate a random function
  struct Environment_T * env = build_new_environment(10);
  randomly_populate_function(env->main, 500);
  env->main->name[4] = '2';
  env->main->name[5] = '\0';
  // Print the function
  print_function(env->main);
  // Generate to buffer
  char * source = print_function_to_buffer(env->main);

  // Parse it into a new environment
  struct Environment_T * env2 = build_new_environment(10);

  //load_from_file(env2, "examples/hello world/hello_world_orig.c");
  load_from_data(env2, source);
  //printf("%s", source);
  // Print the new function
  print_function(env2->last_function);

  char * second_source = print_function_to_buffer(env2->last_function);

  if (strcmp(source, second_source)) {
	  printf("Not equal\n");
  }
  else {
	  printf("Perfectly equal!\n");
  }
}
