/*
 * ttyutils.c
 *
 *  Created on: Feb 27, 2018
 *      Author: Yuri
 */

#include "ttyutils.h"
#include "wordwrap.h"

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>

struct flag_and_name {
	tcflag_t	value;
	char *		name;
};

static struct flag_and_name control_chars [] = { /* not bitwise, */
	{VINTR, "intr"},	/* corresponding to the termios.c_cc[] array */
	{VQUIT, "quit"},
	{VERASE, "erase"},
	{VKILL, "kill"},
	{VEOF, "eof"},
	{VEOL, "eol"},
	{VEOL2, "eol2"},
	{VSWTC, "swtch"},
	{VSTART, "start"},
	{VSTOP, "stop"},
	{VSUSP, "susp"},
	{VREPRINT, "rprnt"},
	{VWERASE, "werase"},
	{VLNEXT, "lnext"},
	{VDISCARD, "discard"},
	{0, NULL}
};

static struct flag_and_name term_speed [] = { /* not bitwise */
	{B0, "0"},			/* corresponding to the start of termios.c_cflag */
	{B50, "50"},
	{B75, "75"},
	{B110, "110"},
	{B134, "134"},
	{B150, "150"},
	{B200, "200"},
	{B300, "200"},
	{B600, "600"},
	{B1200, "1200"},
	{B1800, "1800"},
	{B2400, "2400"},
	{B4800, "4800"},
	{B9600, "9600"},
	{B19200, "19200"},
	{B38400, "38400"},
	{0, NULL} /* can use 0, even though it's already present -
	 	 	 	 no one looks up in this array by value */
};

static struct flag_and_name c_cflags [] = { /* bitwise */
	{CSTOPB, "cstopb"},	/* corresponding to the rest of termios.c_cflag */
	{CREAD, "cread"},
	{PARENB, "parenb"},
	{PARODD, "parodd"},
	{HUPCL, "hupcl"},
	{CLOCAL, "clocal"},
	{0, NULL}
};

static struct flag_and_name c_iflags [] = { /* bitwise */
	{IGNBRK, "ignbrk"},	/* corresponding to termios.c_iflag */
	{BRKINT, "brkint"},
	{IGNPAR, "ignpar"},
	{PARMRK, "parmrk"},
	{INPCK, "inpck"},
	{ISTRIP, "istrip"},
	{INLCR, "inclr"},
	{IGNCR, "igncr"},
	{ICRNL, "icrnl"},
	{IUCLC, "iuclc"},
	{IXON, "ixon"},
	{IXANY, "ixany"},
	{IXOFF, "ixoff"},
	{IMAXBEL, "imaxbel"},
	{IUTF8, "iutf8"},
	{0, NULL}
};

static struct flag_and_name c_oflags [] = { /* bitwise */
	{OPOST, "opost"},	/* corresponding to termios.c_iflag */
	{OLCUC, "olcuc"},
	{ONLCR, "onclr"},
	{OCRNL, "ocrnl"},
	{ONOCR, "onocr"},
	{ONLRET, "onlret"},
	{OFILL, "ofill"},
	{OFDEL, "ofdel"},
	{0, NULL}
};


static struct flag_and_name c_lflags [] = { /* bitwise */
	{ISIG, "isig"},		/* corresponding to termios.c_lflag */
	{ICANON, "icanon"},
	{ECHO, "echo"},
	{ECHOE, "echoe"},
	{ECHOK, "echok"},
	{ECHONL, "echonl"},
	{NOFLSH, "noflsh"},
	{TOSTOP, "tostop"},
	{IEXTEN, "iexten"},
	{-1, NULL}
};

struct flag_handler {
	struct flag_and_name *tbl;
	unsigned offset;	/* from the start of struct termios */
};

static struct flag_handler all_flags [] = {
		{c_iflags, offsetof (struct termios, c_iflag)},
		{c_lflags, offsetof (struct termios, c_lflag)},
		{c_oflags, offsetof (struct termios, c_oflag)},
		{c_cflags, offsetof (struct termios, c_cflag)},
		{NULL, 0}
};

/* constants for the attributes that have values */
#define IS_ROWS 1		/* get/set number of terminal rows */
#define IS_COLUMNS 2	/* get/set number of terminal columns */
#define IS_SPEED 3		/* get/set terminal output speed in baud */

/**
 * special_value_flag - give a command line option, returns one of the
 * IS_* flags if it corresponds to a special value name (size or speed)
 * args:
 * option - a command line option
 * return:
 *  an IS_* flag or 0 if this argument is not a special option
 */
int special_value_flag (char *option) {
	int special_values = 0;

	if (!strcmp (option, "speed"))
		special_values = IS_SPEED;
	else if (!strcmp (option, "rows"))
		special_values = IS_ROWS;
	else if (!strcmp (option, "columns"))
		special_values = IS_COLUMNS;

	return (special_values);
}

/**
 * format_one_char - writes a string corresponding to a control character
 * to the beginning of the supplied character buffer
 * args:
 * this_char - the control character to format
 * buf - the output buffer
 */
void format_one_char (unsigned char this_char, char *buf) {
	if (this_char == 0 || this_char == 0377) { /* NUL or EOF */
		sprintf (buf, "%s", "M-^?");
	} else if (this_char == 0177) { /* DEL */
		sprintf (buf, "%s", "^?");
	} else if (iscntrl (this_char)) { /* a real control character */
		sprintf (buf, "^%c", (this_char - 1 + 'A'));
	} else { /* it's already printable */
		sprintf (buf, "%c", this_char);
	}
}

/**
 * find_idx_by_name - checks the input array of flags for a flag with the
 * specified name
 * args:
 * tbl - the array of flags and their names to search (last element must
 * have NULL name)
 * name - the name to search for
 * return:
 * the index of this flag in the array, or -1 if not found
 */
int find_idx_by_name (struct flag_and_name *tbl, char const *name) {
	int tbl_idx;
	for (tbl_idx = 0; tbl [tbl_idx].name != NULL; tbl_idx ++) {
		if (!strcmp (tbl [tbl_idx].name, name))
			return (tbl_idx);
	}

	return (-1);
}

/**
 * find_control_char_idx - an externally visible wrapper around find_idx_by_name
 * that searches the array of special characters for a character with the
 * specified name
 * args:
 * name - the name of the control character to search for
 * return:
 * the index of this character in the array, or -1 if not found
 */
int find_control_char_idx (char *name) {
	return (find_idx_by_name (control_chars, name));
}

/**
 * flag_to_option - appends the name of the option indicated by the
 * entry in the flag lookup table if it is set in the bitwise value,
 * or the name prepended with "-" if it is not
 * args:
 *  buf - character buffer to append the value to
 *  this_flag - the pointer to flag value and name information
 *  value - the unsigned integer to check for this flag being set
 *  return:
 *  the number of characters appended
 */
int flag_to_option (char *buf, struct flag_and_name *this_flag,
								tcflag_t value) {
	char is_set = ((value & this_flag->value) == this_flag->value);
	sprintf (buf, "%s%s ", is_set ? "" : "-", this_flag->name);

	return (strlen (buf));
}

/**
 * print_control_chars - collect all control characters from the
 * lookup table and print their names and values to the standard
 * output
 * args:
 * term_info - holder of special character values
 * num_cols - the width of the current terminal
 */
void print_control_chars (struct termios *term_info, unsigned short num_cols) {
	char buf[1024]; /* the entire string */
	memset (buf, 0, sizeof (buf));

	int str_pos = 0;
	char char_buf [128]; /* one character and value */

	int char_idx;
	for (char_idx = 0; control_chars [char_idx].name != NULL; char_idx ++) {
							/* going through the lookup table */
		struct flag_and_name *this_flag = control_chars + char_idx;
		memset (char_buf, 0, sizeof (char_buf));

		format_one_char ((term_info->c_cc)[this_flag->value], char_buf);
		str_pos += sprintf (buf + str_pos, "%s = %s; ",
										this_flag->name, char_buf);
	}

	write_word_wrap (stdout, buf, ';', 1, num_cols); /* try wrapping on ';' */
}

/**
 * print_control_chars - collect all flags values from the
 * lookup table, determine if they are set from the supplied
 * bitwise integer, and print them to standard output
 * args:
 * tbl - lookup table containing flag names and bit values
 * value - bitwise collection to consult for set/unset status
 * num_cols - the width of the current terminal
 */

void print_flags (struct flag_and_name *tbl, tcflag_t value,
						unsigned short num_cols) {
	char buf[1024];
	memset (buf, 0, sizeof (buf));

	int tbl_idx;
	int str_pos = 0; /* will keep pointing at the position of next write */
	for (tbl_idx = 0; tbl [tbl_idx].name != NULL; tbl_idx ++) {
		str_pos += flag_to_option (buf + str_pos, tbl + tbl_idx, value);
	}

	write_word_wrap (stdout, buf, ' ', 0, num_cols);
}

/**
 * set_bit - a helper to set or unset a bitwise value in an integer
 * args:
 * value - pointer to the holder of bitwise flag
 * bit - a bit to set or unset
 * to_clear - if true, clear the flag; otherwise, set it
 */

void set_bit (tcflag_t *value, tcflag_t bit, int to_clear) {
	to_clear ?
		(*value = (*value) & ~bit):
		(*value = (*value) | bit);
}

/**
 * print_speed_rowcol - obtains the current values for output speed and
 * the size of the terminal and prints them out to standard output
 * args:
 * term_info - holder of speed information
 * size - holder of size information
 */
void print_speed_rowcol (struct termios *term_info, struct winsize *size) {
	char buf[1024];
	memset (buf, 0, sizeof (buf));

	speed_t speed = cfgetospeed (term_info);
	sprintf (buf, "Speed %s baud; ", /* get the speed value - */
			/* this is not in baud, but a B* constant */
			speed >= sizeof (term_speed) / sizeof (struct flag_and_name) - 1 ?
					"???" : term_speed [speed].name);
			/* speed beyond B38400 is not standard, so isn't handled */

	sprintf (buf + strlen (buf), "rows %d; columns %d; ", /* write size */
									size->ws_row, size->ws_col);
	write_word_wrap (stdout, buf, ';', 1, size->ws_col);
}

/**
 * set_flag - look up a flag by name in any of the lookup tables, then
 * set the corresponding bit in the field of struct termios where a value
 * with this name was found
 * args:
 * term_info - holder of speed information
 * option - the name of the bitwise flag
 * return: 0 if the bit with this name was found; 1 otherwise
 */

int set_flag (struct termios *term_info, char const *option) {
	int to_clear = 0;
	if (*option == '-') { /* this means that we should unset this bit */
		to_clear = 1;
		option ++; /* move to the beginning of the name */
	}

	int tbl_idx; /* which lookup table does this belong to */
	int idx = -1; /* of the bit in the table */
	for (tbl_idx = 0; all_flags [tbl_idx].tbl != NULL && idx < 0; tbl_idx ++) {
		struct flag_handler *this_tbl = all_flags + tbl_idx;
		idx = find_idx_by_name (this_tbl->tbl, option);
		if (idx >= 0) { /* a bit with this name exists in this table */
			tcflag_t *terminfo_field = (tcflag_t *) ((char *)(term_info) +
					this_tbl->offset); /* calculate which field to set the */
				/* bit in, using the offset from the start of struct termios */
			set_bit (terminfo_field, this_tbl->tbl [idx].value,
											to_clear);
		}
	}

	return (idx >= 0 ? 0 : 1);
}

/**
 * set_control_char - using the supplied string (of 1 or 2 characters - in the
 * former case, the character is taken to be the actual control char, in the
 * latter the first character must be '^', and the next needs to be an alpha
 * character, then the combination is read the same way as if the user typed
 * the control character on the terminal), set the element of the c_cc field
 * of struct termios at the supplied index to the resulting char
 * args:
 * term_info - holder of the control character field
 * char_idx - the index in the c_cc array
 * ctrl - a one- or two-character value for the control character
 * return: 0 if the input can be parsed correctly; 1 otherwise
 */
int set_control_char (struct termios *term_info, int char_idx, char *ctrl) {

	int ctrl_length = strlen (ctrl);
	if (ctrl_length > 2 || ctrl_length < 1 || /* too short or too long */
			(ctrl_length == 2 && *ctrl != '^'))
		return (1);

	char ctrl_char = *ctrl;
	if (ctrl_length == 2) { /* convert the "^?" notation to actual iscntrl () */
		char next_char = *(ctrl + 1);
		if (!isalpha (next_char)) /* will not be able to make an iscntrl () */
			return (1);
		ctrl_char = toupper (next_char) +1 - 'A'; /* handle lowercase as well */
	}

	term_info->c_cc[char_idx] = ctrl_char; /* set the corresponding character */
	return (0);
}

/**
 * set_size - set the column or row number in the size structure to the
 * number represented by incoming string
 * args:
 * size - holder of size information
 * special_values - a flag, either IS_COLUMNS or IS_ROWS, indicating
 * which property to set
 * option - a string representing the integer value
 * return:
 * 0 if the incoming string can be parsed as a short integer, 1 otherwise
 */
int set_size (struct winsize *size, int special_values, char *option) {
	if (sscanf (option, "%hd",
					special_values == IS_COLUMNS ?
						&(size->ws_col) : &(size->ws_row)) != 1)
		return (1);

	return (0);
}

/* set_speed - set the speed information in the struct termios to the
 * speed indicated by the incoming string. The value of the string can only
 * be one of the options listed in term_speed table; no intermediate values
 * are handled.
 * args:
 * term_info - holder of the speed information
 * option - the string representing one of the allowed speed values
 * return:
 * 0 if the value of option is one of the standard speeds; 1 otherwise
 */
int set_speed (struct termios *term_info, char *option) {
	int idx = find_idx_by_name (term_speed, option); /* check lookup table */
	if (idx < 0)
		return (1);

	return (cfsetospeed (term_info, term_speed[idx].value)); /* set speed */
}

/**
 * set_special_valies - sets the number of columns or rows, or the
 * output speed in baud, from the incoming string representing
 * the value of this option
 * args:
 * term_info - holder of the speed information
 * size - holder of the size information
 * return:
 * 0 if the string can be parsed as an integer (and, in case of speed,
 * the integer is also one of the standard speeds), 1 otherwise
 */
int set_special_values (struct termios *term_info, struct winsize *size,
							int special_values, char *option) {
	int ret = 0;

	switch (special_values) {
		case IS_ROWS:
		case IS_COLUMNS:
			ret = set_size (size, special_values, option);
			break;

		case IS_SPEED:
			ret = set_speed (term_info, option);
			break;
	}

	return (ret);
}

/**
 * print_settings - prints to the standard output the infomration about
 * the current terminal's speed, size, control characters, and bitwise flags
 * as they are set in the c_*flags fields of the struct termios
 * args:
 * term_info - holder of speed, control characters and flag values
 * size - holder of size information
 */
void print_settings (struct termios *term_info, struct winsize *size) {
	print_speed_rowcol (term_info, size); /* 1st line - speed, rows, columns */
	print_control_chars (term_info, size->ws_col); /* control characters */

	int tbl_idx; /* of a lookup table tied to one of the c_*flags fields */
	for (tbl_idx = 0; all_flags [tbl_idx].tbl != NULL; tbl_idx ++) {
		tcflag_t *terminfo_field = (tcflag_t *) ((char *) term_info +
											all_flags[tbl_idx].offset);
				/* calculate the offset of this field from the beginning of */
				/* struct termios */
		print_flags (all_flags[tbl_idx].tbl, *terminfo_field, size->ws_col);
						/* print flags from this field */
	}
}
