/*
 * wtmputils.h
 *
 *  Created on: Feb 3, 2018
 *      Author: Yuri Machkasov
 */

#ifndef WTMPUTILS_H_
#define WTMPUTILS_H_

#include <utmp.h>

#define WTMP_BUF_SIZE 64

struct wtmp_buffer {
	struct utmp	wtrec [WTMP_BUF_SIZE];
	int			start_idx;
	int			num_filled;
	int			count_accessed;
	int			count_refilled;
};

void wtmp_open (const char *fname);

void wtmp_close ();

int wtmp_len ();

struct utmp *wtmp_getrec (int rec_idx);

void wtmp_stats (int a [2]);

void show_utrec (const struct utmp *rp);

#endif /* WTMPUTILS_H_ */
