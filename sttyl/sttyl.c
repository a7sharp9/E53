/*
 * sttyl.c
 *
 *  Created on: Feb 22, 2018
 *      Author: Yuri
 */

#include "ttyutils.h"
#include <stdio.h>
#include <string.h>

/**
 * handle_args: for each argument (or a pair of arguments in case of a
 * control character or size/speed) attempt to parse and set the corresponding
 * flag in the incoming structures
 * args:
 * term_info - holder for flags, speed and control characters
 * size - holder for number of columns and rows
 * argc, argv - standard C main arguments
 * return:
 * 0 if all arguments parsed and set successfully, an error code otherwise
 */
int handle_args (struct termios *term_info, struct winsize *size,
								int argc, char *argv []) {
	char *prog_name = argv [0];
	int arg_idx;
	int ret = 0;

	for (arg_idx = 1; arg_idx < argc && !ret; arg_idx ++) {
		char *this_arg = argv [arg_idx];
		int char_idx = find_control_char_idx (this_arg); /* a control char? */
		int special_values = special_value_flag (this_arg); /* size/speed? */
		if (char_idx >= 0 || special_values) {
			/*the next argument is the value for the current flag */
			if (arg_idx < argc - 1) {
				ret = special_values ? /* if yes, setting size or speed */
					set_special_values (term_info, size, special_values,
															argv [++arg_idx]):
				/* otherwise, found a control char with this name */
					set_control_char (term_info, char_idx, argv [++arg_idx]);
				if (ret > 0) /* could not parse the argument */
					fprintf (stderr, "%s: invalid value for '%s'\n",
								prog_name, this_arg);
			} else { /* unexpected end of argument list */
				fprintf (stderr, "%s: missing value for '%s'\n",
							prog_name, this_arg);
				ret = 1;
			}
		} else { /* a standalone flag, possibly prepended with "-" */
			ret = set_flag (term_info, this_arg);
			if (ret) /* could not find such flag to set */
				fprintf (stderr, "%s: invalid argument '%s'\n",
							prog_name, this_arg);
		}
	}

	return (ret);
}

/**
 * main - accesses the terminal info; if no arguments, prints it out and exits,
 * otherwise passes the arguments and structures to handle_args (), then sets
 * the terminal info from the modified structures if no error
 * args:
 * argc, argv - standard C main arguments
 * return:
 * a non-0 status from a failed system call, or 1 from any of the parsing
 * utilities if cannot parse or cannot find correct value, or 0 if successful
 */
int main (int argc, char *argv []) {
	struct termios term_info;
	struct winsize size;

	int ret = tcgetattr (0, &term_info); /* get flags and control characters */
	if (ret) {
		perror ("could not read terminal settings");
	} else {
		ret = ioctl (0, TIOCGWINSZ, &size); /* get number of columns and rows */
		if (ret) {
			perror ("could not access ioctl");
		}
	}

	if (!ret) {
		if (argc < 2) /* no arguments - print out the settings and exit */ {
			print_settings (&term_info, &size);
			return (0);
		}
				/* otherwise, parse arguments and change the structures */
		ret = handle_args (&term_info, &size, argc, argv);
	}

	if (!ret) {
		ret = tcsetattr (0, TCSANOW, &term_info); /* write flags */
		if (ret)
			perror ("could not write terminal settings");
		else {
			ret = ioctl (0, TIOCSWINSZ, &size); /* write columns and rows */
			if (ret) {
				perror ("could not set ioctl");
			}
		}
	}

	return (ret);
}
