#include <stdio.h>
#include <stdlib.h>

#include "pentae.h"
#include "executor.h"
#include "c_types.h"
#include "c_parser.h"
#include "tree_search.h"
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
	load_from_file(env, "games/pentae/testagent.c");
	enum MoveResult_T r;
	while (1) {
		int next_x;
		int next_y;
		// Player turn
		//print_board(game);
		//printf("\nYour move:\nX:");
		//scanf("%d", &next_x);
		//printf("Y:");
		//scanf("%d", &next_y);
		//printf("\n");
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

	}

	print_board(game);
}


enum MoveResult_T make_agent_move(struct Function_T * function, struct Game_T * game, enum Player_T player) {
	enum MoveResult_T r;
	int x = 0;
	int y = 0;
	int * x_ptr = &x;
	int * y_ptr = &y;
	int board_width = BOARD_WIDTH;
	void * function_args_data[4] = {&(game), &(x_ptr), &(y_ptr), &(board_width)};
	size_t function_args_size[4] = {sizeof(int*), sizeof(int*), sizeof(int*), sizeof(int)};
	struct MemoryDMZ_T g_dmz, x_dmz, y_dmz;
	g_dmz.data_start = game;
	g_dmz.data_end = game + sizeof(struct SafeArea_T);
	g_dmz.next = &x_dmz;
	x_dmz.data_start = &x;
	x_dmz.data_end = (&x + 1);
	x_dmz.next = &y_dmz;
	y_dmz.data_start = &y;
	y_dmz.data_end = (&y + 1);
	y_dmz.next = NULL;
	// Execute execute execute!!!
	function->executor_report.times_not_returned = 0;
	function->executor_report.segfaults_attempted = 0;
	execute_function(function, &g_dmz, function_args_data, function_args_size, NULL, 0, 1000);
	if (function->executor_report.times_not_returned ||  function->executor_report.segfaults_attempted) {
		r = make_dumb_move(game, player);
		return r;
	}
	r = make_move(game, player, x, y);
	if (r == MOVE_RESULT_INVALID)
		r = make_dumb_move(game, player);
	return r;
}


enum Player_T run_agent_agent_game(struct Function_T * agent1, struct Function_T * agent2) {
	struct Game_T game;
	new_game(&game);
	enum Player_T winner;
	while (1) {
		if (make_agent_move(agent1, &game, PLAYER_1) == MOVE_RESULT_WIN)
			return PLAYER_1;

		if (make_agent_move(agent2, &game, PLAYER_2) == MOVE_RESULT_WIN)
			return PLAYER_2;
	}
	return winner;
}

void run_agent_agent_games() {

	struct TreeSearchNode_T * tree_root = tree_search_add_agent_from_file(NULL, "games/pentae/emptyagent.c");
	struct Environment_T * env1 = build_new_environment(10000);
	struct Environment_T * env2 = build_new_environment(10000);
	int iter_1 = 1;

	int games_played = 0;

	while (1) {
		if (!iter_1) {
			free_function(env1->last_function);
			free_function(env2->last_function);
		}
		iter_1 = 0;

		// Load players
		struct TreeSearchNode_T * node1 = tree_search_pick_random_agent(tree_root);
		struct TreeSearchNode_T * node2 = tree_search_pick_random_agent(tree_root);
		load_from_data(env1, node1->c_code);
		load_from_data(env2, node2->c_code);
		if (assert_environment_integrity(env1))
			exit(1);
		if (assert_environment_integrity(env2))
		    exit(1);

		// Play a game
		enum Player_T winner = run_agent_agent_game(env1->last_function, env2->last_function);
		if (winner == PLAYER_1)
			tree_search_report_agent_win(node1, node2);
		else
			tree_search_report_agent_win(node2, node1);

		// Apply random changes
	    free_change(apply_change(propose_random_change(env1->last_function)));
	    free_change(apply_change(propose_random_change(env2->last_function)));
		node1 = tree_search_add_agent_from_function(node1, env1->last_function);
		node2 = tree_search_add_agent_from_function(node2, env2->last_function);

	    // Play another game
		winner = run_agent_agent_game(env1->last_function, env2->last_function);

		if (winner == PLAYER_1)
			tree_search_report_agent_win(node1, node2);
		else
			tree_search_report_agent_win(node2, node1);

		games_played += 2;

		if (games_played > 10000) {
			break;
		}
	}
	struct TreeSearchNode_T * best_node = tree_search_pick_random_agent(tree_root);
	printf(best_node->c_code);
	printf("Played %d games.\n", games_played);
	printf("Made %d agents.\n", global_agent_count);
}

int main(int argc, char * argv[]) {
	global_agent_count = 0;
	run_agent_agent_games();
	/*
	if (argc < 2)
		return 1;
	if (!strcmp(argv[1], "human")) {
		run_agent_human_game();
		return 0;
	}
	if (!strcmp(argv[1], "self")) {
		run_agent_agent_games();
		return 0;
	}
	*/
}
