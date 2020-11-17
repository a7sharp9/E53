/* controlflow.c
 * "if" processing is done with two state variables
 * if_state and if_result
 */
#include <stdio.h>
#include <string.h>
#include "smsh.h"

enum states {
	NEUTRAL, WANT_THEN, THEN_BLOCK, ELSE_BLOCK
};

enum cmdtype {
	NONE, IF, THEN, ELSE, FI
};

static struct cmd {
	char *cmd;
	enum cmdtype code;
} commands[] = {
		{ "if", IF },
		{ "then", THEN },
		{ "else", ELSE },
		{ "fi", FI },
		{ 0, 0 }
};

/**
 * get_control_command: if there is a controlflow command with the specified
 * name, returns its code; otherwise, returns 0
 * args:
 * cmd - the command name
 * return:
 * one of the enum cmdtype constants (NONE if there is no such command)
 */
static enum cmdtype get_control_command (char *cmd) {
	struct cmd *cur_cmd = commands;
	if (cmd != 0)
		for (cur_cmd = commands; cur_cmd->cmd != 0; cur_cmd ++)
			if (!strcmp (cur_cmd->cmd, cmd))
				return (cur_cmd->code);

	return (NONE);
}

enum results {
	SUCCESS, FAIL
};

static enum states if_state = NEUTRAL;
static enum results if_result = SUCCESS;
static int last_stat = 0;

int syn_err (char *);

/* purpose: determine to the shell should execute a command
 * returns: 1 for yes, 0 for no
 * details:
 * 			if in THEN_BLOCK and if_result was SUCCESS then yes
 *          if in THEN_BLOCK and if_result was FAIL then no
 * 			if in ELSE_BLOCK and if_result was SUCCESS then no
 *          if in ELSE_BLOCK and if_result was FAIL then yes
 *          if in WANT_THEN then syntax error (sh is different)
 */
int ok_to_execute () {
	int rv = 1; //default is ok

	switch (if_state) {
		case WANT_THEN: // the only thing that can be here is "then"
			syn_err ("then expected");
			rv = 0;
		break;

		case THEN_BLOCK: // execute only if the condition in "if" was true
			rv = (if_result == SUCCESS);
			break;

		case ELSE_BLOCK: // execute only if the condition in "if" was false
			rv = (if_result == FAIL);
		break;

		case NEUTRAL: // we're not inside if-then-else-fi
		break;
	}

	return rv;
}

/**
 * is_control_command - checks the name to see if it is a controlflow
 * command
 * args:
 * s - command name
 * return:
 * 0 if it is not a controlflow command; not-0 otherwise
 */
int is_control_command (char *s) {
	return (get_control_command (s) != NONE);
}

/**
 * is_neutral - checks if we are inside an if-then-else-fi block
 * return: 0 if in the block, not-0 otherwise
 */
int is_neutral () {
	return (if_state == NEUTRAL);
}

/**
 * do_control_command - perform the control flow handling.
 * args:
 * args - the parsed command line tokens, with the first token representing
 * the command
 */
int do_control_command (char ** args) {
	int process (char **);
	char *cmd = args[0];
	int rv = -1;

	switch (get_control_command (cmd)) {
		case IF:
			if (if_state != NEUTRAL) // not handling nested ifs
				rv = syn_err ("if unexpected");
			else {
				last_stat = process (args + 1); // check the condition
				if_result = (last_stat == 0) ? SUCCESS : FAIL; // store it
				if_state = WANT_THEN; // next line must contain "then"
				rv = 0;
			}
		break;

		case THEN:
			if (if_state != WANT_THEN)
				rv = syn_err ("then unexpected");
			else {
				if_state = THEN_BLOCK; // start then block
				rv = 0;
			}
		break;

		case ELSE:
			if (if_state != THEN_BLOCK)
				rv = syn_err ("else unexpected");
			else {
				if_state = ELSE_BLOCK; // start else block
				rv = 0;
			}
		break;

		case FI:
			if (if_state != THEN_BLOCK && if_state != ELSE_BLOCK)
				rv = syn_err ("fi unexpected");
			else {
				if_state = NEUTRAL; // end control flow handling
				rv = 0;
			}
		break;

		default:
		break;
	}

	return rv;
}

int syn_err (char* msg) {
	if_state = NEUTRAL;
	fprintf (stderr, "syntax error: %s\n", msg);
	return -1;
}
