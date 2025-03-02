#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

// Set the shell prompt.
char *get_prompt(const char *env)
{
    char *prompt = getenv(env); // custom prompt from the environment variable
    if (!prompt)
    {
        prompt = "shell>"; // default prompt
    }
    return strdup(prompt);
}

// Changes the current working directory of the shell.
int change_dir(char **argv)
{
    const char *dir = argv[1] ? argv[1] : getenv("HOME"); // default is HOME
    if (!dir)                                             // nothing found, get home directory from the passwd struct
    {
        struct passwd *pw = getpwuid(getuid());
        dir = pw ? pw->pw_dir : "/"; // Default is root
    }
    if (chdir(dir) != 0) // actually change the directory
    {
        perror("cd failed");
        return -1;
    }
    return 0;
}

// Parse the user input into a format suitable for execvp.
char **cmd_parse(const char *line)
{
    long arg_max = sysconf(_SC_ARG_MAX);                  // max args
    char **args = malloc(sizeof(char *) * (arg_max + 1)); // space for args
    if (!args)                                            // memory issue error
        return NULL;
    char *temp = strdup(line);       // store input line
    char *token = strtok(temp, " "); // tokenize it via spaces
    int i = 0;
    while (token && i < arg_max) // puts args into an array
    {
        args[i++] = strdup(token);
        token = strtok(NULL, " "); // Get the next token/arg
    }
    args[i] = NULL; // Null-terminated
    free(temp);
    return args;
}

// Free the memory allocated by cmd_parse for the argument list.
void cmd_free(char **args)
{
    if (!args)
        return;
    for (int i = 0; args[i]; i++) // free each argument
    {
        free(args[i]);
    }
    free(args); // Free the argument array
}

// Trim whitespace from both the start and the end of a string.
char *trim_white(char *line)
{
    char *end;
    while (*line == ' ') // skip leading spaces
        line++;
    if (*line == 0) // return empty strings
        return line;
    end = line + strlen(line) - 1;    // end points to last character
    while (end > line && *end == ' ') // remove trailing spaces
        end--;
    *(end + 1) = '\0'; // Null-terminated, trimmed string
    return line;
}

// Check if the first argument is a built-in command (like exit, cd).
bool do_builtin(struct shell *sh, char **argv)
{
    if (!argv[0])
        return false;

    if (strcmp(argv[0], "exit") == 0) // 'exit' frees args, then destroys and exits shell
    {
        cmd_free(argv);
        sh_destroy(sh);
        exit(0);
    }

    if (strcmp(argv[0], "cd") == 0) //'cd' changes dir
    {
        change_dir(argv);
        return true;
    }

     if (strcmp(argv[0], "history") == 0) {
        HIST_ENTRY **history = history_list();
        if (history) {
            for (int i = 0; history[i]; i++) {
                printf("%d %s\n", i + 1, history[i]->line);
            }
        }
        return true;
    }
    return false;
}

// Set up signal handlers for the shell, ignoring certain signals.
void setup_signals()
{
    signal(SIGINT, SIG_IGN);  //(Ctrl+C)
    signal(SIGQUIT, SIG_IGN); //(Ctrl+\)
    signal(SIGTSTP, SIG_IGN); //(Ctrl+Z)
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

// Initialize the shell. Allocate necessary data structures, gain control of the terminal, and set up the shell's process group.
void sh_init(struct shell *sh)
{
    sh->shell_terminal = STDIN_FILENO; // standard input is the terminal
    sh->shell_is_interactive = isatty(sh->shell_terminal);
    if (sh->shell_is_interactive) // If the shell is interactive, set up process group
    {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) // Wait until we control the terminal
        {
            kill(-sh->shell_pgid, SIGTTIN); // by killing the shell when we dont control the terminal
        }
        sh->shell_pgid = getpid();                        // process group ID
        setpgid(sh->shell_pgid, sh->shell_pgid);          // process group
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);    // terminal control
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes); // terminal settings
    }
}

// Clean up the shell. Free allocated resources and exit the program.
void sh_destroy(struct shell *sh)
{
    UNUSED(sh);
    exit(0);
}

// Parse command-line arguments when the shell is launched.
void parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0) // Check for the '-v' flag
        {
            printf("Shell Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR); // Print the version
            exit(EXIT_SUCCESS);                                                     // Exit after printing the version
        }
    }
}
