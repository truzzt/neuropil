/**
 *  neuropil is copyright 2015 by pi-lar GmbH
 */

static np_msgproperty_t __default_properties =
{
		.msg_subject = _DEFAULT,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = DEFAULT_TYPE,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 0,
		.clb_inbound = _np_in_received,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

static np_msgproperty_t __handshake_properties =
{
		.msg_subject = _NP_MSG_HANDSHAKE,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | TRANSFORM,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 0,
		.clb_outbound = _np_out_handshake,
		.clb_inbound = _np_in_handshake,
		.clb_transform = _np_out_handshake,
		.clb_route = _np_never_called,
		.ttl = 20.0,
		.max_threshold = 5
};

// we don't need to ack the ack the ack the ack ...
np_msgproperty_t __ack_properties =
{
		.msg_subject = _NP_MSG_ACK,
		.rep_subject = NULL,
		.mode_type = OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 0,
		.clb_inbound = _np_never_called,
		.clb_outbound = _np_out_ack,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

// join request: node unknown yet, therefore send without ack, explicit ack handling via extra messages
np_msgproperty_t __join_req =
{
		.msg_subject = _NP_MSG_JOIN_REQUEST,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND,
		.mep_type = REQ_REP,
		.priority = 5,
		.ack_mode = ACK_DESTINATION,
		.retry = 5,
		.clb_inbound = _np_in_join_req,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_out_send,
		.ttl = 20.0,
		.max_threshold = 5
};

np_msgproperty_t __join_ack =
{
		.msg_subject = _NP_MSG_JOIN_ACK,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 5,
		.clb_inbound = _np_in_join_ack,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_never_called,
		.ttl = 5.0,
		.max_threshold = 5
};

np_msgproperty_t __join_nack =
{
		.msg_subject = _NP_MSG_JOIN_NACK,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 5,
		.clb_inbound = _np_in_join_nack,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_never_called,
		.ttl = 5.0,
		.max_threshold = 5
};

// leave the network and clean up the mess
np_msgproperty_t __leave =
{
		.msg_subject = _NP_MSG_LEAVE_REQUEST,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 5,
		.clb_inbound = _np_in_leave_req,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

np_msgproperty_t __ping =
{
		.msg_subject = _NP_MSG_PING_REQUEST,
		.rep_subject = _NP_MSG_PING_REPLY,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = REQ_REP,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 5,
		.clb_inbound = _np_in_ping,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 2.0,
		.max_threshold = 5
};

np_msgproperty_t __ping_reply =
{
		.msg_subject = _NP_MSG_PING_REPLY,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 5,
		.clb_inbound = _np_in_pingreply,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 2.0,
		.max_threshold = 5
};

np_msgproperty_t __piggy =
{
		.msg_subject = _NP_MSG_PIGGY_REQUEST,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | TRANSFORM | ROUTE,
		.mep_type = DEFAULT_TYPE,
		.priority = 5,
		.ack_mode = ACK_NONE,
		.retry = 0,
		.clb_inbound = _np_in_piggy,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_send_rowinfo,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

np_msgproperty_t __update =
{
		.msg_subject = _NP_MSG_UPDATE_REQUEST,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 5,
		.clb_inbound = _np_in_update,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl =20.0,
		.max_threshold = 5
};

np_msgproperty_t __discover_receiver =
{
		.msg_subject = _NP_MSG_DISCOVER_RECEIVER,
		.rep_subject = _NP_MSG_AVAILABLE_RECEIVER,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type =	A2A_STICKY_REPLY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 2,
		.clb_inbound = _np_in_discover_receiver,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

np_msgproperty_t __discover_sender =
{
		.msg_subject = _NP_MSG_DISCOVER_SENDER,
		.rep_subject = _NP_MSG_AVAILABLE_SENDER,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = A2A_STICKY_REPLY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 2,
		.clb_inbound = _np_in_discover_sender,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

np_msgproperty_t __available_receiver =
{
		.msg_subject = _NP_MSG_AVAILABLE_RECEIVER,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 2,
		.clb_inbound = _np_in_available_receiver,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

np_msgproperty_t __available_sender =
{
		.msg_subject = _NP_MSG_AVAILABLE_SENDER,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 2,
		.clb_inbound = _np_in_available_sender,
		.clb_outbound = _np_out_send,
		.clb_transform = _np_never_called,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 5
};

np_msgproperty_t __authenticate =
{
		.msg_subject = _NP_MSG_AUTHENTICATION_REQUEST,
		.rep_subject = _NP_MSG_AUTHENTICATION_REPLY,
		.mode_type = OUTBOUND | TRANSFORM | ROUTE,
		.mep_type = A2G_STICKY_REPLY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 5,
		.clb_inbound = _np_in_authenticate,
		.clb_outbound = _np_out_send,
		.clb_route = _np_route_lookup,
		.clb_transform = np_send_authentication_request,
		.cache_policy = FIFO | OVERFLOW_PURGE,
		.ttl = 20.0,
		.max_threshold = 10,
		.lock = PTHREAD_MUTEX_INITIALIZER
};

np_msgproperty_t __authenticate_reply =
{
		.msg_subject = _NP_MSG_AUTHENTICATION_REPLY,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY | STICKY_REPLY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 5,
		.clb_inbound = _np_in_authenticate_reply,
		.clb_outbound = _np_out_send,
		.clb_transform = np_send_authentication_reply,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 10,
		.lock = PTHREAD_MUTEX_INITIALIZER
};

np_msgproperty_t __authorize =
{
		.msg_subject = _NP_MSG_AUTHORIZATION_REQUEST,
		.rep_subject = _NP_MSG_AUTHORIZATION_REPLY,
		.mode_type = OUTBOUND | TRANSFORM | ROUTE,
		.mep_type = G2G_STICKY_REPLY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 5,
		.clb_inbound = _np_in_authorize,
		.clb_outbound = _np_out_send,
		.clb_route = _np_route_lookup,
		.clb_transform = np_send_authorization_request,
		.cache_policy = FIFO | OVERFLOW_PURGE,
		.ttl = 20.0,
		.max_threshold = 10,
		.lock = PTHREAD_MUTEX_INITIALIZER
};

np_msgproperty_t __authorize_reply =
{
		.msg_subject = _NP_MSG_AUTHORIZATION_REPLY,
		.rep_subject = NULL,
		.mode_type = INBOUND | OUTBOUND | ROUTE,
		.mep_type = ONE_WAY,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 5,
		.clb_inbound = _np_in_authorize_reply,
		.clb_outbound = _np_out_send,
		.clb_transform = np_send_authorization_reply,
		.clb_route = _np_route_lookup,
		.ttl = 20.0,
		.max_threshold = 10,
		.lock = PTHREAD_MUTEX_INITIALIZER
};

np_msgproperty_t __account =
{
		.msg_subject = _NP_MSG_ACCOUNTING_REQUEST,
		.rep_subject = NULL,
		.mode_type = OUTBOUND | TRANSFORM | ROUTE,
		.mep_type = GROUP_TO_GROUP,
		.priority = 5,
		.ack_mode = ACK_EACHHOP,
		.retry = 5,
		.clb_inbound = _np_in_account,
		.clb_outbound = _np_out_send,
		.clb_route = _np_route_lookup,
		.clb_transform = np_send_accounting_request,
		.cache_policy = FIFO | OVERFLOW_PURGE,
		.ttl = 20.0,
		.max_threshold = 10,
		.lock = PTHREAD_MUTEX_INITIALIZER
};

np_msgproperty_t* __np_internal_messages[] =
{
		&__default_properties,
		&__handshake_properties,
		&__ack_properties,
		&__join_req,
		&__join_ack,
		&__join_nack,
		&__leave,
		&__ping,
		&__ping_reply,
		&__piggy,
		&__update,
		&__discover_receiver,
		&__discover_sender,
		&__available_receiver,
		&__available_sender,
		&__authenticate,
		&__authenticate_reply,
		&__authorize,
		&__authorize_reply,
		&__account
};
