/**
 * @file regexp_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/22
 *
 * @brief Virtual machine approach to implement a Regexp Interpreter.
 * @reference
 * 		Implementation detail: https://swtch.com/%7Ersc/regexp/regexp2.html.
 * 		Reference for features of Regexp: https://quickref.me/regex.
 *
 * @example
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/utils/core_utils.h"

#define MAX_PATTERN_SIZE (512)
#define MAX_SUB_MATCHES_SIZE (50)
#define MAX_COMMAND_SIZE (512)
#define MAX_THREAD_SIZE (512)

#define REGEXP_IS_S_CHAR(x) ((x) == ' ' || (x) == '\n' || (x) == '\r' || (x) == '\t' || (x) == '\v' || (x) == '\f')
#define REGEXP_IS_D_CHAR(x) ((x) >= '0' && (x) <= '9')
#define REGEXP_IS_W_CHAR(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z') || ((x) >= '0' && (x) <= '9') || (x) == '_')

enum {
	REGEXP_UNKNOWN = 0,
	REGEXP_ANY_CHAR,
	REGEXP_CHAR,   /* char */
	REGEXP_CHOOSE, /* n_choices choice1 choice2 ... */
	REGEXP_SPLIT,  /* offset1 offset2 */
	REGEXP_GOTO,   /* offset */
	REGEXP_SAVE,   /* slot */
};

typedef struct regexp regexp_t;

struct regexp {
	char     pattern[MAX_PATTERN_SIZE];
	int      commands[MAX_COMMAND_SIZE];
	uint32_t commands_len;
	uint32_t sub_matches[MAX_SUB_MATCHES_SIZE];
	uint32_t sub_matches_len;
};

/* pattern[s_left] should be a bracket */
uint32_t get_matching_bracket(const char *pattern, uint32_t s_left, uint32_t s_right, char l, char r)
{
	int      count = 0;
	uint32_t i     = s_left + 1;

	if (pattern[s_left] != l) return -1;
	while (count != -1 && i <= s_right) {
		if (pattern[i] == r) count--;
		if (pattern[i] == l) count++;
		i++;
	}

	return i > s_right + 1 ? -1 : i - 1;
}

static void regexp_compile_(char *pattern, int *commands, uint32_t *len, uint32_t l, uint32_t r, uint32_t *n_sub_matches)
{
#define is_quantifier(c) ((c) == '?' || (c) == '+' || (c) == '*')
#define is_left_bracket(c) ((c) == '[' || (c) == '(')
#define is_right_bracket(c) ((c) == ']' || (c) == ')')
#define right_bracket(c) ((c) == '(' ? ')' : ']')

#define copy_commands(dst_commands, dst_len, src_commands, src_len)                          \
	do {                                                                                     \
		memcpy((int *) (dst_commands) + (dst_len), (src_commands), sizeof(int) * (src_len)); \
		(dst_len) += (src_len);                                                              \
	} while (0)

	uint32_t i = l;
	int      temp_commands[MAX_COMMAND_SIZE];
	uint32_t temp_commands_len;

	/*char cur[100];
	snprintf(cur, r - l + 2, "%s", pattern + l);
	printf("current text(%s .. %d, %d): %s\n", pattern, l, r, cur);*/

	*len = 0;
	while (i <= r) {
		uint32_t cl = i, cr = i;
		char    *p = pattern + i;
		if (*p == '(' || *p == '[') {
			cr = get_matching_bracket(pattern, i, r, *p, right_bracket(*p));
		}


		if (cr + 1 <= r && is_quantifier(pattern[cr + 1])) {
			/* cope with the quantifier */

			int      is_greedy       = pattern[cr + 2] == '?';

			uint32_t cur_sub_matches = *n_sub_matches;
			if (is_left_bracket(pattern[cl])) {
				commands[(*len)++] = REGEXP_SAVE;
				commands[(*len)++] = 2 * (int) cur_sub_matches;
				(*n_sub_matches)++;
				regexp_compile_(pattern, temp_commands, &temp_commands_len, cl + 1, cr - 1, n_sub_matches);
			} else {
				regexp_compile_(pattern, temp_commands, &temp_commands_len, cl, cr, n_sub_matches);
			}

			switch (pattern[cr + 1]) {
				case '?':
					commands[(*len)++] = REGEXP_SPLIT;
					if (!is_greedy) {
						commands[(*len)++] = 3;
						commands[(*len)++] = (int) temp_commands_len + 3;
					} else {
						commands[(*len)++] = (int) temp_commands_len + 3;
						commands[(*len)++] = 3;
					}
					copy_commands(commands, *len, temp_commands, temp_commands_len);
					break;
				case '*':
					commands[(*len)++] = REGEXP_SPLIT;
					if (!is_greedy) {
						commands[(*len)++] = 3;
						commands[(*len)++] = (int) temp_commands_len + 3 + 2;
					} else {
						commands[(*len)++] = (int) temp_commands_len + 3 + 2;
						commands[(*len)++] = 3;
					}
					copy_commands(commands, *len, temp_commands, temp_commands_len);
					commands[(*len)++] = REGEXP_GOTO;
					commands[(*len)++] = -(int) temp_commands_len - 3;
					break;
				case '+':
					copy_commands(commands, *len, temp_commands, temp_commands_len);
					commands[(*len)++] = REGEXP_SPLIT;
					if (!is_greedy) {
						commands[(*len)++] = -(int) temp_commands_len;
						commands[(*len)++] = 3;
					} else {
						commands[(*len)++] = 3;
						commands[(*len)++] = -(int) temp_commands_len;
					}
					break;
				default:
					break;
			}

			if (is_right_bracket(pattern[cr])) {
				commands[(*len)++] = REGEXP_SAVE;
				commands[(*len)++] = 2 * (int) cur_sub_matches + 1;
			}

			i = cr + 2; /* skip the quantifier */
		} else {
			if (*p == '.') {
				commands[(*len)++] = REGEXP_ANY_CHAR;
			} else {
				commands[(*len)++] = REGEXP_CHAR;
				commands[(*len)++] = (int) *p;
			}
			i++;
		}
	}
}

void regexp_init(regexp_t *regexp, const char *pattern)
{
	strncpy(regexp->pattern, pattern, MAX_PATTERN_SIZE);
	regexp->commands_len    = 0;
	regexp->sub_matches_len = 0;
}

void regexp_inspect(regexp_t *regexp)
{
	uint32_t i;
	int     *commands = regexp->commands;
	for (i = 0; i < regexp->commands_len; i++) {
		int cmd = commands[i];
		switch (cmd) {
			case REGEXP_ANY_CHAR:
				printf("any_char\n");
				break;
			case REGEXP_CHAR:
				printf("char %c\n", (char) commands[i + 1]);
				i++;
				break;
			case REGEXP_GOTO:
				printf("goto %d\n", commands[i + 1]);
				i++;
				break;
			case REGEXP_SPLIT:
				printf("split %d, %d\n", commands[i + 1], commands[i + 2]);
				i += 2;
				break;
			case REGEXP_SAVE:
				printf("save %d\n", commands[i + 1]);
				i++;
				break;
			default:
				printf("(unknown command)\n");
				break;
		}
	}
}

void regexp_compile(regexp_t *regexp)
{
	uint32_t  n_sub_matches = 0, r = strlen(regexp->pattern) - 1;
	int      *commands     = regexp->commands;
	uint32_t *commands_len = &regexp->commands_len;

	/* add whole match save outside */
	n_sub_matches++;
	regexp->sub_matches[0] = 0;
	regexp_compile_(regexp->pattern, commands, commands_len, 0, r, &n_sub_matches);
	commands[(*commands_len)++] = REGEXP_SAVE;
	commands[(*commands_len)++] = 1;
	regexp->sub_matches_len     = 2 * n_sub_matches;
}

int regexp_match(regexp_t *regexp, const char *string)
{
#define push_thread(_sp, _pc)            \
	do {                                 \
		threads[threads_len].pc = (_pc); \
		threads[threads_len].sp = (_sp); \
		threads_len++;                   \
	} while (0)
#define pop_thread() ({ threads_len--; })

	/* personally I think Thompson's VM is basically no difference in efficiency compared to this */
	struct search_thread {
		uint32_t pc; /* program counter */
		uint32_t sp; /* string pointer */
		uint32_t sub_matches[MAX_SUB_MATCHES_SIZE * 2];
	} threads[MAX_THREAD_SIZE];
	uint32_t string_len   = strlen(string);
	uint32_t threads_len  = 0;
	uint32_t commands_len = regexp->commands_len;
	int     *commands     = regexp->commands;

	push_thread(0, 0);

	do {
		struct search_thread cur = threads[threads_len - 1];
		pop_thread();

		printf("thread pc:%u sp:%u\n", cur.pc, cur.sp);

		/* execute this thread */
		while (cur.pc < commands_len) {
			printf("(%d, %d): %d\n", cur.pc, cur.sp, commands[cur.pc]);
			switch (commands[cur.pc]) {
				case REGEXP_ANY_CHAR:
					if (cur.sp < string_len) {
						cur.pc++;
						cur.sp++;
					} else {
						cur.pc = (uint32_t) -1;
					}
					break;
				case REGEXP_CHAR:
					if (cur.sp < string_len && string[cur.sp] == (char) commands[cur.pc + 1]) {
						cur.pc += 2;
						cur.sp++;
					} else {
						cur.pc = (uint32_t) -1; /* exit this thread */
					}
					break;
				case REGEXP_GOTO:
					cur.pc += commands[cur.pc + 1];
					break;
				case REGEXP_SPLIT:
					push_thread(cur.sp, cur.pc + commands[cur.pc + 2]);
					cur.pc += commands[cur.pc + 1];
					break;
				case REGEXP_SAVE:
					printf("save %d %d\n", commands[cur.pc + 1], cur.sp);
					cur.sub_matches[commands[cur.pc + 1]] = cur.sp;
					cur.pc += 2;
					break;
				default:
					ERROR("REGEXP::match::Unknown command in commands list(pc: %d)\n");
					return 0; /* no match */
			}
		}

		if (cur.pc == commands_len) {
			memcpy(regexp->sub_matches, cur.sub_matches, sizeof(uint32_t) * regexp->sub_matches_len * 2);
			return 1;
		}
	} while (threads_len != 0);

	return 0; /* all threads executed and no thread returns positive */
}

int main()
{
#define ERR_RETURN(msg) ({printf("%s\n", (msg)); return 1; })

	regexp_t regexp;

	regexp_init(&regexp, ".*?");
	regexp_compile(&regexp);
	printf("-------\n");
	regexp_inspect(&regexp);
	printf("-------\n");
	printf(">>> res %d\n", regexp_match(&regexp, "ccccccc"));
	printf("-------\n");
	{
		uint32_t i;
		for (i = 0; i < regexp.sub_matches_len; i++) printf("%u ", regexp.sub_matches[i]);
		printf("\n");
	}


	/* get_matching_bracket */
	if (1) {
		char s1[] = "as(sdasd() asd())"; /* 2 8 9 14 15 16*/
		if (get_matching_bracket(s1, 2, strlen(s1) - 1, '(', ')') != 16) ERR_RETURN("0");
		if (get_matching_bracket(s1, 8, strlen(s1) - 1, '(', ')') != 9) ERR_RETURN("1");
		if (get_matching_bracket(s1, 14, strlen(s1) - 1, '(', ')') != 15) ERR_RETURN("2");
		if (get_matching_bracket(s1, 0, strlen(s1) - 1, '(', ')') != -1) ERR_RETURN("3");
	}

	return 0;
}