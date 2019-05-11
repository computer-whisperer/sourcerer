#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "c_parser.h"
#include "c_types.h"

void load_from_file(struct Environment_T * env, char * fname) {
	FILE * f = fopen(fname, "r");
	int buffer_len = 1000;
	char * buffer = malloc(buffer_len);
	int i = 0;
	while (!i || buffer[i-1] != EOF) {
		buffer[i] = fgetc(f);
		i++;

		// Simple reallocating buffer growth
		if (i == buffer_len) {
			int new_buffer_len = buffer_len*5;
			char * new_buffer = malloc(new_buffer_len);
			memcpy(new_buffer, buffer, buffer_len);
			free(buffer);
			buffer = new_buffer;
			buffer_len = new_buffer_len;
		}
	}
	fclose(f);
	buffer[i-1] = '\0'; // Overwrite EOF with empty string
	load_from_data(env, buffer);
}

void remove_blank_sub_chains(struct ParseChain_T * parent_chain) {
	struct ParseChain_T * chain = parent_chain->first_sub_chain;
	while (chain) {
		int good = 0;
		char * ptr = chain->data_start;
		while (ptr < chain->data_end) {
			if (*ptr != ' ') {
				good = 1;
				break;
			}
		}
		if (good)
			chain = chain->next;
		else {
			// Fly, be free
			struct ParseChain_T * to_free = chain;
			if (chain->prev)
				chain->prev->next = chain->next;
			else
				parent_chain->first_sub_chain = chain->next;

			if (chain->next)
				chain->next->prev = chain->prev;
			else
				parent_chain->last_sub_chain = chain->prev;
			chain = chain->next;
			free(to_free);
		}
	}
}

void partition_nesting_region(struct ParseChain_T * parent_chain, char open, char close, enum ParseChainType_T on_type, enum ParseChainType_T off_type) {
	int nest_level = 0;
	char * ptr = parent_chain->data_start;

	struct ParseChain_T * chain = malloc(sizeof(struct ParseChain_T));
	chain->type = off_type;
	chain->parent = parent_chain;
	chain->first_sub_chain = NULL;
	chain->last_sub_chain = NULL;
	chain->prev = NULL;
	chain->parent->first_sub_chain = chain;
	chain->data_start = ptr;

	int in_single_quote = 0;

	while (ptr < parent_chain->data_end) {
		if (in_single_quote) {
			if (*ptr == '\'') {
				in_single_quote = 0;
			}
		}
		else if (*ptr == '\'') {
			in_single_quote = 1;
		}
		else if (*ptr == open) {
			if (nest_level == 0) {
				chain->data_end = ptr;
				chain->next = malloc(sizeof(struct ParseChain_T));
				chain->next->prev = chain;
				chain = chain->next;
				chain->type = on_type;
				chain->parent = parent_chain;
				chain->data_start = ptr+1;
				chain->first_sub_chain = NULL;
				chain->last_sub_chain = NULL;
			}
			nest_level++;
		}
		else if (*ptr == close) {
			nest_level--;
			if (nest_level == 0) {
				chain->data_end = ptr;
				chain->next = malloc(sizeof(struct ParseChain_T));
				chain->next->prev = chain;
				chain = chain->next;
				chain->type = off_type;
				chain->parent = parent_chain;
				chain->data_start = ptr+1;
				chain->first_sub_chain = NULL;
				chain->last_sub_chain = NULL;
			}
		}
		ptr++;
	}

	chain->data_end = ptr;
	chain->next = NULL;
	parent_chain->last_sub_chain = chain;

	remove_blank_sub_chains(parent_chain);
}




// Partition comma separated list
void partition_list(struct ParseChain_T * parent_chain, enum ParseChainType_T type, char delimiter) {
	purge_children(parent_chain);
	int nest_level = 0;
	char * ptr = parent_chain->data_start;

	struct ParseChain_T * chain = malloc(sizeof(struct ParseChain_T));
	chain->type = type;
	chain->parent = parent_chain;
	chain->first_sub_chain = NULL;
	chain->last_sub_chain = NULL;
	chain->prev = NULL;
	chain->data_start = NULL;
	chain->data_end = NULL;
	chain->parent->first_sub_chain = chain;

	int in_single_quote = 0;
	int blank = 1;

	while(ptr < parent_chain->data_end) {
		if (*ptr == '\t')
			;
		else if (in_single_quote) {
			if (*ptr == '\'') {
				in_single_quote = 0;
				blank = 0;
				chain->data_end = ptr+1;
			}
		}
		else if (*ptr == '\'') {
			in_single_quote = 1;
			if (!chain->data_start)
				chain->data_start = ptr;
		}
		else if (*ptr == delimiter) {
			if (!blank) {
				chain->next = malloc(sizeof(struct ParseChain_T));
				chain->next->prev = chain;
				chain = chain->next;
				chain->type = type;
				chain->parent = parent_chain;
				chain->data_start = ptr;
				chain->first_sub_chain = NULL;
				chain->last_sub_chain = NULL;
				chain->data_start = NULL;
				chain->data_end = NULL;

				blank = 1;
			}
		}
		else if (*ptr != ' ') {
			blank = 0;
			chain->data_end = ptr+1;
			if (!chain->data_start)
				chain->data_start = ptr;
		}

		ptr++;
	}
	if (blank) {
		struct ParseChain_T * to_free = chain;
		chain = chain->prev;
		free(to_free);
		if (parent_chain->first_sub_chain == to_free)
			parent_chain->first_sub_chain = NULL;
	}

	if (chain)
		chain->next = NULL;
	parent_chain->last_sub_chain = chain;
}


/* Removes a chain from a parent chain, giving all children to their grandparent. */
void remove_chain_save_children(struct ParseChain_T * chain) {
	if (chain->first_sub_chain) {
		// Insert children into parent chain
		chain->first_sub_chain->prev = chain->prev;
		if (chain->prev)
			chain->prev->next = chain->first_sub_chain;
		else
			chain->parent->first_sub_chain = chain->first_sub_chain;

		// last_sub_chain should also exist

		chain->last_sub_chain->next = chain->next;
		if (chain->next)
			chain->next->prev = chain->last_sub_chain;
		else
			chain->parent->last_sub_chain = chain->first_sub_chain;

		// Remap parents
		struct ParseChain_T * child_chain = chain->first_sub_chain;
		while (child_chain != chain->last_sub_chain) {
			child_chain->parent = chain->parent;
			child_chain = child_chain->next;
		}
		if (chain->last_sub_chain)
			chain->last_sub_chain->parent = chain->parent;
	}
	else {
		// No children
		if (chain->prev)
			chain->prev->next = chain->next;
		else
			chain->parent->first_sub_chain = chain->next;

		if (chain->next)
			chain->next->prev = chain->prev;
		else
			chain->parent->last_sub_chain = chain->prev;
	}
	// Free it
	free(chain);
}

// Inserts a new chain as the parent of the child chain specified, removing the children from their current chain.
struct ParseChain_T * insert_chain_and_adopt_children(struct ParseChain_T * first_child, struct ParseChain_T * last_child, enum ParseChainType_T type) {
	struct ParseChain_T * chain = malloc(sizeof(struct ParseChain_T));
	chain->type = type;
	chain->data_start = first_child->data_start;
	chain->data_end = last_child->data_end;

	chain->prev = first_child->prev;
	first_child->prev = NULL;
	chain->next = last_child->next;
	last_child->next = NULL;
	chain->first_sub_chain = first_child;
	chain->last_sub_chain = last_child;

	chain->parent = first_child->parent;

	if (chain->prev)
		chain->prev->next = chain;
	else
		chain->parent->first_sub_chain = chain;

	if (chain->next)
		chain->next->prev = chain;
	else
		chain->parent->last_sub_chain = chain;

	// Remap parents
	struct ParseChain_T * child_chain = first_child;
	while (child_chain != last_child) {
		child_chain->parent = chain;
		child_chain = child_chain->next;
	}
	if (last_child)
		last_child->parent = chain;

	return chain;
}

void purge_children(struct ParseChain_T * parent_chain) {
	if (!parent_chain->first_sub_chain)
		return;
	struct ParseChain_T * chain = parent_chain->first_sub_chain;
	struct ParseChain_T * next_chain;
	while (chain) {
		next_chain = chain->next;
		purge_children(chain);
		free(chain);

		chain = next_chain;
	}
}

// Partitions a block of text into bare lines and blocks, merging block bodies and block declaration lines into larger chains.
void partition_blocks_lines(struct ParseChain_T * parent_chain, enum ParseChainType_T line_type, enum ParseChainType_T block_outer_type, enum ParseChainType_T block_decl_type, enum ParseChainType_T block_body_type) {
	// First level {} scan and partition
	partition_nesting_region(parent_chain, '{', '}', block_body_type, PARSE_CHAIN_TYPE_UNKNOWN);

	// Line partition all unknown chains and identify declaration lines.
	struct ParseChain_T * chain = parent_chain->first_sub_chain;
	struct ParseChain_T * next_chain;
	while (chain){
		next_chain = chain->next;
		if (chain->type == block_body_type) {
			// Encapsulate both the body and declaration
			chain = insert_chain_and_adopt_children(chain->prev, chain, block_outer_type);
			// Rename declaration
			chain->first_sub_chain->type = block_decl_type;
		}
		else {
			partition_list(chain, line_type, '\n');
			remove_chain_save_children(chain);
		}
		chain = next_chain;
	}
}

// Splits something like "char ** my_bed"
void partition_declaration(struct ParseChain_T * parent_chain) {
	char * ptr = parent_chain->data_end-1;

	struct ParseChain_T * chain = malloc(sizeof(struct ParseChain_T));
	chain->type = PARSE_CHAIN_TYPE_DECLARATION_NAME;
	chain->parent = parent_chain;
	chain->first_sub_chain = NULL;
	chain->last_sub_chain = NULL;
	chain->next = NULL;
	chain->parent->last_sub_chain = chain;

	// Eat trailing spaces
	while (ptr >= parent_chain->data_start && *ptr == ' ') {ptr--;};
	// Grab name
	chain->data_end = ptr + 1;
	while (ptr >= parent_chain->data_start && *ptr != ' ') {ptr--;};
	chain->data_start = ptr + 1;

	// Grab datatype
	chain->prev = malloc(sizeof(struct ParseChain_T));
	chain->prev->next = chain;
	chain = chain->prev;
	chain->type = PARSE_CHAIN_TYPE_DECLARATION_DATATYPE;
	chain->parent = parent_chain;
	chain->first_sub_chain = NULL;
	chain->last_sub_chain = NULL;
	chain->prev = NULL;
	chain->parent->first_sub_chain = chain;

	// Eat trailing spaces
	while (ptr >= parent_chain->data_start && *ptr == ' ') {ptr--;};
	chain->data_end = ptr + 1;

	// Flip and eat leading spaces
	ptr = parent_chain->data_start;
	while (ptr < chain->data_end && *ptr == ' ') {ptr++;};
	chain->data_start = ptr;
}

struct DataType_T * extract_data_type(struct ParseChain_T * parse_chain, struct Environment_T * env) {
	// Get pointer count
	int pointer_count = 0;
	char * ptr = parse_chain->data_end - 1;
	while (*ptr == '*' || *ptr == ' ') {
		if (*ptr == '*')
			pointer_count++;
		ptr--;
	}

	// Search for datatype name that matches the remaining name
	struct DataType_T * datatype = env->first_datatype;
	while (datatype) {
		if (datatype->pointer_degree == 0 && !strncmp(datatype->name, parse_chain->data_start, (ptr + 1) - parse_chain->data_start) && !datatype->name[(ptr + 1) - parse_chain->data_start]) {
			// Found it
			return datatype_pointer_jump(datatype, -pointer_count);
		}
		datatype = datatype->next;
	}
	return NULL;
	// TODO: Handle errors
}

int test_chain_equals(struct ParseChain_T * chain, char* str) {
	char * ptr = chain->data_start;
	while (ptr < chain->data_end) {
		if (*ptr != *str) {
			return 0;
		}
		ptr++;
		str++;
	}
	return *str == '\0';
}

int test_chain_starts_with(struct ParseChain_T * chain, char* str) {
	char * ptr = chain->data_start;
	while (*str != '\0') {
		if (*ptr != *str)
			return 0;
		if (ptr == chain->data_end)
			return 0;
		ptr++;
		str++;
	}
	return 1;
}

int test_chain_for_character(struct ParseChain_T * chain, char value) {
	char * ptr = chain->data_start;
	while (ptr < chain->data_end) {
		if (*ptr == value) {
			return 1;
		}
		ptr++;
	}
	return 0;
}

void parse_variable_declaration(struct Function_T * function, struct ParseChain_T * chain) {
	purge_children(chain);
	struct Variable_T * variable = malloc(sizeof(struct Variable_T));
	variable->function = function;
	variable->prev = function->last_variable;
	variable->next = NULL;
	if (function->last_variable)
		function->last_variable->next = variable;
	else
		function->first_variable = variable;
	function->last_variable = variable;
	variable->references = 0;
	variable->is_arg = 0;

	partition_declaration(chain);
	variable->data_type = extract_data_type(chain->first_sub_chain, function->environment);

	strncpy(variable->name, chain->last_sub_chain->data_start, chain->last_sub_chain->data_end - chain->last_sub_chain->data_start);
	variable->name[chain->last_sub_chain->data_end - chain->last_sub_chain->data_start] = '\0';
}

struct Variable_T * parse_variable_reference(struct Function_T * function, struct ParseChain_T * chain, int * pointer_degree) {
	int i;
	if (!pointer_degree)
		pointer_degree = &i;
	// Check pointer count
	*pointer_degree = 0;
	// Count pointer degree and chop off *s and &s
	char * ptr = chain->data_start;
	while (ptr < chain->data_end) {
		if (*ptr == '*')
			(*pointer_degree)++;
		else if (*ptr == '&')
			(*pointer_degree)--;
		else if (*ptr != ' ')
			break;
		ptr++;
	}
	// Search for matching variable
	struct Variable_T * variable = function->first_variable;
	while (variable) {
		if (!strncmp(variable->name, ptr, chain->data_end - ptr) && !variable->name[chain->data_end - ptr]) {
			variable->references++;
			return variable;
		}
		variable = variable->next;
	}
	return NULL;
}

int parse_number(struct ParseChain_T * chain) {
	char * ptr = chain->data_start;
	while (*ptr == ' '){ptr++;}
	int num = 0;
	int sign = 1;
	int base = 10;
	if (*ptr == '0')
		base = 8;
	while (ptr < chain->data_end) {
		if (*ptr == '+') {
			sign = 1;
		}
		else if (*ptr == '-') {
			sign = -1;
		}
		else if (*ptr >= '0' && *ptr <= '9') {
			num = num*base + (*ptr - '0');
		}
		else {
			break;
		}
		ptr++;
	}
	return num*sign;
}

void parse_and_load_single_codeline(struct Function_T * function, struct ParseChain_T * chain) {
	purge_children(chain);
	int last_codeline_count = function->codeline_count;

	// Setup new codeline instance
	struct CodeLine_T * codeline = malloc(sizeof(struct CodeLine_T));
	codeline->function = function;
	codeline->prev = function->last_codeline;
	codeline->next = NULL;
	if (function->last_codeline)
		function->last_codeline->next = codeline;
	else
		function->first_codeline = codeline;
	function->last_codeline = codeline;
	for (int i = 0; i < FUNCTION_ARG_COUNT; i++)
		codeline->args[i] = NULL;
	function->codeline_count++;

	// Search for an = character
	partition_list(chain, PARSE_CHAIN_TYPE_UNKNOWN, '=');
	if (chain->first_sub_chain == chain->last_sub_chain) {
		// No =, check if it is a return
		if (chain->data_end - chain->data_start >= 6 && !strncmp("return", chain->data_start, 6)) {
			partition_list(chain, PARSE_CHAIN_TYPE_UNKNOWN, ' ');
			codeline->type = CODELINE_TYPE_RETURN;
			codeline->assigned_variable = parse_variable_reference(function, chain->last_sub_chain, NULL);
			return;
		}
		else
			parse_variable_declaration(function, chain);
	}
	else {
		codeline->assigned_variable = parse_variable_reference(function, chain->first_sub_chain, &(codeline->assigned_variable_reference_count));
		// Identify right side

		// Is this a char constant
		if (codeline->assigned_variable->data_type == function->environment->char_datatype) {
			// Do we see a ' character?
			if (*(chain->last_sub_chain->data_start) == '\'') {
				codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
				codeline->constant.c = chain->last_sub_chain->data_start[1];
				return;
			}
			if (*(chain->last_sub_chain->data_start) == '(') {
				int i  = 2;
			}
			// Does this start with (char)?
			if (test_chain_starts_with(chain->last_sub_chain, "(char)")) {
				// Parse in number
				codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
				partition_nesting_region(chain->last_sub_chain, '(', ')', PARSE_CHAIN_TYPE_UNKNOWN, PARSE_CHAIN_TYPE_UNKNOWN);
				codeline->constant.c = parse_number(chain->last_sub_chain->last_sub_chain);
				return;
			}
		}
		// Is this an int constant
		if (codeline->assigned_variable->data_type == function->environment->int_datatype) {
			if (*(chain->last_sub_chain->data_start) == '+' ||
					*(chain->last_sub_chain->data_start) == '-' ||
					(*(chain->last_sub_chain->data_start) >= '0' && *(chain->last_sub_chain->data_start) <= '9')) {
				codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
				codeline->constant.i = parse_number(chain->last_sub_chain);
				return;
			}
		}
		// Is this a function call?
		partition_nesting_region(chain->last_sub_chain, '(', ')', PARSE_CHAIN_TYPE_UNKNOWN, PARSE_CHAIN_TYPE_UNKNOWN);
		if (chain->last_sub_chain->first_sub_chain != chain->last_sub_chain->last_sub_chain) {
			// Yes, this is a function
			codeline->type = CODELINE_TYPE_FUNCTION_CALL;
			codeline->target_function = function->environment->first_function;
			while (codeline->target_function && !test_chain_equals(chain->last_sub_chain->first_sub_chain, codeline->target_function->name))
				codeline->target_function = codeline->target_function->next;

			partition_list(chain->last_sub_chain->last_sub_chain, PARSE_CHAIN_TYPE_FUNCTION_ARGUMENT, ',');

			int i = 0;
			struct ParseChain_T * argument = chain->last_sub_chain->last_sub_chain->first_sub_chain;
			while (argument) {
				codeline->args[i] = parse_variable_reference(function, argument, NULL);
				i++;
				argument = argument->next;
			}
			return;
		}
		// Break apart simple ops
		int success = 0;
		char op = '\0';
		if (!success) {
			partition_list(chain->last_sub_chain, PARSE_CHAIN_TYPE_VARIABLE_REFERENCE, '+');
			if (chain->last_sub_chain->first_sub_chain != chain->last_sub_chain->last_sub_chain) {
				success = 1;
				op = '+';
			}
		}
		if (!success) {
			partition_list(chain->last_sub_chain, PARSE_CHAIN_TYPE_VARIABLE_REFERENCE, '-');
			if (chain->last_sub_chain->first_sub_chain != chain->last_sub_chain->last_sub_chain) {
				success = 1;
				op = '-';
			}
		}
		if (!success) {
			partition_list(chain->last_sub_chain, PARSE_CHAIN_TYPE_VARIABLE_REFERENCE, '*');
			if (chain->last_sub_chain->first_sub_chain != chain->last_sub_chain->last_sub_chain) {
				success = 1;
				op = '*';
			}
		}
		if (!success) {
			partition_list(chain->last_sub_chain, PARSE_CHAIN_TYPE_VARIABLE_REFERENCE, '/');
			if (chain->last_sub_chain->first_sub_chain != chain->last_sub_chain->last_sub_chain) {
				success = 1;
				op = '/';
			}
		}
		if (success) {
			codeline->type = CODELINE_TYPE_FUNCTION_CALL;
			codeline->target_function = function->environment->first_function;
			while (codeline->target_function && !(codeline->target_function->type == FUNCTION_TYPE_BASIC_OP && codeline->target_function->name[0] == op))
				codeline->target_function = codeline->target_function->next;

			codeline->args[0] = parse_variable_reference(function, chain->last_sub_chain->first_sub_chain, &codeline->arg0_reference_count);
			codeline->args[1] = parse_variable_reference(function, chain->last_sub_chain->last_sub_chain, NULL);
			// Test for a disguised pointer assignment
			if (op == '+' && (codeline->assigned_variable->data_type->pointer_degree != 0)) {
				codeline->type = CODELINE_TYPE_POINTER_ASSIGNMENT;
			}
			return;
		}
		// Now assume this is a pointer assignment
		codeline->type = CODELINE_TYPE_POINTER_ASSIGNMENT;
		codeline->args[0] = parse_variable_reference(function, chain->last_sub_chain->first_sub_chain, &codeline->arg0_reference_count);
		return;
	}

	// Unsuccessful, free codeline
	if (codeline->prev)
		codeline->prev->next = NULL;
	function->last_codeline = codeline->prev;
	free(codeline);
	function->codeline_count = last_codeline_count;
}

void parse_condition_chain(struct CodeLine_T * codeline, struct ParseChain_T * parent_chain) {
	purge_children(parent_chain);

	char * ptr = parent_chain->data_start;
	struct ParseChain_T * chain = malloc(sizeof(struct ParseChain_T));
	struct ParseChain_T * last_chain;
	chain->type = PARSE_CHAIN_TYPE_VARIABLE_REFERENCE;
	chain->parent = parent_chain;
	chain->prev = NULL;
	chain->first_sub_chain = NULL;
	chain->last_sub_chain = NULL;
	parent_chain->first_sub_chain = chain;

	// Scan past spaces
	while (*ptr==' '){ptr++;}
	chain->data_start = ptr;

	while (1) {
		if (*ptr == ' ')
			break;
		if (*ptr == '=')
			break;
		if (*ptr == '<')
			break;
		if (*ptr == '>')
			break;
		if (*ptr == '!')
			break;
		ptr++;
	}
	chain->data_end = ptr;
	int i;
	codeline->arg0_reference_count = 0;
	codeline->args[0] = parse_variable_reference(codeline->function, chain, NULL);

	last_chain = chain;
	chain = malloc(sizeof(struct ParseChain_T));
	chain->type = PARSE_CHAIN_TYPE_UNKNOWN;
	chain->parent = parent_chain;
	chain->prev = last_chain;
	last_chain->next = chain;
	chain->first_sub_chain = NULL;
	chain->last_sub_chain = NULL;

	// Scan past spaces
	while (*ptr==' '){ptr++;}
	chain->data_start = ptr;

	int less_thans = 0;
	int greater_thans = 0;
	int equals = 0;
	int nots = 0;
	while (1) {
		if (*ptr == '=')
			equals++;
		else if (*ptr == '<')
			less_thans++;
		else if (*ptr == '>')
			greater_thans++;
		else if (*ptr == '!')
			nots++;
		else
			break;
		ptr++;
	}

	chain->data_end = ptr;
	if (less_thans > 0) {
		if (equals > 0)
			codeline->condition = CONDITION_LESS_THAN_OR_EQUAL;
		else
			codeline->condition = CONDITION_LESS_THAN;
	}
	else if (greater_thans > 0) {
		if (equals > 0)
			codeline->condition = CONDITION_GREATER_THAN_OR_EQUAL;
		else
			codeline->condition = CONDITION_GREATER_THAN;
	}
	else if (nots > 0)
		codeline->condition = CONDITION_NOT_EQUAL;
	else
		codeline->condition = CONDITION_EQUAL;

	last_chain = chain;
	chain = malloc(sizeof(struct ParseChain_T));
	chain->type = PARSE_CHAIN_TYPE_VARIABLE_REFERENCE;
	chain->parent = parent_chain;
	chain->prev = last_chain;
	last_chain->next = chain;
	chain->first_sub_chain = NULL;
	chain->last_sub_chain = NULL;

	// Scan past spaces
	while (*ptr==' '){ptr++;}
	chain->data_start = ptr;

	while (1) {
		if (*ptr == ' ')
			break;
		if (*ptr == ')')
			break;
		ptr++;
	}
	chain->data_end = ptr;
	chain->next = NULL;
	parent_chain->last_sub_chain = chain;
	codeline->args[1] = parse_variable_reference(codeline->function, chain, NULL);
}

void parse_and_load_codeline_block(struct Function_T * function, struct ParseChain_T * codeline_chain) {
	partition_blocks_lines(codeline_chain, PARSE_CHAIN_TYPE_CODELINE, PARSE_CHAIN_TYPE_BLOCK, PARSE_CHAIN_TYPE_BLOCK_DECLARATION, PARSE_CHAIN_TYPE_BLOCK_BODY);
	struct ParseChain_T * chain = codeline_chain->first_sub_chain;

	// First pass: break apart multi-semicolon lines (outside of blocks).
	struct ParseChain_T * next_chain;
	while (chain) {
		next_chain = chain->next;

		if (chain->type == PARSE_CHAIN_TYPE_CODELINE) {
			// Split apart by semicolons
			partition_list(chain, PARSE_CHAIN_TYPE_CODELINE, ';');
			remove_chain_save_children(chain);
		}
		chain = next_chain;
	}

	chain = codeline_chain->first_sub_chain;
	while (chain) {
		if (chain->type == PARSE_CHAIN_TYPE_BLOCK) {
			// Setup new codeline instance
			struct CodeLine_T * codeline = malloc(sizeof(struct CodeLine_T));
			codeline->function = function;
			codeline->assigned_variable = NULL;
			codeline->prev = function->last_codeline;
			codeline->next = NULL;
			if (function->last_codeline)
				function->last_codeline->next = codeline;
			else
				function->first_codeline = codeline;
			function->last_codeline = codeline;
			for (int i = 0; i < FUNCTION_ARG_COUNT; i++)
				codeline->args[i] = NULL;
			function->codeline_count++;

			partition_nesting_region(chain->first_sub_chain, '(', ')', PARSE_CHAIN_TYPE_FUNCTION_ARGUMENTS, PARSE_CHAIN_TYPE_DECLARATION);

			if (!strncmp(chain->first_sub_chain->data_start, "if", 2))
				codeline->type = CODELINE_TYPE_IF;
			if (!strncmp(chain->first_sub_chain->data_start, "while", 5))
				codeline->type = CODELINE_TYPE_WHILE;

			parse_condition_chain(codeline, chain->first_sub_chain->last_sub_chain);

			// Handle inside block
			parse_and_load_codeline_block(function, chain->last_sub_chain);

			// Setup new codeline instance
			struct CodeLine_T * other_codeline = malloc(sizeof(struct CodeLine_T));
			other_codeline->function = function;
			other_codeline->prev = function->last_codeline;
			other_codeline->next = NULL;
			other_codeline->assigned_variable = NULL;
			if (function->last_codeline)
				function->last_codeline->next = other_codeline;
			function->last_codeline = other_codeline;
			for (int i = 0; i < FUNCTION_ARG_COUNT; i++)
				other_codeline->args[i] = NULL;
			other_codeline->block_other_end = codeline;
			codeline->block_other_end = other_codeline;
			other_codeline->type = CODELINE_TYPE_BLOCK_END;
			function->codeline_count++;
		}
		if (chain->type == PARSE_CHAIN_TYPE_CODELINE)
			parse_and_load_single_codeline(function, chain);
		chain = chain->next;
	}


}

void parse_and_load_function_declaration(struct Environment_T * env, struct ParseChain_T * function_chain) {
	purge_children(function_chain->first_sub_chain);
	partition_nesting_region(function_chain->first_sub_chain, '(', ')', PARSE_CHAIN_TYPE_FUNCTION_ARGUMENTS, PARSE_CHAIN_TYPE_DECLARATION);

	struct Function_T * function = malloc(sizeof(struct Function_T));
	function->type = FUNCTION_TYPE_CUSTOM;
	function->environment = env;
	function->next = NULL;
	function->prev = env->last_function;
	function->first_variable = NULL;
	function->last_variable = NULL;
	function->first_codeline = NULL;
	function->last_codeline = NULL;
	function->codeline_count = 0;
	env->last_function = function;

	struct ParseChain_T * function_declaration_line = function_chain->first_sub_chain;

	// Grab the name
	partition_declaration(function_declaration_line->first_sub_chain);
	strncpy(function->name, function_declaration_line->first_sub_chain->last_sub_chain->data_start, function_declaration_line->first_sub_chain->last_sub_chain->data_end - function_declaration_line->first_sub_chain->last_sub_chain->data_start);
	function->name[function_declaration_line->first_sub_chain->last_sub_chain->data_end - function_declaration_line->first_sub_chain->last_sub_chain->data_start] = '\0';

	// Grab the return datatype
	function->return_datatype = extract_data_type(function_declaration_line->first_sub_chain->first_sub_chain, env);

	// Handle no argument list
	int i = 0;
	if (function_chain->first_sub_chain->last_sub_chain->type == PARSE_CHAIN_TYPE_DECLARATION) {
		while (i < FUNCTION_ARG_COUNT) {
			function->args[i] = NULL;
			i++;
		}
	}
	else {
		// Parse argument list
		struct ParseChain_T * function_argument_chain = function_declaration_line->last_sub_chain;
		partition_list(function_argument_chain, PARSE_CHAIN_TYPE_FUNCTION_ARGUMENT, ',');

		struct ParseChain_T * argument = function_argument_chain->first_sub_chain;
		while (argument) {
			parse_variable_declaration(function, argument);
			function->last_variable->is_arg = 1;
			function->last_variable->references = 1;
			function->args[i] = function->last_variable;
			i++;
			argument = argument->next;
		}
		while (i < FUNCTION_ARG_COUNT) {
			function->args[i] = NULL;
			i++;
		}
	}

	// Parse function lines of code
	parse_and_load_codeline_block(function, function_chain->last_sub_chain);

	if (function->prev)
		function->prev->next = function;
	else
		function->environment->first_function = function;
}

void load_from_data(struct Environment_T * env, char * data) {
	struct ParseChain_T * root_chain = malloc(sizeof(struct ParseChain_T));
	root_chain->next = NULL;
	root_chain->prev = NULL;
	root_chain->parent = NULL;
	root_chain->type = PARSE_CHAIN_TYPE_ROOT;
	root_chain->data_start = data;
	root_chain->first_sub_chain=NULL;
	root_chain->last_sub_chain=NULL;

	// Find the end of the data
	root_chain->data_end = data;
	while(*root_chain->data_end != '\0') root_chain->data_end++;

	//
	partition_blocks_lines(root_chain, PARSE_CHAIN_TYPE_UNKNOWN, PARSE_CHAIN_TYPE_BLOCK, PARSE_CHAIN_TYPE_BLOCK_DECLARATION, PARSE_CHAIN_TYPE_BLOCK_BODY);

	// Iterate over blocks and identify them
	struct ParseChain_T * chain = root_chain->first_sub_chain;
	while (chain) {
		if (chain->type == PARSE_CHAIN_TYPE_BLOCK) {
			// Test if it is a function
			if (test_chain_for_character(chain->first_sub_chain, '(')) {
				// Label parse chains
				chain->type = PARSE_CHAIN_TYPE_FUNCTION;
				chain->first_sub_chain->type = PARSE_CHAIN_TYPE_FUNCTION_DECLARATION;
				chain->last_sub_chain->type = PARSE_CHAIN_TYPE_FUNCTION_BODY;

				parse_and_load_function_declaration(env, chain);
			}
		}
		chain = chain->next;
	}
	// Free all chains
	purge_children(root_chain);
	free(root_chain);


	/*

  // First level {} scan and partition
  partition_nesting_region(root_chain, '{', '}', PARSE_CHAIN_TYPE_BLOCK_BODY, PARSE_CHAIN_TYPE_UNKNOWN);

  struct ParseChain_T * block_body_chain = root_chain->first_sub_chain;
  char * ptr;

  // Iterate over block chains
  while (block_body_chain) {
    if (block_body_chain->type == PARSE_CHAIN_TYPE_UNKNOWN) {
      block_body_chain = block_body_chain->next;
      continue;
    }

    // Partition the previous unknown block
    partition_lines(block_body_chain->prev);

    // Partition the body chain
    partition_lines(block_body_chain);

    // Partition (presumed) body chain declaration
    struct ParseChain_T * block_declaration_chain = block_body_chain->prev->first_sub_chain;
    block_declaration_chain->type = PARSE_CHAIN_TYPE_BLOCK_DECLARATION;
    partition_nesting_region(block_declaration_chain, '(', ')', PARSE_CHAIN_TYPE_ARGUMENTS, PARSE_CHAIN_TYPE_DECLARATION);

    if (block_declaration_chain->last_sub_chain->type == PARSE_CHAIN_TYPE_ARGUMENTS) {
      // This is a function declaration

      // Ignore functions without the macro
      char * macro_name = "SOURCERER_FUNCTION";
      if (!block_body_chain->first_sub_chain || strncmp(macro_name, block_body_chain->first_sub_chain->data_start, strlen(macro_name)))
        continue;

      // We found a function we are interested in, start building the tree
      struct Function_T * function = malloc(sizeof(struct Function_T));
      function->environment = env;
      function->next = NULL;
      function->prev = env->last_function;
      env->last_function = function;

      partition_declaration(block_declaration_chain->first_sub_chain);
      strncpy(function->name, block_declaration_chain->first_sub_chain->last_sub_chain->data_start, block_declaration_chain->first_sub_chain->last_sub_chain->data_start - block_declaration_chain->first_sub_chain->last_sub_chain->data_end);


      // functionality test
	 *block_declaration_chain->data_start = '!';
	 *block_declaration_chain->data_end = '!';

    }

    block_body_chain = block_body_chain->next;
  }
  printf("%s", data);
	 */
}



