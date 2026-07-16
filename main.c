/**
 * main.c - Main entry point for the escape game
 * COMP2002 Unix Systems Programming
 */

#define _POSIX_C_SOURCE 200809L

#include "game.h"
#include "logic.h"
#include "player.h"
#include <signal.h>
#include <sys/wait.h>

/**
 * main - Entry point for the escape game
 */
int main(int argc, char *argv[]) {
    GameState *state;
    int fd;
    pid_t snake_pid, wolf_pid;
    int status;
    int debug_mode = 0;

    /* Validate command line arguments */
    if (argc == 3 && my_strlen(argv[2]) == 5 && argv[2][0] == 'd' &&
        argv[2][1] == 'e' && argv[2][2] == 'b' && argv[2][3] == 'u' &&
        argv[2][4] == 'g') {
        debug_mode = 1;
    } else if (argc != 2) {
        const char *usage = "Usage: ./escape <map_file> [debug]\n";
        write(STDOUT_FILENO, usage, my_strlen(usage));
        return 1;
    }

    /* Check if map file exists and can be opened */
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        const char *err = "Error: Cannot open map file\n";
        write(STDOUT_FILENO, err, my_strlen(err));
        return 1;
    }
    close(fd);

    /* Load the map */
    state = load_map(argv[1]);
    if (!state) {
        const char *err = "Error: Failed to load map file\n";
        write(STDOUT_FILENO, err, my_strlen(err));
        return 1;
    }

    /* Create the shared state file with the initial map contents */
    fd = open(STATE_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 || !write_state_fd(state, fd)) {
        const char *err = "Error: Failed to create state file\n";
        write(STDOUT_FILENO, err, my_strlen(err));
        if (fd >= 0) {
            close(fd);
        }
        unlink(STATE_FILE);
        free_game_state(state);
        return 1;
    }
    close(fd);

    /*
     * fork() duplicates the whole process image, including the heap,
     * so each child automatically inherits its own independent copy
     * of *state (and its malloc'd grid) -- no manual deep copy needed.
     */
    snake_pid = fork();
    if (snake_pid < 0) {
        const char *err = "Error: Fork failed for Snake\n";
        write(STDOUT_FILENO, err, my_strlen(err));
        unlink(STATE_FILE);
        free_game_state(state);
        return 1;
    }

    if (snake_pid == 0) {
        /* Child process - Snake */
        enemy_process(state, 1, STATE_FILE);
        /* enemy_process() frees state and calls exit(), never returns */
    }

    /* Fork for Wolf process */
    wolf_pid = fork();
    if (wolf_pid < 0) {
        const char *err = "Error: Fork failed for Wolf\n";
        write(STDOUT_FILENO, err, my_strlen(err));
        kill(snake_pid, SIGTERM);
        waitpid(snake_pid, &status, 0);
        unlink(STATE_FILE);
        free_game_state(state);
        return 1;
    }

    if (wolf_pid == 0) {
        /* Child process - Wolf */
        enemy_process(state, 0, STATE_FILE);
        /* enemy_process() frees state and calls exit(), never returns */
    }

    /* Parent process - run the game with player control */
    parent_process(state, STATE_FILE, snake_pid, wolf_pid, debug_mode);

    /* Clean up */
    free_game_state(state);
    unlink(STATE_FILE);

    return 0;
}
