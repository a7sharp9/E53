/*
 * msgutils.c
 *
 *  Created on: Feb 18, 2018
 *      Author: Yuri
 */

#include <string.h>
#include "msgutils.h"

#define MSG_BLOCK_SIZE 10

struct message_buffer {
	int num_msgs;	/* messages currently added */
	int max_num;	/* buffer capacity */
	char **msgs;	/* array of message pointers */
} msgbuf;

/**
 * init_messaging - allocates the initial buffer and initializes counters
 */
void init_messaging () {
	msgbuf.msgs = calloc (MSG_BLOCK_SIZE, sizeof (char *));
	msgbuf.num_msgs = 0;
	msgbuf.max_num = MSG_BLOCK_SIZE;
}

/**
 * shutdown_messaging - frees allocated memory and resets counters
 */
void shutdown_messaging () {
	int msg_idx;
	for (msg_idx = 0; msg_idx < msgbuf.num_msgs; msg_idx ++)
		free (msgbuf.msgs [msg_idx]);
	free (msgbuf.msgs);
	msgbuf.msgs = NULL;
	msgbuf.num_msgs = msgbuf.max_num = 0;
}

/**
 * get_num_messages - returns the current count of added messages
 *
 * return:
 * the number of valid messages in the buffer
 */
int get_num_messages () {
	return (msgbuf.num_msgs);
}

/**
 * get_message - returns the message at requested index
 *
 * args:
 * idx - the index of the message
 * return:
 * the message, or NULL if index is out of bounds
 */
char *get_message (int idx) {
	return ((idx < msgbuf.num_msgs) ? msgbuf.msgs [idx] : NULL);
}

/**
 * realloc_buffer - adds space for another block of messages, reallocating
 * the message holder and copying the old pointers to the new buffer
 */
void realloc_buffer () {
	char **new_msgs_buf = calloc (msgbuf.max_num + MSG_BLOCK_SIZE,
														sizeof (char *));
	if (msgbuf.max_num > 0) {
		memcpy (new_msgs_buf, msgbuf.msgs, msgbuf.max_num * sizeof (char *));
		free (msgbuf.msgs);
	}

	msgbuf.msgs = new_msgs_buf;
	msgbuf.max_num += MSG_BLOCK_SIZE;
}

/**
 * add_message - adds a copy of the incoming message to the holder,
 * reallocating the buffer if it is full
 * args:
 * msg - the message to be added
 */
void add_message (char *msg) {
	if (msgbuf.num_msgs > msgbuf.max_num - 1)
		realloc_buffer ();

	char *msgcopy = malloc (strlen (msg) + 1);
	memcpy (msgcopy, msg, strlen (msg));
	msgcopy [strlen (msg)] = 0;
	msgbuf.msgs [msgbuf.num_msgs++] = msgcopy;
}

