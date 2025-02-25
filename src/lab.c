#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "lab.h"

/**
 * @brief Set the shell prompt. This function attempts to load a prompt
 * from the requested environment variable. If the environment variable is
 * not set, it returns a default prompt ("shell>"). This function allocates
 * memory for the prompt which the caller must free.
 *
 * @param env The environment variable to fetch the prompt from
 * @return const char* A string containing the prompt to be used by the shell
 */
char *get_prompt(const char *env)
{
    char *prompt = getenv(env);  //custom prompt from the environment variable
    if (!prompt)
    {
        prompt = "shell> ";     //default prompt
    }
    return strdup(prompt);
}

/**
 * Changes the current working directory of the shell. If no directory is
 * provided, it changes to the user's home directory. If the directory cannot
 * be changed, an error message is printed.
 *
 * @param argv Arguments passed to the 'cd' command
 * @return 0 if successful, -1 if an error occurs
 */
int change_dir(char **argv)
{
    const char *dir = argv[1] ? argv[1] : getenv("HOME");  // default is HOME
    if (!dir)  //nothing found, get home directory from the passwd struct
    {
        struct passwd *pw = getpwuid(getuid());
        dir = pw ? pw->pw_dir : "/";  // Default is root
    }
    if (chdir(dir) != 0)  //actually change the directory
    {
        perror("cd failed"); 
        return -1;
    }
    return 0;
}

/**
 * @brief Parse the user input into a format suitable for execvp.
 * This function splits the input line into tokens, with the number of tokens
 * limited by ARG_MAX. Memory is allocated for the argument array, which must
 * be freed by the caller using cmd_free.
 *
 * @param line The line to process
 * @return char** An array of arguments suitable for execvp
 */
char **cmd_parse(const char *line)
{
    long arg_max = sysconf(_SC_ARG_MAX);  //max args
    char **args = malloc(sizeof(char *) * (arg_max + 1));  //space for args
    if (!args)  //memory issue error
        return NULL;
    char *temp = strdup(line);  //store input line
    char *token = strtok(temp, " ");  //tokenize it via spaces
    int i = 0;
    while (token && i < arg_max)  //put args into an array
    {
        args[i++] = strdup(token);
        token = strtok(NULL, " ");  // Get the next token
    }
    args[i] = NULL;  // Null-terminated array
    free(temp);
    return args;
}

/**
 * @brief Free the memory allocated by cmd_parse for the argument list.
 *
 * @param args The argument array to free
 */
void cmd_free(char **args)
{
    if (!args)
        return;
    for (int i = 0; args[i]; i++)  //free each argument
    {
        free(args[i]);
    }
    free(args);  //Free the argument array
}

/**
 * @brief Trim whitespace from both the start and the end of a string.
 * For example, "   ls -a   " becomes "ls -a".
 *
 * @param line The string to trim
 * @return The trimmed string
 */
char *trim_white(char *line)
{
    char *end;
    while (*line == ' ')  //skip leading spaces
        line++;
    if (*line == 0)  //return empty strings
        return line;
    end = line + strlen(line) - 1;  // end points to last character
    while (end > line && *end == ' ')  // remove trailing spaces
        end--;
    *(end + 1) = '\0';  // Null-terminated, trimmed string
    return line;
}

/**
 * @brief Check if the first argument is a built-in command (like exit, cd).
 * If it is a built-in command, the function executes it and returns true.
 * Otherwise, it returns false.
 *
 * @param sh The shell context
 * @param argv The command arguments
 * @return true if the command is a built-in command, false otherwise
 */
bool do_builtin(struct shell *sh, char **argv)
{
    if (!argv[0])
        return false;

    if (strcmp(argv[0], "exit") == 0)  // 'exit' frees args, then destroys and exits shell
    {
        cmd_free(argv);
        sh_destroy(sh);
        exit(0);
    }

    if (strcmp(argv[0], "cd") == 0)  //'cd' changes dir
    {
        change_dir(argv);
        return true;
    }
    return false;
}

/**
 * @brief Set up signal handlers for the shell, ignoring certain signals.
 */
void setup_signals()
{
    signal(SIGINT, SIG_IGN);  //(Ctrl+C)
    signal(SIGQUIT, SIG_IGN);  //(Ctrl+\)
    signal(SIGTSTP, SIG_IGN);  //(Ctrl+Z)
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN); 
}

/**
 * @brief Initialize the shell. Allocate necessary data structures, gain
 * control of the terminal, and set up the shell's process group.
 *
 * @param sh The shell context to initialize
 */
void sh_init(struct shell *sh)
{
    sh->shell_terminal = STDIN_FILENO;  // standard input is the terminal
    sh->shell_is_interactive = isatty(sh->shell_terminal);
    if (sh->shell_is_interactive)  // If the shell is interactive, set up process group
    {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))  // Wait until we control the terminal
        {
            kill(-sh->shell_pgid, SIGTTIN);  // by killing the shell when we dont control the terminal
        }
        sh->shell_pgid = getpid();  // process group ID
        setpgid(sh->shell_pgid, sh->shell_pgid);  // process group
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);  //terminal control
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);  //terminal settings
    }
}

/**
 * @brief Clean up the shell. Free allocated resources and exit the program.
 *
 * @param sh The shell context to destroy
 */
void sh_destroy(struct shell *sh)
{
    UNUSED(sh);
    exit(0);
}

/**
 * @brief Parse command-line arguments when the shell is launched.
 * This function checks for the "-v" flag to print the shell version.
 *
 * @param argc The number of command-line arguments
 * @param argv The array of command-line arguments
 */
void parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)  // Check for the '-v' flag
        {
            printf("Shell Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);  // Print the version
            exit(EXIT_SUCCESS);  // Exit after printing the version
        }
    }
}
