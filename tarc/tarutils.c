/*
 * tarutils.c
 *
 *  Created on: Feb 12, 2018
 *      Author: Yuri
 */


#include "tarutils.h"
#include "msgutils.h"

#include <tar.h>
#include <time.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define TAR_ERR_NOERR 0
#define TAR_ERR_CANNOT_STAT 1
#define TAR_ERR_CANNOT_OPEN 2
#define TAR_ERR_CANNOT_READDIR 3
#define TAR_ERR_NAME_TOO_LONG 4
#define TAR_ERR_TYPE_UNIMPLEMENTED 5

/* keep this array in sync with the constants defined above - they are used */
/* to index it */
static const char * const tar_err_message_formats [] = {
		"", 		/* no error */
		"%s: Cannot stat: Permission denied",
		"%s: Cannot open: Permission denied",
		"%s: Cannot savedir: Permission denied",
		"%s: Cannot archive: Name too long",
		"%s: Cannot archive: File type not handled",
};

struct file_info {
	char abs_path[PATH_MAX]; /* for file operations */
	char rel_path[PATH_MAX]; /* for the header name */
	char is_abs_path;		/* whether tar was called on absolute path */
};

/**
 * convert_file_mode - converts the bitmask file attribute (from struct stat)
 * into a character flag defined in tar.h
 *
 * args:
 * mode - attribute bitmask
 * return:
 * one of *TYPE types from tar.h, or -1 if the type is not handled
 */
char convert_file_mode (int mode) {
	char ret = -1;
	if (S_ISREG (mode))
		ret = REGTYPE;
	else if (S_ISDIR (mode))
		ret = DIRTYPE;
	else if (S_ISFIFO (mode))
		ret = FIFOTYPE;
	else if (S_ISLNK (mode))
		ret = SYMTYPE;

	return (ret);
}

/**
 * calculate_block_checksum - arithmetically adds the first BLOCKSIZE
 * characters in a buffer
 *
 * args:
 * buf - a buffer of at least BLOCKSIZE characters
 * return:
 * an integer sum of all character values
 */
unsigned calculate_block_checksum (char *buf) {
	int idx;
	int ret = 0;
	for (idx = 0; idx < BLOCKSIZE; idx ++) {
		ret += buf [idx];
	}

	return (ret);
}

/**
 * return_with_msg - handles TAR_ERR_* error codes, adding appropriate messages
 * to the messaging system if an error is indicated
 * the format of the message is taken from an array that is keyed by the
 * error code, and each of the available formats takes one %s argument -
 * the path of the offending file
 *
 * args:
 * err - a TAR_ERR_ return code or 0
 * file_path - a file path to report the error on
 * return:
 * the same return code that's passed as a parameter
 */
int return_with_msg_path (int err, char *file_path) {
	char msg [PATH_MAX];
	if (err) {
		sprintf (msg, tar_err_message_formats [err], file_path);
		add_message (msg);
	}
	return (err);
}

/**
 * return_with_msg_path - wrapper around return_with_msg () that passes
 * the correct path (absolute or relative) to it
 *
 * args:
 * err - as above
 * file_path - a file_info struct containing tha paths to report the error on
 * return:
 * as above
 */
int return_with_msg (int err, struct file_info *file_path) {
	return (return_with_msg_path (err, file_path->is_abs_path ?
								file_path->abs_path:
								file_path->rel_path));
}

/**
 * format_symlink_name - if the file path points to a symlink, follows it
 * and writes the name of the file it links to into the appropriate field
 * in the ustar header, after checking that it is not too long; otherwise,
 * does nothing
 *
 * args:
 * buf - the beginning of the ustar header buffer
 * path - path to a file
 * mode - file type as defined in tar.h
 * return: TAR_ERR_NAME_TOO_LONG if the path fof the linked-to file is
 * too long; 0 otherwise
 */
int format_symlink_name (char *buf, struct file_info *path, int mode) {
	if (mode == SYMTYPE) {
		char	link_name [PATH_MAX];
		int link_length = readlink (path->abs_path, link_name, PATH_MAX);
		if (link_length > TAR_NAME_MAX_LENGTH)
			return (return_with_msg_path (TAR_ERR_NAME_TOO_LONG, link_name));

		sprintf (buf + 157, "%s", link_name);
	}

	return (0);
}

/**
 * format_file_stats - fills in the parts of the ustar header that are related
 * to the stat information about a file
 *
 * args:
 * buf - the beginning of the ustar header buffer
 * stat_buffer - a pointer to a struct stat with file information
 * mode - file type as defined in tar.h
 */
void format_file_stats (char *buf, struct stat *stat_buffer, char mode) {
	sprintf (buf + 100, "%07o", (stat_buffer->st_mode & ACCESSPERMS));
								/* masking just the access bits */
	sprintf (buf + 108, "%07o", ((unsigned) stat_buffer->st_uid));
								/* user id */
	sprintf (buf + 116, "%07o", ((unsigned) stat_buffer->st_gid));
								/* group id */
	sprintf (buf + 124, "%011o", (mode == REGTYPE) ?
									((unsigned) stat_buffer->st_size) : 0);
						/* size; 0 for anything except regular file */
	sprintf (buf + 136, "%011o", ((unsigned) stat_buffer->st_mtim.tv_sec));
					/* time of last modificaton in seconds since UNIX age */
	struct passwd *userpwd = getpwuid (stat_buffer->st_uid);
	sprintf (buf + 265, "%s", userpwd->pw_name); /* decipher user name */
	struct group *usergrp = getgrgid (stat_buffer->st_gid);
	sprintf (buf + 297, "%s", usergrp->gr_name); /* decipher group name */
}

/**
 * format_ustar_magic - formats the "magic" field and version numbers
 * in the ustar header
 *
 * args:
 * buf - the beginning of the ustar header buffer
 */
void format_ustar_magic (char *buf) {
	sprintf (buf + 257, "%s", TMAGIC);	/* the word "ustar" */
	buf [263] = buf [264] = '0';

	sprintf (buf + 329, "%s", "0000000"); /* major version */
	sprintf (buf + 337, "%s", "0000000"); /* minor version */
}

/**
 * format_file_name - add the name of the file to the top of the header block,
 * except in case of "/", where the name is written as "\0/"
 *
 * args:
 * buf - the header block
 * rel_path - the path to the file
 */
void format_file_name (char *buf, char *rel_path) {
	if (strlen (rel_path) == 0) { /* trying to tar root, special case */
		*buf = 0;
		*(buf + 1) = '/';
	} else {
		sprintf (buf, "%-s", rel_path);
	}
}

/**
 * format_header_block - fills out ustar-standard tar header for the incoming
 * file into the provided buffer
 *
 * args:
 * buf - the buffer of at least BLOCKSIZE characters
 * path - the relative (to program invocation) and absolute path to the file
 * being archived
 * return:
 * 0 if successful, one of the TAR_ERR_* constants otherwise
 */
int format_header_block (char *buf, struct file_info *path) {
	if (strlen (path->rel_path) > TAR_NAME_MAX_LENGTH) /* not more than 100 */
		return (return_with_msg (TAR_ERR_NAME_TOO_LONG, path));

	memset (buf, 0, BLOCKSIZE);

	struct stat stat_buffer;
	if (lstat (path->abs_path, &stat_buffer)) /* use lstat - we are concerned */
									/* with the attributes of the file itself */
		return (return_with_msg (TAR_ERR_CANNOT_STAT, path));

	char mode = convert_file_mode (stat_buffer.st_mode);
	if (mode < 0)	/* unknown file type - not handled */
		return (return_with_msg (TAR_ERR_TYPE_UNIMPLEMENTED, path));

	format_file_name (buf, path->rel_path);			/* add file path */
	format_file_stats (buf, &stat_buffer, mode);	/* add stat fields */
	buf [156] = mode;								/* add *TYPE type */
	int ret = format_symlink_name (buf, path, mode);/* if symlink, add */
	if (ret)										/* real file name */
		return (ret);

	format_ustar_magic (buf);						/* add magic and versions */
	memset (buf + 148, ' ', 8);						/* blank out the checksum */
	int checksum = calculate_block_checksum (buf);	/* calculate the checksum */
	sprintf (buf + 148, "%06o", checksum);			/* add the checksum */

	return (0);

}

/**
 * write_header_block - a wrapper to write out the supplied header block to
 * the archive file descriptor
 *
 * args:
 * into_fd - the open file descriptor of the archive
 * header_block - fully formed header
 */
void write_header_block (int into_fd, char *header_block) {
	write (into_fd, header_block, BLOCKSIZE);
}

/**
 * write_contents - write out the header block and the contents of the
 * supplied file into the archive file descriptor in blocks
 *
 * args:
 * into_fd - the open file descriptor of the archive
 * path - the structure containing relative and absolute path to the file
 * header_block - fully formed header
 * return:
 * TAR_ERR_CANNOT_OPEN if file unreadable, 0 otherwise
 */
int write_contents (int into_fd, struct file_info *path, char *header_block) {
	int from_fd = open (path->abs_path, O_RDONLY);
	if (from_fd < 0)			/* not even header for unreadable files */
		return (return_with_msg (TAR_ERR_CANNOT_OPEN, path));

	write_header_block (into_fd, header_block);	/* write the header */

	char buf [BLOCKSIZE];
	int num_read;
	while ((num_read = read (from_fd, buf, BLOCKSIZE)) > 0) {
									/* read contents in block-sized buffers */
		if (num_read < BLOCKSIZE)	/* buffer not full - pad with 0 bytes */
			memset (buf + num_read, 0, BLOCKSIZE - num_read);
		write (into_fd, buf, BLOCKSIZE); /* write this block */
	}

	close (from_fd);
	return (0);
}

/**
 * write_empty_block - a wrapper to dump empty blocks into the archive file
 * descriptor
 *
 * args:
 * into_fd - the open file descriptor of the archive
 * how_many - the number of empty blocks to be added
 */
void write_empty_block (int fd, int how_many) {
	char buf [BLOCKSIZE];
	memset (buf, 0, BLOCKSIZE);
	int idx;
	for (idx = 0; idx < how_many; idx ++) {
		write (fd, buf, BLOCKSIZE);
	}
}

/**
 * ensure_slash - for a directory, makes sure that the path has a trailing
 * slash, and adds it in place if it doesn't
 *
 * args:
 * path - a path to a directory
 */
void ensure_slash (char *path) {
	char *path_end = path + strlen (path);

	if (*(path_end - 1) != '/') {
		*path_end = '/';
		*(path_end + 1) = 0;
	}
}

int write_file_from_path (int into_fd, struct file_info *path); /* signature */

/**
 * add_dir_entry - adds the file name to both absolute and relative paths
 * indicated in the supplied struct file_info
 *
 * args:
 * new_path - the holder of relative and absolute path
 * entry_name - the file name to be added
 */
void add_dir_entry (struct file_info *new_path, char *entry_name) {
	sprintf (new_path->abs_path + strlen (new_path->abs_path),
								"%s", entry_name);
	sprintf (new_path->rel_path + strlen (new_path->rel_path),
								"%s", entry_name);
}

/**
 * write_dir_contents - descends recursively into a directory depth-first, and
 * writes out entries corresponding to the contents
 *
 * args:
 * into_fd - the open file descriptor of the archive
 * path - the holder of relative and absolute path
 * header_block - fully formed header for the directory itself
 * return: 0 if successful; the first TAR_ERR_* code encountered otherwise
 */
int write_dir_contents (int into_fd,
			struct file_info *path, char *header_block) {
	int ret = 0;

	DIR *dir_ptr = opendir (path->abs_path); /* try reading contents */
	if (dir_ptr != NULL) {
		struct dirent *dir_entry = malloc (sizeof (struct dirent));
		struct dirent *dir_flag = NULL;
		if (readdir_r (dir_ptr, dir_entry, &dir_flag) ||
								dir_flag == NULL) /* unreadable directory */
			return (return_with_msg (TAR_ERR_CANNOT_READDIR, path));

		write_header_block (into_fd, header_block);	/* write header */
		do {										/* descend */
			char *entry_name = dir_entry->d_name;
			if (strcmp (entry_name, ".") && strcmp (entry_name, "..")) {
				struct file_info new_path; /* to avoid removing on the way up */
				memcpy (&new_path, path, sizeof (struct file_info));
				add_dir_entry (&new_path, entry_name); /* form file paths */
				int recursive_ret = write_file_from_path (into_fd, &new_path);
														/* write this entry */
				if (!ret)
					ret = recursive_ret; /* to report an error to the caller */
			}
		} while (!readdir_r (dir_ptr, dir_entry, &dir_flag) && 
												dir_flag != NULL);
		free (dir_entry);
	} else {
		/* unreadable directory */
		return (return_with_msg (TAR_ERR_CANNOT_OPEN, path));
	}

	return (ret); /* the messages have been added, just report the error */
}

/**
 * write_file_from_path - given the absolute and relative paths to a file,
 * write into the archive file descriptor the header and the contents (if any)
 * of this file, recursively descending into contents of directories
 * writes out entries corresponding to the contents
 * args:
 * into_fd - the open file descriptor of the archive
 * path - the holder of relative and absolute path
 * return:
 * 0 if successful; the first TAR_ERR_* code encountered otherwise
  */
int write_file_from_path (int into_fd, struct file_info *path) {
	struct stat stat_buffer;
	if (lstat (path->abs_path, &stat_buffer)) /* unstatable - can't write */
		return (return_with_msg (TAR_ERR_CANNOT_STAT, path));

	char file_type = convert_file_mode (stat_buffer.st_mode);
	if (file_type == DIRTYPE) { /* directories must have trailing slashes */
		ensure_slash (path->abs_path);
		if (strcmp (path->abs_path, "/"))
			ensure_slash (path->rel_path);
	}

	char header_block [BLOCKSIZE];
	int ret = format_header_block (header_block, path); /* ustar header */
	if (!ret) {
		switch (file_type) {
			case DIRTYPE:	/* recursively write the contents */
				ret = write_dir_contents (into_fd, path, header_block);
				break;
			case REGTYPE:	/* write out the contents of the file in blocks */
				ret = write_contents (into_fd, path, header_block);
				break;
			default:		/* no contents - just write the header */
				write_header_block (into_fd, header_block);
				break;
		}
	}

	return (ret); /* the messages have been added, just report the error */
}

/**
 * init_file_paths - given the path to a file to be archived, initializes the
 * holder of an absolute and relative paths to it. If the supplied path is
 * absolute (starts with /), strips the leading slashes for the relative path
 * and prints out a message to that effect; otherwise, appends the path
 * to the absolute path of the current directory.
 * args:
 * prog_name - the string corresponding to the call path of current program,
 * if needed to report of stripping of leading slashes or "../"
 * path - the holder of relative and absolute path to be initialized
 * fname - the incoming file path
 * return:
 * the length of the incoming file name without leading slashes
 *
 */
void init_file_paths (char *prog_name, struct file_info *fpath, char *fname) {
	memset (fpath->abs_path, 0, sizeof (fpath->abs_path));
	memset (fpath->rel_path, 0, sizeof (fpath->rel_path));

	if (*fname == '/') { /* this is an absolute path */
		memcpy (fpath->abs_path, fname, strlen (fname)); /* goes in unchanged */
		fprintf (stdout,
					"%s: Removing leading '/' from member names\n", prog_name);
		for (; *fname == '/'; fname ++) {} /* strip leading slashes */
		memcpy (fpath->rel_path, fname, strlen (fname));
		fpath->is_abs_path = (char) 1;
	} else {
		getcwd (fpath->abs_path, sizeof (fpath->abs_path));
		sprintf (fpath->abs_path + strlen (fpath->abs_path), "/%s", fname);
		char already_printed_msg = 0;
		for (; !strncmp (fname, "../", 3); fname += 3) { /* strip all "../" */
			if (!already_printed_msg) {
				fprintf (stdout,
				"%s: Removing leading '../' from member names\n", prog_name);
				already_printed_msg = 1;
			}
		}
		memcpy (fpath->rel_path, fname, strlen (fname));
		fpath->is_abs_path = (char) 0;
	}
}

/**
 * write_file - top-level function, for each file passed in adds the entry for
 * it (and the full hierarchy of entries below it if a directory) to the
 * specified archive file descriptor. If encounters errors, collects them
 * in the messaging system, then appends to the standard error
 * args:
 * prog_name - the string corresponding to the call path of current program,
 * to report of errors or warnings
 * fd - the open file descriptor of the archive
 * fname - the incoming file path; can be relative or absolute
 * return:
 * 0 if all entries in the hierarchy were written out successfully; -1
 * otherwise
 *
 */
int write_file (char *prog_name, int fd, char *fname) {
	if (fname == NULL) {
		fprintf (stderr, "%s: NULL file name?\n", prog_name);
		return (-1);
	}

	struct file_info fpath;
	init_file_paths (prog_name, &fpath, fname);
							/* construct absolute and relative path */

	init_messaging ();		/* allocate and initialize messaging buffer */

	int ret = write_file_from_path (fd, &fpath); /* write the file */
								 /* or recursively write the directory*/

	int msg_idx = 0;
	int num_msgs = get_num_messages (); /* check if any errors were reported */
	for (msg_idx = 0; msg_idx < num_msgs; msg_idx ++)
		fprintf (stderr, "%s: %s\n", prog_name, get_message (msg_idx));

	shutdown_messaging ();	/* free the messaging buffer */

	return (ret ? -1 : 0);
}


