#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "event/ev.h"

#include "log.h"
#include "dtime.h"
#include "neuropil.h"
#include "np_memory.h"
#include "np_message.h"
#include "np_msgproperty.h"

#include "include.h"

#define USAGE "neuropil_sender [ -j bootstrap:port ] [ -p protocol] [-b port]"
#define OPTSTR "j:p:b:"

#define DEBUG 0
#define NUM_HOST 120

extern char *optarg;
extern int optind;

np_node_t *driver;
np_state_t *state;

np_key_t* key;
np_key_t* destinations[100];

int seq = -1;
int joinComplete = 0;


int main(int argc, char **argv) {

	int opt;
	char *b_hn = NULL;
	char *b_port = NULL;
	char* proto = NULL;
	char* port = NULL;
	int i;

	while ((opt = getopt(argc, argv, OPTSTR)) != EOF) {
		switch ((char) opt) {
		case 'j':
			for (i = 0; optarg[i] != ':' && i < strlen(optarg); i++);
			optarg[i] = 0;
			b_hn = optarg;
			b_port = optarg + (i+1);
			break;
		case 'p':
			proto = optarg;
			break;
		case 'b':
			port = optarg;
			break;
		default:
			fprintf(stderr, "invalid option %c\n", (char) opt);
			fprintf(stderr, "usage: %s\n", USAGE);
			exit(1);
		}
	}

	char log_file[256];
	sprintf(log_file, "%s_%s.log", "./neuropil_node", port);
	// int level = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_DEBUG | LOG_TRACE | LOG_ROUTING | LOG_NETWORKDEBUG | LOG_KEYDEBUG;
	// int level = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_DEBUG | LOG_TRACE | LOG_NETWORKDEBUG | LOG_KEYDEBUG;
	int level = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_DEBUG | LOG_MESSAGE;
	// int level = LOG_ERROR | LOG_WARN | LOG_INFO;
	log_init(log_file, level);

	state = np_init(proto, port, FALSE);

	log_msg(LOG_DEBUG, "starting job queue");
	np_start_job_queue(state, 8);
	np_waitforjoin(state);
	char* msg_subject = "this.is.a.test";
	char* msg_data = "testdata";
	unsigned long k = 1;

	while (1) {

		ev_sleep(1.0);

		np_send_text(state, msg_subject, msg_data, k);
		log_msg(LOG_DEBUG, "send message %lu", k);

		k++;
	}
}
