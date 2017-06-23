/*
 * np_scache.c
 *
 *  Created on: 12.04.2017
 *      Author: sklampt
 *
 *      Based upon http://stackoverflow.com/a/1234738
 */

#include <string.h>
#include <stdlib.h>
#include "np_scache.h"
#include "np_list.h"

#include "np_scache.h"

#include "np_threads.h"
#include "np_log.h"
#include "inttypes.h"

NP_SLL_GENERATE_IMPLEMENTATION(np_cache_item_t);

np_cache_item_t* np_simple_cache_get(np_simple_cache_table_t *table, const char *key)
{
    log_msg(LOG_TRACE, "start: np_cache_item_t* np_simple_cache_get(np_simple_cache_table_t *table, const char *key){");
	if(NULL == key){
		log_msg(LOG_ERROR, "cache key cannot be NULL!");
		exit(EXIT_FAILURE);
	}
	unsigned int bucket = _np_simple_cache_strhash(key) % SIMPLE_CACHE_NR_BUCKETS;

	np_cache_item_t_sll_t* bucket_list = table->buckets[bucket];
    sll_iterator(np_cache_item_t) iter = sll_first(bucket_list);
	do
	{
		if(NULL != iter && NULL != iter->val && strcmp(iter->val->key, key) == 0){
			return iter->val;
		}
	} while (NULL != ( sll_next(iter)) );

	return NULL;
}

int np_simple_cache_insert(np_simple_cache_table_t *table, char *key, void *value) {
    log_msg(LOG_TRACE, "start: int np_simple_cache_insert(np_simple_cache_table_t *table, char *key, void *value) {");

	if(NULL == key){
		log_msg(LOG_ERROR, "cache key cannot be NULL!");
		exit(EXIT_FAILURE);
	}

	unsigned int bucket = _np_simple_cache_strhash(key) % SIMPLE_CACHE_NR_BUCKETS;

	np_cache_item_t_sll_t* bucket_list = table->buckets[bucket];

    sll_iterator(np_cache_item_t) iter = sll_first(bucket_list);
	do
	{
		if(NULL != iter && NULL != iter->val && strcmp(iter->val->key, key) == 0){
			break;
		}
	} while (NULL != (sll_next(iter)) );

    np_cache_item_t* item;

    if(NULL == iter) {
    	item = (np_cache_item_t*) malloc(sizeof (np_cache_item_t));
    	CHECK_MALLOC(item);

    	if(item < 0){
    		log_msg(LOG_ERROR, "cannot allocate memory for np_cache_item");
    	}
    	sll_append(np_cache_item_t, bucket_list, item);
    	item->key = strdup(key);
    }else{
    	item = iter->val;
    }
	item->value = value;
	item->insert_time = ev_time();

	return 0;
}

unsigned int _np_simple_cache_strhash(const char *str) {
	uint32_t hash = 0;
	for (; *str; str++)
		hash = 31 * hash + *str;
	return hash;
}
