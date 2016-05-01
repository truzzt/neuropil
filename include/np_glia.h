/**
 *  copyright 2015 pi-lar GmbH
 *  original version was taken from chimera project (MIT licensed), but heavily modified
 *  Stephan Schwichtenberg
 **/
#ifndef _NP_GLIA_H_
#define _NP_GLIA_H_

#include "np_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void _np_route_lookup (np_jobargs_t* args);

// critical self invoking functions
void _np_check_leafset(np_jobargs_t* args);
void _np_retransmit_tokens(np_jobargs_t* args);
void _np_cleanup(np_jobargs_t* args);
void _np_write_log(np_jobargs_t* args);
void _np_events_read(np_jobargs_t* args);

void _np_never_called(np_jobargs_t* args);

void _np_suspend_event_loop();
void _np_resume_event_loop();

// other helper functions
void _np_send_rowinfo (np_jobargs_t* args);

np_aaatoken_t* _np_create_msg_token(np_msgproperty_t* msg_request);

void _np_send_msg_interest(const char* subject);
void _np_send_msg_availability(const char* subject);

np_bool _np_send_msg (char* subject, np_message_t* msg, np_msgproperty_t* msg_prop);

#ifdef __cplusplus
}
#endif

#endif // _NP_GLIA_H_
