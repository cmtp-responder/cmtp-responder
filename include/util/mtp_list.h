/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MTP_LIST_H_
#define _MTP_LIST_H_

#include "mtp_datatype.h"
#include "mtp_util.h"

typedef struct _slist_node {
	void *value;
	struct _slist_node *link;
} slist_node_t;

typedef struct {
	slist_node_t *start;
	slist_node_t *end;
	mtp_uint32 nnodes;
} slist_t;

typedef struct {
	slist_node_t *node_ptr;
} slist_iterator;

void _util_init_list(slist_t *l_ptr);
mtp_bool _util_add_node(slist_t *l_ptr, void *data);
slist_node_t *_util_delete_node(slist_t *l_ptr, void *data);
slist_iterator *_util_init_list_iterator(slist_t *l_ptr);
void *_util_get_list_next(slist_iterator *iter);
slist_node_t *_util_get_first_node(slist_t *l_ptr);
void _util_deinit_list_iterator(slist_iterator *iter);

#endif /* _MTP_LIST_H_ */
