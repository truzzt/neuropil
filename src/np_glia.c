/**
 *  copyright 2015 pi-lar GmbH
 *  Stephan Schwichtenberg
 **/
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netdb.h>
#include <inttypes.h>

#include "sodium.h"
#include "event/ev.h"

#include "np_glia.h"

#include "dtime.h"
#include "np_log.h"
#include "neuropil.h"
#include "np_axon.h"
#include "np_aaatoken.h"
#include "np_jobqueue.h"
#include "np_tree.h"
#include "np_key.h"
#include "np_keycache.h"
#include "np_list.h"
#include "np_message.h"
#include "np_msgproperty.h"
#include "np_network.h"
#include "np_node.h"
#include "np_route.h"
#include "np_threads.h"
#include "np_util.h"
#include "np_val.h"

static np_bool __exit_libev_loop = FALSE;

static pthread_mutex_t __libev_mutex = PTHREAD_MUTEX_INITIALIZER;

// the optimal libev run interval remains to be seen
// if set too low, base cpu usage increases on no load
static uint8_t __suspended_libev_loop = 0;
static double  __libev_interval = 0.0031415;

static uint8_t __leafset_check_type = 0;
static double  __leafset_check_period = 3.1415;

static double  __token_retransmit_period = 3.1415;

static double  __logfile_flush_period = 0.31415;

static double  __cleanup_interval = 0.31415;

/**
 ** np_route:
 ** routes a message one step closer to its destination key. Delivers
 ** the message to its destination if it is the current host through the
 ** deliver upcall, otherwise it makes the route upcall
 **/
void _np_route_lookup(np_jobargs_t* args)
{
	log_msg(LOG_TRACE, ".start.np_route_lookup");
	np_state_t* state = _np_state();

	np_sll_t(np_key_t, tmp) = NULL;
	np_key_t* target_key = NULL;
	np_message_t* msg_in = args->msg;

	char* msg_subject = tree_find_str(msg_in->header, NP_MSG_HEADER_SUBJECT)->val.value.s;
	char* msg_address = tree_find_str(msg_in->header, NP_MSG_HEADER_TO)->val.value.s;

	np_bool is_a_join_request = FALSE;
	if (0 == strncmp(msg_subject, _NP_MSG_JOIN_REQUEST, strlen(_NP_MSG_JOIN_REQUEST)) )
	{
		is_a_join_request = TRUE;
	}

	np_dhkey_t search_key;
	_str_to_dhkey(msg_address, &search_key);
	np_key_t k_msg_address = { .dhkey = search_key };

	// first lookup call for target key
	log_msg(LOG_DEBUG, "message target is key %s", _key_as_str(&k_msg_address));

	_LOCK_MODULE(np_routeglobal_t)
	{
		// 1 means: always send out message to another node first, even if it returns
		tmp = route_lookup(&k_msg_address, 1);
		if ( 0 < sll_size(tmp) )
			log_msg(LOG_DEBUG, "route_lookup result 1 = %s", _key_as_str(sll_first(tmp)->val));
	}

	if ( NULL != tmp                &&
		 0    < sll_size(tmp)       &&
		 FALSE == is_a_join_request &&
		 (_dhkey_equal(&sll_first(tmp)->val->dhkey, &state->my_node_key->dhkey)) )
	{
		// the result returned the sending node, try again with a higher count parameter
		sll_free(np_key_t, tmp);

		_LOCK_MODULE(np_routeglobal_t)
		{
			tmp = route_lookup(&k_msg_address, 2);
			log_msg(LOG_DEBUG, "route_lookup result 2 = %s", _key_as_str(sll_first(tmp)->val));
		}
		// TODO: increase count parameter again ?
	}

	_np_key_t_del(&k_msg_address);

	if (NULL  != tmp           &&
		0     <  sll_size(tmp) &&
		FALSE == _dhkey_equal(&sll_first(tmp)->val->dhkey, &state->my_node_key->dhkey))
	{
		target_key = sll_first(tmp)->val;
		log_msg(LOG_DEBUG, "route_lookup result   = %s", _key_as_str(target_key));
	}

	/* if I am the only host or the closest host is me, deliver the message */
	// TODO: not working ?
	if (NULL  == target_key &&
		FALSE == is_a_join_request)
	{
		// the message has to be handled by this node (e.g. msg interest messages)
		log_msg(LOG_DEBUG, "internal routing for subject '%s'", msg_subject);
		np_message_t* msg_to_submit = NULL;

		if (TRUE == args->msg->is_single_part)
		{
			// sum up message parts if the message is for this node
			msg_to_submit = np_message_check_chunks_complete(args);
			if (NULL == msg_to_submit)
			{
				sll_free(np_key_t, tmp);
				log_msg(LOG_TRACE, ".end  .np_route_lookup");
				return;
			}
			np_message_deserialize_chunked(msg_to_submit);
		}
		else
		{
			msg_to_submit = args->msg;
		}

		np_msgproperty_t* prop = np_msgproperty_get(INBOUND, msg_subject);
		if (prop != NULL)
		{
			_np_job_submit_msgin_event(0.0, prop, state->my_node_key, msg_to_submit);
		}
	}
	else /* otherwise, hand it over to the np_axon sending unit */
	{
		log_msg(LOG_DEBUG, "forward routing for subject '%s'", msg_subject);

		if (NULL == target_key || TRUE == is_a_join_request)
		{
			target_key = args->target;
		}

		np_msgproperty_t* prop = np_msgproperty_get(OUTBOUND, msg_subject);
		if (NULL == prop)
			prop = np_msgproperty_get(OUTBOUND, _DEFAULT);

		if (TRUE == args->is_resend)
			_np_job_resubmit_msgout_event(0.0, prop, target_key, args->msg);
		else
			_np_job_submit_msgout_event(0.0, prop, target_key, args->msg);

		/* set next hop to the next node */
// 		// TODO: already routed by forward message call ?
// 		// why is there an additional message_send directive here ?
//	    while (!message_send (state->messages, host, message, TRUE, 1))
//		{
//		    host->failuretime = dtime ();
//		    log_msg(LOG_WARN,
//				    "message send to host: %s:%hd at time: %f failed!",
//				    host->dns_name, host->port, host->failuretime);
//
//		    /* remove the faulty node from the routing table */
//		    if (host->success_avg < BAD_LINK) route_update (state->routes, host, 0);
//		    if (tmp != NULL) free (tmp);
//		    tmp = route_lookup (state->routes, *key, 1, 0);
//		    host = tmp[0];
//		    log_msg(LOG_WARN, "re-route through %s:%hd!", host->dns_name, host->port);
//		}
	}

	sll_free(np_key_t, tmp);
	log_msg(LOG_TRACE, ".end  .np_route_lookup");
}

void _np_never_called(np_jobargs_t* args)
{
	log_msg(LOG_WARN, "!!!                               !!!");
	log_msg(LOG_WARN, "!!! wrong job execution requested !!!");
	if (NULL != args)
	{
		log_msg(LOG_WARN, "!!! a: %p m: %p p: %p t: %p", args, args->msg, args->properties, args->target);
		if (args->properties)
			log_msg(LOG_WARN, "!!! properties: %s ", args->properties->msg_subject);
		if (args->target)
			log_msg(LOG_WARN, "!!! target: %s ", _key_as_str(args->target));
	}
	log_msg(LOG_WARN, "!!!                               !!!");
	log_msg(LOG_WARN, "!!!                               !!!");
}

/**
 ** flushes the data of the log buffer to the filesystem in a async callback way
 **/
void _np_write_log(np_jobargs_t* args)
{
	// log_msg(LOG_TRACE, "start np_write_log");
	_np_log_fflush();
	np_job_submit_event(__logfile_flush_period, _np_write_log);
	// log_msg(LOG_TRACE, "end   np_write_log");
}

/** np_check_leafset: runs as a separate thread.
 ** it should send a PING message to each member of the leafset frequently and
 ** sends the leafset to other members of its leafset periodically.
 ** pinging frequency is LEAFSET_CHECK_PERIOD.
 **/
void _np_check_leafset(np_jobargs_t* args)
{
	log_msg(LOG_TRACE, ".start.np_check_leafset");

	np_sll_t(np_key_t, leafset) = NULL;
	np_key_t *tmp_node_key = NULL;

	log_msg(LOG_DEBUG, "leafset check for neighbours started");

	// each time to try to ping our leafset hosts
	_LOCK_MODULE(np_routeglobal_t)
	{
		leafset = route_neighbors();
	}

	while (NULL != (tmp_node_key = sll_head(np_key_t, leafset)))
	{
		// check for bad link nodes
		if (tmp_node_key->node->success_avg < BAD_LINK
				&& tmp_node_key->node->handshake_status > HANDSHAKE_UNKNOWN)
		{
			log_msg(LOG_DEBUG, "deleting from neighbours: %s", _key_as_str(tmp_node_key));
			// request a new handshake with the node
			_LOCK_MODULE(np_routeglobal_t)
			{
				// if (NULL != tmp_node_key->aaa_token)
				// tmp_node_key->aaa_token->state &= AAA_INVALID;
				// TODO: crashes at this point when another node has left the building :-(
				// tmp_node_key->node->handshake_status = HANDSHAKE_UNKNOWN;

				np_key_t *added = NULL, *deleted = NULL;
				leafset_update(tmp_node_key, 0, &deleted, &added);
				if (deleted == tmp_node_key)
				{
					np_unref_obj(np_key_t, deleted);
				}
			}

		}
		else
		{
			/* otherwise request reevaluation of peer */
			double delta = ev_time() - tmp_node_key->node->last_success;
			if (delta > (3 * __leafset_check_period))
				_np_ping(tmp_node_key);
		}
	}
	sll_free(np_key_t, leafset);

	if (__leafset_check_type == 1)
	{
		log_msg(LOG_DEBUG, "leafset check for table started");
		np_sll_t(np_key_t, table) = NULL;
		_LOCK_MODULE(np_routeglobal_t)
		{
			table = _np_route_get_table();
		}

		while ( NULL != (tmp_node_key = sll_head(np_key_t, table)))
		{
			// send update of new node to all nodes in my routing table
			/* first check for bad link nodes */
			if (tmp_node_key->node->success_avg < BAD_LINK &&
				tmp_node_key->node->handshake_status > HANDSHAKE_UNKNOWN)
			{
				log_msg(LOG_DEBUG, "deleting from table: %s", _key_as_str(tmp_node_key));
				// request a new handshake with the node
				_LOCK_MODULE(np_routeglobal_t)
				{
					if (NULL != tmp_node_key->aaa_token)
						tmp_node_key->aaa_token->state &= AAA_INVALID;
					tmp_node_key->node->handshake_status = HANDSHAKE_UNKNOWN;

					np_key_t *added = NULL, *deleted = NULL;
					route_update(tmp_node_key, FALSE, &deleted, &added);
					if (deleted == tmp_node_key)
					{
						np_unref_obj(np_key_t, deleted);
					}
				}
			}
			else
			{
				/* otherwise request re-evaluation of node stats */
				double delta = ev_time() - tmp_node_key->node->last_success;
				if (delta > (3 * __leafset_check_period))
					_np_ping(tmp_node_key);
			}
		}
		sll_free(np_key_t, table);
	}

	if (__leafset_check_type == 2)
	{
		/* send leafset exchange data every 3 times that pings the leafset */
		log_msg(LOG_DEBUG, "leafset exchange for neighbours started");

		_LOCK_MODULE(np_routeglobal_t)
		{
			leafset = route_neighbors();
		}

		while ( NULL != (tmp_node_key = sll_head(np_key_t, leafset)))
		{
			// send a piggy message to the the nodes in our routing table
			np_msgproperty_t* piggy_prop = np_msgproperty_get(TRANSFORM, _NP_MSG_PIGGY_REQUEST);
			_np_job_submit_transform_event(0.0, piggy_prop, tmp_node_key, NULL);
		}
		sll_free(np_key_t, leafset);

		__leafset_check_type = 0;
	}
	else
	{
		__leafset_check_type++;
	}
	// np_mem_printpool();
	np_job_submit_event(__leafset_check_period, _np_check_leafset);
	log_msg(LOG_TRACE, ".end  .np_check_leafset");
}

/**
 ** np_retransmit_tokens
 ** retransmit tokens on a regular interval
 ** default ttl value for message exchange tokens is ten seconds, afterwards they will be invalid
 ** and a new token is required. this also ensures that the correct encryption key will be transmitted
 **/
void _np_retransmit_tokens(np_jobargs_t* args)
{
	// log_msg(LOG_TRACE, "start np_retransmit_tokens");
	np_state_t* state = _np_state();

	np_tree_elem_t *iter = NULL;
	np_tree_elem_t *deleted = NULL;
	np_msgproperty_t* msg_prop = NULL;

	// TODO: crahses sometimes ...
	RB_FOREACH(iter, np_tree_s, state->msg_tokens)
	{
		// double now = dtime();
		// double last_update = iter->val.value.d;
		np_dhkey_t target_dhkey = dhkey_create_from_hostport(iter->key.value.s, "0");
		np_key_t* target = NULL;
		np_new_obj(np_key_t, target);
		target->dhkey = target_dhkey;

		msg_prop = np_msgproperty_get(TRANSFORM, iter->key.value.s);
		if (NULL != msg_prop)
		{
			// _np_send_msg_interest(iter->key.value.s);
			// _np_send_msg_availability(iter->key.value.s);
			_np_job_submit_transform_event(0.0, msg_prop, target, NULL);
		}
		else
		{
			deleted = RB_REMOVE(np_tree_s, state->msg_tokens, iter);
			free(deleted->key.value.s);
			free(deleted);
			break;
		}
	}

	if (TRUE == state->enable_realm_master)
	{
		np_msgproperty_t* msg_prop = NULL;

		np_dhkey_t target_dhkey = dhkey_create_from_hostport(state->my_identity->aaa_token->realm, "0");
		np_key_t* target;
		np_new_obj(np_key_t, target);
		target->dhkey = target_dhkey;

		msg_prop = np_msgproperty_get(INBOUND, _NP_MSG_AUTHENTICATION_REQUEST);
		msg_prop->clb_transform = _np_send_sender_discovery;
		// _np_send_sender_discovery(0.0, msg_prop, target, NULL);
		_np_job_submit_transform_event(0.0, msg_prop, target, NULL);

		msg_prop = np_msgproperty_get(INBOUND, _NP_MSG_AUTHORIZATION_REQUEST);
		msg_prop->clb_transform = _np_send_sender_discovery;
		if (NULL == msg_prop->msg_audience)
		{
			strncpy(msg_prop->msg_audience, state->my_identity->aaa_token->realm, 255);
		}
		_np_job_submit_transform_event(0.0, msg_prop, target, NULL);

		msg_prop = np_msgproperty_get(INBOUND, _NP_MSG_ACCOUNTING_REQUEST);
		msg_prop->clb_transform = _np_send_sender_discovery;
		if (NULL == msg_prop->msg_audience)
		{
			strncpy(msg_prop->msg_audience, state->my_identity->aaa_token->realm, 255);
		}
		_np_job_submit_transform_event(0.0, msg_prop, target, NULL);

		np_free_obj(np_key_t, target);
	}
	np_job_submit_event(__token_retransmit_period, _np_retransmit_tokens);
}

/**
 ** _np_events_read
 ** schedule the libev event loop one time and reschedule again
 **/
void _np_events_read(np_jobargs_t* args)
{
	if (TRUE == __exit_libev_loop) return;

	pthread_mutex_lock(&__libev_mutex);
	if (__suspended_libev_loop == 0)
	{
		EV_P = ev_default_loop(EVFLAG_AUTO | EVFLAG_FORKCHECK);
		ev_run(EV_A_ (EVRUN_ONCE | EVRUN_NOWAIT));
		// ev_run(EV_A_ 0);
	}
	pthread_mutex_unlock(&__libev_mutex);

	np_job_submit_event(__libev_interval, _np_events_read);
}

void _np_suspend_event_loop()
{
	pthread_mutex_lock(&__libev_mutex);
	__suspended_libev_loop++;
	pthread_mutex_unlock(&__libev_mutex);
}

void _np_resume_event_loop()
{
	pthread_mutex_lock(&__libev_mutex);
	__suspended_libev_loop--;
	pthread_mutex_unlock(&__libev_mutex);
}

/**
 ** _np_cleanup
 ** general resend mechanism. all message which have an acknowledge indicator set are stored in
 ** memory. If the acknowledge has not been send in time, we try to redeliver the message, otherwise
 ** the message gets deleted or dropped (if max redelivery has been reached)
 ** redelivery has two aspects -> simple resend or reroute because of bad link nodes in the routing table
 **/
void _np_cleanup(np_jobargs_t* args)
{
	log_msg(LOG_TRACE, ".start.np_cleanup");

	np_state_t* state = _np_state();
	np_network_t* ng = state->my_node_key->network;

	np_tree_elem_t *jrb_ack_node = NULL;

	// wake up and check for acknowledged messages
	pthread_mutex_lock(&ng->lock);

	np_tree_elem_t* iter = RB_MIN(np_tree_s, ng->waiting);
	while (iter != NULL)
	{
		jrb_ack_node = iter;
		iter = RB_NEXT(np_tree_s, ng->waiting, iter);

		np_ackentry_t *ackentry = (np_ackentry_t *) jrb_ack_node->val.value.v;
		if (TRUE == ackentry->acked &&
			ackentry->expected_ack == ackentry->received_ack)
		{
			// update latency and statistics for a node
			double latency = ackentry->acktime - ackentry->transmittime;

			np_node_update_latency(ackentry->dest_key->node, latency);
			np_node_update_stat(ackentry->dest_key->node, 1);

			np_unref_obj(np_key_t, ackentry->dest_key);

			RB_REMOVE(np_tree_s, ng->waiting, jrb_ack_node);
			free(ackentry);
			free(jrb_ack_node->key.value.s);
			free(jrb_ack_node);

			continue;
		}

		if (ev_time() > ackentry->expiration)
		{
			np_node_update_stat(ackentry->dest_key->node, 0);
			np_unref_obj(np_key_t, ackentry->dest_key);

			RB_REMOVE(np_tree_s, ng->waiting, jrb_ack_node);
			free(ackentry);
			free(jrb_ack_node->key.value.s);
			free(jrb_ack_node);

			continue;
		}
	}
	pthread_mutex_unlock(&ng->lock);

	// submit the function itself for additional execution
	np_job_submit_event(__cleanup_interval, _np_cleanup);
	log_msg(LOG_TRACE, ".end  .np_cleanup");
}

/**
 ** np_network_read:
 ** puts the network layer into listen mode. This thread manages acknowledgements,
 ** delivers incoming messages to the message handlers, and drives the network layer.
 **/

/**
 ** np_send_rowinfo:
 ** sends matching row of its table to the target node
 **/
void _np_send_rowinfo(np_jobargs_t* args)
{
	log_msg(LOG_TRACE, "start np_send_rowinfo");

	np_state_t* state = _np_state();
	np_key_t* target_key = args->target;

	// check for correct target
	log_msg(LOG_DEBUG, "job submit route row info to %s:%s!",
			target_key->node->dns_name, target_key->node->port);

	np_sll_t(np_key_t, sll_of_keys) = NULL;
	/* send one row of our routing table back to joiner #host# */
	_LOCK_MODULE(np_routeglobal_t)
	{
		sll_of_keys = route_row_lookup(target_key);
	}

	if (0 < sll_size(sll_of_keys))
	{
		np_tree_t* msg_body = make_jtree();
		_LOCK_MODULE(np_keycache_t)
		{
			// TODO: maybe locking the cache is not enough and we have to do it more fine grained
			np_encode_nodes_to_jrb(msg_body, sll_of_keys, FALSE);
		}
		np_msgproperty_t* outprop = np_msgproperty_get(OUTBOUND, _NP_MSG_PIGGY_REQUEST);

		np_message_t* msg_out = NULL;
		np_new_obj(np_message_t, msg_out);
		np_message_create(msg_out, target_key, state->my_node_key, _NP_MSG_PIGGY_REQUEST, msg_body);
		_np_job_submit_route_event(0.0, outprop, target_key, msg_out);
		np_free_obj(np_message_t, msg_out);
	}
	sll_free(np_key_t, sll_of_keys);
}


np_aaatoken_t* _np_create_msg_token(np_msgproperty_t* msg_request)
{	log_msg(LOG_TRACE, ".start.np_create_msg_token");

	np_state_t* state = _np_state();

	np_aaatoken_t* msg_token = NULL;
	np_new_obj(np_aaatoken_t, msg_token);

	char msg_uuid_subject[255];
	snprintf(msg_uuid_subject, 255, "urn:np:msg:%s:%d:%f",
			msg_request->msg_subject, msg_request->mode_type, msg_request->ttl);

	// create token
	strncpy(msg_token->realm, state->my_identity->aaa_token->realm, 255);
	strncpy(msg_token->issuer, (char*) _key_as_str(state->my_identity), 255);
	strncpy(msg_token->subject, msg_request->msg_subject, 255);
	if (NULL != msg_request->msg_audience)
	{
		strncpy(msg_token->audience, (char*) msg_request->msg_audience, 255);
	}

	msg_token->uuid =  np_create_uuid(msg_uuid_subject, 0);

	msg_token->not_before = ev_time();
	// TODO: make it configurable for the user
	msg_token->expiration = ev_time() + 10.0; // 10 second valid token

	// add e2e encryption details for sender
	strncpy((char*) msg_token->public_key,
			(char*) state->my_identity->aaa_token->public_key,
			crypto_sign_BYTES);

	tree_insert_str(msg_token->extensions, "mep_type",
			new_val_ush(msg_request->mep_type));
	tree_insert_str(msg_token->extensions, "ack_mode",
			new_val_ush(msg_request->ack_mode));
	tree_insert_str(msg_token->extensions, "max_threshold",
			new_val_ui(msg_request->max_threshold));
	tree_insert_str(msg_token->extensions, "msg_threshold",
			new_val_ui(msg_request->msg_threshold));

	tree_insert_str(msg_token->extensions, "target_node",
			new_val_s((char*) _key_as_str(state->my_node_key)));

	// TODO: useful extension ?
	// unsigned char key[crypto_generichash_KEYBYTES];
	// randombytes_buf(key, sizeof key);

	unsigned char hash[crypto_generichash_BYTES];

	crypto_generichash_state gh_state;
	crypto_generichash_init(&gh_state, NULL, 0, sizeof hash);
	crypto_generichash_update(&gh_state, (unsigned char*) msg_token->realm, strlen(msg_token->realm));
	crypto_generichash_update(&gh_state, (unsigned char*) msg_token->issuer, strlen(msg_token->issuer));
	crypto_generichash_update(&gh_state, (unsigned char*) msg_token->subject, strlen(msg_token->subject));
	crypto_generichash_update(&gh_state, (unsigned char*) msg_token->audience, strlen(msg_token->audience));
	crypto_generichash_update(&gh_state, (unsigned char*) msg_token->uuid, strlen(msg_token->uuid));
	crypto_generichash_update(&gh_state, (unsigned char*) msg_token->public_key, crypto_sign_BYTES);
	// TODO: hash 'not_before' and 'expiration' values as well ?
	crypto_generichash_final(&gh_state, hash, sizeof hash);

	char signature[crypto_sign_BYTES];
	unsigned long long real_size = 0;
	int16_t ret = crypto_sign_detached((unsigned char*)       signature,  &real_size,
							           (const unsigned char*) hash,  crypto_generichash_BYTES,
									   state->my_identity->aaa_token->private_key);
	if (ret < 0)
	{
		log_msg(LOG_WARN, "checksum creation for msgtoken failed, using unsigned msgtoken");
		log_msg(LOG_TRACE, ".end  .np_create_msg_token");
		return msg_token;
	}
	else
	{
		log_msg(LOG_DEBUG, "checksum creation for successful, adding %llu bytes signature", real_size);
		tree_insert_str(msg_token->extensions, NP_HS_SIGNATURE, new_val_bin(signature, real_size));
	}

	log_msg(LOG_TRACE, ".end  .np_create_msg_token");
	return msg_token;
}

void _np_send_msg_interest(const char* subject)
{
	log_msg(LOG_TRACE, ".start.np_send_msg_interest");

	// insert into msg token token renewal queue
	if (NULL == tree_find_str(_np_state()->msg_tokens, subject))
	{
		tree_insert_str(_np_state()->msg_tokens, subject, new_val_v(NULL));

		np_msgproperty_t* msg_prop = np_msgproperty_get(INBOUND, subject);
		msg_prop->mode_type |= TRANSFORM;
		msg_prop->clb_transform = _np_send_sender_discovery;

		np_dhkey_t target_dhkey = dhkey_create_from_hostport(subject, "0");
		np_key_t* target = NULL;
		np_new_obj(np_key_t, target);
		target->dhkey = target_dhkey;

		log_msg(LOG_DEBUG, "registering for message interest token handling");
		_np_job_submit_transform_event(0.0, msg_prop, target, NULL);
		np_free_obj(np_key_t, target);
	}

	log_msg(LOG_TRACE, ".end  .np_send_msg_interest");
}

void _np_send_msg_availability(const char* subject)
{
	log_msg(LOG_TRACE, ".start.np_send_msg_availability");

	// set correct transform handler
	if (NULL == tree_find_str(_np_state()->msg_tokens, subject))
	{
		tree_insert_str(_np_state()->msg_tokens, subject, new_val_v(NULL));

		np_msgproperty_t* msg_prop = np_msgproperty_get(OUTBOUND, subject);
		msg_prop->clb_transform = _np_send_receiver_discovery;

		np_dhkey_t target_dhkey = dhkey_create_from_hostport(subject, "0");
		np_key_t* target = NULL;
		np_new_obj(np_key_t, target);
		target->dhkey = target_dhkey;

		log_msg(LOG_DEBUG, "registering for message available token handling");
		_np_job_submit_transform_event(0.0, msg_prop, target, NULL);
		np_free_obj(np_key_t, target);
	}

	log_msg(LOG_TRACE, ".end  .np_send_msg_availability");
}

// TODO: move this to a function which can be scheduled via jobargs
np_bool _np_send_msg (char* subject, np_message_t* msg, np_msgproperty_t* msg_prop)
{
	msg_prop->msg_threshold++;

	np_aaatoken_t* tmp_token = _np_get_receiver_token(subject);

	if (NULL != tmp_token)
	{
		tree_find_str(tmp_token->extensions, "msg_threshold")->val.value.ui++;

		// first encrypt the relevant message part itself
		np_message_encrypt_payload(msg, tmp_token);

		char* target_node_str = NULL;

		np_tree_elem_t* tn_node = tree_find_str(tmp_token->extensions, "target_node");
		if (NULL != tn_node)
		{
			target_node_str = tn_node->val.value.s;
		}
		else
		{
			target_node_str = tmp_token->issuer;
		}

		np_key_t* receiver_key = NULL;
		np_new_obj(np_key_t, receiver_key);

		np_dhkey_t receiver_dhkey;
		_str_to_dhkey(target_node_str, &receiver_dhkey);
		receiver_key->dhkey = receiver_dhkey;

		tree_replace_str(msg->header, NP_MSG_HEADER_TO, new_val_s(tmp_token->issuer));
		np_msgproperty_t* out_prop = np_msgproperty_get(OUTBOUND, subject);
		_np_job_submit_route_event(0.0, out_prop, receiver_key, msg);

		// decrease threshold counters
		msg_prop->msg_threshold--;

		np_unref_obj(np_aaatoken_t, tmp_token);
		np_free_obj(np_key_t, receiver_key);

		return TRUE;
	}
	else
	{
		LOCK_CACHE(msg_prop)
		{
			// cache already full ?
			if ( msg_prop->max_threshold <= msg_prop->msg_threshold )
			{
				log_msg(LOG_DEBUG,
						"msg cache full, checking overflow policy ...");

				if (0 < (msg_prop->cache_policy & OVERFLOW_PURGE))
				{
					log_msg(LOG_DEBUG,
							"OVERFLOW_PURGE: discarding first message");
					np_message_t* old_msg = NULL;

					if (0 < (msg_prop->cache_policy & FIFO))
						old_msg = sll_tail(np_message_t, msg_prop->msg_cache);
					if (0 < (msg_prop->cache_policy & FILO))
						old_msg = sll_head(np_message_t, msg_prop->msg_cache);

					// initial check could lead to false positive messages
					// so better check again
					if (NULL != old_msg)
					{
						msg_prop->msg_threshold--;
						np_unref_obj(np_message_t, old_msg);
					}
				}

				if (0 < (msg_prop->cache_policy & OVERFLOW_REJECT))
				{
					log_msg(LOG_DEBUG,
							"rejecting new message because cache is full");
					// np_free_obj(np_message_t, msg);
					// jump out of LOCK_CACHE
					continue;
				}
			}

			// always prepend, FIFO / FILO handling done when fetching messages
			sll_prepend(np_message_t, msg_prop->msg_cache, msg);

			log_msg(LOG_DEBUG, "added message to the msgcache (%p / %d) ...",
					msg_prop->msg_cache, sll_size(msg_prop->msg_cache));
			np_ref_obj(np_message_t, msg);
		}
	}
	return FALSE;
}
