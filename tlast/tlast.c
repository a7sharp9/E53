/*
 * tlast.c
 *
 *  Created on: Feb 3, 2018
 *      Author: Yuri Machkasov
 */

#include <stdlib.h>
#include <utmp.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "wtmputils.h"

struct run_options {
	char *file_name;
	int report_efficiency;
	char *term_name;
};

int parse_options (struct run_options *runopts, int argc, char * argv[]) {
	int		option_flag;
	char *	file_name = WTMP_FILE;
	int		report_efficiency = 0;
	const char * valid_options = "ef:";
	char *	term_name = NULL;
	int		return_code = 0;

	opterr = 0; /* we'll handle the error messages ourselves */
	while ((option_flag = getopt (argc, argv, valid_options)) > 0) {
		switch (option_flag) {
		case 'f':
			file_name = optarg;
			break;

		case 'e':
			report_efficiency = 1;
			break;

		case '?':
			if (optopt == 'f') /* '-f' specified, but no value provided */ {
				fprintf (stderr, "Option \"-f\" requires a value - aborting\n");
				return_code = -1;
			}
			else
				fprintf (stderr, "Unknown option %d - ignoring.\n", optopt);
		}
	}

	if (optind >= argc) {
		fprintf (stderr, "Terminal line not specified - aborting.");
		return_code = -1;
	} else if (optind < argc - 1) {
		fprintf (stderr, "Extra arguments specified - aborting.");
		return_code = -1;
	} else {
		term_name = argv [optind];
	}

	if (!return_code) {
		runopts->file_name = file_name;
		runopts->term_name = term_name;
		runopts->report_efficiency = report_efficiency;
	}

	return (return_code);
}

#define FULL_FMT "%a %b %e %H:%M"
#define LAST_LINE_FMT "%a %b %e %H:%M:%S %Y"
#define HM_FMT "%H:%M"
#define MAXDATELEN 30

void format_time (time_t timeval, char *fmt, char *formatted_time) {
	struct tm *time_str = localtime (&timeval);				/* convert time	*/
	strftime (formatted_time, MAXDATELEN, fmt, time_str);	/* format it	*/
}

#define SECONDS_IN_DAY (60 * 60 * 24)

void format_interval (time_t interval, char *formatted_interval) {
	struct tm *time_str = gmtime (&interval);
	int num_days = (int) interval / SECONDS_IN_DAY;
	int str_pos = sprintf (formatted_interval, "%1.1s", "(");
	if (num_days > 0)
		str_pos += sprintf (formatted_interval + str_pos, "%d+", num_days);
	str_pos += strftime (formatted_interval + str_pos, MAXDATELEN - str_pos, HM_FMT, time_str);
	sprintf (formatted_interval + str_pos, "%1.1s", ")");
}

void print_wtmp (struct run_options *runopts) {
	wtmp_open (runopts->file_name);
	int num_entries = wtmp_len();
	int rec_idx;
	char strbuf [1024];
	char fmttime [MAXDATELEN];
	time_t  logged_out = -1;
	int	 str_pos;
	time_t	wtmp_started = -1;

	for (rec_idx = num_entries - 1; rec_idx >= 0; rec_idx --) {
		struct utmp *entry = wtmp_getrec (rec_idx);
		if (wtmp_started < 0 || wtmp_started > entry->ut_time)
			wtmp_started = entry->ut_time;
		if (strcmp (entry->ut_line, runopts->term_name) == 0) {
			if (entry->ut_type == USER_PROCESS) {
				str_pos = sprintf (strbuf, "%-8.8s ", entry->ut_user);
				str_pos += sprintf (strbuf + str_pos, "%-12.12s ", entry->ut_line);
				str_pos += sprintf (strbuf + str_pos, "%-16.16s ", entry->ut_host);
				format_time (entry->ut_time, FULL_FMT, fmttime);
				str_pos += sprintf (strbuf + str_pos, "%-16.16s - ", fmttime);
				if (logged_out > 0) {
					format_time (logged_out, HM_FMT, fmttime);
					str_pos += sprintf (strbuf + str_pos, "%-5.5s ", fmttime);
					format_interval (logged_out - entry->ut_time, fmttime);
					int align = (10 - (int) strlen (fmttime)) / 2;
					int trail = 12 - (int) strlen (fmttime) - align;
					str_pos += sprintf (strbuf + str_pos,
							"%*s%s%*s",
							align, "",
							fmttime,
							trail, "");
				} else {
					str_pos += sprintf (strbuf + str_pos, "%s", "still logged in");
				}
				fprintf (stdout, "%s\n", strbuf);
			} else if (entry->ut_type == DEAD_PROCESS) {
				logged_out = entry->ut_time;
			}
		}
	}

	if (!logged_out) {
		fprintf (stdout, "%s %s\n", strbuf, "still logged in");
	}

	format_time (wtmp_started, LAST_LINE_FMT, strbuf);
	fprintf (stdout, "\nwtmp begins %s\n", strbuf);

}

int main (int argc, char * argv[]) {
	struct run_options run_opts;

	if (parse_options (&run_opts, argc, argv))
		exit (1);

	print_wtmp (&run_opts);

	if (run_opts.report_efficiency) {
		int bufstats [2];
		wtmp_stats (bufstats);
		fprintf (stderr, "%d records read, %d buffer misses\n",
				bufstats [0], bufstats [1]);
	}

	exit (0);
}
