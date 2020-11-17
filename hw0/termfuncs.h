#ifndef	TERMFUNCS_H
#define	TERMFUNCS_H

#include <stdio.h>

/*
 * declarations for code in termfuncs.c
 */

int get_term_size( int rows_cols[2] );
int rawgetc(FILE *f);


#endif
