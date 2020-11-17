#ifndef	PROCESS_H
#define	PROCESS_H

int process(char **args);
int do_command(char **args);
int execute(char **args);
void add_script_args (char **script_args, int num_args);
void add_int_value_var (char *name, int value);

#endif
