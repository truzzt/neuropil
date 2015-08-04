/**
 *  copyright 2015 pi-lar GmbH
 *  original version was taken from chimera project (MIT licensed), but heavily modified
 *  Stephan Schwichtenberg
 **/
#ifndef _NP_NETWORK_H_
#define _NP_NETWORK_H_

#include "include.h"
#include "np_memory.h"

/** 
 ** NETWORK_PACK_SIZE is the maximum packet size that will be handled by neuropil network layer
 */
#define NETWORK_PACK_SIZE 65536

/** 
 ** TIMEOUT is the number of seconds to wait for receiving ack from the destination, if you want 
 ** the sender to wait forever put 0 for TIMEOUT. 
 */
#define TIMEOUT 1.0

struct np_network_s
{
    int socket;

    np_jtree_t* waiting;
    np_jtree_t* retransmit;

    uint32_t seqend;

	pthread_attr_t attr;
    pthread_mutex_t lock;
};

typedef struct np_ackentry_s np_ackentry_t;
struct np_ackentry_s {
	np_bool acked;
	double acktime; // the time when the packet is acked
};

typedef struct np_prioq_s np_prioq_t;
struct np_prioq_s {

	np_key_t* dest_key; // the destination key / next/final hop of the message
	np_message_t* msg;  // message to send

	uint8_t retry;     // number of retries
	uint32_t seqnum; // seqnum to identify the packet to be retransmitted
	double transmittime; // this is the time the packet is transmitted (or retransmitted)
};


/** network_address:
 ** returns the ip address of the #hostname#
 **/
unsigned long get_network_address (char *hostname);

np_ackentry_t* get_new_ackentry();
np_prioq_t* get_new_pqentry();

/** network_init:
 ** initiates the networking layer by creating socket and bind it to #port# 
 **/
np_network_t* network_init (uint16_t port);

/**
 ** network_send: host, data, size
 ** Sends a message to host, updating the measurement info.
 ** type are 1 or 2, 1 indicates that the data should be acknowledged by the
 ** receiver, and 2 indicates that no ack is necessary.
 **/
np_bool network_send_udp (np_state_t* state, np_key_t* node,  np_message_t* msg);

/**
 ** Resends a message to host
 **/
// int network_resend (np_state_t* state, np_node_t *host, np_message_t* message, size_t size, int ack, unsigned long seqnum, double *transtime);

#endif /* _CHIMERA_NETWORK_H_ */
