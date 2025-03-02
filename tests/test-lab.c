#include <string.h>
#include "harness/unity.h"
#include "../src/lab.h"

void setUp(void)
{
     // set stuff up here
}

void tearDown(void)
{
     // clean stuff up here
}

void test_cmd_parse(void)
{
     char **rval = cmd_parse("ls -a -l");
     TEST_ASSERT_TRUE(rval);
     TEST_ASSERT_EQUAL_STRING("ls", rval[0]);
     TEST_ASSERT_EQUAL_STRING("-a", rval[1]);
     TEST_ASSERT_EQUAL_STRING("-l", rval[2]);
     TEST_ASSERT_EQUAL_STRING(NULL, rval[3]);
     TEST_ASSERT_FALSE(rval[3]);
     cmd_free(rval);
}

void test_cmd_parse2(void)
{
     // The string we want to parse from the user.
     // foo -v
     char *stng = (char *)malloc(sizeof(char) * 7);
     strcpy(stng, "foo -v");
     char **actual = cmd_parse(stng);
     // construct our expected output
     size_t n = sizeof(char *) * 6;
     char **expected = (char **)malloc(sizeof(char *) * 6);
     memset(expected, 0, n);
     expected[0] = (char *)malloc(sizeof(char) * 4);
     expected[1] = (char *)malloc(sizeof(char) * 3);
     expected[2] = (char *)NULL;

     strcpy(expected[0], "foo");
     strcpy(expected[1], "-v");
     TEST_ASSERT_EQUAL_STRING(expected[0], actual[0]);
     TEST_ASSERT_EQUAL_STRING(expected[1], actual[1]);
     TEST_ASSERT_FALSE(actual[2]);
     free(expected[0]);
     free(expected[1]);
     free(expected);
}

void test_trim_white_no_whitespace(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "ls -a", 10);
     char *rval = trim_white(line);
     TEST_ASSERT_EQUAL_STRING("ls -a", rval);
     free(line);
}

void test_trim_white_start_whitespace(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "  ls -a", 10);
     char *rval = trim_white(line);
     TEST_ASSERT_EQUAL_STRING("ls -a", rval);
     free(line);
}

void test_trim_white_end_whitespace(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "ls -a  ", 10);
     char *rval = trim_white(line);
     TEST_ASSERT_EQUAL_STRING("ls -a", rval);
     free(line);
}

void test_trim_white_both_whitespace_single(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, " ls -a ", 10);
     char *rval = trim_white(line);
     TEST_ASSERT_EQUAL_STRING("ls -a", rval);
     free(line);
}

void test_trim_white_both_whitespace_double(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "  ls -a  ", 10);
     char *rval = trim_white(line);
     TEST_ASSERT_EQUAL_STRING("ls -a", rval);
     free(line);
}

void test_trim_white_all_whitespace(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "  ", 10);
     char *rval = trim_white(line);
     TEST_ASSERT_EQUAL_STRING("", rval);
     free(line);
}

void test_trim_white_mostly_whitespace(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "    a    ", 10);
     char *rval = trim_white(line);
     TEST_ASSERT_EQUAL_STRING("a", rval);
     free(line);
}

void test_get_prompt_default(void)
{
     char *prompt = get_prompt("MY_PROMPT");
     TEST_ASSERT_EQUAL_STRING(prompt, "shell>");
     free(prompt);
}

void test_get_prompt_custom(void)
{
     const char *prmpt = "MY_PROMPT";
     if (setenv(prmpt, "foo>", true))
     {
          TEST_FAIL();
     }

     char *prompt = get_prompt(prmpt);
     TEST_ASSERT_EQUAL_STRING(prompt, "foo>");
     free(prompt);
     unsetenv(prmpt);
}

void test_ch_dir_home(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "cd", 10);
     char **cmd = cmd_parse(line);
     char *expected = getenv("HOME");
     change_dir(cmd);
     char *actual = getcwd(NULL, 0);
     TEST_ASSERT_EQUAL_STRING(expected, actual);
     free(line);
     free(actual);
     cmd_free(cmd);
}

void test_ch_dir_root(void)
{
     char *line = (char *)calloc(10, sizeof(char));
     strncpy(line, "cd /", 10);
     char **cmd = cmd_parse(line);
     change_dir(cmd);
     char *actual = getcwd(NULL, 0);
     TEST_ASSERT_EQUAL_STRING("/", actual);
     free(line);
     free(actual);
     cmd_free(cmd);
}

// new TC's!
void test_free(void)
{
     char **cmd = cmd_parse("cd /");
     cmd_free(cmd);
     // passes if no leaks or crashes
}

// tests empty cmd string
void test_cmd_parse0(void)
{
     char **result = cmd_parse("");
     TEST_ASSERT_NOT_NULL(result);
     TEST_ASSERT_NULL(result[0]);
     cmd_free(result);
}

// tests 1 arg cmd string
void test_cmd_parse1(void)
{
     char **result = cmd_parse("foo");
     TEST_ASSERT_EQUAL_STRING("foo", result[0]);
     TEST_ASSERT_NULL(result[1]);
     cmd_free(result);
}

// tests invalid command string
void test_invalid_builtin(void)
{
     char *line = "notacommand";
     char **cmd = cmd_parse(line);
     int result = do_builtin(NULL, cmd);
     TEST_ASSERT_FALSE(result);
     cmd_free(cmd);
}

// tests if history runs
// TODO: test if prints out correct history
void test_history(void)
{
     char *stng1 = "ls -a";
     char *stng2 = "pwd";
     char *stng3 = "cd /";
     char **cmd1 = cmd_parse(stng1);
     char **cmd2 = cmd_parse(stng2);
     char **cmd3 = cmd_parse(stng3);
     cmd_free(cmd1);
     cmd_free(cmd2);
     cmd_free(cmd3);

     char **history = cmd_parse("history"); // true if it works, else false
     TEST_ASSERT_TRUE(do_builtin(NULL, history));
     cmd_free(history);
}

// maybe test the signal handling...? I cant figure that out

int main(void)
{
     UNITY_BEGIN();
     RUN_TEST(test_cmd_parse);
     RUN_TEST(test_cmd_parse2);
     RUN_TEST(test_trim_white_no_whitespace);
     RUN_TEST(test_trim_white_start_whitespace);
     RUN_TEST(test_trim_white_end_whitespace);
     RUN_TEST(test_trim_white_both_whitespace_single);
     RUN_TEST(test_trim_white_both_whitespace_double);
     RUN_TEST(test_trim_white_all_whitespace);
     RUN_TEST(test_get_prompt_default);
     RUN_TEST(test_get_prompt_custom);
     RUN_TEST(test_ch_dir_home);
     RUN_TEST(test_ch_dir_root);
     RUN_TEST(test_free);
     RUN_TEST(test_cmd_parse0);
     RUN_TEST(test_cmd_parse1);
     RUN_TEST(test_invalid_builtin);
     RUN_TEST(test_history);

     return UNITY_END();
}
