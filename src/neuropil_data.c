//
// neuropil is copyright 2016-2019 by pi-lar GmbH
// Licensed under the Open Software License (OSL 3.0), please see LICENSE file for details
//
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "msgpack/cmp.h"

#include "neuropil.h"
#include "neuropil_data.h"
#include "np_serialization.h"
#include "np_util.h"

struct np_datablock
{
    uint32_t magic_no;
    size_t total_length;
    size_t used_length;
    cmp_ctx_t cmp;
    unsigned char * data;
} NP_PACKED(1);

enum np_return np_init_datablock(np_datablock_t *block, size_t block_length)
{
    if (block_length <= sizeof(struct np_datablock))
    {
        return np_invalid_argument;
    }

    struct np_datablock * s_block = block;
    s_block->total_length = block_length;
    s_block->used_length = sizeof(struct np_datablock);
    s_block->magic_no = NP_DATA_MAGIC_NO;
    s_block->data = (s_block+1);

    printf("%p + %"PRIu32" = %p\n",s_block, sizeof(struct np_datablock), s_block->data);
    cmp_init(&(s_block->cmp), s_block->data, _np_buffer_reader, _np_buffer_skipper, _np_buffer_writer);

    return np_ok;
}

enum np_return np_set_data(np_datablock_t *block, struct np_data_conf data_conf, unsigned char *data)
{
    struct np_datablock *s_block = block;
    if (block == NULL || s_block->magic_no != NP_DATA_MAGIC_NO)
    {
        return np_invalid_argument;
    }

    // check for data space
    size_t key_size = strnlen(data_conf.key, 255) * sizeof(char) + sizeof(uint8_t)/*msgpack marker*/;
    size_t new_size = key_size + s_block->used_length;
    if(data_conf.type == NP_DATA_TYPE_BIN) {
        new_size += data_conf.data_size + sizeof(uint8_t)/*msgpack marker*/;
    }
    if (new_size > s_block->total_length || new_size < s_block->used_length /*uint overflow check*/)
    {
        return np_invalid_argument;
    }
    s_block->used_length = new_size;
    cmp_write_str(&s_block->cmp, data_conf.key, strnlen(data_conf.key, 255));
    if(data_conf.type == NP_DATA_TYPE_BIN) {
        cmp_write_bin(&s_block->cmp, data, data_conf.data_size);
    }

    return np_ok;
}

enum np_return np_get_data(np_datablock_t *block, char key[255], struct np_data_conf *out_data_config, unsigned char **out_data)
{
    enum np_return ret = np_key_not_found;
    if (key == NULL || block == NULL || out_data_config == NULL)
    {
        return np_invalid_argument;
    }

    struct np_datablock * s_block = block;

    if(s_block->magic_no != NP_DATA_MAGIC_NO){ 
        return np_invalid_argument; 
    }

    cmp_ctx_t cmp;
    cmp_init(&cmp, s_block->data, _np_buffer_reader, _np_buffer_skipper, _np_buffer_writer);
    size_t remaining_read = s_block->used_length - sizeof(struct np_datablock);
    unsigned char * data;
    uint32_t key_size;
    while (remaining_read > 0)
    {
        key_size = MIN(remaining_read, 255);
        out_data_config->data_size = remaining_read;        
        cmp_read_str(&s_block->cmp, out_data_config->key, &key_size);
        cmp_read_bin(&s_block->cmp, data, &out_data_config->data_size);
        
        printf("s_block->data:              %p\n",s_block->data);
        printf("remaining_read:             %"PRIu32"\n",remaining_read);
        printf("key_size:                   %"PRIu32"\n",key_size);
        printf("out_data_config->key:       \"%s\"\n",out_data_config->key);
        printf("out_data_config->data_size: %"PRIu32"\n",out_data_config->data_size);

        if (strncmp(key, out_data_config->key, 255) == 0)
        {
            printf("  J1\n");
            ret = np_ok;
            break;
        }
        if( remaining_read <= out_data_config->data_size + key_size){
            printf("  J2\n");
            break;
        }else{
            printf("  J3\n");
            remaining_read -= out_data_config->data_size + key_size + 2 * sizeof(uint8_t)/*msgpack marker*/;
        }
       

    }
    return ret;
}

enum np_return  np_serialize_datablock(np_datablock_t * block, void ** out_raw_block, size_t * out_raw_block_size)
{
    enum np_return ret = np_not_implemented;
    if (block == NULL || out_raw_block == NULL)
    {
        return np_invalid_argument;
    }

    struct np_datablock *s_block = block;
    out_raw_block_size = s_block->used_length - sizeof(struct np_datablock);
    *out_raw_block = s_block->data;

    return np_ok;
}

enum np_return np_deserialize_datablock(np_datablock_t ** out_block, void *raw_block, size_t raw_block_length)
{
    enum np_return ret = np_not_implemented;

    struct np_datablock * s_block = malloc(sizeof(struct np_datablock));
    s_block->magic_no = NP_DATA_MAGIC_NO;
    s_block->total_length = raw_block_length + sizeof(struct np_datablock);
    s_block->used_length = s_block->total_length;
    s_block->data = raw_block;
    cmp_init(&s_block->cmp, s_block->data, _np_buffer_reader, _np_buffer_skipper, _np_buffer_writer);

    *out_block = s_block;
    return np_ok;
}
