#ifndef	SPLITLINE_H
#define	SPLITLINE_H

#define	YES	1
#define	NO	0

#include <stddef.h>

char	*next_cmd();
char	**splitline(char *);
void	freelist(char **);
void	*emalloc(size_t);
void	*erealloc(void *, size_t);

#endif
