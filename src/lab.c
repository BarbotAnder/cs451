//error "include errors detected)
#include "lab.h"

//error "cannot open source file..."
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

char *get_prompt(const char *env) {
    char *prompt = getenv(env);
    return prompt ? prompt : "shell> ";
}

int change_dir(char **dir) {
    if (dir[1] == NULL) {
        char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            if (pw) home = pw->pw_dir;
        }
        if (home) return chdir(home);
    } else {
        return chdir(dir[1]);
    }
    return -1;
}

bool do_builtin(struct shell *sh, char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        free(argv);
        exit(EXIT_SUCCESS);
    }
    if (strcmp(argv[0], "cd") == 0) {
        if (change_dir(argv) != 0) {
            perror("cd failed");
        }
        return true;
    }
    if (strcmp(argv[0], "history") == 0) {
        HIST_ENTRY **history_list = history_list();
        if (history_list) {
            for (int i = 0; history_list[i]; i++) {
                printf("%d %s\n", i + history_base, history_list[i]->line);
            }
        }
        return true;
    }
    return false;
}

void execute_command(char **cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        /* This is the child process */
        pid_t child = getpid();
        setpgid(child, child);
        tcsetpgrp(STDIN_FILENO, child);

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        execvp(cmd[0], cmd);
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        /* This is the parent process */
        setpgid(pid, pid);
        tcsetpgrp(STDIN_FILENO, pid);
        
        int status;
        waitpid(pid, &status, 0);
        
        tcsetpgrp(STDIN_FILENO, getpgrp());
    } else {
        perror("fork failed");
    }
}

void parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                printf("Shell Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

void setup_signals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}
