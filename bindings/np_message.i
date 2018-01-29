//
// neuropil is copyright 2016-2017 by pi-lar GmbH
// Licensed under the Open Software License (OSL 3.0), please see LICENSE file for details
//

%module(package="neuropil") np_message

%{
#include "np_message.h"
%}

%rename(np_message) np_message_s;
%rename(np_message) np_message_t;


%extend np_message_s {

    %feature ("ref") np_message_s "np_mem_refobj($this->obj, NULL);"
    %feature ("unref") np_message_s "np_mem_unrefobj($this->obj, NULL);"

    %immutable uuid;

	%immutable header;
	%immutable instructions;
	%immutable properties;
	%immutable body;
	%immutable footer;

    %immutable msg_property;

	// only used if the message has to be split up into chunks
    %ignore obj;
	%ignore is_single_part;
	%ignore no_of_chunks;
	%ignore msg_chunks;
	%ignore msg_chunks_lock;
	%ignore on_ack;
    %ignore on_timeout;
	%ignore TSP;
	%ignore is_acked;
	%ignore is_in_timeout;
};

%ignore _np_message_buffer_container_s;

%ignore _np_message_setfooter;
%ignore _np_message_add_bodyentry;
%ignore _np_message_add_instruction;
%ignore _np_message_add_property;
%ignore _np_message_del_bodyentry;
%ignore _np_message_del_footerentry;
%ignore _np_message_del_instruction;
%ignore _np_message_del_property;
%ignore _np_message_setfooter;
%ignore _np_message_add_footerentry;
%ignore _np_message_serialize_chunked;

%include "np_message.h"
