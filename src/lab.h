#ifndef LAB_H
#define LAB_H
<<<<<<< HEAD
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define lab_VERSION_MAJOR 1
#define lab_VERSION_MINOR 0
#define UNUSED(x) (void)x;

=======
//prevents include loops. there are alternatives to this
#include <stdlib.h>
#include <stdbool.h>
//c++ linker 
>>>>>>> 7506b9948063c2439e05fb54ffd34c31830dbd3a
#ifdef __cplusplus
extern "C"
{
#endif

<<<<<<< HEAD
  struct shell
  {
    int shell_is_interactive;
    pid_t shell_pgid;
    struct termios shell_tmodes;
    int shell_terminal;
    char *prompt;
  };



  /**
   * @brief Set the shell prompt. This function will attempt to load a prompt
   * from the requested environment variable, if the environment variable is
   * not set a default prompt of "shell>" is returned.  This function calls
   * malloc internally and the caller must free the resulting string.
   *
   * @param env The environment variable
   * @return const char* The prompt
   */
  char *get_prompt(const char *env);

  /**
   * Changes the current working directory of the shell. Uses the linux system
   * call chdir. With no arguments the users home directory is used as the
   * directory to change to.
   *
   * @param dir The directory to change to
   * @return  On success, zero is returned.  On error, -1 is returned, and
   * errno is set to indicate the error.
   */
  int change_dir(char **dir);

  /**
   * @brief Convert line read from the user into to format that will work with
   * execvp. We limit the number of arguments to ARG_MAX loaded from sysconf.
   * This function allocates memory that must be reclaimed with the cmd_free
   * function.
   *
   * @param line The line to process
   *
   * @return The line read in a format suitable for exec
   */
  char **cmd_parse(char const *line);

  /**
   * @brief Free the line that was constructed with parse_cmd
   *
   * @param line the line to free
   */
  void cmd_free(char ** line);

  /**
   * @brief Trim the whitespace from the start and end of a string.
   * For example "   ls -a   " becomes "ls -a". This function modifies
   * the argument line so that all printable chars are moved to the
   * front of the string
   *
   * @param line The line to trim
   * @return The new line with no whitespace
   */
  char *trim_white(char *line);


  /**
   * @brief Takes an argument list and checks if the first argument is a
   * built in command such as exit, cd, jobs, etc. If the command is a
   * built in command this function will handle the command and then return
   * true. If the first argument is NOT a built in command this function will
   * return false.
   *
   * @param sh The shell
   * @param argv The command to check
   * @return True if the command was a built in command
   */
  bool do_builtin(struct shell *sh, char **argv);

  /**
   * @brief Initialize the shell for use. Allocate all data structures
   * Grab control of the terminal and put the shell in its own
   * process group. NOTE: This function will block until the shell is
   * in its own program group. Attaching a debugger will always cause
   * this function to fail because the debugger maintains control of
   * the subprocess it is debugging.
   *
   * @param sh
   */
  void sh_init(struct shell *sh);

  /**
   * @brief Destroy shell. Free any allocated memory and resources and exit
   * normally.
   *
   * @param sh
   */
  void sh_destroy(struct shell *sh);

  /**
   * @brief Parse command line args from the user when the shell was launched
   *
   * @param argc Number of args
   * @param argv The arg array
   */
  void parse_args(int argc, char **argv);
=======
    /**
     * @brief A node in the list
     *
     */
    typedef struct node
    {
        void *data; //TODO: research 'void *'
        struct node *next;
        struct node *prev;
    } node_t;

    /**
    * @brief Struct to represent a list. The list maintains 2 function pointers to help
    * with the management of the data it is storing. These functions must be provided by the
    * user of this library.
    */
    typedef struct list
    {
        void (*destroy_data)(void *);                  /*free's any memory that data allocated*/
        int (*compare_to)(const void *, const void *); /* returns 0 if data are the same*/
        size_t size;                                   /* How many elements are in the list */
        struct node *head;                             /* sentinel node*/
    } list_t;

    /**
    * @brief Create a new list with callbacks that know how to deal with the data that
    * list is storing. The caller must pass the list to list_destroy when finished to
    * free any memory that was allocated.
    *
    * @param destroy_data Function that will free the memory for user supplied data
    * @param compare_to Function that will compare two user data elements
    * @return struct list* pointer to the newly allocated list.
    */
    list_t *list_init(void (*destroy_data)(void *), int (*compare_to)(const void *, const void *));

    /**
     * @brief Destroy the list and and all associated data. This functions will call
     * destroy_data on each nodes data element.
     *
     * @param list a pointer to the list that needs to be destroyed
     */
    void list_destroy(list_t **list);    //unnecessary in C++ and rust, due to use of safe pointers.

    /**
     * Adds data to the front of the list
     *
     * @param list a pointer to an existing list.
     * @param data the data to add
     * @return A pointer to the list
     */
    list_t *list_add(list_t *list, void *data);

    /**
     * @brief Removes the data at the specified index. If index is invalid
     * then this function does nothing and returns NULL
     *
     * @param list The list to remove the element from
     * @param index The index
     * @return void* The data that was removed or NULL if nothing was removed
     */
    void *list_remove_index(list_t *list, size_t index);

    /**
     * @brief Search for any occurrence of data from the list.
     * Internally this function will call compare_to on each item in the list
     * until a match is found or the end of the list is reached. If there are
     * multiple copies of the same data in the list the first one will be returned.
     *
     * @param list the list to search for data
     * @param data the data to look for
     * @return The index of the item if found or -1 if not
     */
    int list_indexof(list_t *list, void *data);
>>>>>>> 7506b9948063c2439e05fb54ffd34c31830dbd3a



#ifdef __cplusplus
<<<<<<< HEAD
} // extern "C"
=======
} //extern "C"
>>>>>>> 7506b9948063c2439e05fb54ffd34c31830dbd3a
#endif

#endif
