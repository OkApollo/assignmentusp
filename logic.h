/**
 * logic.h - Utility functions and game logic header
 * COMP2002 Unix Systems Programming
 */

#ifndef LOGIC_H
#define LOGIC_H

#include "game.h"

/* Direction constants */
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3
#define DIR_UP_LEFT 4
#define DIR_UP_RIGHT 5
#define DIR_DOWN_LEFT 6
#define DIR_DOWN_RIGHT 7

/* Utility function prototypes */
int my_strlen(const char *str);
int my_atoi(const char *str);
void my_itoa(int num, char *str);
void my_strcpy(char *dest, const char *src);
int my_abs(int x);
int read_line(int fd, char *buffer, int max_len);
int parse_integers(const char *line, int *numbers, int max_count);

/* Movement function prototypes */
int is_valid_position(GameState *state, int row, int col);
int is_valid_position_for_enemy(GameState *state, int row, int col);
int move_player(GameState *state, int direction);
int move_enemy(GameState *state, int row, int col, int new_row, int new_col, int is_snake);
int get_direction_towards_player(GameState *state, int enemy_row, int enemy_col, int is_snake);
int is_player_close(GameState *state, int enemy_row, int enemy_col, int is_snake);
void get_direction_offset(int direction, int *dr, int *dc);
int enemy_can_move(GameState *state, int row, int col, int direction);
int get_attack_direction(GameState *state, int enemy_row, int enemy_col, int is_snake);

#endif /* LOGIC_H */
