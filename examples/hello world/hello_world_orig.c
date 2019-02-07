#include "sourcerer.h"
#include "math.h"

int main(int argc, char * argv[]) {
  // Initialize sourcerer
  struct Environment_T * env = build_new_environment(1000);
  env->load_from_file("hello_world.c");
  
  long cumulative_error = 0;
  for (int i = 0; i < 100; i++) {
    long buffer = 0;
    for (int j = 0; j < 100; j++) {
      // Normal execution
      buffer += do_a_thing(i, j);
      
      // Need to do this
      sourcerer_update(env);
    }
    // Online value report
    long value = -abs(i*100 + 4950 - buffer);
    sourcerer_report_value(env, value);
    printf("%i\n", value);
  }
  
  // TODO:
  //env->save_to_file("hello_world.c");
}

int do_a_thing(int a, int b) {
  SOURCERER_FUNCTION_REDIRECT(env, "do_a_thing", a, b);
}
