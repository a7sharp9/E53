/*
 * read.h
 *
 *  Created on: Apr 18, 2018
 *      Author: Yuri
 */

#ifndef READ_H_
#define READ_H_
#include <stdio.h>

#define	MAX_RQ_LEN	4096
#define	LINELEN		1024

char * full_hostname ();
int read_request (FILE *fp, char rq[], int rqlen);
char *readline (char *buf, int len, FILE *fp);

#endif /* READ_H_ */
