/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2019-2020 The Fluent Bit Authors
 *  Copyright (C) 2015-2018 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include "stackdriver.h"
#include "stackdriver_operation.h"

typedef enum {
    NO_OPERATION = 1,
    OPERATION_EXISTED = 2
} operation_status;


void add_operation_field(flb_sds_t *operation_id, flb_sds_t *operation_producer, 
                         int *operation_first, int *operation_last, 
                         msgpack_packer *mp_pck)
{    
    msgpack_pack_str(mp_pck, 9);
    msgpack_pack_str_body(mp_pck, "operation", 9);
    msgpack_pack_map(mp_pck, 4);
    msgpack_pack_str(mp_pck, 2);
    msgpack_pack_str_body(mp_pck, "id", 2);
    msgpack_pack_str(mp_pck, flb_sds_len(*operation_id));
    msgpack_pack_str_body(mp_pck, *operation_id, flb_sds_len(*operation_id));
    msgpack_pack_str(mp_pck, 8);
    msgpack_pack_str_body(mp_pck, "producer", 8);
    msgpack_pack_str(mp_pck, flb_sds_len(*operation_producer));
    msgpack_pack_str_body(mp_pck, *operation_producer, flb_sds_len(*operation_producer));
    msgpack_pack_str(mp_pck, 5);
    msgpack_pack_str_body(mp_pck, "first", 5);
    if (*operation_first == FLB_TRUE) {
        msgpack_pack_true(mp_pck);
    }
    else {
        msgpack_pack_false(mp_pck);
    }
    
    msgpack_pack_str(mp_pck, 4);
    msgpack_pack_str_body(mp_pck, "last", 4);
    if (*operation_last == FLB_TRUE) {
        msgpack_pack_true(mp_pck);
    }
    else {
        msgpack_pack_false(mp_pck);
    }
}

/* Return true if operation extracted */
int extract_operation(flb_sds_t *operation_id, flb_sds_t *operation_producer, 
                      int *operation_first, int *operation_last, 
                      msgpack_object *obj, int *extra_subfields)
{
    operation_status op_status = NO_OPERATION;

    if (obj->via.map.size != 0) {    	
        msgpack_object_kv *p = obj->via.map.ptr;
        msgpack_object_kv *const pend = obj->via.map.ptr + obj->via.map.size;

        for (; p < pend && op_status == NO_OPERATION; ++p) {
            if (p->val.type == MSGPACK_OBJECT_MAP && p->key.type == MSGPACK_OBJECT_STR
                && strncmp(OPERATION_FIELD_IN_JSON, p->key.via.str.ptr, p->key.via.str.size) == 0) {
                
                op_status = OPERATION_EXISTED;
                msgpack_object sub_field = p->val;
                
                msgpack_object_kv *tmp_p = sub_field.via.map.ptr;
                msgpack_object_kv *const tmp_pend = sub_field.via.map.ptr + sub_field.via.map.size;

                /* Validate the subfields of operation */
                for (; tmp_p < tmp_pend; ++tmp_p) {
                    if (tmp_p->key.type != MSGPACK_OBJECT_STR) {
                        continue;
                    }
                    if (strncmp("id", tmp_p->key.via.str.ptr, tmp_p->key.via.str.size) == 0) {
                        if (tmp_p->val.type != MSGPACK_OBJECT_STR) {
                            continue;
                        }
                        *operation_id = flb_sds_copy(*operation_id, tmp_p->val.via.str.ptr, tmp_p->val.via.str.size);
                    }
                    else if (strncmp("producer", tmp_p->key.via.str.ptr, tmp_p->key.via.str.size) == 0) {
                        if (tmp_p->val.type != MSGPACK_OBJECT_STR) {
                            continue;
                        }
                        *operation_producer = flb_sds_copy(*operation_producer, tmp_p->val.via.str.ptr, tmp_p->val.via.str.size);
                    }
                    else if (strncmp("first", tmp_p->key.via.str.ptr, tmp_p->key.via.str.size) == 0) {
                        if (tmp_p->val.type != MSGPACK_OBJECT_BOOLEAN) {
                            continue;
                        }
                        if (tmp_p->val.via.boolean) {
                            *operation_first = FLB_TRUE;
                        }
                    }
                    else if (strncmp("last", tmp_p->key.via.str.ptr, tmp_p->key.via.str.size) == 0) {
                        if (tmp_p->val.type != MSGPACK_OBJECT_BOOLEAN) {
                            continue;
                        }
                        if (tmp_p->val.via.boolean) {
                            *operation_last = FLB_TRUE;
                        }
                    }
                    else {
                        /* extra sub-fields */ 
                        *extra_subfields += 1;
                    }

                }
            }
        }
    }
    
    return op_status == OPERATION_EXISTED;
}

void pack_extra_operation_subfields(msgpack_packer *mp_pck, msgpack_object *operation, int extra_subfields) {
    msgpack_object_kv *p = operation->via.map.ptr;
    msgpack_object_kv *const pend = operation->via.map.ptr + operation->via.map.size;

    msgpack_pack_map(mp_pck, extra_subfields);

    for (; p < pend; ++p) {
        if (strncmp("id", p->key.via.str.ptr, p->key.via.str.size) != 0 
            && strncmp("producer", p->key.via.str.ptr, p->key.via.str.size) != 0
            && strncmp("first", p->key.via.str.ptr, p->key.via.str.size) != 0
            && strncmp("last", p->key.via.str.ptr, p->key.via.str.size) != 0) {
            msgpack_pack_object(mp_pck, p->key);
            msgpack_pack_object(mp_pck, p->val);
        }
    }

}
