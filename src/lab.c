#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "lab.h"

void parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            printf("Shell Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
            exit(EXIT_SUCCESS);
        }
    }
}

char *get_prompt(const char *env) {
    char *prompt = getenv(env);
    if (!prompt) {
        prompt = "shell> ";
    }
    return strdup(prompt);
}

char *trim_white(char *line) {
    char *end;
    while (*line == ' ') line++;
    if (*line == 0) return line;
    end = line + strlen(line) - 1;
    while (end > line && *end == ' ') end--;
    *(end + 1) = '\0';
    return line;
}

char **cmd_parse(const char *line) {
    long arg_max = sysconf(_SC_ARG_MAX);
    char **args = malloc(sizeof(char *) * (arg_max + 1));
    if (!args) return NULL;
    
    char *temp = strdup(line);
    char *token = strtok(temp, " ");
    int i = 0;
    while (token && i < arg_max) {
        args[i++] = strdup(token);
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
    free(temp);
    return args;
}

void cmd_free(char **args) {
    if (!args) return;
    for (int i = 0; args[i]; i++) {
        free(args[i]);
    }
    free(args);
}

int change_dir(char **argv) {
    const char *dir = argv[1] ? argv[1] : getenv("HOME");
    if (!dir) {
        struct passwd *pw = getpwuid(getuid());
        dir = pw ? pw->pw_dir : "/";
    }
    if (chdir(dir) != 0) {
        perror("cd failed");
        return -1;
    }
    return 0;
}

bool do_builtin(struct shell *sh, char **argv) {
    if (!argv[0]) return false;

    if (strcmp(argv[0], "exit") == 0) {
        cmd_free(argv);
        sh_destroy(sh);
        exit(0);
    }

    if (strcmp(argv[0], "cd") == 0) {
        change_dir(argv);
        return true;
    }
    return false;
}

void setup_signals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);
    if (sh->shell_is_interactive) {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) {
            kill(-sh->shell_pgid, SIGTTIN);
        }
        sh->shell_pgid = getpid();
        setpgid(sh->shell_pgid, sh->shell_pgid);
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }
}

void sh_destroy(struct shell *sh) {
    UNUSED(sh);
    exit(0);
}
