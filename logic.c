/**
 * logic.c - Utility functions and game logic implementation
 * COMP2002 Unix Systems Programming
 */

#include "logic.h"
#include <stdlib.h>
#include <limits.h>

/* ============================================================
 * UTILITY FUNCTIONS (custom implementations of banned functions)
 * ============================================================ */

/**
 * my_strlen - Custom implementation of strlen
 */
int my_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * my_atoi - Custom implementation of atoi
 */
int my_atoi(const char *str) {
    unsigned int result = 0;
    unsigned int limit;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }

    limit = (sign < 0) ? (unsigned int)INT_MAX + 1U : (unsigned int)INT_MAX;

    while (str[i] >= '0' && str[i] <= '9') {
        unsigned int digit = (unsigned int)(str[i] - '0');
        if (result > (limit - digit) / 10U) {
            result = limit;
        } else {
            result = result * 10U + digit;
        }
        i++;
    }

    if (sign < 0) {
        if (result == (unsigned int)INT_MAX + 1U) {
            return INT_MIN;
        }
        return -(int)result;
    }
    return (int)result;
}

/**
 * my_itoa - Custom integer to string conversion
 */
void my_itoa(int num, char *str) {
    int i = 0;
    int is_negative = 0;
    int start, end;
    unsigned int magnitude;
    char temp;

    if (num < 0) {
        is_negative = 1;
        magnitude = (unsigned int)(-(num + 1)) + 1U;
    } else {
        magnitude = (unsigned int)num;
    }

    if (magnitude == 0U) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    while (magnitude > 0U) {
        str[i++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    start = 0;
    end = i - 1;
    while (start < end) {
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/**
 * my_strcpy - Custom string copy
 */
void my_strcpy(char *dest, const char *src) {
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
}

/**
 * my_abs - Custom absolute value function
 */
int my_abs(int x) {
    return (x < 0) ? -x : x;
}

/**
 * read_line - Reads a line from a file descriptor using read(), one
 * byte at a time.
 *
 * Returns the number of characters placed into buffer (0 for a
 * blank line -- the newline was still consumed, so the file
 * position advanced normally). Returns -1 only when the file was
 * already at end-of-file before anything at all could be read, so
 * callers can tell "empty line" apart from "no more data" and stop
 * safely instead of looping forever on a truncated file.
 */
int read_line(int fd, char *buffer, int max_len) {
    int i = 0;
    char ch;
    ssize_t bytes_read;
    int consumed_any = 0;

    if (max_len <= 0) {
        return -1;
    }

    while (1) {
        bytes_read = read(fd, &ch, 1);
        if (bytes_read <= 0) {
            break;
        }
        consumed_any = 1;
        if (ch == '\n') {
            break;
        }
        if (i < max_len - 1) {
            buffer[i] = ch;
            i++;
        }
    }
    buffer[i] = '\0';

    if (!consumed_any) {
        return -1;
    }
    return i;
}

/**
 * parse_integers - Parses integers from a space-separated line
 */
int parse_integers(const char *line, int *numbers, int max_count) {
    int count = 0;
    int i = 0;
    int in_num = 0;
    int value = 0;

    while (line[i] != '\0' && count < max_count) {
        if (line[i] >= '0' && line[i] <= '9') {
            int digit = line[i] - '0';
            if (value > (INT_MAX - digit) / 10) {
                value = INT_MAX;
            } else {
                value = value * 10 + digit;
            }
            in_num = 1;
        } else if (in_num) {
            numbers[count] = value;
            count++;
            in_num = 0;
            value = 0;
        }
        i++;
    }

    if (in_num && count < max_count) {
        numbers[count] = value;
        count++;
    }

    return count;
}

/* ============================================================
 * MOVEMENT FUNCTIONS
 * ============================================================ */

/**
 * is_valid_position - Checks if a position is within bounds and not a wall
 */
int is_valid_position(GameState *state, int row, int col) {
    if (row < 0 || row >= (*state).rows || col < 0 || col >= (*state).cols) {
        return 0;
    }
    if ((*state).grid[row][col] == WALL) {
        return 0;
    }
    return 1;
}

/**
 * is_valid_position_for_enemy - Checks if position is valid for enemy movement
 */
int is_valid_position_for_enemy(GameState *state, int row, int col) {
    if (row < 0 || row >= (*state).rows || col < 0 || col >= (*state).cols) {
        return 0;
    }
    /* Enemies cannot occupy the goal or each other's square.  Allowing an
     * enemy to overwrite another enemy made its tracked position disagree
     * with the grid, which could subsequently corrupt later moves. */
    if ((*state).grid[row][col] == WALL || (*state).grid[row][col] == GOAL ||
        (*state).grid[row][col] == SNAKE || (*state).grid[row][col] == WOLF) {
        return 0;
    }
    return 1;
}

/**
 * move_player - Moves the player in a direction. Sets GAME_LOSE if
 * moving into an enemy, GAME_WIN if moving onto the Goal.
 */
int move_player(GameState *state, int direction) {
    int new_row = (*state).player.row;
    int new_col = (*state).player.col;
    int old_row, old_col;

    switch (direction) {
        case DIR_UP:    new_row--; break;
        case DIR_DOWN:  new_row++; break;
        case DIR_LEFT:  new_col--; break;
        case DIR_RIGHT: new_col++; break;
        default: return 0;
    }

    if (!is_valid_position(state, new_row, new_col)) {
        return 0;
    }

    if ((*state).grid[new_row][new_col] == SNAKE ||
        (*state).grid[new_row][new_col] == WOLF) {
        (*state).game_state = GAME_LOSE;
        return 1;
    }

    old_row = (*state).player.row;
    old_col = (*state).player.col;
    (*state).grid[old_row][old_col] = EMPTY;
    (*state).player.row = new_row;
    (*state).player.col = new_col;
    (*state).grid[new_row][new_col] = PLAYER;

    if (new_row == (*state).goal.row && new_col == (*state).goal.col) {
        (*state).game_state = GAME_WIN;
    }

    return 1;
}

/**
 * move_enemy - Moves an enemy from (row, col) to (new_row, new_col).
 * Sets GAME_LOSE if the destination is the Player, then performs the
 * move either way. Returns 1 if applied, 0 if the destination was
 * invalid (wall/border/goal).
 */
int move_enemy(GameState *state, int row, int col, int new_row, int new_col, int is_snake) {
    if (!is_valid_position_for_enemy(state, new_row, new_col)) {
        return 0;
    }

    if ((*state).grid[new_row][new_col] == PLAYER) {
        (*state).game_state = GAME_LOSE;
    }

    (*state).grid[row][col] = EMPTY;
    (*state).grid[new_row][new_col] = is_snake ? SNAKE : WOLF;

    if (is_snake) {
        (*state).snake.row = new_row;
        (*state).snake.col = new_col;
    } else {
        (*state).wolf.row = new_row;
        (*state).wolf.col = new_col;
    }

    return 1;
}

/**
 * get_direction_towards_player - Returns direction for enemy to attack player
 */
int get_direction_towards_player(GameState *state, int enemy_row, int enemy_col, int is_snake) {
    int dr = (*state).player.row - enemy_row;
    int dc = (*state).player.col - enemy_col;

    if (is_snake) {
        if (dr < 0 && dc == 0) return DIR_UP;
        if (dr > 0 && dc == 0) return DIR_DOWN;
        if (dr == 0 && dc < 0) return DIR_LEFT;
        if (dr == 0 && dc > 0) return DIR_RIGHT;
        if (dr < 0 && dc < 0) return DIR_UP_LEFT;
        if (dr < 0 && dc > 0) return DIR_UP_RIGHT;
        if (dr > 0 && dc < 0) return DIR_DOWN_LEFT;
        if (dr > 0 && dc > 0) return DIR_DOWN_RIGHT;
    } else {
        if (dr == 0 && dc > 0) return DIR_RIGHT;
        if (dr == 0 && dc < 0) return DIR_LEFT;
        if (dc == 0 && dr > 0) return DIR_DOWN;
        if (dc == 0 && dr < 0) return DIR_UP;
    }
    return -1;
}

/**
 * is_player_close - Checks if player is within 1 legal move of the
 * enemy. Snake checks all 8 neighbours (Chebyshev distance); Wolf
 * only checks the 4 orthogonal neighbours (Manhattan distance),
 * since it cannot reach a diagonal cell in a single move.
 */
int is_player_close(GameState *state, int enemy_row, int enemy_col, int is_snake) {
    int dr = my_abs((*state).player.row - enemy_row);
    int dc = my_abs((*state).player.col - enemy_col);

    if (is_snake) {
        return (dr <= 1 && dc <= 1) ? 1 : 0;
    }

    return ((dr == 1 && dc == 0) || (dr == 0 && dc == 1)) ? 1 : 0;
}

/**
 * get_direction_offset - Returns row and column offset for a direction
 */
void get_direction_offset(int direction, int *dr, int *dc) {
    switch (direction) {
        case DIR_UP:         *dr = -1; *dc =  0; break;
        case DIR_DOWN:        *dr =  1; *dc =  0; break;
        case DIR_LEFT:        *dr =  0; *dc = -1; break;
        case DIR_RIGHT:       *dr =  0; *dc =  1; break;
        case DIR_UP_LEFT:     *dr = -1; *dc = -1; break;
        case DIR_UP_RIGHT:    *dr = -1; *dc =  1; break;
        case DIR_DOWN_LEFT:   *dr =  1; *dc = -1; break;
        case DIR_DOWN_RIGHT:  *dr =  1; *dc =  1; break;
        default:              *dr =  0; *dc =  0; break;
    }
}

/**
 * enemy_can_move - Checks if enemy can move in a direction
 */
int enemy_can_move(GameState *state, int row, int col, int direction) {
    int dr, dc;
    get_direction_offset(direction, &dr, &dc);
    return is_valid_position_for_enemy(state, row + dr, col + dc);
}

/**
 * get_attack_direction - Gets direction for enemy to attack player,
 * or -1 if that direct step is blocked, meaning the enemy should
 * fall back to a random move instead.
 */
int get_attack_direction(GameState *state, int enemy_row, int enemy_col, int is_snake) {
    int dr, dc;
    int direction = get_direction_towards_player(state, enemy_row, enemy_col, is_snake);

    if (direction < 0) {
        return -1;
    }

    get_direction_offset(direction, &dr, &dc);
    if (is_valid_position_for_enemy(state, enemy_row + dr, enemy_col + dc)) {
        return direction;
    }

    return -1;
}
