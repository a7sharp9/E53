/*
 * ttyutils.h
 *
 *  Created on: Feb 27, 2018
 *      Author: Yuri
 */

#ifndef TTYUTILS_H
#define TTYUTILS_H

#include <sys/ioctl.h>
#include <termios.h>

int find_control_char_idx (char *control_char);

int set_control_char (struct termios *term_info, int char_idx, char *value);

int set_flag (struct termios *term_info, char const *option);

int special_value_flag (char *option);

int set_special_values (struct termios *term_info, struct winsize *size,
						int special_values, char *option);

void print_settings (struct termios *term_info, struct winsize *size);

#endif /* TTYUTILS_H */
