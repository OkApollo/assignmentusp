/**
 * player.c - Terminal I/O, display, and process control implementation
 * COMP2002 Unix Systems Programming
 */

#define _POSIX_C_SOURCE 200809L

#include "player.h"
#include "logic.h"
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

/* ANSI escape codes for terminal control */
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define COLOR_RESET "\033[0m"
#define BG_WHITE "\033[47m"
#define FG_RED "\033[31m"
#define FG_GREEN "\033[32m"

/* ============================================================
 * TERMINAL I/O FUNCTIONS
 * ============================================================ */

/**
 * set_terminal_mode - Sets terminal to raw mode for single character input
 */
int set_terminal_mode(struct termios *orig_termios) {
    struct termios new_termios;

    if (tcgetattr(STDIN_FILENO, orig_termios) != 0) {
        return 0;
    }
    new_termios = *orig_termios;

    new_termios.c_lflag &= ~((unsigned int)(ICANON | ECHO));
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
        return 0;
    }
    return 1;
}

/**
 * restore_terminal_mode - Restores terminal to original mode
 */
void restore_terminal_mode(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSANOW, orig_termios);
}

/**
 * get_char - Reads a single character from stdin
 */
int get_char(void) {
    char ch;
    ssize_t bytes_read = read(STDIN_FILENO, &ch, 1);
    if (bytes_read > 0) {
        return (unsigned char)ch;
    }
    return -1;
}

/**
 * clear_screen - Clears the terminal screen
 */
void clear_screen(void) {
    write(STDOUT_FILENO, CLEAR_SCREEN, my_strlen(CLEAR_SCREEN));
    write(STDOUT_FILENO, CURSOR_HOME, my_strlen(CURSOR_HOME));
}

/**
 * print_character - Prints a character with appropriate color/background
 */
void print_character(int obj_code) {
    switch (obj_code) {
        case EMPTY:
            write(STDOUT_FILENO, " ", 1);
            break;
        case WALL:
            write(STDOUT_FILENO, "*", 1);
            break;
        case PLAYER:
            write(STDOUT_FILENO, FG_GREEN, my_strlen(FG_GREEN));
            write(STDOUT_FILENO, "P", 1);
            write(STDOUT_FILENO, COLOR_RESET, my_strlen(COLOR_RESET));
            break;
        case GOAL:
            write(STDOUT_FILENO, FG_GREEN, my_strlen(FG_GREEN));
            write(STDOUT_FILENO, "G", 1);
            write(STDOUT_FILENO, COLOR_RESET, my_strlen(COLOR_RESET));
            break;
        case SNAKE:
            write(STDOUT_FILENO, FG_RED, my_strlen(FG_RED));
            write(STDOUT_FILENO, "~", 1);
            write(STDOUT_FILENO, COLOR_RESET, my_strlen(COLOR_RESET));
            break;
        case WOLF:
            write(STDOUT_FILENO, FG_RED, my_strlen(FG_RED));
            write(STDOUT_FILENO, "W", 1);
            write(STDOUT_FILENO, COLOR_RESET, my_strlen(COLOR_RESET));
            break;
        default:
            write(STDOUT_FILENO, "?", 1);
            break;
    }
}

/**
 * display_game - Displays the game interface
 */
void display_game(GameState *state, int debug_mode) {
    int i, j;
    const char *title        = "=== ESCAPE GAME ===\n";
    const char *instructions = "Use WASD to move Player (P) to Goal (G)\n";
    const char *avoid        = "Avoid Snake (~) and Wolf (W)!\n";
    const char *quit_hint    = "Press 'q' to quit\n\n";
    const char *win_msg      = "\n*** YOU WIN! ***\n";
    const char *lose_msg     = "\n*** YOU LOSE! ***\n";
    const char *quit_msg     = "\n*** GAME QUIT ***\n";
    const char *prompt       = "\nEnter move: ";

    clear_screen();

    write(STDOUT_FILENO, title, my_strlen(title));
    write(STDOUT_FILENO, instructions, my_strlen(instructions));
    write(STDOUT_FILENO, avoid, my_strlen(avoid));
    write(STDOUT_FILENO, quit_hint, my_strlen(quit_hint));

    for (i = 0; i < (*state).rows; i++) {
        for (j = 0; j < (*state).cols; j++) {
            print_character((*state).grid[i][j]);
        }
        write(STDOUT_FILENO, "\n", 1);
    }

    if (debug_mode) {
        char number[16];
        const char *debug_title = "\n--- DEBUG MODE ---\n";
        const char *player_label = "Player: (";
        const char *snake_label = "\nSnake: (";
        const char *wolf_label = "\nWolf: (";
        const char *moves_label = ") moves=";
        const char *time_label = " last move=";
        const char *ms_label = "ms\n";

        write(STDOUT_FILENO, debug_title, my_strlen(debug_title));
        write(STDOUT_FILENO, player_label, my_strlen(player_label));
        my_itoa((*state).player.row, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, ",", 1);
        my_itoa((*state).player.col, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, ")", 1);
        write(STDOUT_FILENO, snake_label, my_strlen(snake_label));
        my_itoa((*state).snake.row, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, ",", 1);
        my_itoa((*state).snake.col, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, moves_label, my_strlen(moves_label));
        my_itoa((*state).snake_moves, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, time_label, my_strlen(time_label));
        my_itoa((*state).snake_move_ms, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, ms_label, my_strlen(ms_label));
        write(STDOUT_FILENO, wolf_label, my_strlen(wolf_label));
        my_itoa((*state).wolf.row, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, ",", 1);
        my_itoa((*state).wolf.col, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, moves_label, my_strlen(moves_label));
        my_itoa((*state).wolf_moves, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, time_label, my_strlen(time_label));
        my_itoa((*state).wolf_move_ms, number); write(STDOUT_FILENO, number, my_strlen(number));
        write(STDOUT_FILENO, ms_label, my_strlen(ms_label));
    }

    if ((*state).game_state == GAME_WIN) {
        write(STDOUT_FILENO, win_msg, my_strlen(win_msg));
    } else if ((*state).game_state == GAME_LOSE) {
        write(STDOUT_FILENO, lose_msg, my_strlen(lose_msg));
    } else if ((*state).game_state == GAME_QUIT) {
        write(STDOUT_FILENO, quit_msg, my_strlen(quit_msg));
    }

    if ((*state).game_state == GAME_ONGOING) {
        write(STDOUT_FILENO, prompt, my_strlen(prompt));
    }
}

/* ============================================================
 * PROCESS FUNCTIONS
 * ============================================================ */

/**
 * enemy_process - Child process function for enemy movement.
 * Opens the state file exactly once and keeps it open for the
 * lifetime of the process, locking/unlocking it on each wake cycle
 * rather than reopening it every time.
 */
void enemy_process(GameState *state, int is_snake, const char *state_filename) {
    struct timespec delay;
    int dr, dc;
    int new_row, new_col;
    int fd;
    int row, col;
    int attack_dir;
    int moved;
    int total_directions;
    int candidates[8];
    int candidate_count;
    int d;
    int game_over = 0;
    struct timespec started_at;
    struct timespec finished_at;

    delay.tv_sec = is_snake ? 2 : 1;
    delay.tv_nsec = 0;

    srand((unsigned int)getpid() ^ (unsigned int)time(NULL));

    fd = open(state_filename, O_RDWR);
    if (fd < 0) {
        free_game_state(state);
        exit(1);
    }

    while (!game_over) {
        clock_gettime(CLOCK_MONOTONIC, &started_at);
        nanosleep(&delay, NULL);

        if (flock(fd, LOCK_EX) != 0) {
            continue;
        }

        if (!read_state_fd(state, fd)) {
            flock(fd, LOCK_UN);
            continue;
        }

        if ((*state).game_state != GAME_ONGOING) {
            flock(fd, LOCK_UN);
            break;
        }

        if (is_snake) {
            row = (*state).snake.row;
            col = (*state).snake.col;
        } else {
            row = (*state).wolf.row;
            col = (*state).wolf.col;
        }

        moved = 0;

        if (is_player_close(state, row, col, is_snake)) {
            attack_dir = get_attack_direction(state, row, col, is_snake);
            if (attack_dir >= 0) {
                get_direction_offset(attack_dir, &dr, &dc);
                moved = move_enemy(state, row, col, row + dr, col + dc, is_snake);
            }
        }

        if (!moved) {
            /*
             * Build the list of directions that are actually legal
             * right now (Wolf only ever considers the first 4 -- the
             * orthogonal ones -- since DIR_UP..DIR_RIGHT are 0..3 in
             * the direction enum; Snake considers all 8), then pick
             * uniformly at random among only those. This still
             * satisfies the "randomly decided" requirement, but
             * guarantees the enemy actually moves on every wake
             * cycle whenever at least one legal move exists, instead
             * of gambling on a fixed number of random retries.
             */
            total_directions = is_snake ? 8 : 4;
            candidate_count = 0;
            for (d = 0; d < total_directions; d++) {
                if (enemy_can_move(state, row, col, d)) {
                    candidates[candidate_count] = d;
                    candidate_count++;
                }
            }

            if (candidate_count > 0) {
                int chosen_direction = candidates[rand() % candidate_count];
                get_direction_offset(chosen_direction, &dr, &dc);
                new_row = row + dr;
                new_col = col + dc;
                moved = move_enemy(state, row, col, new_row, new_col, is_snake);
            }
        }

        if (moved) {
            clock_gettime(CLOCK_MONOTONIC, &finished_at);
            if (is_snake) {
                (*state).snake_moves++;
                (*state).snake_move_ms = (int)((finished_at.tv_sec - started_at.tv_sec) * 1000L +
                    (finished_at.tv_nsec - started_at.tv_nsec) / 1000000L);
            } else {
                (*state).wolf_moves++;
                (*state).wolf_move_ms = (int)((finished_at.tv_sec - started_at.tv_sec) * 1000L +
                    (finished_at.tv_nsec - started_at.tv_nsec) / 1000000L);
            }
            write_state_fd(state, fd);
        }

        game_over = ((*state).game_state != GAME_ONGOING);

        flock(fd, LOCK_UN);
    }

    close(fd);
    free_game_state(state);
    exit(0);
}

/**
 * parent_process - Main parent process for player control. Opens
 * the state file exactly once for the whole session, locking and
 * re-reading it before every accepted move.
 */
void parent_process(GameState *state, const char *state_filename, pid_t snake_pid, pid_t wolf_pid,
                    int debug_mode) {
    struct termios orig_termios;
    int ch;
    int moved;
    int fd;
    int status;
    int direction;
    int quit_requested;
    int game_over = 0;
    int terminal_configured;

    terminal_configured = set_terminal_mode(&orig_termios);

    fd = open(state_filename, O_RDWR);
    if (fd < 0) {
        if (terminal_configured) {
            restore_terminal_mode(&orig_termios);
        }
        kill(snake_pid, SIGTERM);
        kill(wolf_pid, SIGTERM);
        waitpid(snake_pid, &status, 0);
        waitpid(wolf_pid, &status, 0);
        return;
    }

    while (!game_over) {
        display_game(state, debug_mode);

        ch = get_char();
        direction = -1;
        quit_requested = 0;

        switch (ch) {
            case 'w': case 'W': direction = DIR_UP;    break;
            case 's': case 'S': direction = DIR_DOWN;  break;
            case 'a': case 'A': direction = DIR_LEFT;  break;
            case 'd': case 'D': direction = DIR_RIGHT; break;
            case 'q': case 'Q': quit_requested = 1;     break;
            case -1: quit_requested = 1;                 break;
            default: break;
        }

        if (quit_requested) {
            if (flock(fd, LOCK_EX) == 0) {
                if (read_state_fd(state, fd) && (*state).game_state == GAME_ONGOING) {
                    (*state).game_state = GAME_QUIT;
                    write_state_fd(state, fd);
                }
                flock(fd, LOCK_UN);
            }
            game_over = 1;
        } else if (direction >= 0) {
            if (flock(fd, LOCK_EX) != 0) {
                continue;
            }

            if (!read_state_fd(state, fd)) {
                flock(fd, LOCK_UN);
                continue;
            }

            if ((*state).game_state != GAME_ONGOING) {
                flock(fd, LOCK_UN);
                game_over = 1;
                continue;
            }

            moved = move_player(state, direction);
            if (moved) {
                write_state_fd(state, fd);
            }

            game_over = ((*state).game_state != GAME_ONGOING);

            flock(fd, LOCK_UN);
        }
    }

    display_game(state, debug_mode);
    close(fd);

    if (terminal_configured) {
        restore_terminal_mode(&orig_termios);
    }

    waitpid(snake_pid, &status, 0);
    waitpid(wolf_pid, &status, 0);
}
