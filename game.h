/**
 * game.h - Game state management header
 * COMP2002 Unix Systems Programming
 */

#ifndef GAME_H
#define GAME_H

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

/* Game object codes (from map file) */
#define EMPTY 0
#define WALL 1
#define PLAYER 2
#define GOAL 3
#define SNAKE 4
#define WOLF 5

/* Game state codes */
#define GAME_ONGOING 0
#define GAME_WIN 1
#define GAME_LOSE 2
#define GAME_QUIT 3

/* Maximum map size */
#define MAX_ROWS 20
#define MAX_COLS 20

/* State file name */
#define STATE_FILE "state.txt"

/* Position structure */
typedef struct {
    int row;
    int col;
} Position;

/* Game state structure */
typedef struct {
    int rows;
    int cols;
    int **grid;
    Position player;
    Position goal;
    Position snake;
    Position wolf;
    int game_state;
    int snake_moves;
    int wolf_moves;
    int snake_move_ms;
    int wolf_move_ms;
} GameState;

/* Function prototypes */
GameState *create_game_state(int rows, int cols);
void free_game_state(GameState *state);
GameState *load_map(const char *filename);
int write_state_fd(GameState *state, int fd);
int read_state_fd(GameState *state, int fd);

#endif /* GAME_H */
