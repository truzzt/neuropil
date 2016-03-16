#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include "include.h"

#include "np_memory.h"
#include "neuropil.h"
#include "log.h"
#include "dtime.h"
#include "np_jobqueue.h"
#include "np_jtree.h"
#include "np_message.h"
#include "np_msgproperty.h"
#include "np_node.h"
#include "np_threads.h"

/**
.. highlight:: c
*/

#define USAGE "neuropil [ -j bootstrap:port ] [ -p protocol] [-b port]"
#define OPTSTR "j:p:b:"

#define DEBUG 0
#define NUM_HOST 120

extern char *optarg;
extern int optind;
/**
first we have to define a global np_state_t variable

.. code-block:: c

   np_state_t *state;
*/
np_state_t *state;

int seq = -1;
int joinComplete = 0;

int main(int argc, char **argv)
{
	int opt;
	char *b_hn = NULL;
	char *b_port = NULL;
	char* proto = NULL;
	char* port = NULL;
	int i;

	while ((opt = getopt(argc, argv, OPTSTR)) != EOF)
	{
		switch ((char) opt)
		{
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

	/**
	in your main program, initialize the logging of neuopil

	.. code-block:: c

	   char log_file[256];
	   sprintf(log_file, "%s_%d.log", "./neuropil_controller", getpid());
	   int level = LOG_ERROR | LOG_WARN | LOG_INFO;
	   log_init(log_file, level);
	*/
	char log_file[256];
	sprintf(log_file, "%s_%d.log", "./neuropil_controller", getpid());
	// int level = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_DEBUG | LOG_TRACE | LOG_ROUTING | LOG_NETWORKDEBUG | LOG_KEYDEBUG;
	// int level = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_DEBUG | LOG_TRACE | LOG_NETWORKDEBUG | LOG_KEYDEBUG;
	int level = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_DEBUG | LOG_MESSAGE;
	// int level = LOG_ERROR | LOG_WARN | LOG_INFO;
	log_init(log_file, level);

	/**
	initialize the global variable with the np_init function

	.. code-block:: c

	   state = np_init(proto, port);
	*/
	state = np_init(proto, port, TRUE);
	state->my_node_key->node->joined_network = 1;

	/**
	check the log file because it will contain this nodes hashvalue / connect string, e.g.

	.. code-block:: c

       2f96848a8c490e0f0f71c74caa900423bcf2d32882a9a0b3510c50085f7ec0e5:udp6:localhost:3333
	*/

	/**
	start up the job queue with 8 concurrent threads competing for job execution.
	you should start at least 2 threads, because network reading currently is blocking.

	.. code-block:: c

	   np_start_job_queue(state, 8);
	*/


	// dsleep(50);
	log_msg(LOG_DEBUG, "starting job queue");
	np_start_job_queue(state, 8);

	/**
	and finally loop (almost) forever

	.. code-block:: c

	   while (1) {
	       dsleep(1.0);
	   }
	*/

	/**
	your're done ...

	if you plan to connect your nodes to this controller as a bootstrap node.
	The created process can be contacted by other nodes and will forward messages as required.
	By default the authentication / authorization / accounting handler accept nodes/message request
	from everybody.

    .. note::
	   Make sure that you implement and register the appropiate aaa callback functions
	   to control with which nodes you exchange messages. By default everybody is allowed to interact
	   with your node
	*/

	np_message_t* msg_out = NULL;

	while (1)
	{
		size_t nbytes = 255;
		msg_out = NULL;
		char* my_string = (char *) malloc (nbytes);
		printf("enter a node to start (key:host:port)\n");
		fgets(my_string, nbytes, stdin);
		if (strlen(my_string) > 255 || strlen(my_string) < 64)
		{
			printf("given identifier too long or to small, skipping invitation ...\n");
			continue;
		}
		my_string[strcspn(my_string, "\r\n")] = '\0';
		log_msg(LOG_DEBUG, "creating internal structure");

		np_key_t* node_key = NULL;

		LOCK_CACHE(state)
		{
			node_key = np_node_decode_from_str(state, my_string);
			// node_key->network = network_init(FALSE, node_key->node->protocol, node_key->node->dns_name, node_key->node->port);
			// node_key->network->watcher.data = node_key;
		}
		log_msg(LOG_DEBUG, "creating welcome message");
		np_new_obj(np_message_t, msg_out);

		np_jtree_t* jrb_me = make_jtree();
		np_node_encode_to_jrb(jrb_me, state->my_node_key, FALSE);
		np_message_create(msg_out, node_key, state->my_node_key, NP_MSG_JOIN_REQUEST, jrb_me);

		log_msg(LOG_DEBUG, "submitting welcome message");
		np_msgproperty_t* prop = np_msgproperty_get(state, OUTBOUND, NP_MSG_JOIN_REQUEST);
		np_job_submit_msg_event(0.0, prop, node_key, msg_out);

		ev_sleep(1.0);
		// dsleep(1.0);
	}
}
