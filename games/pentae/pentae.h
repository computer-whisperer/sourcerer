#define BOARD_WIDTH 11

#define BOARD_HEIGHT 11

#define CELL_WIDTH_MARGIN 1

#define CELL_HEIGHT_MARGIN 0

enum Player_T {
  PLAYER_NONE,
  PLAYER_1,
  PLAYER_2,
};

struct Game_T {
  enum Player_T board[BOARD_WIDTH][BOARD_HEIGHT];
  int player_1_pairs_captured;
  int player_2_pairs_captured;
};


enum MoveResult_T {
  MOVE_RESULT_INVALID,
  MOVE_RESULT_NORMAL,
  MOVE_RESULT_WIN
};

void new_game(struct Game_T * game);
void print_board(struct Game_T * game);
int make_move(struct Game_T * game, enum Player_T player, int x, int y);
