/*
 * msgutils.h
 *
 *  Created on: Feb 18, 2018
 *      Author: Yuri
 */

#ifndef MSGUTILS_H
#define MSGUTILS_H

#include <stdlib.h>

void init_messaging ();

void shutdown_messaging ();

int get_num_messages ();

char *get_message (int idx);

void add_message (char *msg);


#endif /* MSGUTILS_H */
