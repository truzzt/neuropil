//
// neuropil is copyright 2016-2019 by pi-lar GmbH
// Licensed under the Open Software License (OSL 3.0), please see LICENSE file for details
//
// original version is based on the chimera project

// this file conatins the state machine conditions, transitions and states that an identity can
// have. It is included form np_key.c, therefore there are no extra #include directives.

#include "core/np_comp_identity.h"

#include "neuropil.h"
#include "np_aaatoken.h"
#include "np_key.h"
#include "np_keycache.h"
#include "np_legacy.h"
#include "np_memory.h"
#include "np_message.h"
#include "util/np_event.h"
#include "util/np_statemachine.h"


// IN_SETUP -> IN_USE transition condition / action #1
bool __is_identity_aaatoken(np_util_statemachine_t* statemachine, const np_util_event_t event) {

    np_ctx_memory(statemachine->_user_data);

    log_debug_msg(LOG_DEBUG, "start: void __is_identity_aaatoken(...){");

    bool ret = false;

    if (!ret) ret  = FLAG_CMP(event.type, evt_internal) && FLAG_CMP(event.type, evt_token);
    if ( ret) ret &= (np_memory_get_type(event.user_data) == np_memory_types_np_aaatoken_t);
    if ( ret) 
    {
        NP_CAST(event.user_data, np_aaatoken_t, identity);
        ret &= (  identity->type == np_aaatoken_type_identity                             ) ||
               ( (identity->type == np_aaatoken_type_node) && identity->private_key_is_set);

        ret &= _np_aaatoken_is_valid(identity, identity->type);
    }
    return ret;
}
// IN_USE -> IN_DESTROY transition condition / action #1
bool __is_identity_invalid(np_util_statemachine_t* statemachine, const np_util_event_t event) 
{
    np_ctx_memory(statemachine->_user_data);

    log_debug_msg(LOG_DEBUG, "start: void __is_identity_invalid(...){");

    bool ret = false;
    
    NP_CAST(statemachine->_user_data, np_key_t, my_identity_key);
    
    if (!ret) ret = (sll_size(my_identity_key->entities) == 1);
    if ( ret) {
        NP_CAST(sll_first(my_identity_key->entities)->val, np_aaatoken_t, identity);
        ret &= (  identity->type == np_aaatoken_type_identity                             ) ||
               ( (identity->type == np_aaatoken_type_node) && identity->private_key_is_set);
        ret &= !_np_aaatoken_is_valid(identity, identity->type);
    }
    return ret;
}

bool __is_identity_authn(np_util_statemachine_t* statemachine, const np_util_event_t event)
{
    
}

void __np_identity_update(np_util_statemachine_t* statemachine, const np_util_event_t event) { }

void __np_identity_destroy(np_util_statemachine_t* statemachine, const np_util_event_t event) 
{    
    np_ctx_memory(statemachine->_user_data);

    log_debug_msg(LOG_DEBUG, "start: void __np_identity_destroy(...){");

    NP_CAST(statemachine->_user_data, np_key_t, my_identity_key);

    NP_CAST( sll_first(my_identity_key->entities)->val, np_aaatoken_t, identity );
    np_unref_obj(np_aaatoken_t, identity, ref_key_aaa_token);

    _np_keycache_remove(context, my_identity_key->dhkey);
    my_identity_key->is_in_keycache = false;

    sll_clear(void_ptr, my_identity_key->entities);

    my_identity_key->type = np_key_type_unknown;
}

void __np_set_identity(np_util_statemachine_t* statemachine, const np_util_event_t event) 
{    
    np_ctx_memory(statemachine->_user_data);

    log_debug_msg(LOG_DEBUG, "start: void _np_set_identity(...){");

    NP_CAST(statemachine->_user_data, np_key_t, my_identity_key);
    NP_CAST(event.user_data, np_aaatoken_t, identity);

    np_ref_switch(np_aaatoken_t, identity, ref_key_aaa_token, identity);

    if (identity->type == np_aaatoken_type_node) 
    {
        sll_append(void_ptr, my_identity_key->entities, identity);

        context->my_node_key = my_identity_key;
        if (NULL == context->my_identity) 
        {
            my_identity_key->type |= np_key_type_ident;
            context->my_identity = my_identity_key;
        }
    }
    else if(identity->type == np_aaatoken_type_identity)
    {
        sll_append(void_ptr, my_identity_key->entities, identity);

        if ( identity->private_key_is_set && 
             (NULL == context->my_identity || context->my_identity == context->my_node_key) )
        {
            context->my_identity = my_identity_key;
        }
    }

    my_identity_key->is_in_keycache = true;            

    // to be moved
    if (context->my_node_key != NULL &&
        _np_key_cmp(my_identity_key, context->my_node_key) != 0) 
    {
        np_dhkey_t node_dhkey = np_aaatoken_get_fingerprint(context->my_node_key->aaa_token, false);
        np_aaatoken_set_partner_fp(context->my_identity->aaa_token, node_dhkey);
        _np_aaatoken_update_extensions_signature(context->my_node_key->aaa_token);
        
        np_dhkey_t ident_dhkey = np_aaatoken_get_fingerprint(context->my_identity->aaa_token, false);
        np_aaatoken_set_partner_fp(context->my_node_key->aaa_token, ident_dhkey);
    }
    
    _np_aaatoken_update_extensions_signature(identity);
    identity->state = AAA_VALID | AAA_AUTHENTICATED | AAA_AUTHORIZED;
    
    _np_statistics_update_prometheus_labels(context, NULL);

#ifdef DEBUG
    char ed25519_pk[crypto_sign_ed25519_PUBLICKEYBYTES*2+1]; ed25519_pk[crypto_sign_ed25519_PUBLICKEYBYTES*2] = '\0';
    char curve25519_pk[crypto_scalarmult_curve25519_BYTES*2+1]; curve25519_pk[crypto_scalarmult_curve25519_BYTES*2] = '\0';
    
    sodium_bin2hex(ed25519_pk, crypto_sign_ed25519_PUBLICKEYBYTES*2+1, identity->crypto.ed25519_public_key, crypto_sign_ed25519_PUBLICKEYBYTES);
    sodium_bin2hex(curve25519_pk, crypto_scalarmult_curve25519_BYTES*2+1, identity->crypto.derived_kx_public_key, crypto_scalarmult_curve25519_BYTES);
    
    log_debug_msg(LOG_DEBUG, "identity token: my cu pk: %s ### my ed pk: %s", curve25519_pk, ed25519_pk);
#endif
}

void __np_create_identity_network(np_util_statemachine_t* statemachine, const np_util_event_t event)
{
    np_ctx_memory(statemachine->_user_data);

    log_debug_msg(LOG_TRACE, "start: void __np_create_identity_network(...){");

    NP_CAST(statemachine->_user_data, np_key_t, my_identity_key);
    NP_CAST(event.user_data, np_aaatoken_t, identity);

    if (identity->type == np_aaatoken_type_node)
    {
        // create node structure (do we still need it ???)
        np_node_t* my_node = _np_node_from_token(identity, np_aaatoken_type_node);
        sll_append(void_ptr, my_identity_key->entities, my_node);

        // create incoming network
        np_network_t* my_network = NULL;
        np_new_obj(np_network_t, my_network);
        _np_network_init(my_network, true, my_node->protocol, my_node->dns_name, my_node->port, -1, UNKNOWN_PROTO);
        _np_network_set_key(my_network, my_identity_key);

        sll_append(void_ptr, my_identity_key->entities, my_network);

        log_debug_msg(LOG_DEBUG | LOG_NETWORK, "Network %s is the main receiving network", np_memory_get_id(my_network));

        _np_network_enable(my_network);
        _np_network_start(my_network, true);
    }
}

bool __is_unencrypted_np_message(np_util_statemachine_t* statemachine, const np_util_event_t event)
{
    np_ctx_memory(statemachine->_user_data);
    log_debug_msg(LOG_TRACE, "start: bool __is_unencrypted_np_message(...){");
    bool ret = false;

    if (!ret) ret  = FLAG_CMP(event.type, evt_external) && FLAG_CMP(event.type, evt_message);
    if ( ret) ret &= (np_memory_get_type(event.user_data) == np_memory_types_np_message_t);
    if ( ret) 
    {
        NP_CAST(event.user_data, np_message_t, message);
        // TODO: // ret &= _np_message_validate_format(message);
    }
    return true;
}

void __np_identity_extract_handshake(np_util_statemachine_t* statemachine, const np_util_event_t event) 
{
    np_ctx_memory(statemachine->_user_data);
    log_debug_msg(LOG_TRACE, "start: void __np_identity_extract_handshake(...){");

    NP_CAST(event.user_data, np_message_t, message);

    np_message_t* msg_in = NULL;
    bool is_deserialization_successful = _np_message_deserialize_header_and_instructions(msg_in, message);
    if (is_deserialization_successful == false) 
    {
        log_msg(LOG_WARN, "error deserializing initial message from new partner node");
        goto __np_cleanup__;
    }
    log_debug_msg(LOG_SERIALIZATION | LOG_MESSAGE | LOG_DEBUG,
                  "deserialized message %s (source: \"%s\")", msg_in->uuid, np_network_get_desc(alias_key,tmp));
    _np_message_trace_info("in", msg_in);

    CHECK_STR_FIELD_BOOL(msg_in->header, _NP_MSG_HEADER_SUBJECT, msg_subject, "NO SUBJECT IN MESSAGE (%s)", msg_in->uuid);

    char* str_msg_subject = msg_subject->val.value.s;

    log_debug_msg(LOG_ROUTING | LOG_DEBUG, "(msg: %s) received msg", msg_in->uuid);

    // wrap with bloom filter
    if (0 != strncmp(str_msg_subject, _NP_URN_MSG_PREFIX _NP_MSG_HANDSHAKE, strlen(_NP_URN_MSG_PREFIX _NP_MSG_HANDSHAKE)) );
        goto __np_cleanup__;

    np_msgproperty_t* handshake_prop = _np_msgproperty_get(context, INBOUND, _NP_MSG_HANDSHAKE);
    if (_np_msgproperty_check_msg_uniquety(handshake_prop, msg_in)) 
    {
        np_handshake_token_t* handshake_token = NULL;        
        _np_message_deserialize_chunked(message);
        handshake_token = np_token_factory_read_from_tree(context, message->body);

        np_key_t* alias_key = _np_keycache_find_or_create(context, event.target_dhkey);
        np_util_event_t handshake_evt = { .type=(evt_external|evt_token), .context=context, .user_data=handshake_token};
        _np_util_statemachine_auto_transition(&alias_key->sm, handshake_evt);
    }
    else
    {
        log_msg(LOG_DEBUG, "duplicate handshake message detected, dropping it ...");
        goto __np_cleanup__;
    }

    __np_cleanup__:
        np_memory_free(context, message);
        return;
} 