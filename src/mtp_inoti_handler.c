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

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "mtp_thread.h"
#include "mtp_inoti_handler.h"
#include "mtp_event_handler.h"
#include "mtp_support.h"
#include "mtp_device.h"
#include "mtp_util.h"

/*
 * GLOBAL AND STATIC VARIABLES
 */
pthread_mutex_t g_cmd_inoti_mutex;

#ifdef MTP_SUPPORT_OBJECTADDDELETE_EVENT
mtp_char g_last_created_dir[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
mtp_char g_last_deleted[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
mtp_char g_last_moved[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
mtp_char g_last_copied[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
static pthread_t g_inoti_thrd;
static mtp_int32 g_cnt_watch_folder = 0;
static mtp_int32 g_inoti_fd;
static open_files_info_t *g_open_files_list;
static inoti_watches_t g_inoti_watches[INOTI_FOLDER_COUNT_MAX];
#endif /*MTP_SUPPORT_OBJECTADDDELETE_EVENT*/

/*
 * STATIC FUNCTIONS
 */
#ifdef MTP_SUPPORT_OBJECTADDDELETE_EVENT
static mtp_bool __process_inoti_event(struct inotify_event *event);
static void __remove_inoti_watch(mtp_char *path);
static mtp_bool __add_file_to_inoti_open_files_list(mtp_int32 wd,
		mtp_char *event_name);
static open_files_info_t *__find_file_in_inoti_open_files_list(mtp_int32 wd,
		mtp_char *event_name);
static void __remove_file_from_inoti_open_files_list(open_files_info_t *node);
static mtp_int32 __get_inoti_watch_id(mtp_int32 iwd);
static mtp_bool __get_inoti_event_full_path(mtp_int32 wd, mtp_char *event_name,
		mtp_char *path, mtp_int32 path_len, mtp_char *parent_path);
static void __remove_recursive_inoti_watch(mtp_char *path);
static void __clean_up_inoti(void *data);
static void __delete_children_from_store_inoti(mtp_store_t *store,
		mtp_obj_t *obj);
static void __process_object_added_event(mtp_char *fullpath,
		mtp_char *file_name, mtp_char *parent_path);
static void __process_object_deleted_event(mtp_char *fullpath,
		mtp_char *file_name, mtp_bool isdir);
static void __destroy_inoti_open_files_list();
#endif /* MTP_SUPPORT_OBJECTADDDELETE_EVENT */

/*
 * FUNCTIONS
 */
#ifdef MTP_SUPPORT_OBJECTADDDELETE_EVENT
void *_thread_inoti(void *arg)
{
	mtp_int32 i = 0;
	mtp_int32 length = 0;
	mtp_int64 temp_idx;
	mtp_char buffer[INOTI_BUF_LEN] = { 0 };
	struct inotify_event *event = NULL;

	pthread_cleanup_push(__clean_up_inoti, NULL);

	DBG("START INOTIFY SYSTEM");

	while (1) {
		pthread_testcancel();
		errno = 0;
		i = 0;
		length = read(g_inoti_fd, buffer, sizeof(buffer));
		if (length < 0) {
			ERR("read() Fail");
			_util_print_error();
			break;
		}

		while (i < length) {
			event = (struct inotify_event *)(&buffer[i]);
			__process_inoti_event(event);
			temp_idx = i + event->len + INOTI_EVENT_SIZE;
			if (temp_idx > length) {
				break;
			} else {
				i = temp_idx;
			}
		}
	}

	DBG("Inoti thread exited");
	pthread_cleanup_pop(1);

	return NULL;
}

void _inoti_add_watch_for_fs_events(mtp_char *path)
{
	mtp_int32 i = 0;

	ret_if(path == NULL);

	if (g_cnt_watch_folder == INOTI_FOLDER_COUNT_MAX) {
		/* find empty cell */
		for (i = 0; i < INOTI_FOLDER_COUNT_MAX; i++) {
			/* If not empty */
			if (g_inoti_watches[i].wd != 0) {
				continue;
			} else {
				break;
			}
		}

		if (i == INOTI_FOLDER_COUNT_MAX) {
			ERR("no empty space for a new inotify watch.");
			return;
		}
		DBG("g_watch_folders[%d] add watch : %s\n", i, path);
		g_inoti_watches[i].forlder_name = g_strdup(path);
		g_inoti_watches[i].wd = inotify_add_watch(g_inoti_fd,
				g_inoti_watches[i].forlder_name,
				IN_CLOSE_WRITE | IN_CREATE |
				IN_DELETE | IN_MOVED_FROM |
				IN_MOVED_TO);
		return;
	}

	DBG("g_watch_folders[%d] add watch : %s\n", g_cnt_watch_folder, path);
	g_inoti_watches[g_cnt_watch_folder].forlder_name = g_strdup(path);
	g_inoti_watches[g_cnt_watch_folder].wd = inotify_add_watch(g_inoti_fd,
			g_inoti_watches[g_cnt_watch_folder].forlder_name,
			IN_CLOSE_WRITE |
			IN_CREATE | IN_DELETE |
			IN_MOVED_FROM |
			IN_MOVED_TO);
	g_cnt_watch_folder++;

	return;
}

mtp_bool _inoti_init_filesystem_evnts()
{
	mtp_bool ret = FALSE;

	g_inoti_fd = inotify_init();
	if (g_inoti_fd < 0) {
		ERR("inotify_init() Fail : g_inoti_fd = %d", g_inoti_fd);
		return FALSE;
	}

	ret = _util_thread_create(&g_inoti_thrd, "File system inotify thread\n",
			PTHREAD_CREATE_JOINABLE, _thread_inoti, NULL);
	if (FALSE == ret) {
		ERR("_util_thread_create() Fail");
		_util_print_error();
		close(g_inoti_fd);
		return FALSE;
	}

	return TRUE;
}

static mtp_bool __process_inoti_event(struct inotify_event *event)
{
	static mtp_int32 last_moved_cookie = -1;

	mtp_bool res = FALSE;
	mtp_char full_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char parentpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	if (event->len == 0 || event->len > MTP_MAX_FILENAME_SIZE) {
		ERR_SECURE("Event len is invalid[%d], event->name[%s]\n", event->len,
				event->name);
		return FALSE;
	} else if (event->wd < 1) {
		ERR("invalid wd : %d\n", event->wd);
		return FALSE;
	}

	/* start of one event */
	res = __get_inoti_event_full_path(event->wd, event->name, full_path,
			sizeof(full_path), parentpath);
	if (res == FALSE) {
		ERR("__get_inoti_event_full_path() Fail");
		return FALSE;
	}

	if (_util_is_path_len_valid(full_path) == FALSE) {
		ERR("path len is invalid");
		return FALSE;
	}
	DBG_SECURE("Event full path = %s\n", full_path);
	if (event->mask & IN_MOVED_FROM) {
		if (!g_strcmp0(g_last_moved, full_path)) {
			/* Ignore this case as this is generated due to MTP*/
			DBG("[%s] is moved_from by MTP\n", full_path);
			memset(g_last_moved, 0,
					MTP_MAX_PATHNAME_SIZE + 1);
			last_moved_cookie = event->cookie;
		} else if (event->mask & IN_ISDIR) {
			DBG("IN_MOVED_FROM --> IN_ISDIR");
			UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
			__process_object_deleted_event(full_path,
					event->name, TRUE);
			UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
		} else {
			DBG("IN_MOVED_FROM --> NOT IN_ISDIR");
			UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
			__process_object_deleted_event(full_path,
					event->name, FALSE);
			UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
		}
	} else if (event->mask & IN_MOVED_TO) {
		DBG("Moved To event, path = [%s]\n", full_path);
		if (last_moved_cookie == event->cookie) {
			/* Ignore this case as this is generated due to MTP*/
			DBG("%s  is moved_to by MTP\n", full_path);
			last_moved_cookie = -1;
		} else {
			UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
			__process_object_added_event(full_path,
					event->name, parentpath);
			UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
		}
	} else if (event->mask & IN_CREATE) {
		if (event->mask & IN_ISDIR) {
			DBG("IN_CREATE --> IN_ISDIR");
			if (!g_strcmp0(g_last_created_dir, full_path)) {
				/* Ignore this case as this is generated due to MTP*/
				DBG("%s folder is generated by MTP\n",
						full_path);
				memset(g_last_created_dir, 0,
						MTP_MAX_PATHNAME_SIZE + 1);
			} else {
				UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
				__process_object_added_event(full_path,
						event->name, parentpath);
				UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
			}
		} else {
			if (FALSE == __add_file_to_inoti_open_files_list(event->wd,
						event->name)) {
				DBG_SECURE("__add_file_to_inoti_open_files_list fail\
						%s\n", event->name);
			}
			DBG("IN_CREATE --> NOT IN_ISDIR");
		}
	} else if (event->mask &  IN_DELETE) {
		if (!g_strcmp0(g_last_deleted, full_path)) {
			/* Ignore this case as this is generated due to MTP*/
			DBG("%s  is deleted by MTP\n", full_path);
			memset(g_last_deleted, 0,
					MTP_MAX_PATHNAME_SIZE + 1);
		} else if (event->mask & IN_ISDIR) {
			DBG("IN_DELETE --> IN_ISDIR");
			UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
			__process_object_deleted_event(full_path,
					event->name, TRUE);
			UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
		} else {
			DBG("IN_DELETE --> NOT IN_ISDIR");
			UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
			__process_object_deleted_event(full_path,
					event->name, FALSE);
			UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
		}
	} else if (event->mask & IN_CLOSE_WRITE) {
		DBG_SECURE("IN_CLOSE_WRITE %d, %s\n", event->wd, event->name);
		if (!g_strcmp0(g_last_copied, full_path)) {
			/* Ignore this case as this is generated due to MTP*/
			DBG("[%s] is copied by MTP\n", full_path);
			memset(g_last_copied, 0,
					MTP_MAX_PATHNAME_SIZE + 1);
		} else {
			open_files_info_t *node = NULL;
			node = __find_file_in_inoti_open_files_list(event->wd,
					event->name);

			if (node != NULL) {
				UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
				__process_object_added_event(full_path,
						event->name, parentpath);
				UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
				__remove_file_from_inoti_open_files_list(node);
			}
		}
	} else {
		DBG("This case is ignored");
		return FALSE;
	}

	return TRUE;
}

void _inoti_deinit_filesystem_events()
{
	if (TRUE != _util_thread_cancel(g_inoti_thrd)) {
		ERR("thread cancel fail.");
		return;
	}

	if (_util_thread_join(g_inoti_thrd, 0) == FALSE) {
		ERR("_util_thread_join() Fail");
	}

	return;
}

static void __remove_inoti_watch(mtp_char *path)
{
	mtp_int32 i = 0;

	for (i = 0; i < g_cnt_watch_folder; i++) {
		if (g_inoti_watches[i].forlder_name == NULL) {
			continue;
		}

		if (g_strcmp0(g_inoti_watches[i].forlder_name, path) != 0) {
			continue;
		}

		g_free(g_inoti_watches[i].forlder_name);
		g_inoti_watches[i].forlder_name = NULL;
		inotify_rm_watch(g_inoti_fd, g_inoti_watches[i].wd);
		g_inoti_watches[i].wd = 0;

		break;
	}

	if (i == g_cnt_watch_folder)
		ERR("Path not found in g_noti_watches");
	return;
}

static mtp_bool __add_file_to_inoti_open_files_list(mtp_int32 wd,
		mtp_char *event_name)
{
	open_files_info_t *new_node = NULL;

	new_node = (open_files_info_t *)g_malloc(sizeof(open_files_info_t));
	if (NULL == new_node) {
		ERR("new_node is null malloc fail");
		return FALSE;
	}

	new_node->name = g_strdup(event_name);
	new_node->wd = wd;

	/* First created file */
	if (NULL == g_open_files_list) {
		new_node->previous = NULL;
	} else {
		g_open_files_list->next = new_node;
		new_node->previous = g_open_files_list;
	}
	new_node->next = NULL;
	g_open_files_list = new_node;

	return TRUE;
}

static open_files_info_t *__find_file_in_inoti_open_files_list(mtp_int32 wd,
		mtp_char *event_name)
{
	open_files_info_t *current_node = g_open_files_list;

	while (NULL != current_node) {
		if ((current_node->wd == wd) &&
				(g_strcmp0(current_node->name, event_name) == 0)) {
			return current_node;
		}

		current_node = current_node->previous;
	}

	ERR("Cannot find file in open file's list");
	return NULL;
}

static void __remove_file_from_inoti_open_files_list(open_files_info_t *node)
{
	if (NULL != node->previous) {
		node->previous->next = node->next;
	}

	if (NULL != node->next) {
		node->next->previous = node->previous;
	}

	if (node == g_open_files_list) {
		g_open_files_list = node->previous;
	}
	g_free(node->name);
	g_free(node);

	return;
}

static mtp_int32 __get_inoti_watch_id(mtp_int32 iwd)
{
	mtp_int32 i = 0;

	for (i = 0; i < INOTI_FOLDER_COUNT_MAX; i++) {
		if (iwd == g_inoti_watches[i].wd)
			break;
	}

	if (i >= INOTI_FOLDER_COUNT_MAX) {
		ERR("inoti_folder is not found");
		return -1;
	}

	return i;
}

static mtp_bool __get_inoti_event_full_path(mtp_int32 wd, mtp_char *event_name,
		mtp_char *path, mtp_int32 path_len, mtp_char *parent_path)
{
	mtp_int32 inoti_id = 0;

	retv_if(wd == 0, FALSE);
	retv_if(path == NULL, FALSE);
	retv_if(event_name == NULL, FALSE);

	inoti_id = __get_inoti_watch_id(wd);
	if (inoti_id < 0) {
		ERR("FAIL to find last_inoti_id : %d\n", inoti_id);
		return FALSE;
	}

	/* 2 is for / and null character */
	if (path_len < (strlen(g_inoti_watches[inoti_id].forlder_name) +
				strlen(event_name) + 2))
		return FALSE;

	g_snprintf(path, path_len, "%s/%s",
			g_inoti_watches[inoti_id].forlder_name, event_name);
	g_snprintf(parent_path, path_len, "%s",
			g_inoti_watches[inoti_id].forlder_name);

	return TRUE;
}

static void __remove_recursive_inoti_watch(mtp_char *path)
{
	mtp_int32 i = 0;
	mtp_char *res = NULL;

	for (i = 0; i < g_cnt_watch_folder; i++) {
		if (g_inoti_watches[i].forlder_name == NULL)
			continue;

		res = strstr(g_inoti_watches[i].forlder_name, path);
		if (res == NULL)
			continue;

		g_free(g_inoti_watches[i].forlder_name);
		g_inoti_watches[i].forlder_name = NULL;
		inotify_rm_watch(g_inoti_fd, g_inoti_watches[i].wd);
		g_inoti_watches[i].wd = 0;
	}

	return;
}

static void __clean_up_inoti(void *data)
{
	__remove_recursive_inoti_watch(MTP_STORE_PATH_CHAR);
	__remove_recursive_inoti_watch(MTP_EXTERNAL_PATH_CHAR);
	__destroy_inoti_open_files_list();

	close(g_inoti_fd);
	g_inoti_fd = 0;
	return;
}

static void __delete_children_from_store_inoti(mtp_store_t *store,
		mtp_obj_t *obj)
{
	mtp_uint32 i = 0;
	ptp_array_t child_arr = { 0 };
	mtp_obj_t *child_obj = NULL;
	slist_node_t *node = NULL;

	__remove_inoti_watch(obj->file_path);

	_prop_init_ptparray(&child_arr, UINT32_TYPE);
	_entity_get_child_handles(store, obj->obj_handle, &child_arr);

	for (i = 0; i < child_arr.num_ele; i++) {

		mtp_uint32 *ptr32 = child_arr.array_entry;
		child_obj = _entity_get_object_from_store(store, ptr32[i]);

		if (child_obj != NULL && child_obj->obj_info != NULL &&
				child_obj->obj_info->obj_fmt ==
				PTP_FMT_ASSOCIATION) {
			__delete_children_from_store_inoti(store, child_obj);
		}

		node = _util_delete_node(&(store->obj_list), child_obj);
		g_free(node);
		_entity_dealloc_mtp_obj(child_obj);
	}

	_prop_deinit_ptparray(&child_arr);
	return;
}

static void __process_object_added_event(mtp_char *fullpath,
		mtp_char *file_name, mtp_char *parent_path)
{
	mtp_uint32 store_id = 0;
	mtp_store_t *store = NULL;
	mtp_obj_t *parent_obj = NULL;
	mtp_uint32 h_parent = 0;
	mtp_obj_t *obj = NULL;
	struct stat stat_buf = { 0 };
	mtp_int32 ret = 0;
	dir_entry_t dir_info = { { 0 }, 0 };

	if (NULL != g_strrstr(file_name, MTP_TEMP_FILE)) {
		ERR("File is a temp file");
		return;
	}

	if (file_name[0] == '.') {
		DBG_SECURE("Hidden file filename=[%s]\n", file_name);
		return;
	}

	store_id = _entity_get_store_id_by_path(fullpath);
	store = _device_get_store(store_id);
	if (NULL == store) {
		ERR("store is NULL so return");
		return;
	}
	parent_obj = _entity_get_object_from_store_by_path(store, parent_path);
	if (NULL == parent_obj) {
		if (!g_strcmp0(parent_path, MTP_STORE_PATH_CHAR) ||
				!g_strcmp0(parent_path, MTP_EXTERNAL_PATH_CHAR))
		{
			DBG("parent is the root folder");
			h_parent = 0;
		} else {
			DBG("Cannot find the parent, return");
			return;
		}
	} else {
		h_parent = parent_obj->obj_handle;
	}

	ret = stat(fullpath, &stat_buf);
	if (ret < 0) {
		ERR("stat() Fail");
		_util_print_error();
		return;
	}

	g_strlcpy(dir_info.filename, fullpath, MTP_MAX_PATHNAME_SIZE + 1);
	dir_info.attrs.mtime = stat_buf.st_mtime;
	dir_info.attrs.fsize = stat_buf.st_size;

	/* Reset the attributes */
	dir_info.attrs.attribute = MTP_FILE_ATTR_MODE_NONE;
	if (S_ISBLK(stat_buf.st_mode) || S_ISCHR(stat_buf.st_mode) ||
			S_ISLNK(stat_buf.st_mode) || S_ISSOCK(stat_buf.st_mode)) {
		dir_info.attrs.attribute |= MTP_FILE_ATTR_MODE_SYSTEM;
	}

	if (S_ISREG(stat_buf.st_mode)) {
		dir_info.type = MTP_FILE_TYPE;
		dir_info.attrs.attribute |= MTP_FILE_ATTR_MODE_NONE;

		if (!((S_IWUSR & stat_buf.st_mode) ||
					(S_IWGRP & stat_buf.st_mode) ||
					(S_IWOTH & stat_buf.st_mode))) {
			dir_info.attrs.attribute |= MTP_FILE_ATTR_MODE_READ_ONLY;
		}

		obj = _entity_add_file_to_store(store, h_parent, fullpath,
				file_name, &dir_info);
		if (NULL == obj) {
			ERR("_entity_add_file_to_store fail.");
			return;
		}
	} else if (S_ISDIR(stat_buf.st_mode)) {
		dir_info.type = MTP_DIR_TYPE;
		dir_info.attrs.attribute  |= MTP_FILE_ATTR_MODE_DIR;
		obj = _entity_add_folder_to_store(store, h_parent, fullpath,
				file_name, &dir_info);
		if (NULL == obj) {
			ERR("_entity_add_folder_to_store fail.");
			return;
		}
	} else {
		ERR("%s type is neither DIR nor FILE.\n", fullpath);
		return;
	}

	_eh_send_event_req_to_eh_thread(EVENT_OBJECT_ADDED,
			obj->obj_handle, 0, NULL);

	return;
}

static void __process_object_deleted_event(mtp_char *fullpath,
		mtp_char *file_name, mtp_bool isdir)
{
	mtp_obj_t *obj = NULL;
	mtp_obj_t *parent_obj = NULL;
	mtp_store_t *store = NULL;
	mtp_uint32 storageid = 0;
	mtp_uint32 h_parent = 0;
	mtp_uint32 obj_handle = 0;
	slist_node_t *node = NULL;

	if (NULL != strstr(fullpath, MTP_TEMP_FILE)) {
		ERR("File is a temp file, need to ignore");
		return;
	}

	if (file_name[0] == '.') {
		DBG_SECURE("Hidden file filename=[%s], Ignore\n", file_name);
		return;
	}

	storageid = _entity_get_store_id_by_path(fullpath);
	store = _device_get_store(storageid);
	if (NULL == store) {
		ERR("store is NULL so return");
		return;
	}

	obj = _entity_get_object_from_store_by_path(store, fullpath);
	if (NULL == obj) {
		ERR("object is NULL so return");
		return;
	}

	obj_handle = obj->obj_handle;
	h_parent = obj->obj_info->h_parent;
	if (h_parent != PTP_OBJECTHANDLE_ROOT) {
		parent_obj = _entity_get_object_from_store(store, h_parent);
		if (NULL != parent_obj) {
			_entity_remove_reference_child_array(parent_obj,
					obj->obj_handle);
		}
	}

	if (TRUE == isdir) {
		__delete_children_from_store_inoti(store, obj);
	}

	node = _util_delete_node(&(store->obj_list), obj);
	g_free(node);
	_entity_dealloc_mtp_obj(obj);

	_eh_send_event_req_to_eh_thread(EVENT_OBJECT_REMOVED, obj_handle,
			0, NULL);

	return;
}

static void __destroy_inoti_open_files_list()
{
	open_files_info_t *current = NULL;

	ret_if(g_open_files_list == NULL);

	while(g_open_files_list) {
		current = g_open_files_list;
		g_open_files_list = g_open_files_list->previous;

		if (g_open_files_list) {
			g_open_files_list->next = NULL;
		}

		g_free(current->name);
		current->wd = 0;
		current->previous = NULL;
		current->next = NULL;
		g_free(current);
	}

	g_open_files_list = NULL;
	return;
}
#endif /*MTP_SUPPORT_OBJECTADDDELETE_EVENT*/
