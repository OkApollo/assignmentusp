/**
 * game.c - Game state management implementation
 * COMP2002 Unix Systems Programming
 */

#define _POSIX_C_SOURCE 200809L

#include "game.h"
#include "logic.h"

/**
 * create_game_state - Allocates and initializes a game state
 */
GameState *create_game_state(int rows, int cols) {
    GameState *state;
    int i;

    state = (GameState *)malloc(sizeof(GameState));
    if (!state) {
        return NULL;
    }

    (*state).rows = rows;
    (*state).cols = cols;
    (*state).game_state = GAME_ONGOING;

    (*state).grid = (int **)malloc((size_t)rows * sizeof(int *));
    if (!(*state).grid) {
        free(state);
        return NULL;
    }

    for (i = 0; i < rows; i++) {
        (*state).grid[i] = (int *)malloc((size_t)cols * sizeof(int));
        if (!(*state).grid[i]) {
            while (i > 0) {
                i--;
                free((*state).grid[i]);
            }
            free((*state).grid);
            free(state);
            return NULL;
        }
    }

    return state;
}

/**
 * free_game_state - Frees all memory associated with a game state
 */
void free_game_state(GameState *state) {
    int i;

    if (!state) {
        return;
    }

    if ((*state).grid) {
        for (i = 0; i < (*state).rows; i++) {
            if ((*state).grid[i]) {
                free((*state).grid[i]);
            }
        }
        free((*state).grid);
    }

    free(state);
}

/**
 * load_map - Opens the map file and reads the game state, the
 * row/col dimensions, and the full grid using read(). Returns a
 * fully populated GameState, or NULL if the file cannot be opened.
 */
GameState *load_map(const char *filename) {
    int fd;
    char line[256];
    int numbers[MAX_ROWS * MAX_COLS + 4];
    int expected_count;
    int num_count;
    int i, j;
    GameState *state;
    int rows, cols;
    int game_state;
    int idx;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    /* Line 1: game state */
    read_line(fd, line, (int)sizeof(line));
    game_state = my_atoi(line);

    /* Line 2: rows and cols */
    read_line(fd, line, (int)sizeof(line));
    parse_integers(line, numbers, 2);
    rows = numbers[0];
    cols = numbers[1];

    /*
     * Make sure the map actually fits inside the program's fixed-size
     * buffers before we allocate anything. The assignment caps the
     * playable area at 20x20 (MAX_ROWS x MAX_COLS); anything larger
     * or non-positive is rejected here instead of silently
     * overflowing the numbers[] buffer later.
     */
    if (rows <= 0 || cols <= 0 || rows > MAX_ROWS || cols > MAX_COLS) {
        close(fd);
        return NULL;
    }

    state = create_game_state(rows, cols);
    if (!state) {
        close(fd);
        return NULL;
    }
    (*state).game_state = game_state;

    /*
     * Remaining lines: grid data. If the file has been edited/
     * truncated and runs out before every cell has been supplied,
     * read_line() returns -1 at true end-of-file (as opposed to 0
     * for a legitimate blank line), so we stop reading instead of
     * looping forever, then treat every missing cell as a Wall.
     * This keeps a partially-deleted map file playable and fully
     * enclosed rather than crashing or hanging.
     */
    expected_count = rows * cols;
    idx = 0;
    while (idx < expected_count) {
        int line_len = read_line(fd, line, (int)sizeof(line));
        if (line_len < 0) {
            break;
        }
        num_count = parse_integers(line, numbers + idx, expected_count - idx);
        idx += num_count;
    }
    while (idx < expected_count) {
        numbers[idx] = WALL;
        idx++;
    }

    close(fd);

    idx = 0;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            (*state).grid[i][j] = numbers[idx];

            switch (numbers[idx]) {
                case PLAYER:
                    (*state).player.row = i;
                    (*state).player.col = j;
                    break;
                case GOAL:
                    (*state).goal.row = i;
                    (*state).goal.col = j;
                    break;
                case SNAKE:
                    (*state).snake.row = i;
                    (*state).snake.col = j;
                    break;
                case WOLF:
                    (*state).wolf.row = i;
                    (*state).wolf.col = j;
                    break;
                default:
                    break;
            }
            idx++;
        }
    }

    return state;
}

/**
 * write_state_fd - Serialises the current game state and writes it
 * into an already-open file descriptor, rewinding to offset 0 and
 * truncating any leftover bytes first. The caller must hold an
 * exclusive flock() on fd before calling this (except for the very
 * first write in main(), before any other process exists).
 * Returns 1 on success, 0 on failure.
 */
int write_state_fd(GameState *state, int fd) {
    int i, j;
    char buffer[16];
    char *write_buf;
    int total_len;
    int offset = 0;
    int len;
    ssize_t written;

    total_len = 32 + (*state).rows * (*state).cols * 4;

    write_buf = (char *)malloc((size_t)total_len);
    if (!write_buf) {
        return 0;
    }

    my_itoa((*state).game_state, buffer);
    len = my_strlen(buffer);
    my_strcpy(write_buf + offset, buffer);
    offset += len;
    write_buf[offset] = '\n';
    offset++;

    my_itoa((*state).rows, buffer);
    len = my_strlen(buffer);
    my_strcpy(write_buf + offset, buffer);
    offset += len;
    write_buf[offset] = ' ';
    offset++;

    my_itoa((*state).cols, buffer);
    len = my_strlen(buffer);
    my_strcpy(write_buf + offset, buffer);
    offset += len;
    write_buf[offset] = '\n';
    offset++;

    for (i = 0; i < (*state).rows; i++) {
        for (j = 0; j < (*state).cols; j++) {
            my_itoa((*state).grid[i][j], buffer);
            len = my_strlen(buffer);
            my_strcpy(write_buf + offset, buffer);
            offset += len;
            if (j < (*state).cols - 1) {
                write_buf[offset] = ' ';
                offset++;
            }
        }
        write_buf[offset] = '\n';
        offset++;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        free(write_buf);
        return 0;
    }
    if (ftruncate(fd, 0) != 0) {
        free(write_buf);
        return 0;
    }

    written = write(fd, write_buf, (size_t)offset);
    free(write_buf);

    return (written == offset) ? 1 : 0;
}

/**
 * read_state_fd - Reads the game state back from an already-open
 * file descriptor (rewinding to offset 0 first) and refreshes the
 * grid and object positions inside state. The caller must hold an
 * flock() on fd before calling this. Returns 1 on success, 0 on
 * failure (including a rows/cols mismatch, which should never
 * happen since the map size never changes during a game).
 */
int read_state_fd(GameState *state, int fd) {
    char line[256];
    int numbers[MAX_ROWS * MAX_COLS + 4];
    int rows, cols;
    int idx;
    int i, j;
    int expected_count;
    int num_count;

    if (lseek(fd, 0, SEEK_SET) < 0) {
        return 0;
    }

    read_line(fd, line, (int)sizeof(line));
    (*state).game_state = my_atoi(line);

    read_line(fd, line, (int)sizeof(line));
    parse_integers(line, numbers, 2);
    rows = numbers[0];
    cols = numbers[1];

    if (rows != (*state).rows || cols != (*state).cols) {
        return 0;
    }

    expected_count = rows * cols;
    idx = 0;
    while (idx < expected_count) {
        int line_len = read_line(fd, line, (int)sizeof(line));
        if (line_len < 0) {
            break;
        }
        num_count = parse_integers(line, numbers + idx, expected_count - idx);
        idx += num_count;
    }
    while (idx < expected_count) {
        numbers[idx] = WALL;
        idx++;
    }

    idx = 0;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            (*state).grid[i][j] = numbers[idx];

            switch (numbers[idx]) {
                case PLAYER:
                    (*state).player.row = i;
                    (*state).player.col = j;
                    break;
                case GOAL:
                    (*state).goal.row = i;
                    (*state).goal.col = j;
                    break;
                case SNAKE:
                    (*state).snake.row = i;
                    (*state).snake.col = j;
                    break;
                case WOLF:
                    (*state).wolf.row = i;
                    (*state).wolf.col = j;
                    break;
                default:
                    break;
            }
            idx++;
        }
    }

    return 1;
}
