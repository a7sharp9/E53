#ifndef	BUILTIN_H
#define	BUILTIN_H

int is_builtin(char **args, int *resultp);
int is_assign_var(char *cmd, int *resultp);
void do_list_vars(char **args, int *resultp);
void do_cd (char **args, int *resultp);
void do_exit (char **args, int *resultp);
void do_read (char **args, int *resultp);
void do_export(char **, int *);

void varsub(char **args);
int assign(char *);
int okname(char *);

#endif
