/*
 * smsh.c
 *
 *  Created on: Apr 2, 2018
 *      Author: Yuri
 */

#include "smsh.h"
#include	"splitline.h"
#include	"varlib.h"
#include	"process.h"
#include	"flexstr.h"
#include	"controlflow.h"
#include	"builtin.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

/**
 * exec_from_source - reads commands one by one from the file descriptor and
 * executes them one by one. If prompt is supplied, prints it to the
 * standard out before processing each new command.
 * args:
 * prompt: a string to be printed before accepting a new command, or NULL
 * input_source: an opened file descriptor to read the commands from
 */
int exec_from_source (char *prompt, FILE *input_source) {
	char *cmdline;
	int result;
	char **arglist;

	while ((cmdline = next_cmd (prompt, input_source)) != 0) {
		varsub (&cmdline); // search for variable names and substitute values
		if ((arglist = splitline (cmdline)) != 0) { // parse arguments
			result = process (arglist); // perform the command
			freelist (arglist);
		}

		free (cmdline); // t
	}

	if (!result && !is_neutral ()) {
		result = 1;
		fprintf (stderr, "%s\n", "expected fi");
	}

	return (result);
}

/**
 * main - sets up the environment, ignores intr/kill signals, determines
 * the source of commands, calls exec_from_source
 */
int main (int argc, char *argv[]) {
	char *prompt = NULL;
	FILE *input_source = stdin;

	setup (); // read environment
	add_int_value_var ("$", getpid ()); // put process id into $$
	add_int_value_var ("#", 0); // put 0 into number of arguments
	if (argc > 1) {
		input_source = fopen (argv[1], "r"); // executing a script
		add_script_args (argv + 1, argc - 1); // read args into $[0..9]
	} else
		prompt = DFL_PROMPT; // interactive mode

	int ret = exec_from_source (prompt, input_source); // read and exec commands
	if (argc > 1 && input_source != 0)
		fclose (input_source);

	return (ret);
}


