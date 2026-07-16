/**
 * player.h - Terminal I/O, display, and process control header
 * COMP2002 Unix Systems Programming
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "game.h"
#include <termios.h>
#include <sys/types.h>

/* Function prototypes */
void set_terminal_mode(struct termios *orig_termios);
void restore_terminal_mode(struct termios *orig_termios);
char get_char(void);
void clear_screen(void);
void print_character(int obj_code);
void display_game(GameState *state);
void enemy_process(GameState *state, int is_snake, const char *state_filename);
void parent_process(GameState *state, const char *state_filename, pid_t snake_pid, pid_t wolf_pid);

#endif /* PLAYER_H */
