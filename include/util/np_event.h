//
// neuropil is copyright 2016-2018 by pi-lar GmbH
// Licensed under the Open Software License (OSL 3.0), please see LICENSE file for details
//
#ifndef _NP_UTIL_EVENT_H_
#define _NP_UTIL_EVENT_H_

#include <stdbool.h>
#include "np_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: event definitions need to move to another file
enum event_type {
    evt_noop     = 0x0000, // update time

    evt_internal = 0x0001, // no payload
    evt_external = 0x0002, // no payload

    evt_message  = 0x0010, // payload of type message / external
    evt_token    = 0x0020, // payload of type token
    evt_property = 0x0040, // payload of type msgproperty
    evt_jobargs  = 0x0080, // only for migration: jobargs

    evt_authn_ev = 0x0100,
    evt_authz_ev = 0x0200,

    evt_shutdown = 0x1000,
};

struct np_util_event_s {
    enum event_type type;
    np_state_t* context;
    void *user_data;
};

typedef struct np_util_event_s np_util_event_t;

#ifdef __cplusplus
}
#endif

#endif /* _NP_UTIL_EVENT_H_ */
