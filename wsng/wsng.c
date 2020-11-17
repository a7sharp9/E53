#include	<stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include	<string.h>
#include	<netdb.h>
#include	<errno.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<signal.h>
#include	"socklib.h"

#include	"read.h"
#include	"process.h"

#define	PARAM_LEN	128
#define	VALUE_LEN	512
#define	PORTNUM	80
#define	SERVER_ROOT	"."
#define	CONFIG_FILE	"wsng.conf"

struct server {
	int port;
	char host [VALUE_LEN];
	int socket;
	char root [VALUE_LEN];
};

#define TYPENAME_LEN 20
#define CONTENTTYPE_LEN 40

struct content_return_type {
	char type_name [TYPENAME_LEN];
	char return_type [CONTENTTYPE_LEN];
};

static struct content_return_type *content_type_arr = 0;
static int num_content_types = 0;

#define DEFAULT_CONTENTTYPE "DEFAULT"
/**
 * setup_content_types - initializes the mapping of file extensions to
 * return content types with the known types, and sets the default
 * type to be text/plain. This mappings can be augmented/redefined in
 * a config file
 */
static void setup_content_types () {
	num_content_types = 6;
	content_type_arr = (struct content_return_type *) calloc (
				num_content_types, sizeof (struct content_return_type));

	int idx = 0;
	content_type_arr[idx ++] =
			(struct content_return_type) {DEFAULT_CONTENTTYPE, "text/plain"};
	content_type_arr[idx ++] =
			(struct content_return_type) {"text", "text/plain"};
	content_type_arr[idx ++] =
			(struct content_return_type) {"html", "text/html"};
	content_type_arr[idx ++] =
			(struct content_return_type) {"jpg", "image/jpeg"};
	content_type_arr[idx ++] =
			(struct content_return_type) {"jpeg", "image/jpeg"};
	content_type_arr[idx ++] =
			(struct content_return_type) {"gif", "image/gif"};
}

/**
 * find_content_type_idx - given the file extension, looks up if a mapping
 * exists describing the content type for this file type. If there isn't,
 * expands the array by 1 record and returns its index
 */
int find_content_type_idx (char *type_name) {
	int idx;

	for (idx = 0; idx < num_content_types; idx ++)
		if (!strcmp (type_name, content_type_arr[idx].type_name))
				return (idx);

	struct content_return_type *new_arr = (struct content_return_type *)
					calloc (++num_content_types,
							sizeof (struct content_return_type));
	memcpy (new_arr, content_type_arr,
			idx * sizeof (struct content_return_type));
	memset (new_arr + idx, 0, sizeof (struct content_return_type));
	free (content_type_arr);
	content_type_arr = new_arr;

	return (idx);
}

/**
 * find_content_type - given the file extension, returns the content type
 * mapped to this file type, or the default type as defined by the
 * DEFAULT entry in the table (it's always the first one there)
 */
char *find_content_type (char *type_name) {
	int idx = find_content_type_idx (type_name);
	if (idx >= 0 && content_type_arr [idx].type_name[0] != 0)
		return (content_type_arr [idx].return_type);

	return (content_type_arr [0].return_type);
}

/**
 * set_return_type: for the incoming file extension, either finds an entry
 * in the mapping table or creates a new one (if there is space in the table),
 * and sets the return type for this file type to the specified string
 */
void set_return_type (char *type_name, char *return_type) {
	int at_idx = find_content_type_idx (type_name);

	if (at_idx >= 0) {
		strncpy (content_type_arr [at_idx].type_name,
						type_name, TYPENAME_LEN);
		strncpy (content_type_arr [at_idx].return_type,
						return_type, CONTENTTYPE_LEN);
	}
}

#define CONFIG_LINE_LEN 4096

/**
 * process_config_file: reads a file describing the server configuration
 * Recognizes the entries for the port, the root directory, and multiple
 * lines describing the mappings between file extensions and HTTP content
 * type strings. Any string starting with # (probably after some whitespace)
 * is ignored. Unknown options cause an error.
 *
 */
void process_config_file (char *conf_file, struct server *server) {
	FILE *fp = fopen (conf_file, "r");
	if (fp == NULL) {
		fprintf (stderr, "Cannot open config file %s", conf_file);
		exit (1);
	}
	char line [CONFIG_LINE_LEN];
	int default_type_defined = 0;
	char *readline (char *buf, int len, FILE *fp);
	while (readline (line, CONFIG_LINE_LEN, fp) != NULL) {
		char *param = strtok (line, " \t\r\n");
		if (param == 0 || *param == 0 || *param == '#') { } // comment
		else if (strcasecmp (param, "server_root") == 0)
			strcpy (server->root, strtok (0, " \t\r\n"));
		else if (strcasecmp (param, "port") == 0)
			server->port = atoi (strtok (0, " \t\r\n"));
		else if (!strcasecmp (param, "type")) {
			char *type = strtok (0, " \t\r\n");
			char *typeval = strtok (0, " \t\r\n");
			if (type != 0 && typeval != 0) {
				if (!strcmp (DEFAULT_CONTENTTYPE, type))
					default_type_defined = 1;
				set_return_type (type, typeval);
			}
		} else {
			fprintf (stderr, "Unknown config parameter %s\n", param);
			exit (1);
		}
	}
	fclose (fp);
	if (!default_type_defined) {
		fprintf (stderr, "Default content type not defined\n");
		exit (1);
	}
}

/**
 * respond: forks an executor which reads the request from the incoming socket,
 * calls the processing function, flushes the writing end of the socket and
 * exits. Does not wait for the child process to finish; the collection of
 * zombies is handled by catching SIGCHLD.
 */
void respond (int fd) {

	FILE *in_out;
	char request[MAX_RQ_LEN];

	switch (fork ()) {
		case 0: // child
			in_out = fdopen (fd, "r+");
			if (in_out == 0)
				exit (1);
			if (read_request (in_out, request, MAX_RQ_LEN) < 0)
				exit (1);

			process_request (request, in_out);
			fflush (in_out);
			exit (0);
		break;

		case -1: // error
			perror ("fork");
		break;

		default: // parent
			// not waiting - handle_sigchld will pick up exited children
		break;
	}
}

/**
 * handle_sigchld: a handler for the SIGCHLD signal, waiting on it to finish
 * Does not need to be surrounded by errno saving/resetting brackets, because
 * SA_RESTART does not allow an intervening system call to reset it.
 */
void handle_sigchld (int sig) {
	while (waitpid (-1, 0, WNOHANG) > 0) {
	}
}

/**
 * setup: reads the configuration from the supplied file name and
 * initializes the pointer to the struct server general options. Also
 * sets up the zombie child process reaping.
 */
void setup (char *configfile, struct server *config) {
	setup_content_types (); // initialize content type mappings

	process_config_file (configfile, config);

	strcpy (config->host, full_hostname ()); // full localhost name

	if (chdir (config->root) == -1) {
		perror ("cannot change to rootdir");
		exit (1);
	}

	config->socket = make_server_socket (config->port);
	if (config->socket == -1) {
		perror ("socket");
		exit (1);
	}

	struct sigaction sa;
	sa.sa_handler = &handle_sigchld;
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;  // to prevent race conditions
	if (sigaction (SIGCHLD, &sa, 0) == -1) {
		perror ("sigaction");
		exit (1);
	}
}

/**
 * parse_options: extracts the optional name of the config file from
 * the program arguments
 */
int parse_options (char **config_file, int argc, char * argv[]) {
	int		option_flag;
	const char * valid_options = "c:"; // "-c" is optional, but if present,
										// has to have an argument
	int		return_code = 0;
	char 	*file_name = CONFIG_FILE;

	opterr = 0; // we have our own error handling
	while ((option_flag = getopt (argc, argv, valid_options)) > 0) {
		switch (option_flag) {
		case 'c':
			file_name = optarg;
			break;

		case '?':
			if (optopt == 'c') /* '-c' specified, but no value provided */ {
				fprintf (stderr, "Option \"-c\" requires a value - aborting\n");
			}
			else
				fprintf (stderr, "Unknown option %c - aborting.\n", optopt);
			return_code = -1;
		}
	}

	if (!return_code) {
		if (optind < argc - 1) {
			fprintf (stderr, "Extra arguments specified - aborting.\n");
			return_code = -1;
		}
	}

	if (!return_code) {
		(*config_file) = file_name;
	}

	return (return_code);
}

/**
 * main: reads a config file, prints out info message, then loops indefinitely
 * on the socket, processing the messages
 */
int main (int argc, char *argv[]) {
	struct server ws_config = {PORTNUM, "localhost", -1, "."};
	char *config_file;

	if (parse_options (&config_file, argc, argv))
		exit (1);

	setup (config_file, &ws_config);

	fprintf (stdout, "Server %s started on host %s, port %d\n",
					argv [0], ws_config.host, ws_config.port);

	for (;;) {
		int sock_fd = accept (ws_config.socket, NULL, NULL);
		sock_fd >= 0 ? respond (sock_fd) : perror ("accept");
		close (sock_fd);
	}
}


