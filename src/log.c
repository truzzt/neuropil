/*
 */
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "log.h"

#include <sys/time.h>
#include <time.h>

typedef struct np_log_s {
	char filename[256];
	FILE *fp;
	uint16_t level;
	np_sll_t(char, logentries_l);
} np_log_t;

static np_log_t* logger;
static pthread_mutex_t __log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_message(uint16_t level, const char* srcFile, const char* funcName, uint16_t lineno, const char* msg, ...)
{
	if ( (level & LOG_NOMOD_MASK ) > 0)
		if ( (level & logger->level & LOG_MODUL_MASK) == 0 )
			return;

	if ( (level & logger->level & LOG_LEVEL_MASK) > LOG_NONE)
	{
		char buffer[1024];
		va_list ap;
		va_start (ap, msg);
		vsprintf (buffer, msg, ap);
		va_end(ap);

		struct timeval tval;
		struct tm local_time;
		char timebuf[80];

		gettimeofday(&tval, (struct timezone*)0);
		int32_t millis = tval.tv_usec;

		localtime_r(&tval.tv_sec, &local_time);
		strftime(timebuf, 80, "%Y-%m-%d %H:%M:%S", &local_time);

//  	    char* new_log_entry = malloc(sizeof(char)*1124);
	    // snprintf(new_log_entry, 255, "initialized log system %p: %s (%p) %d\n", logger, logger->filename, logger->fp, logger->level);
//	    snprintf(new_log_entry, 1124, "%s.%06d %-15lu %-15.15s:%-25.25s:%-4hd # %-8hu # %s\n",
//	    							 timebuf, millis,
//									 (unsigned long) pthread_self(),
//									 srcFile, funcName, lineno,
//									 level, buffer);
//		pthread_mutex_lock(&__log_mutex);
//	    sll_append(char, logger->logentries_l, new_log_entry);
//		pthread_mutex_unlock(&__log_mutex);

		fprintf(logger->fp, "%s.%06d %-15lu %-15.15s:%-25.25s:%-4d # %-8d # %s\n",
				timebuf, millis,
				(unsigned long) pthread_self(),
				srcFile, funcName, lineno,
				level, buffer);
		fflush(logger->fp);

	}
	else
	{
		// printf("not logging to file(%p): %d & %d = %d\n", logger, level, logger->level, level & logger->level);
	}
}

void log_fflush()
{
	char* entry = NULL;
	do
	{
		pthread_mutex_lock(&__log_mutex);
		entry = sll_head(char, logger->logentries_l);
		pthread_mutex_unlock(&__log_mutex);

		if (NULL != entry)
		{
			fwrite(entry, strlen(entry), 1, logger->fp);
			free(entry);
		}

	} while(NULL != entry);

	fflush(logger->fp);
}

void log_init(const char* filename, uint16_t level)
{
	logger = (np_log_t *) malloc(sizeof(np_log_t));

    snprintf (logger->filename, 255, "%s", filename);
	logger->fp = fopen(logger->filename, "a");
    logger->level = level;

    sll_init(char, logger->logentries_l);
    char* new_log_entry = malloc(sizeof(char)*256);
    snprintf(new_log_entry, 255, "initialized log system %p: %s (%p) %hu\n", logger, logger->filename, logger->fp, logger->level);
    // fprintf(logger->fp, "initialized log system %p: %s (%p) %d\n", logger, logger->filename, logger->fp, logger->level);
    sll_append(char, logger->logentries_l, new_log_entry);
    // fflush(logger->fp);
}

void log_destroy()
{
	logger->level=LOG_NONE;
	fclose(logger->fp);
	free(logger);
}

