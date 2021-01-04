//
// neuropil is copyright 2016-2021 by pi-lar GmbH
// Licensed under the Open Software License (OSL 3.0), please see LICENSE file for details
//
#include "np_legacy.h"
#include "np_types.h"
#include "np_responsecontainer.h"

#include "np_constants.h"
#include "np_key.h"
#include "np_log.h"
#include "np_memory.h"
#include "np_node.h"

void _np_responsecontainer_t_new(NP_UNUSED np_state_t *context, NP_UNUSED uint8_t type, NP_UNUSED size_t size, void* obj)
{
	np_responsecontainer_t* entry = (np_responsecontainer_t *)obj;

	np_dhkey_t _null = {0};
	// memset(&entry->uuid[0], 0, NP_UUID_BYTES);
	_np_dhkey_assign(&entry->dest_dhkey, &_null);
	_np_dhkey_assign(&entry->msg_dhkey, &_null);

	entry->expires_at  = 0.0;
	entry->received_at = 0.0;
	entry->send_at     = 0.0;
}

void _np_responsecontainer_t_del(np_state_t *context, NP_UNUSED uint8_t type, NP_UNUSED size_t size, void* obj)
{
	// empty
}
