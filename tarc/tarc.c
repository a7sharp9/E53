/*
 * tarc.c
 *
 *  Created on: Feb 16, 2018
 *      Author: Yuri
 */

#include "tarutils.h"

/**
 * error_return - if passed a return code that is not 0,prints out the name
 * of the program and the message of abnormal termination
 *
 * args:
 * prog_name - the name of the program as called
 * ret_code - return code to be handled
 * return:
 * the same error code as passed in
 *
 */
int error_return (char *prog_name, int ret_code) {
	if (ret_code)
		fprintf (stderr,
				"%s: Exiting with failure status due to previous errors.\n",
				prog_name);
	return (ret_code);
}

/**
 * handle_argument_errors - checks that the name of the archive and at least
 * one file to be archived have been passed in the command line; prints
 * appropriate messages and returns an error code if this is not the case
 *
 * args:
 * prog_name - the name of the program as called
 * argc - the number of arguments on the command line
 * return:
 * TAR_ERR_*  error code if invalid arguments, 0 otherwise
 */
int handle_argument_errors (char *prog_name, int argc) {
	if (argc < 3) {
		if (argc < 2)
			fprintf (stderr, "%s: No archive specified.\n", prog_name);
		else
			fprintf (stderr, "%s: No files to archive.\n", prog_name);
		return (error_return (prog_name, 1));
	}

	return (0);
}

/**
 * archive_files - wrapper around the calls to write_file () for each
 * file specified on the command line; also appends 2 empty blocks to
 * the archive as required by the standard
 *
 * args:
 * prog_name - the name of the program as called
 * fd - the open file descriptor of the archive being created
 * num_files - the number of files or directories to be archived
 * file_names - the absolute or relative paths of the files to be archived
 * return:
 * 0 if no errors encountered; -1 otherwise
 */
int archive_files (char *prog_name,
		int fd, int num_files, char *file_names[]) {
	int arg_idx;
	int write_status = 0;
	for (arg_idx = 0; arg_idx < num_files; arg_idx ++) {
		int file_write_status = write_file (prog_name, fd,
												file_names [arg_idx]);
		if (!write_status)
			write_status = file_write_status; /* accumulate for return */
	}

	write_empty_block (fd, 2); /* 2 empty blocks required by the standard */

	return (write_status);
}

/**
 * main - parses the command line arguments, opens the archive for writing,
 * passes the descriptor and file paths to be archived to archive_files ()
 * return:
 * 0 if no errors encountered; -1 otherwise
*/
int main (int argc, char *argv[]) {
	char *prog_name = argv [0];

	int arg_err = handle_argument_errors (prog_name, argc);
	if (arg_err)
		return (arg_err);

	char *archive_name = argv [1];

	int fd = creat (archive_name, 0644);

	if (fd < 0) {
		fprintf (stderr, "%s: Cannot open the archive for writing.\n", prog_name);
		return (error_return (prog_name, 1));
	}

	int write_status = archive_files (prog_name, fd, argc - 2, argv + 2);

	close (fd);

	return (error_return (prog_name, write_status));
}

