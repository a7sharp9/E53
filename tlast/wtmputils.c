/*
 * wtmputils.c
 *
 *  Created on: Feb 3, 2018
 *      Author: Yuri Machkasov
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "wtmputils.h"

static int fd = -1;

static struct wtmp_buffer wtmp_buf;

void init_buffer () {
	wtmp_buf.num_filled = 0;
	wtmp_buf.start_idx = 0;
	wtmp_buf.count_accessed = wtmp_buf.count_refilled = 0;
}
void wtmp_open (const char *fname) {
	fd = open (fname, O_RDONLY );
	init_buffer ();
}

void wtmp_close () {
	if (fd != -1)
		close (fd);
}

int wtmp_len () {
	if (fd != -1) {
		struct stat fstats;
		fstat (fd, &fstats);
		return (fstats.st_size / sizeof (struct utmp));
	}

	return (-1);
}

struct utmp *wtmp_refill (int from_idx) {
	if (from_idx < 0)
		from_idx = 0;	/* check here instead of from the caller */

	struct utmp *return_rec = NULL;

	wtmp_buf.count_refilled ++;	/* keeping statistics */

	off_t seek_to = lseek (fd,
				from_idx * sizeof (struct utmp),
				SEEK_SET);

	if (seek_to >= 0) {
		int num_read = read (fd, wtmp_buf.wtrec,
				WTMP_BUF_SIZE * sizeof (struct utmp));
		if (num_read > 0) {
			wtmp_buf.start_idx = from_idx;
			wtmp_buf.num_filled = num_read / sizeof (struct utmp);
			return_rec = wtmp_buf.wtrec;
		}
	}

	return (return_rec);
}

struct utmp *wtmp_getrec (int rec_idx) {
	wtmp_buf.count_accessed ++;
	if (wtmp_buf.start_idx >= 0 && 			/* the buffer is initialized */
		wtmp_buf.start_idx <= rec_idx &&	/* and we are within the */
		rec_idx < 							/* range of indices */
			wtmp_buf.start_idx + wtmp_buf.num_filled) {
		return (wtmp_buf.wtrec + (rec_idx - wtmp_buf.start_idx));
	} else {
		return (wtmp_refill (rec_idx - WTMP_BUF_SIZE / 2));
											/* refill the buffer on both */
											/* sides of the current index */
	}
}

void wtmp_stats (int a [2]) {
	a [0] = wtmp_buf.count_accessed;
	a [1] = wtmp_buf.count_refilled;
}

