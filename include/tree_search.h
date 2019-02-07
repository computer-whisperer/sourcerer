#include "c_types.h"

#ifndef TREE_SEARCH_H
#define TREE_SEARCH_H

#define TREE_SEARCH_NODE_CHILD_COUNT 4

struct TreeSearchNode_T {
	struct TreeSearchNode_T * parent;
	int generation;
	int total_games;
	int child_wins[TREE_SEARCH_NODE_CHILD_COUNT];
	struct TreeSearchNode_T * children[TREE_SEARCH_NODE_CHILD_COUNT];
	size_t c_code_len;
	char * c_code;
};

int global_agent_count;

struct TreeSearchNode_T * tree_search_add_agent_from_file(struct TreeSearchNode_T * parent, char * fname);
struct TreeSearchNode_T * tree_search_add_agent_from_function(struct TreeSearchNode_T * parent, struct Function_T * function);

struct TreeSearchNode_T * tree_search_pick_random_agent(struct TreeSearchNode_T * tree);

struct TreeSearchNode_T * tree_search_add_new_agent(struct TreeSearchNode_T * parent, char * c_code, size_t c_code_len);

void tree_search_report_agent_win(struct TreeSearchNode_T * winner, struct TreeSearchNode_T * looser);

#endif
