/*
 * tarutils.h
 *
 *  Created on: Feb 16, 2018
 *      Author: Yuri
 */

#ifndef TARUTILS_H
#define TARUTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "msgutils.h"

#define BLOCKSIZE 512

#define MSG_SIZE 1024

#define TAR_NAME_MAX_LENGTH 100

#define TAR_ERR_CANNOT_STAT 1
#define TAR_ERR_CANNOT_OPEN 2
#define TAR_ERR_CANNOT_READDIR 3
#define TAR_ERR_NAME_TOO_LONG 4
#define TAR_ERR_TYPE_UNIMPLEMENTED 5

static const char * const tar_err_message_formats [] = {
		"", 		/* no error; to keep the array indices in sync */
		"%s: Cannot stat: Permission denied",
		"%s: Cannot open: Permission denied",
		"%s: Cannot savedir: Permission denied",
		"%s: Cannot archive: Name too long",
		"%s: Cannot archive: File type not handled",
};

int write_file (char *prog_name, int fd, char *fname);

void write_empty_block (int fd, int how_many);

#endif /* TARUTILS_H */
