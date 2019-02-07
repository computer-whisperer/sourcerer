#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pentae.h"

enum Player_T other_player(enum Player_T player) {
  if (player == PLAYER_1)
    return PLAYER_2;
  return PLAYER_1;
}

int make_move(struct Game_T * game, enum Player_T player, int x, int y) {
  if (x < 0 || x >= BOARD_WIDTH)
    return MOVE_RESULT_INVALID;
  if (y < 0 || y >= BOARD_HEIGHT)
    return MOVE_RESULT_INVALID;
  
  if (game->board[x][y] != PLAYER_NONE)
    return MOVE_RESULT_INVALID;
  game->board[x][y] = player;
  
  int totals[4] = {1, 1, 1, 1};
  
  // Scan in every direction for wins or captures
  for (int dir = 0; dir < 8; dir++) {
    int is_line = 1;
    int is_capture = 1;
    for (int len = 1; len < 5; len++) {
      
      int x_offset = 0;
      int y_offset = 0;
      switch(dir) {
        case 0:
          y_offset = len;
          break;
        case 1:
          y_offset = len;
          x_offset = len;
          break;
        case 2:
          x_offset = len;
          break;
        case 3:
          y_offset = -len;
          x_offset = len;
          break;
        case 4:
          y_offset = -len;
          break;
        case 5:
          y_offset = -len;
          x_offset = -len;
          break;
        case 6:
          x_offset = -len;
          break;
        case 7:
          x_offset = -len;
          y_offset = len;
          break;
      }
      int new_x = x + x_offset;
      int new_y = y + y_offset;
      if (new_x < 0 || new_x >= BOARD_WIDTH)
        break;
      if (new_y < 0 || new_y >= BOARD_HEIGHT)
        break;
        
      
      if (game->board[new_x][new_y] != player)
        is_line = 0;
      if (is_line)
        totals[dir%4]++;
      if (len < 3 && (game->board[new_x][new_y] == player || game->board[new_x][new_y] == PLAYER_NONE))
        is_capture = 0;
      if (len == 3) {
        if (is_capture && game->board[new_x][new_y] == player) {
          game->board[x + (x_offset / 3)][y + (y_offset / 3)] = PLAYER_NONE;
          game->board[x + (x_offset / 3)*2][y + (y_offset / 3)*2] = PLAYER_NONE;
          if (player == PLAYER_1)
            game->player_1_pairs_captured++;
          if (player == PLAYER_2)
            game->player_2_pairs_captured++;  
        }
        else
          is_capture = 0;
      }
      if (!is_line && !is_capture)
        break;
    }
  }
  
  int pentae = 0;
  for (int i = 0; i < 4; i++)
    pentae |= totals[i]>=5;
  
  // Scan for victories
  if (pentae || game->player_1_pairs_captured >= 5 || game->player_2_pairs_captured >= 5)
    return MOVE_RESULT_WIN;

  return MOVE_RESULT_NORMAL;
}

int bruteforce_move_value(struct Game_T * game, enum Player_T player, int x, int y, int levels) {
  // Clone game and make move
  struct Game_T * new_game = malloc(sizeof(struct Game_T));
  memcpy(new_game, game, sizeof(struct Game_T));
  enum MoveResult_T result = make_move(new_game, player, x, y);
  if (result == MOVE_RESULT_INVALID){
    free(new_game);
    return -1;
  }
  if (result == MOVE_RESULT_WIN) {
    free(new_game);
    return 1;
  }
  if (levels == 0) {
    free(new_game);
    return 0;
  }
  
  int value = -1;
  for (x = 0; x < BOARD_WIDTH; x++) {
    for (y = 0; y < BOARD_HEIGHT; y++) {
      int new_value = bruteforce_move_value(new_game, other_player(player), x, y, levels-1);
      if (new_value > value)
        value = new_value;
      if (value == 1)
        break;
    }
    if (value == 1)
      break;
  }
  free(new_game);

  return -value;
}

void print_board(struct Game_T * game) {
  
  putchar(' ');
  for (int x = 0; x < BOARD_WIDTH; x++) {
    for (int j = 0; j < CELL_WIDTH_MARGIN; j++) {
      putchar(' ');
    }
    putchar(x + 65);
      
    for (int j = 0; j < CELL_WIDTH_MARGIN; j++) {
      putchar(' ');
    }
    putchar(' ');
  }
  putchar('\n');
    
  for (int x = 0; x < BOARD_WIDTH*(CELL_WIDTH_MARGIN*2 + 2) + 1; x++)
    putchar('-');
  putchar('\n');
  for (int y = 0; y < BOARD_HEIGHT; y
  ++) {
    for (int i = 0; i < CELL_HEIGHT_MARGIN; i++) {
      putchar('|');
      for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int j = 0; j < CELL_WIDTH_MARGIN*2 + 1; j++) {
          putchar(' ');
        }
        putchar('|');
      }
      putchar('\n');
    }
    
    putchar('|');
    for (int x = 0; x < BOARD_WIDTH; x++) {
      for (int j = 0; j < CELL_WIDTH_MARGIN; j++) {
        putchar(' ');
      }
      int r;
      switch (game->board[x][y]) {
        case PLAYER_NONE:
          putchar(' ');
          break;
        case PLAYER_1:
          putchar('X');
          break;
        case PLAYER_2:
          putchar('O');
          break;
      }
      for (int j = 0; j < CELL_WIDTH_MARGIN; j++) {
        putchar(' ');
      }
      putchar('|');
    }
    printf(" %i\n", y);
    
    for (int i = 0; i < CELL_HEIGHT_MARGIN; i++) {
      putchar('|');
      for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int j = 0; j < CELL_WIDTH_MARGIN*2 + 1; j++) {
          putchar(' ');
        }
        putchar('|');
      }
      putchar('\n');
    }
    
    for (int x = 0; x < BOARD_WIDTH*(CELL_WIDTH_MARGIN*2 + 2) + 1; x++) {
      putchar('-');
    }
    putchar('\n');
  }
  
}

void new_game(struct Game_T * game) {
  for (int x = 0; x < BOARD_WIDTH; x++) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
      game->board[x][y] = PLAYER_NONE;
    }
  }
  game->player_1_pairs_captured = 0;
  game->player_2_pairs_captured = 0;
}




