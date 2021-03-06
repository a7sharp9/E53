/*
 * process.c
 *
 *  Created on: Apr 17, 2018
 *      Author: Yuri
 */

#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>

#include "process.h"
#include "read.h"

char *find_content_type (char *);

/*
 * modify_argument
 *  purpose: many roles
 *		security - remove all ".." components in paths
 *		cleaning - if arg is "/" convert to "."
 *  returns: pointer to modified string
 *     args: array containing arg and length of that array
 */

char *modify_argument (char *arg, int len) {
	char *nexttoken;
	char *copy = malloc (len);

	if (copy == 0) {
		fprintf (stderr, "can't malloc");
		exit (1);
	}

	/* remove all ".." components from path */
	/* by tokeninzing on "/" and rebuilding */
	/* the string without the ".." items	*/

	*copy = '\0';

	nexttoken = strtok (arg, "/");
	while (nexttoken != 0) {
		if (strcmp (nexttoken, "..") != 0) {
			if (*copy)
				strcat (copy, "/");
			strcat (copy, nexttoken);
		}
		nexttoken = strtok (0, "/");
	}
	strcpy (arg, copy);
	free (copy);

	/* the array is now cleaned up */
	/* handle a special case       */

	if (strcmp (arg, "") == 0)
		strcpy (arg, ".");
	return arg;
}

enum http_codes {
	OK = 200,
	BAD_REQUEST = 400,
	NOT_ALLOWED = 403,
	NOT_FOUND = 404,
	SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501
};

struct http_status {
	enum http_codes code;
	char *code_string;
	char *msg_fmt;
};

#define E_NOT_FOUND {NOT_FOUND, "Not Found", \
"The item you requested: %s\r\nis not found\r\n"}

static const struct http_status GENERAL_ERROR =
	{SERVER_ERROR, "Server Error", "General server error"};

static const struct http_status STATUS_OK =
	{OK, "OK", 0};

static const struct http_status STATUS_NOT_FOUND = E_NOT_FOUND;

static const struct http_status status_codes [] = {
		{BAD_REQUEST, "Bad Request",
				"\r\nI cannot understand your request\r\n"},
		{NOT_IMPLEMENTED, "Not Implemented",
				"That command is not yet implemented\r\n"},
		{NOT_ALLOWED, "Not Allowed",
				"You are not allowed to access the item you requested: %s\r\n"},
		E_NOT_FOUND,
		{0, 0, 0}
};

/**
 * given the http code, finds the corresponding string format for printing
 * its informational message; if the specific error is not explicitly defined,
 * returns the string for the 500 general server error
 */
static const struct http_status *get_status_format (enum http_codes code) {
	int idx;
	for (idx = 0; status_codes [idx].code != 0; idx ++)
		if (status_codes [idx].code == code)
			return (status_codes + idx);

	return (&GENERAL_ERROR);
}

#define TIME_FORMAT "%a, %e %b %Y %H:%M:%S GMT"
#define MAXDATELEN 40

/**
 * format_time: prints a "web time" version of the incoming time_t
 */
void format_time (time_t timeval, char *formatted_time) {
	struct tm *time_str = gmtime (&timeval);
	strftime (formatted_time, MAXDATELEN, TIME_FORMAT, time_str);
}

/**
 * format_current_time: prints a "web time" version of the current time
 */
void format_current_time (char *formatted_time) {
	format_time (time (0), formatted_time);
}

#define SERVER_NAME "wsng"
#define SERVER_VERSION "0.1"
#define CONTENT_TYPE_STRING "Content-type:"

/**
 * header: forms an HTTP header corresponding to the incoming status code
 * (contained in the struct http_status), adds the Content-type line (and the
 * required empty string after it) is specified. The header contains the
 * current time in the web time format and the name/version of the server
 */
void header (FILE *fp, const struct http_status *format, char *content_type) {
	fprintf (fp, "HTTP/1.1 %d %s\r\n", format->code, format->code_string);
	char time [MAXDATELEN];
	format_current_time (time);
	fprintf (fp, "Date: %s\r\n", time);
	fprintf (fp, "Server: %s/%s\r\n", SERVER_NAME, SERVER_VERSION);
	if (content_type) {
		fprintf (fp, "%s %s\r\n", CONTENT_TYPE_STRING, content_type);
		fprintf (fp, "\r\n");
	}
}

/**
 * a handler for success/error response; forms a correct header corresponding
 * to the specified status code (or a 500 general error if the specific
 * format is not defined), and adds the message if supplied in plain text
 */
void do_status (char *item, FILE *fp, enum http_codes status) {
	const struct http_status *format = get_status_format (status);

	header (fp, format, "text/plain");
	if (format->msg_fmt)
		fprintf (fp, format->msg_fmt, item);
	fflush (fp);
}

char *file_type (char *f)
/* returns 'extension' of file */
{
	char *cp;
	if ( f != 0 && (cp = strrchr (f, '.')) != 0)
		return cp + 1;
	return "";
}

/**
 * checks if the file at the specified path is a directory; returns 0 if it
 * is not, not-0 if it is
 */
int isadir (char *f) {
	struct stat info;
	return (stat (f, &info) != -1 && S_ISDIR (info.st_mode));
}

/**
 * checks that there is no file at the specified path; returns 0 is the
 * file is present, not-0 if it isn't
 */
int not_exist (char *f) {
	struct stat info;

	return (stat (f, &info) == -1 && errno == ENOENT);
}

/**
 * checks that the first 3 letters of the file type after the "." are "cgi"
 */
int is_cgi (char *f) {
	return (strncmp (file_type (f), "cgi", 3) == 0);
}

/**
 * handler for a cgi script/program. Tries to read the program at the
 * specified file path, checks if it is executable, sets the minimum
 * required environment for its execution (including parsing the
 * optional query string after '?' on the path), forms and prints the correct
 * HTTP header, then if the program exists and is executable, redirects its
 * stdout/stderr to the incoming socket and passes control to it.
 */
void do_exec_method (char *prog, FILE *fp, char *method) {
	char *cp;
	if ((cp = strrchr (prog, '?')) != 0)
		*cp = 0; // otherwise we wouldn't find the file

	if (access (prog, X_OK)) { // need to check here - the execution
		// of cgi happens before the general check in get_reqtype_and_status
		do_status (prog, fp, errno == ENOENT ? NOT_FOUND : NOT_ALLOWED);
		return;
	}

	header (fp, &STATUS_OK, 0); // we can execute; status is 200
						// Content-type expected to be set by the prog
	fflush (fp);

	setenv ("REQUEST_METHOD", method, 1);
	if (cp != 0) // there were arguments after "?"; pass them down
		setenv ("QUERY_STRING", cp + 1, 1);
	else
		setenv ("QUERY_STRING", "", 1);

	int fd = fileno (fp);
	dup2 (fd, 1); // close stdout and redirect to socket
	dup2 (fd, 2); // close stderr and redirect to socket
	execl (prog, prog, (char *) 0);
	perror (prog);
}

/**
 * wrapper around the cgi handler with the arguments required for
 * struct request_handler
 */
void do_exec (char *prog, FILE *fp, enum http_codes status) {
	do_exec_method (prog, fp, "GET");
}

/**
 * handler for dumping the contents of the file into the socket, setting the
 * content type according to its extension. The incoming status is ignored.
 * if the file cannot be found or opened, prints a 404 header; otherwise,
 * sends over the file contents under the 200 header
 */
void do_cat (char *f, FILE *fpsock, enum http_codes status) {
	char *extension = file_type (f); // find file type
	char *content = find_content_type (extension); // content type or default
	FILE *fpfile;
	int c;

	fpfile = fopen (f, "r");
	if (fpfile != 0) {
		header (fpsock, &STATUS_OK, content);
		while ( (c = getc(fpfile)) != EOF)
			putc(c, fpsock); // pump characters
		fclose (fpfile);
		fflush (fpsock);
	} else {
		do_status (f, fpsock, NOT_FOUND);
	}
}

/**
 * tries to execute the cgi, expecting the first line of its output to contain
 * the Content-type: string.
 * return: 0 if successful, not-0 otherwise
 */
static int read_cgi_content_type (char *filepath, char *type) {
	FILE *prog_stream = popen (filepath, "r");
	if (prog_stream == 0)
		return (-1);

	char first_line [LINELEN];
	int ret = (readline (first_line, LINELEN, prog_stream) == 0);
	if (!ret) { // successfully read the first line of stdout
		char *ctype_string = strtok (first_line, " \t");
		if (strcmp (ctype_string, CONTENT_TYPE_STRING))
			ret = 1; // does not start with Content-type
		else
			strncpy (type, strtok (0, " \t"), LINELEN);
	}

	pclose (prog_stream); // will most likely sigpipe, so what
	return (ret);
}

/**
 * a handler for the HEAD request; executes cgi scripts to find their type,
 * otherwise just prints the 200 OK header with the content type determined by
 * file type
 */
void do_head (char *item, FILE *fp, enum http_codes status) {
	char *content_type;

	if (not_exist (item)){
		header (fp, &STATUS_NOT_FOUND, 0);
	} else if (is_cgi (item)) {
		// this should have sufficed:
		/* do_exec_method (item, fp, "HEAD"); */
		// but the test example cgis don't respect this, so:
		char cgitype [LINELEN];
		if (read_cgi_content_type (item, cgitype)) {
			header (fp, &GENERAL_ERROR, 0); // cgi didn't say what type it is
			return;
		} else
			content_type = cgitype;
	} else {
		char *extension = file_type (item);
		content_type = find_content_type (extension);
	}

	header (fp, &STATUS_OK, content_type);
	fflush (fp);
}

/**
 * for a given directory, check first if it exists and is capable of being
 * read/executed; if so, tries to find a file with the name "index.html" in
 * it. If there is, prints it out into the socket with the content type
 * text/html. If there isn't, tries the same for a file with the name
 * "index.cgi" - printing its output into the socket if found.
 * Returns: 0 if the directory is accessible and does not contain index files,
 * or not-0 if there should be no further processing done on it (either
 * it is not accessible, or an index file has already been handles)
 */
int check_access_and_index (char *dir, FILE *fp) {
	struct dirent dir_entry;
	struct dirent *dir_flag = 0;
	int has_html = 0;
	int has_cgi = 0;
	int ret = 0;

	DIR *dir_ptr = opendir (dir);
	if (dir_ptr == 0) { // either not there, or wrong permissions
		do_status (dir, fp, errno == EACCES? NOT_ALLOWED : NOT_FOUND);
							// either 404 or 403
		return (1);
	}
	if (readdir_r (dir_ptr, &dir_entry, &dir_flag) ||
							dir_flag == 0) { // listing not permitted
		do_status (dir, fp, NOT_ALLOWED);
		closedir  (dir_ptr);
		return (1);
	}

	do { // we don't care for the order of entries here
		char *entry_name = dir_entry.d_name;
		if (!strcasecmp (entry_name, "index.html"))
			has_html = 1;
		else if (!strcasecmp (entry_name, "index.cgi"))
			has_cgi = 1;
	} while (!readdir_r (dir_ptr, &dir_entry, &dir_flag) && dir_flag != 0);

	char index_file [PATH_MAX];
	if (has_html) { // found "index.html"
		snprintf (index_file, PATH_MAX, "%s/%s", dir, "index.html");
		do_cat (index_file, fp, OK); // write it out
		ret = 1;
	} else if (has_cgi) { // found "index.cgi"
		snprintf (index_file, PATH_MAX, "%s/%s", dir, "index.cgi");
		do_exec (index_file, fp, OK); // execute it
		ret = 1;
	}
	closedir (dir_ptr);
	return (ret);
}

/**
 * forms and prints a correct HTML header and the start of the directory
 * listing table
 */
void ls_header (FILE *fp, char *dir) {
	header (fp, &STATUS_OK, "text/html");
	fprintf (fp, "<html>\r\n<head>\r\n");
	fprintf (fp, "<title>\r\nIndex of /%s\r\n</title>\r\n", dir);
	fprintf (fp, "</head>\r\n\r\n<body>\r\n");
	fprintf (fp, "<h1>Index of /%s</h1>\r\n", dir);
	fprintf (fp, "<table>\r\n");
	fprintf (fp, "<tr><th>Name</th><th>Last modified</th><th>Size</th></tr>");
	fprintf (fp, "<tr><th colspan=3><hr></th></tr>");
}

/**
 * forms and prints a "Parent Directory" line in the directory listing,
 * including name and hgref tag, unless we are already at top level
 */
void print_parent_dir (char *dir, struct dirent *e, FILE *fp, int is_top) {
	if (!is_top) {
		char *last_sep = strrchr (dir, '/');
		if (last_sep != 0) {
			*last_sep = 0;
			fprintf (fp, "<a href=\"/%s\">Parent Directory</a>\r\n", dir);
			*last_sep = '/';
		} else {
			fprintf (fp, "<a href=\".\">Parent Directory</a>\r\n");
		}
	}
}

#define LONGEST_LONG_STR_SIZE ((CHAR_BIT * sizeof (long) - 1) / 3 + 2)

/**
 * forms and prints the HTML line in the directory listing table for a
 * directory entry.
 */
void print_one_entry (char *dir, struct dirent *dir_entry,
						FILE *fp, int is_top) {
	char *entry_name = dir_entry->d_name;
	if (strcmp (entry_name, ".") && strcmp (entry_name, "..")) {
		(!is_top) ?
		fprintf (fp, "<a href=\"/%s/%s\">%s</a></td><td>\r\n",
						dir, entry_name, entry_name) :
		fprintf (fp, "<a href=\"/%s\">%s</a></td><td>\r\n",
						entry_name, entry_name);

		char fname [MAXNAMLEN];
		snprintf (fname, MAXNAMLEN, "%s/%s", dir, entry_name);
		struct stat stats;
		if (!lstat (fname, &stats)) {
			char time [MAXDATELEN];
			format_time (stats.st_mtim.tv_sec, time);
			fprintf (fp, "%s</td><td>", time);
			if (S_ISDIR (stats.st_mode)) {
				fprintf (fp, "-");
			} else {
				char size [LONGEST_LONG_STR_SIZE];
				snprintf (size, LONGEST_LONG_STR_SIZE, "%ld", stats.st_size);
				fprintf (fp, "%s", size);
			}
		}
		fprintf (fp, "</td></tr>\r\n");
	}
}

/**
 * a <tr><td>XXX</td/tr> bracket around a function that prints out a line
 * in the directory listing
 */
void print_one_ls_line (void f (char *, struct dirent *, FILE *, int),
		char *dir, struct dirent *dir_entry, FILE *fp, int is_top) {
	fprintf (fp, "<tr><td valign=top>");
	f (dir, dir_entry, fp, is_top);
	fprintf (fp, "</td></tr>\r\n");
}

void do_ls (char *dir, FILE *fp, enum http_codes status) {
	if (check_access_and_index (dir, fp))// cannot list or there's an index file
		return; // either way, no listing required

	int is_top = !strcmp (dir, ".");

	ls_header (fp, dir); // HEAD and header row of the table
	print_one_ls_line (print_parent_dir, dir, 0, fp, is_top);

	struct dirent **entries;
    int num_entries = scandir (dir, &entries, 0, alphasort);
    					// for the alphabetical listing
    // To do first all the directories and then all the files, would need
    // to create 2 filter functions that check an entry for being or not being
    // a directory, and call scandir with each
    int entry_idx;
	for (entry_idx = 0; entry_idx < num_entries; entry_idx ++) {
    	struct dirent *dir_entry = entries [entry_idx];
    	print_one_ls_line (print_one_entry, dir, dir_entry, fp, is_top);
		free (dir_entry); // scandir allocates each entry
		entries [entry_idx] = 0;
	}
	free (entries); // scandir allocates the table
	entries = 0;

	fprintf (fp, "</table>\r\n</body>\r\n</html>\r\n"); // close the tags
	fflush (fp);
}

enum reqtype {
	NONE, LS, HEAD, CGI, CAT, ERR, UNIMP
};

static struct request_handler {
	enum reqtype type;
	void (*handle) (char *, FILE *, enum http_codes);
} handlers [] = {
		{LS, do_ls},
		{HEAD, do_head},
		{CGI, do_exec},
		{CAT, do_cat},
		{ERR, do_status},
		{UNIMP, do_status},
		{NONE, 0}
};

/**
 * from the request type and file path, derive the handler for this request
 * and the applicable status
 * returns: one of the enum reqtype constants, which are also keys in the
 * handler table
 */
enum reqtype get_reqtype_and_status (char *cmd, char *item,
										enum http_codes *status) {
	*status = OK;

	if (!strcmp (cmd, "HEAD"))
		return (HEAD);
	else if (strcmp (cmd, "GET")) { // only handles HEAD and GET
		*status = NOT_IMPLEMENTED;
		return (UNIMP);
	}

	else if (is_cgi (item)) // check before not_exist () to allow "?arg"
		return (CGI);
	else if (not_exist (item)) {
		*status = NOT_FOUND; // 404, call the status handler
		return (ERR);
	}
	else if (isadir (item)) // listing the directory
		return (LS);
	else // dump the file into the socket, handling content type
		return (CAT);
}

/**
 * find the correct handler for this request type
 */
struct request_handler *get_handler (enum reqtype type) {
	int idx;
	for (idx = 0; handlers[idx].handle != 0; idx ++)
		if (handlers[idx].type == type)
			return (handlers + idx);

	return (0);
}

/**
 * top-level function to read the HTTP request from the file pointer attached
 * to the socket, act on it and write the HTTP response back into the socket.
 */
void process_request (char *rq, FILE *fp) {
	char cmd[MAX_RQ_LEN], arg[MAX_RQ_LEN];
	char *item, *modify_argument ();

	if (sscanf (rq, "%s%s", cmd, arg) != 2) { // 2 arguments required
		do_status (0, fp, BAD_REQUEST); // 400; cannot do anything else
		return;
	}

	item = modify_argument (arg, MAX_RQ_LEN); // handle ".." entries in path

	enum http_codes status;

	// determine the type of this request
	enum reqtype request_type = get_reqtype_and_status (cmd, item, &status);

	// determine the handler for this request
	struct request_handler *handler = get_handler (request_type);

	if (handler != 0) { // found the handler; pass the file path and status
		handler->handle (item, fp, status);
	} else {
		do_status (item, fp, SERVER_ERROR); // HTTP 500
	}

	fflush (fp);
}


