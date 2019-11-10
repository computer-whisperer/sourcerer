#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#include "academy.h"
#include "pentae.h"
#include "executor.h"
#include "c_types.h"
#include "c_parser.h"
#include "changes.h"
#include "change_proposers.h"

struct SafeArea_T{
	struct Game_T game;
	int move_out_x;
	int move_out_y;
};

enum MoveResult_T make_dumb_move(struct Game_T * game, enum Player_T player) {
	int x = 0;
	int y = 0;
	while (game->board[x][y] != PLAYER_NONE) {
		x++;
		if (x == BOARD_WIDTH) {
			y++;
			x = 0;
		}
	}
	return make_move(game, player, x, y);
}


enum MoveResult_T make_agent_move(struct Function_T * function, struct Game_T * game, enum Player_T player) {
	enum MoveResult_T r;
	int x = -1;
	int y = -1;
	int * x_ptr = &x;
	int * y_ptr = &y;
	int board_width = BOARD_WIDTH;
	void * function_args_data[4] = {&(game), &(x_ptr), &(y_ptr), &(board_width)};
	size_t function_args_size[4] = {sizeof(int*), sizeof(int*), sizeof(int*), sizeof(int)};
	struct MemoryDMZ_T g_dmz, x_dmz, y_dmz;
	g_dmz.data_start = game;
	g_dmz.data_end = (game + 1);
	g_dmz.next = &x_dmz;
	g_dmz.writable = 0;
	x_dmz.data_start = &x;
	x_dmz.data_end = (&x + 1);
	x_dmz.next = &y_dmz;
	x_dmz.writable = 1;
	y_dmz.data_start = &y;
	y_dmz.data_end = (&y + 1);
	y_dmz.next = NULL;
	y_dmz.writable = 1;
	// Execute execute execute!!!
	function->executor_report.times_not_returned = 0;
	function->executor_report.segfaults_attempted = 0;
	function->executor_report.lines_executed = 0;
	execute_function(function, &g_dmz, function_args_data, function_args_size, NULL, 0, 100);
	if (function->executor_report.times_not_returned ||  function->executor_report.segfaults_attempted) {
		return MOVE_RESULT_INVALID;
	//	r = make_dumb_move(game, player);
	//	return r;
	}
	r = make_move(game, player, x, y);
	//if (r == MOVE_RESULT_INVALID)
	//	r = make_dumb_move(game, player);
	return r;
}


void run_agent_human_game() {

	struct SafeArea_T safearea;
	struct Game_T * game = &safearea.game;
	char * board_ptr = (char *) &(game->board);
	int * x_ptr = &safearea.move_out_x;
	int * y_ptr = &safearea.move_out_y;
	int board_size = BOARD_WIDTH;
	void * function_args_data[4] = {&(board_ptr), &(x_ptr), &(y_ptr), &(board_size)};
	size_t function_args_size[4] = {sizeof(int*), sizeof(int*), sizeof(int*), sizeof(int)};

	struct MemoryDMZ_T dmz;
	dmz.data_start = &safearea;
	dmz.data_end = &safearea + sizeof(struct SafeArea_T);
	dmz.next = NULL;

	new_game(game);

	struct Environment_T * env = build_new_environment(10000);
	load_from_file(env, "games/pentae/testagent2.c");
	enum MoveResult_T r;
	while (1) {
		int next_x;
		int next_y;
		// Player turn
		print_board(game);
		printf("\nYour move:\nX:");
		scanf("%d", &next_x);
		printf("Y:");
		scanf("%d", &next_y);
		printf("\n");
		r = make_move(game, PLAYER_2, next_x, next_y);
		if (r == MOVE_RESULT_INVALID) {
			printf("That was a bad move, using dummy move instead!\n");
			make_dumb_move(game, PLAYER_2);
		}
		if (r == MOVE_RESULT_WIN) {
			printf("You win!\n");
			break;
		}
		// Agent turn


		if (make_agent_move(env->last_function, game, PLAYER_1) == MOVE_RESULT_WIN) {
			printf("Agent won!\n");
			return;
		}
		/*

		execute_function(env->last_function, &dmz, function_args_data, function_args_size, NULL, 0, 1000);
		if (env->last_function->executor_report.times_not_returned) {
			printf("Agent did not return.\n");
			break;
		}
		if (env->last_function->executor_report.segfaults_attempted) {
			printf("Agent segfaulted.\n");
			break;
		}
		r = make_move(game, PLAYER_1, *x_ptr, *y_ptr);
		if (r == MOVE_RESULT_INVALID) {
			printf("Agent made a bad move.\n");
			make_dumb_move(game, PLAYER_1);
		}
		if (r == MOVE_RESULT_WIN) {
			printf("The agent wins!\n");
			break;
		}
		*/
	}

	print_board(game);
}

enum Player_T run_agent_agent_game(struct Function_T * agent1, struct Function_T * agent2) {
	struct Game_T game;
	new_game(&game);
	enum Player_T winner;
	int rounds = 0;
	enum MoveResult_T r;
	while (1) {
		r = make_agent_move(agent1, &game, PLAYER_1);
		if (r == MOVE_RESULT_WIN)
			return PLAYER_1;
		if (r == MOVE_RESULT_INVALID)
			return PLAYER_2;

		r = make_agent_move(agent2, &game, PLAYER_2);
		if (r == MOVE_RESULT_WIN)
			return PLAYER_2;
		if (r == MOVE_RESULT_INVALID)
			return PLAYER_1;

		rounds++;
		if (rounds > 20) {
			return PLAYER_1;
		}
	}
	return winner;
}

#define MIN_AGENT_COUNT 2
void run_agent_agent_games() {

	struct Environment_T * env1 = build_new_environment(10000);
	struct Environment_T * env2 = build_new_environment(10000);

	struct Academy_T * academy = build_new_academy();
	struct Academy_Agent_T * tree_root = academy_add_agent_from_file(academy, NULL, "games/pentae/emptyagent.c");
	// Populate tree up to minimum starting count
	while (academy->loaded_agent_count < MIN_AGENT_COUNT) {
		load_from_data(env1, tree_root->c_code);
		free_change(apply_change(propose_random_change(env1->last_function)));
		academy_add_agent_from_function(academy, tree_root, env1->last_function);
		if (env1->last_function->name[0] == 's')
			free_function(env1->last_function);
	}

	int iter_1 = 1;

	int games_played = 0;

	while (1) {
		// Load players
		struct Academy_Agent_T * node1;
		struct Academy_Agent_T * node2;
		// Alternate player numbers
		while (node1 == node2) {
			if (games_played %2)
				academy_select_matchup(academy, &node1, &node2);
			else
				academy_select_matchup(academy, &node2, &node1);
		}

		iter_1 = 0;
		load_from_data(env1, node1->c_code);
		load_from_data(env2, node2->c_code);
		if (assert_environment_integrity(env1))
			exit(1);
		if (assert_environment_integrity(env2))
		    exit(1);

		// Play a game
		enum Player_T winner = run_agent_agent_game(env1->last_function, env2->last_function);
		if (winner == PLAYER_1)
			academy_report_agent_win(node1, 1-(env1->last_function->codeline_count/200), node2, 0-(env2->last_function->codeline_count/100));
		else
			academy_report_agent_win(node2, 1-(env2->last_function->codeline_count/200), node1, 0-(env1->last_function->codeline_count/100));

		games_played++;

		if ((fast_rand()%8)==0) {
			// Apply and store random changes
			free_change(apply_change(propose_random_change(env1->last_function)));
			free_change(apply_change(propose_random_change(env2->last_function)));
			node1 = academy_add_agent_from_function(academy, node1, env1->last_function);
			node2 = academy_add_agent_from_function(academy, node2, env2->last_function);
			/*
			unsigned long h1, h2;
			h1 = 0;
			h2 = 0;
			if (node1)
				h1 = node1->c_code_hash;
			if (node2)
				h2 = node2->c_code_hash;
			printf("game: %i, hash1: %li, hash2: %li\n", games_played, h1, h2);
			*/
		}

		if (env1->last_function->name[0] == 's')
			free_function(env1->last_function);
		if (env2->last_function->name[0] == 's')// +
			free_function(env2->last_function);

		if (!(games_played % 100000)) {
			academy_select_matchup(academy, &node1, &node2);
			printf(node1->c_code);
			printf("Generation: %d \n", node1->generation);
			printf("Played %d games.\n", games_played);
			printf("Made %d agents.\n", academy->agent_count);
			printf("Remembering %d agents.\n", academy->loaded_agent_count);
			printf("Rejected %d duplicate agents.\n", academy->duplicates_rejected);
			printf("Root value: %f \n", tree_root->own_value);
			//if (games_played >= 100000000)
			//	break;
		}
	}
	academy_prune_node(tree_root);

}

int main(int argc, char * argv[]) {
	//run_agent_agent_games();

	if (argc < 2) {
		run_agent_agent_games();
		return 0;
	}
	else if (!strcmp(argv[1], "human")) {
		run_agent_human_game();
		return 0;
	}
	else if (!strcmp(argv[1], "self")) {
		run_agent_agent_games();
		return 0;
	}

}
