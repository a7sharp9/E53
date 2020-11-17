/*
 * wordwrap.h
 *
 *  Created on: Feb 28, 2018
 *      Author: Yuri
 */

#ifndef WORDWRAP_H
#define WORDWRAP_H

#include <stdio.h>

void write_word_wrap (FILE *file, char *buf,
						char wrap_on, int wrap_after, unsigned short num_cols);

#endif /* WORDWRAP_H */
