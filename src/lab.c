#include "lab.h"
#include <stdio.h>
<<<<<<< HEAD
#include <unistd.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

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
=======

list_t *list_init(void (*destroy_data)(void *), int (*compare_to)(const void *, const void *)) {
    list_t *list = (list_t *)malloc(sizeof(list_t));
    if (list == NULL) {
        return NULL;
    }
    list->destroy_data = destroy_data;
    list->compare_to = compare_to;
    list->size = 0;
    //sentinel node contains no data and begins by pointing to self in both directions
    node_t *sentinel = (node_t *)malloc(sizeof(node_t));
    if (sentinel == NULL) {
        free(list);
        return NULL;
    }
    sentinel->data = NULL;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    list->head = sentinel;
    return list;
}

void list_destroy(list_t **list) {
    if (list == NULL || *list == NULL) {
        return;
    }
    node_t *current = (*list)->head->next;
    while (current != (*list)->head) {
        node_t *next = current->next;
        (*list)->destroy_data(current->data);
        free(current);
        current = next;
    }
    free((*list)->head);
    free(*list);
    *list = NULL;
}

list_t *list_add(list_t *list, void *data) {
    if (list == NULL || data == NULL) {
        return NULL;
    }
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) {
        return NULL;
    }
    new_node->data = data;
    new_node->next = list->head->next;
    new_node->prev = list->head;
    list->head->next->prev = new_node;
    list->head->next = new_node;
    list->size++;
    return list;
}

void *list_remove_index(list_t *list, size_t index) {
    if (list == NULL || index >= list->size) {
        return NULL;
    }
    node_t *curr = list->head->next;
    for (size_t i = 0; i < index; i++) {
        curr = curr->next;
    }
    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;
    void *data = curr->data;
    free(curr);
    list->size--;
    return data;
}

int list_indexof(list_t *list, void *data) {
    if (list == NULL || data == NULL) {
        return -1;
    }
    node_t *current = list->head->next;
    size_t index = 0;
    while (index < list->size) {
        if (list->compare_to(current->data, data) == 0) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}
>>>>>>> 7506b9948063c2439e05fb54ffd34c31830dbd3a
