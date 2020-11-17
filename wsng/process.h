/*
 * process.h
 *
 *  Created on: Apr 17, 2018
 *      Author: Yuri
 */

#ifndef PROCESS_H_
#define PROCESS_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void process_request (char *rq, FILE *fp);
char * modify_argument (char *arg, int len);
void respond (int fd);

#endif /* PROCESS_H_ */
