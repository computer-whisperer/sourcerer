#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree_search.h"
#include "c_types.h"
#include "c_printer.h"
#include "utils.h"

#define FREE_GAMES 1

struct TreeSearchNode_T * tree_search_add_agent_from_file(struct TreeSearchNode_T * parent, char * fname) {
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
	return tree_search_add_new_agent(parent, buffer, i);
}

struct TreeSearchNode_T * tree_search_add_agent_from_function(struct TreeSearchNode_T * parent, struct Function_T * function) {
	char * buffer = print_function_to_buffer(function);
	return tree_search_add_new_agent(parent, buffer, -1);
}

struct TreeSearchNode_T * tree_search_pick_random_agent(struct TreeSearchNode_T * tree) {
	while(tree) {
		int rand_max = tree->total_games + TREE_SEARCH_NODE_CHILD_COUNT*FREE_GAMES;
		int r = fast_rand()%rand_max;
		int i = 0;
		while (tree->children[i]){
			r -= FREE_GAMES;
			r -= tree->child_wins[i];
			if (r <= 0) {
				tree = tree->children[i];
				break;
			}
			i++;
			if (i == TREE_SEARCH_NODE_CHILD_COUNT)
				return NULL; // sum ting wong
		}
		if (r > 0)
			return tree;
	}
	return NULL;
}

struct TreeSearchNode_T * tree_search_add_new_agent(struct TreeSearchNode_T * parent, char * c_code, size_t c_code_len) {
	if (parent && parent->children[TREE_SEARCH_NODE_CHILD_COUNT-1])
		return NULL;
	struct TreeSearchNode_T * node = malloc(sizeof(struct TreeSearchNode_T));
	node->parent = parent;
	if (parent)
		node->generation = parent->generation + 1;
	else
		node->generation = 0;
	node->total_games = 0;
	int i;
	for (i=0; i < TREE_SEARCH_NODE_CHILD_COUNT; i++) {
		node->children[i] = NULL;
		node->child_wins[i] = 0;
	}
	node->c_code = c_code;
	node->c_code_len = c_code_len;
	if (parent) {
		i = 0;
		while (parent->children[i]) i++;
		parent->children[i] = node;
	}
	global_agent_count++;
	if (global_agent_count > 100000) {
		return NULL;
	}
	return node;
}

void tree_search_report_agent_win(struct TreeSearchNode_T * winner, struct TreeSearchNode_T * looser) {
	if (winner == looser)
		return;
	// Find the common parent
	while (winner->generation > looser->generation) {
		winner = winner->parent;
	}
	while (looser->generation > winner->generation) {
		looser = looser->parent;
	}
	while (looser->parent != winner->parent) {
		looser = looser->parent;
		winner = winner->parent;
	}
	struct TreeSearchNode_T * parent = winner->parent;
	if (parent) {
	parent->total_games++;
		int i = 0;
		while (parent->children[i] != winner && i < TREE_SEARCH_NODE_CHILD_COUNT)
			i++;
		parent->child_wins[i]++;
	}
}
