/*
 * wordwrap.c
 *
 *  Created on: Feb 28, 2018
 *      Author: Yuri
 */

#include <stdio.h>

/**
 * write_word_wrap: a simple in-place word wrapper that replaces either a
 * a specified character or the one after it, depending on the flag, with a
 * carriage return if the next word is expected to make the current line longer
 * than the specified number of columns
 * args:
 * file - a file structure to write the output to
 * buf - the string to wrap (null-terminated)
 * wrap_on - the character on which to wrap
 * wrap_after - if true, insert the CR after the found wrap_on character
 * num_cols - the width to try and not exceed
 *
 * note: does nothing if the length of a single word is more than num_cols
 */
void write_word_wrap (FILE *file, char *buf,
						char wrap_on, int wrap_after, unsigned short num_cols) {
	int forward_idx;
	int back_idx;
    int last_wrap = 0; /* index of last place where we wrapped */
	int cur_line = 0;  /* index from the start of current line */

    for (forward_idx = 0; buf[forward_idx] != '\0'; forward_idx ++) {
    	int wrapped = 0; /* flag to indicate that we should wrap here */
        if (cur_line >= num_cols) {/* length of this line exceeds num_cols */
            for (back_idx = forward_idx; /* travel back */
            		back_idx > 0 && !wrapped; /* until we hit the character */
            		back_idx --) {       /* on which to wrap */
            	wrapped = (buf[back_idx] == wrap_on && /* end of last word? */
            				back_idx - last_wrap <= num_cols);

                if (wrapped) { /* found it */
                	buf[wrap_after ? ++back_idx : back_idx] = '\n';
                								/* replace with CR */
                	last_wrap = back_idx + 1;   /* mark start of next line */
                }
            }
        }
        cur_line = forward_idx - last_wrap; /* from start of this line */
    }

 	fprintf (file, "%s\n", buf); /* print out the resulting string */
}

