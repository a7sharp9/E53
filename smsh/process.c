#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<string.h>
#include	<sys/wait.h>
#include	<limits.h>
#include	"smsh.h"
#include	"builtin.h"
#include	"varlib.h"
#include	"controlflow.h"
#include	"process.h"

#define LONGEST_INT_STR_SIZE ((CHAR_BIT * sizeof (int) - 1) / 3 + 2)

/* process.c
 * command processing layer: handles layers of processing
 * 
 * The process(char **arglist) function is called by the main loop
 * It sits in front of the do_command function which sits 
 * in front of the execute() function.  This layer handles
 * two main classes of processing:
 *	a) process - checks for flow control (if, while, for ...)
 * 	b) do_command - does the command by 
 *		         1. Is command built-in? (exit, set, read, cd, ...)
 *                       2. If not builtin, run the program (fork, exec...)
 *                    - also does variable substitution (should be earlier)
 */

/**
 * add_int_value_var - converts an integer value into a string and stores
 * it in the variable lookup table
 * args:
 * name - the variable name
 * int - the value to be stored
 */
void add_int_value_var (char *name, int value) {
	char buf[LONGEST_INT_STR_SIZE];
	snprintf (buf, LONGEST_INT_STR_SIZE, "%d", value);
	VLstore (name, buf);
}

int is_source_command (char *cmd) {
	return (!strcmp (cmd, "."));
}

int is_exec_command (char *cmd) {
	return (!strcmp (cmd, "exec"));
}

/**
 * add_script_args - store the first 10 tokens on the incoming line into
 * variables with names [0..9]; also puts the number of variables (excluding
 * $0) into $#
 * args:
 * script_args - parsed command line tokens
 * num_args - the number of tokens
 */
void add_script_args (char **script_args, int num_args) {
	int arg_idx;
	char digit_name[2];
	num_args = (num_args > 10 ? 10 : num_args); // discard the rest
	for (arg_idx = 0; arg_idx < num_args; arg_idx++) {
		sprintf (digit_name, "%d", arg_idx); // the index is the name
		VLstore (digit_name, script_args[arg_idx]);
	}

	add_int_value_var ("#", num_args - 1);
}

int process (char *args[])
/*
 * purpose: process user command: this level handles flow control and
 * two different ways of executing programs/scripts - "." and "exec"
 * returns: result of processing command
 *  errors: arise from subroutines, handled there
 */
{
	int rv = 0;

	int exec_from_source (char *, FILE *);
	extern char **environ;

	if (args[0] == NULL)
		rv = 0; // nothing to do
	else if (is_source_command (args[0])) { // found '.'
		if (args[1] != NULL) { // the first argument is the program/script
			rv = exec_from_source (NULL, fopen (args[1], "r"));
		}
	} else if (is_exec_command (args[0])) { // found "exec"
		environ = VLtable2environ (); //
		signal (SIGINT, SIG_DFL);
		signal (SIGQUIT, SIG_DFL);
		execvp (args[1], args + 1); // execute and die
	} else if (is_control_command (args[0]))
		rv = do_control_command (args); // handle control flow
	else if (ok_to_execute ())
		rv = do_command (args); // regular command
	return rv;
}

/*
 * do_command
 *   purpose: do a command - either builtin or external
 *   returns: result of the command
 *    errors: returned by the builtin command or from exec,fork,wait
 *
 */
int do_command (char **args) {
	void varsub (char **);
	int is_builtin (char **, int *);
	void add_int_value_var (char *, int);
	int rv;

	if (!is_builtin (args, &rv))
		rv = WEXITSTATUS (execute (args)); // handle the return from wait ()
							// in theory, should check for WIFEXITED
							// and handle WIFSIGNALED and WIFSTOPPED

	add_int_value_var ("?", rv); // store the exit code in $?
	return rv;
}

int execute (char *argv[])
/*
 * purpose: run a program passing it arguments
 * returns: status returned via wait, or -1 on error
 *  errors: -1 on fork() or wait() errors
 */
{
	extern char **environ; /* note: declared in <unistd.h>	*/
	int pid;
	int child_info = -1;

	if (argv[0] == NULL) /* nothing succeeds		*/
		return 0;

	if ( (pid = fork ()) == -1)
		perror ("fork");
	else if (pid == 0) {
		environ = VLtable2environ ();
		char *fmt = "cannot execute command %s";
		char msg[strlen (fmt) + strlen (argv[0])]; // 1 char too long, so what
		sprintf (msg, fmt, argv[0]);
		signal (SIGINT, SIG_DFL);
		signal (SIGQUIT, SIG_DFL);
		execvp (argv[0], argv);
		perror (msg);
		exit (1);
	} else {
		if (wait (&child_info) == -1)
			perror ("wait");
	}
	return child_info;
}
