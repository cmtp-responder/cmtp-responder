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

#include <glib.h>
#include "mtp_list.h"

/*
 * FUNCTIONS
 */
static slist_node_t *__util_del_first_node(slist_t *l_ptr);

void _util_init_list(slist_t *l_ptr)
{
	ret_if(l_ptr == NULL);

	l_ptr->start = NULL;
	l_ptr->end = NULL;
	l_ptr->nnodes = 0;

	return;
}

mtp_bool _util_add_node(slist_t *l_ptr, void *data)
{
	retv_if(l_ptr == NULL, FALSE);

	slist_node_t *node = NULL;

	node = (slist_node_t *)g_malloc(sizeof(slist_node_t));
	if (node == NULL) {
		ERR("g_malloc() Fail");
		return FALSE;
	}

	node->value = data;
	node->link = NULL;

	if (l_ptr->start == NULL)
		l_ptr->start = node;
	else
		l_ptr->end->link = node;

	l_ptr->end = node;
	l_ptr->nnodes += 1;
	return TRUE;
}

slist_node_t* _util_delete_node(slist_t *l_ptr, void *data)
{
	retv_if(data == NULL, NULL);
	retv_if(l_ptr == NULL, NULL);
	retv_if(l_ptr->start == NULL, NULL);

	slist_node_t *nptr = l_ptr->start;
	slist_node_t *temp = NULL;

	if (nptr->value == data) {
		return __util_del_first_node(l_ptr);
	}

	while (nptr->link) {
		if (nptr->link->value == data)
			break;
		nptr = nptr->link;
	}

	if (nptr->link == NULL) {
		ERR("Node not found in the list");
		return NULL;
	}

	temp = nptr->link;
	nptr->link = nptr->link->link;
	l_ptr->nnodes -= 1;

	if (temp == l_ptr->end)
		l_ptr->end = nptr;

	return temp;
}

static slist_node_t *__util_del_first_node(slist_t *l_ptr)
{
	slist_node_t *temp = NULL;

	temp = l_ptr->start;
	l_ptr->nnodes -= 1;
	l_ptr->start = temp->link;
	if (temp == l_ptr->end) {
		l_ptr->end = NULL;
	}

	return temp;
}

/* This API will send NULL if list does not have elements */
slist_iterator* _util_init_list_iterator(slist_t *l_ptr)
{
	slist_iterator *temp = NULL;

	retv_if(l_ptr == NULL, NULL);
	retv_if(l_ptr->start == NULL, NULL);

	temp = (slist_iterator *)g_malloc(sizeof(slist_iterator));
	if (temp == NULL) {
		ERR("g_malloc() Fail");
		return NULL;
	}

	temp->node_ptr = l_ptr->start;
	return temp;
}

/* Befor calling this please make sure
   next element exists using _util_check_list_next */
void* _util_get_list_next(slist_iterator *iter)
{
	slist_node_t *temp = NULL;

	retv_if(iter == NULL, NULL);

	temp = iter->node_ptr;
	iter->node_ptr = iter->node_ptr->link;

	return temp->value;
}

slist_node_t* _util_get_first_node(slist_t *l_ptr)
{
	retv_if(l_ptr == NULL, NULL);

	return l_ptr->start;
}

void _util_deinit_list_iterator(slist_iterator *iter)
{
	ret_if(iter == NULL);

	g_free(iter);
}
