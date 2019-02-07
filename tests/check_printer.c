#include "c_types.h"
#include "c_parser.h"
#include "c_printer.h"
#include "changes.h"
#include "change_proposers.h"
#include <stdlib.h>

void main() {
	// Generate a random function
	struct Environment_T * env = build_new_environment(10);
	randomly_populate_function(env->main, 100);
	// Print the function
	print_function(env->main);

}
