//
// neuropil is copyright 2016-2017 by pi-lar GmbH
// Licensed under the Open Software License (OSL 3.0), please see LICENSE file for details
//
/**
.. highlight:: c
*/

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "np_types.h"
#include "np_log.h"
#include "neuropil.h"
#include "np_tree.h"
#include "np_keycache.h"
#include "np_message.h"
#include "np_msgproperty.h"
#include "np_node.h"

#define USAGE "neuropil_receiver_cb [ -j key:proto:host:port ] [ -p protocol] [-b port] [-t worker_thread_count]"
#define OPTSTR "j:p:b:t:"

extern char *optarg;
extern int optind;

uint32_t _ping_count = 0;
uint32_t _pong_count = 0;

/**
right, let's define two callback functions that will be called each time
a ping or pong message is received by the nodes that you are going to start

.. code-block:: c
	\code
*/

np_bool receive_ping(const np_message_t* const msg, np_tree_t* properties, np_tree_t* body)
{
	/** \endcode */

	char* text = np_tree_find_str(body, NP_MSG_BODY_TEXT)->val.value.s;
	uint32_t seq = np_tree_find_str(properties, _NP_MSG_INST_SEQ)->val.value.ul;

	fprintf(stdout, "RECEIVED: %05d -> %s\n", seq, text);
	log_msg(LOG_INFO, "RECEIVED: %d -> %s", seq, text);
	log_msg(LOG_INFO, "SENDING: %d -> %s", _pong_count++, "pong");
	np_send_text("pong", "pong", _pong_count,NULL);

	return TRUE;
}
/**
	And

	.. code-block:: c
 	\code
*/
np_bool receive_pong(const np_message_t* const msg, np_tree_t* properties, np_tree_t* body)
{
	/** \endcode */
	char* text = np_tree_find_str(body, NP_MSG_BODY_TEXT)->val.value.s;
	uint32_t seq = np_tree_find_str(properties, _NP_MSG_INST_SEQ)->val.value.ul;

	fprintf(stdout, "RECEIVED: %05d -> %s\n", seq, text);
	log_msg(LOG_INFO, "RECEIVED: %d -> %s", seq, text);
	log_msg(LOG_INFO, "SENDING: %d -> %s", _ping_count++, "ping");

	np_send_text("ping", "ping", _ping_count,NULL);

	return TRUE;
}


int main(int argc, char **argv)
{
	int opt;
	int no_threads = 8;
	char *j_key = NULL;
	char* proto = NULL;
	char* port = NULL;

	while ((opt = getopt(argc, argv, OPTSTR)) != EOF)
	{
		switch ((char) opt)
		{
		case 'j':
			// for (i = 0; optarg[i] != ':' && i < strlen(optarg); i++);
			// optarg[i] = 0;
			j_key = optarg;
			// j_proto = optarg + (i+1);
			// j_hn = optarg + (i+2);
			// j_port = optarg + (i+3);
			break;
		case 't':
			no_threads = atoi(optarg);
			if (no_threads <= 0) no_threads = 2;
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
	in your main program, initialize the logging of neuopil, but this time use the port for the filename

	.. code-block:: c
	\code
	*/
	char log_file[256];
	sprintf(log_file, "%s_%s.log", "./neuropil_pingpong", port);
	int level = LOG_ERROR | LOG_WARN | LOG_INFO;
	np_log_init(log_file, level);
	/** \endcode */

	/**
	initialize the neuropil subsystem with the np_init function

	.. code-block:: c
	\code
	*/
	np_state_t* state = np_init(proto, port, FALSE, NULL);
	/** \endcode

		The port may change due to default setting for NULL,
		so we need to reevaluate the port to print it out later on

			.. code-block:: c
			\code
	*/
	port =  state->my_node_key->node->port;
	/** \endcode */
	/**
	start up the job queue with 8 concurrent threads competing for job execution.
	you should start at least 2 threads (network io is non-blocking).

	.. code-block:: c
	\code
	*/
	log_msg(LOG_DEBUG, "starting job queue");
	np_start_job_queue(no_threads);
	/** \endcode */

	/**
	If this is your first start of the program copy the connections tring from stout and
	start a second instance of the program and provide the connection string via the -j parameter.

	the next step in the  program is to check if the j_key was provided. if so we will try to join the node.
	If not we will print out the connection string of this node and wait for a node to join this network.

	.. code-block:: c
	\code
	*/

	if (NULL != j_key)
	{
		np_send_join(j_key);
	} else {
		fprintf(stdout, "Node waits for connections.\n");
		fprintf(stdout, "Please start another node with the following arguments:\n");
		fprintf(stdout, "\n\t-b %d -j %s\n", atoi(port) + 1, np_get_connection_string());
	}

	np_waitforjoin();

	fprintf(stdout, "Connection established.\n");
	/** \endcode */

	/**
	*.. note::

	*   Make sure that you have implemented and registered the appropiate aaa callback functions
	*   to control with which nodes you exchange messages. By default everybody is allowed to interact
	*   with your node
	 */

	/**
	 Now we need to register this node as interested in "ping" and "pong" messages.
	 For this we will configure two message properties with the appropiate callbacks to our handlers.

	 .. code-block:: c
	 \code
	 */
	np_msgproperty_t* ping_props = NULL;
	np_new_obj(np_msgproperty_t, ping_props);
	ping_props->msg_subject = "ping";
	ping_props->ack_mode = ACK_NONE;
	ping_props->ttl = 20.0;
	np_msgproperty_register(ping_props);
//register the listener function to receive data from the sender
	np_set_listener(receive_ping, "ping");

	np_msgproperty_t* pong_props = NULL;
	np_new_obj(np_msgproperty_t, pong_props);
	pong_props->msg_subject = "pong";
	pong_props->ack_mode = ACK_NONE;
	pong_props->ttl = 20.0;
	np_msgproperty_register(pong_props);
	//register the listener function to receive data from the sender
	np_set_listener(receive_pong, "pong");
	/** \endcode */

	log_msg(LOG_INFO, "Sending initial ping");
	// send an initial ping
	np_send_text("ping", "ping", _ping_count++, NULL);

	/**
 	loop (almost) forever, you're done :-)

 	.. code-block:: c
	\code
 	*/
	while (1)
	{
		ev_sleep(0.9);
	}
	/** \endcode */
}