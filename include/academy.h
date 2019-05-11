#include "c_types.h"

#ifndef ACADEMY_H
#define ACADEMY_H

#define ACADEMY_MAX_LOADED_AGENT_COUNT 10000000

enum Academy_Agent_State_T {
	ACADEMY_AGENT_STATE_EMPTY,
	ACADEMY_AGENT_STATE_ALIVE,
	ACADEMY_AGENT_STATE_UNLOADED,
};

struct Academy_Agent_Child_T {
	struct Academy_Agent_T * agent;
	enum Academy_Agent_State_T state;
	int local_points;
	int local_games_played;
	int games_played;
	float value;
};

struct Academy_Agent_T {
	struct Academy_T * academy;
	struct Academy_Agent_T * parent;
	int generation;
	int times_queried;
	float own_local_points;
	int own_local_games;
	int own_games;
	float own_value;
	struct Academy_Agent_Child_T * children;
	int children_slots_allocated;
	int aborted_children_count;
	int children_count;
	size_t c_code_len;
	char * c_code;
	unsigned long c_code_hash;
};

struct Academy_Hashtable_Row_T {
	struct Academy_Agent_T * agent;
	unsigned long hash;
};

struct Academy_T {
	struct Academy_Agent_T * root_agent;
	int agent_count;
	int duplicates_rejected;
	int loaded_agent_count;
	struct Academy_Hashtable_Row_T * hashtable;
	int hashtable_len;
};

struct Academy_T * build_new_academy();

struct Academy_Agent_T * academy_add_agent_from_file(struct Academy_T * academy, struct Academy_Agent_T * parent, char * fname);
struct Academy_Agent_T * academy_add_agent_from_function(struct Academy_T * academy, struct Academy_Agent_T * parent, struct Function_T * function);

void academy_prune_node(struct Academy_Agent_T * tree);

void academy_select_matchup(struct Academy_T * academy, struct Academy_Agent_T **, struct Academy_Agent_T **);

struct Academy_Agent_T * academy_add_new_agent(struct Academy_T * academy, struct Academy_Agent_T * parent, char * c_code, size_t c_code_len);

void academy_report_agent_win(struct Academy_Agent_T * winner, float winner_points, struct Academy_Agent_T * looser, float looser_points);

#endif
