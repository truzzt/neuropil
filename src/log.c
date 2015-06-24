/*
** $Id: log.c,v 1.15 2006/09/05 06:09:35 krishnap Exp $
**
** Matthew Allen
** description: 
*/

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "log.h"

// TODO OS dependant import ?
#include <sys/time.h>
#include <time.h>

// extern FILE *stdin;
extern LOG *logger;

void log_message(int level, const char* srcFile, const char* funcName, int lineno, const char* msg, ...)
{
	if ( (level & logger->level) > LOG_NONE) {
		char buffer[1024];
		va_list ap;
		va_start (ap, msg);
		vsprintf (buffer, msg, ap);
		va_end(ap);

		struct timeval tval;
		gettimeofday(&tval, (struct timezone*)0);
		int millis = tval.tv_usec;
		char timebuf[80];
		strftime(timebuf, 80, "%Y-%m-%d %H:%M:%S", localtime(&tval.tv_sec));

	    char* new_log_entry = malloc(sizeof(char)*256);
	    // snprintf(new_log_entry, 255, "initialized log system %p: %s (%p) %d\n", logger, logger->filename, logger->fp, logger->level);
	    snprintf(new_log_entry, 255, "%s.%i %-15lu %-15.15s:%-25.25s:%-4d # %-8d # %s\n",
	    							 timebuf, millis,
									 (unsigned long) pthread_self(),
									 srcFile, funcName, lineno,
									 level, buffer);
		pthread_mutex_lock(&logger->lock);
	    sll_append(char, logger->logentries_l, new_log_entry);
		pthread_mutex_unlock(&logger->lock);

//		fprintf(logger->fp, "%s.%i %-15lu %-15.15s:%-25.25s:%-4d # %-8d # %s\n",
//				timebuf, millis,
//				(unsigned long) pthread_self(),
//				srcFile, funcName, lineno,
//				level, buffer);
//		fflush(logger->fp);

	} else {
		// printf("not logging to file(%p): %d & %d = %d\n", logger, level, logger->level, level & logger->level);
	}
}

void log_fflush() {
	char* entry = NULL;
	do {
		pthread_mutex_lock(&logger->lock);
		entry = sll_head(char, logger->logentries_l);
		pthread_mutex_unlock(&logger->lock);
		if (NULL != entry) {
			fprintf(logger->fp, "%s", entry);
			free(entry);
		}
	} while(NULL != entry);

	fflush(logger->fp);
}

void log_init(const char* filename, int level) {

	logger = (LOG *) malloc(sizeof(struct np_log_t));

    snprintf (logger->filename, 255, "%s", filename);
	logger->fp = fopen(logger->filename, "a");
    logger->level = level;

    pthread_mutex_init (&logger->lock, NULL);

    sll_init(char, logger->logentries_l);
    char* new_log_entry = malloc(sizeof(char)*256);
    snprintf(new_log_entry, 255, "initialized log system %p: %s (%p) %d\n", logger, logger->filename, logger->fp, logger->level);
    // fprintf(logger->fp, "initialized log system %p: %s (%p) %d\n", logger, logger->filename, logger->fp, logger->level);
    sll_append(char, logger->logentries_l, new_log_entry);
    // fflush(logger->fp);
}

LOG* log_get() {
	return logger;
}

void log_destroy() {
	logger->level=LOG_NONE;
	fclose(logger->fp);
	free(logger);
}


void log_direct (void *logs, int type, FILE * fp)
{
    FILE **log_fp = (FILE **) logs;

    if (fp == NULL)
	{
	    fprintf (stderr,
		     "The file pointer given to log_direct is NULL; No messages would be printed to the file \n");
	}

    log_fp[type] = fp;
}
