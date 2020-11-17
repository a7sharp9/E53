/* builtin.c
 * contains the switch and the functions for builtin commands
 */

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdlib.h>
#include 	<unistd.h>
#include	"smsh.h"
#include	"varlib.h"
#include	"builtin.h"
#include	"flexstr.h"

struct builtin_handler {
	char *cmd;
	void (*handler) (char **, int *);
} builtins [] = {
		{"cd", do_cd},
		{"export", do_export},
		{"set", do_list_vars},
		{"exit", do_exit},
		{"read", do_read},
		{NULL, NULL}
};

/**
 * find_handler - if there is an entry in the builtins [] table with the
 * same "cmd" key as the incoming string, return it
 * args:
 * cmd - the command string to lookup
 * return:
 * a pointer to the handler entry, or NULL if not found
 */
struct builtin_handler * find_handler (char *cmd) {
	struct builtin_handler *ret = builtins;
	while (ret->cmd != NULL) {
		if (!strcmp (ret->cmd, cmd))
			return (ret);
		ret ++;
	}

	return (NULL);
}

int is_builtin(char **args, int *resultp)
/*
 * purpose: run a builtin command 
 * returns: 1 if args[0] is builtin, 0 if not
 * details: test args[0] against all known builtins.  Call functions
 */
{
	if (is_assign_var(args[0], resultp)) // has to be done separately,
		return 1;	// because it checks not for name, but for "=" inside

	struct builtin_handler *handler_struct = find_handler (args[0]);
	if (handler_struct != NULL) { // this is a handled built-in
		(*handler_struct->handler) (args, resultp); // call the handler
		if (*resultp) { // report error
			fprintf (stderr, "can't %s\n", args[0]);
		}
		return (1); // this was a builtin, even though it could have failed
	}

	return 0; // not a builtin
}

/**
 * do_cd - handler for the cd builtin. Takes first token to be the directory
 * to change to; goes to ~ if no argument specified
 * args:
 * args - the parsed command line tokens
 * resultp - pointer to status, initialized from the result of the system call
 */
void do_cd (char **args, int *resultp) {
	char *cd_to = args [1];
	if (cd_to == NULL) { // just "cd" - go to ~
		cd_to = getenv ("HOME");
	}
	*resultp = chdir (cd_to);
}

/**
 * do_exit - handler for the exit builtin. Takes first token to be the
 * exit code; if no argument specified, exits with 0
 * args:
 * args - the parsed command line tokens
 * resultp - pointer to status, rather useless
 */
void do_exit (char **args, int *resultp) {
	int exit_code = 0;
	if (args [1] != NULL)
		exit_code = (int) strtol (args [1], NULL, 10); // parse exit code
	(*resultp) = exit_code; // not needed, but makes it not "unused arg")
	exit (exit_code); // die
}

#define MAX_READ_ARG_SIZE 1024
/**
 * do_read - handler for the read builtin. Takes first token to be the
 * name of the variable to read into; if it is a valid name, reads one line
 * from standard inout and stores it as the value of this variable
 * args:
 * args - the parsed command line tokens
 * resultp - pointer to status; initialized to 0 if successful, 1 otherwise
 */
void do_read (char **args, int *resultp) {
	char *var_name = args [1];
	*resultp = EXIT_FAILURE;

	if (okname (var_name)) { // this is a valid name
		char val [MAX_READ_ARG_SIZE];
		if (fgets (val, MAX_READ_ARG_SIZE, stdin)) { // read the value
			val [strcspn (val, "\n")] = 0; // replace EOL with 0 if present
			*resultp = VLstore (var_name, val); // store it
		}
	}
}

/* checks if a legal assignment cmd
 * if so, does it and retns 1
 * else return 0
 */
int is_assign_var(char *cmd, int *resultp)
{
	if (strchr(cmd, '=') != NULL) { // '=' in the string means assignment
		*resultp = assign(cmd);
		if (*resultp != -1)
			return 1;
	}
	return 0;
}

/**
 * do_list_vars - handler for the read builtin. Takes first token to be the
 * name of the variable to read into; if it is a valid name, reads one line
 * from standard input and stores it as the value of this variable
 * args:
 * args - the parsed command line tokens
 * resultp - pointer to status; initialized to 0 if successful, 1 otherwise
 */
void do_list_vars(char **args, int *resultp)
{
	VLlist();
	*resultp = 0;
	(void) args;
}

/**
 * do_export - handler for the export builtin. Takes first token to be the
 * name of the variable to export; if it is a valid name, tries to find
 * its value in the var table and export
 * args:
 * args - the parsed command line tokens
 * resultp - pointer to status; initialized to 0 if successful, 1 otherwise
 * note: if no name supplied, the result is also 1
 */
void do_export(char **args, int *resultp)
{
	*resultp = (args[1] != NULL && okname(args[1])) ? VLexport(args[1]) : 1;
}

int assign(char *str)
/*
 * purpose: execute name=val AND ensure that name is legal
 * returns: -1 for illegal lval, or result of VLstore 
 * warning: modifies the string, but restores it to normal
 */
{
	char	*cp;
	int	rv ;

	cp = strchr(str,'=');
	*cp = '\0';
	rv = (okname(str) ? VLstore(str,cp+1) : -1);
	*cp = '=';
	return rv;
}

int okname(char *str)
/*
 * purpose: determines if a string is a legal variable name
 * returns: 0 for no, 1 for yes
 */
{
	if (str == NULL)
		return (0);

	char	*cp;

	for(cp = str; *cp; cp++){
		if ((isdigit(*cp) && cp==str) || !(isalnum(*cp) || *cp=='_'))
			return 0;
	}
	return (cp != str);	/* no empty strings, either */
}


#define is_start_var(c) ((c) == '$')
#define is_escape(c) ((c) == '\\')
#define is_valid_varname_char(c) (isalnum (c) || (c) == '_')
#define is_single_varname_char(c) (isdigit (c) || (c) == '?' || (c) == '#')

/**
 * add_var - adds the value of the specified variable to the output string
 * does nothing if there is no such variable defined
 * args:
 * ret - the string to add the variable value to
 * var - the variable name
 * return:
 * 1 if variable was found and appended, 0 otherwise
 * Note: will also reinitialize the FLEXSTR contained in the second argument
 */
int add_var (FLEXSTR *ret, FLEXSTR *var) {
	int rv = 0;
	char *this_var = fs_getstr (var); // terminate
	char *subst = VLlookup (this_var); // and lookup
	if (subst != NULL) { // found it
		fs_addstr (ret, subst); // make the substitution
		rv = 1;
	} // otherwise add nothing

	fs_free (var);
	fs_init (var, 0); // reinit the variable name
	return (rv);
}

/**
 * varsub - takes a pointer to a string and replaces all instances of
 * any $<variable> in it with the values of these variables, if defined
 * also correctly handles escaped characters (including \\ and \$)
 * args:
 * cmd_ptr - pointer to the command line containing the variables
 * Note: all references to the previous character occur inside if-blocks
 * that check that we are parsing a variable name, so at least one
 * character ($) must have already been read, and the pointer will not
 * travel before the start of the line
 */
void varsub (char **cmd_ptr) {
	char *cur;
	FLEXSTR ret;
	FLEXSTR var;

	fs_init (&ret, 0);
	fs_init (&var, 0);
	int inside_var = 0; // if true, we are parsing variable name
	int inside_escape = 0; // if true, we are parsing an escaped character

	for (cur = *cmd_ptr; (*cur) != 0; cur ++) {
		if (is_escape (*cur)) { // found '\'
			if (inside_escape) // and the previous character was also '\'
				fs_addch (&ret, '\\'); // this is an escaped backslash
			inside_escape = !inside_escape; // if \\, escape sequence ended
			continue;						// otherwise it started
		}

		if (is_start_var (*cur) && !inside_escape) { // found $
			int dd = 0;
			if (inside_var) {	  // already tracking previous substitution
				dd = is_start_var (*(cur - 1)); // is this $$?
				if (dd)
					fs_addch (&var, *cur); // the variable name is '$'
				add_var (&ret, &var); // substitute what's in the var buffer
			}

			inside_var = !dd; // start tracking new variable name if not $$
		} else {
			if (inside_var) { // tracking variable name
				if (is_single_varname_char (*cur) && // [0..9] or special name
						is_start_var (*(cur - 1))) { // and previous was $
					fs_addch (&var, *cur); // this char is the name
					add_var (&ret, &var); // substitute
					inside_var = 0; // var name ended
				}
				else if (!is_valid_varname_char (*cur)) {
					inside_var = 0; // var name ended
					add_var (&ret, &var); // substitute what's in the var buffer
					fs_addch (&ret, *cur); // this character goes in as is
				} else {
					fs_addch (&var, *cur); // var name continues
				}
			} else {
				fs_addch (&ret, *cur); // this character goes in as is
			}
		}

		inside_escape = 0; // if we're here, then escape sequence ended
	}

	if (inside_var) {  // EOL, but var name hasn't ended yet
		add_var (&ret, &var); // substitute what's in the var buffer
	}

	free (*cmd_ptr);
	*cmd_ptr = fs_getstr (&ret);
}


