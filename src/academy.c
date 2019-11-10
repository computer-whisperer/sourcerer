#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "c_types.h"
#include "c_printer.h"
#include "utils.h"
#include "academy.h"

#define UCB_C 2


struct Academy_Hashtable_Row_T * academy_hashtable_lookup(struct Academy_T * academy, unsigned long hash) {
	struct Academy_Hashtable_Row_T * row;
	// Quadratic probing
	for (int i = 0; i < academy->hashtable_len; i++) {
		row = &(academy->hashtable[(i*i + hash)%academy->hashtable_len]);
		if (!row->agent)
			return row;
		if (row->hash == hash)
			return row;
	}
	return NULL;
}

struct Academy_Agent_T * academy_add_agent_from_file(struct Academy_T * academy, struct Academy_Agent_T * parent, char * fname) {
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
	return academy_add_new_agent(academy, parent, buffer, i);
}

struct Academy_Agent_T * academy_add_agent_from_function(struct Academy_T * academy, struct Academy_Agent_T * parent, struct Function_T * function) {
	char * buffer = print_function_to_buffer(function);
	return academy_add_new_agent(academy, parent, buffer, strlen(buffer));
}


struct Academy_T * build_new_academy() {
	struct Academy_T * academy = malloc(sizeof(struct Academy_T));
	academy->agent_count = 0;
	academy->duplicates_rejected = 0;
	academy->loaded_agent_count = 0;
	academy->root_agent = NULL;
	academy->hashtable_len = 1000;
	academy->hashtable = malloc(sizeof(struct Academy_Hashtable_Row_T)*academy->hashtable_len);
	for (int i = 0; i < academy->hashtable_len; i++) {
		academy->hashtable[i].agent = NULL;
		academy->hashtable[i].hash = 0;
	}
	return academy;
}

void academy_prune_node(struct Academy_Agent_T * tree) {
	int i;
	for (i = 0; i < tree->children_count; i++)
		if (tree->children[i].state == ACADEMY_AGENT_STATE_ALIVE) {
			academy_prune_node(tree->children[i].agent);
			academy_hashtable_lookup(tree->academy, tree->c_code_hash)->agent = NULL;
			free(tree->children[i].agent);
			tree->children[i].agent = NULL;
		}
	if (tree->c_code) {
		free(tree->c_code);
		tree->c_code = NULL;
	}
	tree->academy->loaded_agent_count--;
}

void tree_search_test_prune_from_node(struct Academy_Agent_T * tree) {
	float threshold = (float)tree->academy->loaded_agent_count/ACADEMY_MAX_LOADED_AGENT_COUNT;
	for (int i = 0; i < tree->children_count; i++) {
		 if (tree->children[i].state == ACADEMY_AGENT_STATE_ALIVE && tree->children[i].local_games_played > 100 && tree->children[i].value < threshold) {
			 academy_prune_node(tree->children[i].agent);
			 tree->children[i].state = ACADEMY_AGENT_STATE_UNLOADED;
			 threshold = (float)tree->academy->loaded_agent_count/ACADEMY_MAX_LOADED_AGENT_COUNT;
		 }
	}
}

void academy_select_matchup_greedy(struct Academy_T * academy, struct Academy_Agent_T ** agent1, struct Academy_Agent_T ** agent2) {
	struct Academy_Agent_T * current_agent = academy->root_agent;
	float next_best_gap = 100.0;

	int next_best_agent_should_search = 0;
	struct Academy_Agent_T * next_best_agent = NULL;

	int pass = 0;
	while(current_agent) {
		current_agent->times_queried++;

		// Randomly check for prunable nodes to keep memory usage sane
		if (fast_rand()%30 == 0)
			tree_search_test_prune_from_node(current_agent);

		// Score all candidates (first is parent)
		float scores[current_agent->children_count+1];
		float child_failure_ratio = (current_agent->aborted_children_count)/(current_agent->children_count+current_agent->aborted_children_count+1);
		//  - fastPow(child_failure_ratio, 8)
		scores[0] = current_agent->own_value + UCB_C*sqrt(ceil_log2(current_agent->times_queried) / (float)current_agent->own_games);
		int i;
		for (i = 0; i < current_agent->children_count; i++) {
			if (current_agent->children[i].state == ACADEMY_AGENT_STATE_ALIVE)
				scores[i+1] = current_agent->children[i].value + UCB_C*sqrt(ceil_log2(current_agent->times_queried) / (float)current_agent->children[i].games_played);
			else {
				scores[i+1] = 0.0;
			}
		}

		// Identify best score;
		float best = 0;
		int best_index = 0;
		for (i = 0; i < current_agent->children_count+1; i++) {
			if (scores[i] > best) {
				best_index = i;
				best = scores[i];
			}
		}

		// Store next best score if this is the first pass
		if (pass == 0) {
			// Identify next best score;
			// Identify best score;
			float next_best = 0;
			int next_best_index = 0;
			for (i = 0; i < current_agent->children_count+1; i++) {
				if (i == best_index)
					continue;
				if (scores[i] > next_best) {
					next_best_index = i;
					next_best = scores[i];
				}
			}

			float gap = best - next_best;
			if (next_best_gap > gap) {
				if (next_best_index == 0) {
					next_best_agent = current_agent;
					next_best_agent_should_search = 0;
				}
				else {
					next_best_agent = current_agent->children[next_best_index-1].agent;
					next_best_agent_should_search = 1;
				}
				next_best_gap = gap;
			}
		}



		// I select ME!
		if (best_index == 0) {
			current_agent->own_games++;
			if (pass == 0) {
				*agent1 = current_agent;
				if (!next_best_agent_should_search) {
					*agent2 = next_best_agent;
					next_best_agent->own_games++;
					return;
				}
				else {
					current_agent = next_best_agent;
					for (i = 0; i < current_agent->parent->children_count; i++) {
						if (current_agent == current_agent->parent->children[i].agent) {
							current_agent->parent->children[i].games_played++;
							break;
						}
					}
					pass = 1;
				}
			}
			else {
				*agent2 = current_agent;
				return;
			}
		}
		else {
			current_agent->children[best_index-1].games_played++;
			current_agent = current_agent->children[best_index-1].agent;
		}
	}
	return;
}


void academy_get_probability_field(struct Academy_Agent_T * agent, float * probabilities) {
	// Score all candidates (first is parent)

	probabilities[0] = agent->own_value + UCB_C*sqrt(ceil_log2(agent->times_queried) / (float)agent->own_games);
	int i;
	float sum = probabilities[0];
	for (i = 0; i < agent->children_count; i++) {
		if (agent->children[i].state == ACADEMY_AGENT_STATE_ALIVE) {
			probabilities[i+1] = agent->children[i].value + UCB_C*sqrt(ceil_log2(agent->times_queried) / (float)agent->children[i].games_played);
			sum += probabilities[i+1];
		}
		else {
			probabilities[i+1] = 0.0;
		}
	}
	// normalize
	for (i = 0; i < agent->children_count+1; i++) {
		probabilities[i] = probabilities[i]/sum;
	}
}

void academy_update_expected_values(struct Academy_Agent_T * agent) {
	float probabilities[agent->children_count + 1];
	academy_get_probability_field(agent, probabilities);
}


void academy_select_matchup_probablistic(struct Academy_T * academy, struct Academy_Agent_T ** agent1, struct Academy_Agent_T ** agent2) {
	struct Academy_Agent_T * current_agent = academy->root_agent;

	int pass = 0;
	int i;
	while(current_agent) {
		current_agent->times_queried++;

		// Randomly check for prunable nodes to keep memory usage sane
		if (fast_rand()%30 == 0)
			tree_search_test_prune_from_node(current_agent);

		float scores[current_agent->children_count + 1];
		academy_get_probability_field(current_agent, scores);

		// Select agent
		float selector = ((float)fast_rand())/FAST_RAND_MAX;
		int chosen_index;
		for (chosen_index = 0; chosen_index < current_agent->children_count; chosen_index++) {
			selector -= scores[chosen_index];
			if (selector <= 0)
				break;
		}

		// I select ME!
		if (chosen_index == 0) {
			current_agent->own_games++;
			if (pass == 0) {
				*agent1 = current_agent;
				pass = 1;
				current_agent = academy->root_agent;
			}
			else {
				*agent2 = current_agent;
				return;
			}
		}
		else {
			current_agent->children[chosen_index-1].games_played++;
			current_agent = current_agent->children[chosen_index-1].agent;
		}
	}
	return;
}



void academy_select_matchup(struct Academy_T * academy, struct Academy_Agent_T ** agent1, struct Academy_Agent_T ** agent2) {
	academy_select_matchup_probablistic(academy, agent1, agent2);
}


void academy_expand_hashtable(struct Academy_T * academy) {
	int old_hashtable_len = academy->hashtable_len;
	struct Academy_Hashtable_Row_T * old_hashtable = academy->hashtable;

	// Allocate new table
	academy->hashtable_len *= 2;
	academy->hashtable = malloc(sizeof(struct Academy_Hashtable_Row_T) * academy->hashtable_len);

	// Clear new table
	for (int i = 0; i < academy->hashtable_len; i++) {
		academy->hashtable[i].hash = 0;
		academy->hashtable[i].agent = NULL;
	}

	// Move in all old data
	for (int i = 0; i < old_hashtable_len; i++) {
		if (old_hashtable[i].agent) {
			struct Academy_Hashtable_Row_T * row = academy_hashtable_lookup(academy, old_hashtable[i].hash);
			if (!row) {
				printf("Something went really, really wrong. Consider religion.");
				exit(1);
			}
			row->agent = old_hashtable[i].agent;
			row->hash = old_hashtable[i].hash;
		}
	}

	free(old_hashtable);
}

struct Academy_Agent_T * academy_add_new_agent(struct Academy_T * academy, struct Academy_Agent_T * parent, char * c_code, size_t c_code_len) {

	// Hash and check for duplicate agents first
	unsigned long agent_hash = hash(c_code);
	struct Academy_Hashtable_Row_T * row = academy_hashtable_lookup(academy, agent_hash);
	if (!row) {
		printf("The academy's hashtable is completely full!!!\n");
		exit(1);
	}
	if (row->agent) {
		// Busted! The given code is identical to another agent!
		parent->aborted_children_count++;
		academy->duplicates_rejected++;
		free(c_code);
		return NULL;
	}

	// Check if we should expand the academy hashtable
	if ((float)academy->agent_count/(float)academy->hashtable_len > 0.75) {
		academy_expand_hashtable(academy);
		// Need to find row again!
		row = academy_hashtable_lookup(academy, agent_hash);
	}

	struct Academy_Agent_T * agent = malloc(sizeof(struct Academy_Agent_T));

	row->agent = agent;
	row->hash = agent_hash;

	if (parent) {
		// Add child data to parent

		// Expand parent array if neccessary
		if (parent->children_slots_allocated == parent->children_count) {
			parent->children_slots_allocated *= 2;
			parent->children = realloc(parent->children, sizeof(struct Academy_Agent_Child_T)*parent->children_slots_allocated);
		}
		int index = parent->children_count++;

		parent->children[index].agent = agent;
		parent->children[index].local_games_played = 2;
		parent->children[index].games_played = 2;
		parent->children[index].local_points = 1;
		parent->children[index].value = (float)parent->children[index].local_points/(float)parent->children[index].local_games_played;
		parent->children[index].state = ACADEMY_AGENT_STATE_ALIVE;
		agent->generation = parent->generation + 1;
	}
	else {
		agent->generation = 0;
		if (academy->root_agent) {
			printf("An impostor is trying to assassinate the root node, oh no!\n");
			exit(1);
		}
		academy->root_agent = agent;
	}

	agent->academy = academy;
	agent->parent = parent;
	agent->times_queried = 1; // Some lies here to keep the math finite

	agent->own_games = 2;
	agent->own_local_games = 2;
	agent->own_local_points = 1;
	agent->own_value = (float)agent->own_local_points/(float)agent->own_local_games;

	agent->children_count = 0;
	agent->aborted_children_count = 0;
	agent->children_slots_allocated = 10;
	agent->children = malloc(sizeof(struct Academy_Agent_Child_T) * agent->children_slots_allocated);
	agent->c_code = c_code;
	agent->c_code_len = c_code_len;
	agent->c_code_hash = agent_hash;

	academy->agent_count++;
	academy->loaded_agent_count++;
	return agent;
}

void academy_report_agent_win(struct Academy_Agent_T * winner, float winner_points, struct Academy_Agent_T * looser, float looser_points) {
	struct Academy_Agent_T * winner_child = winner;
	struct Academy_Agent_T * looser_child = looser;

	int i;

	while (winner != looser) {
		struct Academy_Agent_T * next_winner = winner;
		struct Academy_Agent_T * next_looser = looser;
		if (winner->generation >= looser->generation) {
			next_winner = winner->parent;
			winner_child = winner;
		}
		if (winner->generation <= looser->generation) {
			next_looser = looser->parent;
			looser_child = looser;
		}
		winner = next_winner;
		looser = next_looser;
	}
	// Now winner == looser

	// use winner_child and looser_child to figure out where scores go

	// Winner scores
	if (winner == winner_child) {
		winner->own_local_games++;
		winner->own_local_points += winner_points;
		winner->own_value = (float)winner->own_local_points/(float)winner->own_local_games;
	}
	else {
		// Identify child
		for (i = 0; i < winner->children_count; i++){
			if (winner->children[i].agent == winner_child)
				break;
		}
		winner->children[i].local_games_played++;
		winner->children[i].local_points += winner_points;
		winner->children[i].value = (float)winner->children[i].local_points/(float)winner->children[i].local_games_played;
	}

	// Looser scores
	if (looser == looser_child) {
		looser->own_local_games++;
		looser->own_local_points += looser_points;
		looser->own_value = (float)looser->own_local_points/(float)looser->own_local_games;
	}
	else {
		// Identify child
		for (i = 0; i < looser->children_count; i++){
			if (looser->children[i].agent == looser_child)
				break;
		}
		looser->children[i].local_games_played++;
		looser->children[i].local_points += looser_points;
		looser->children[i].value = (float)looser->children[i].local_points/(float)looser->children[i].local_games_played;
	}
}
