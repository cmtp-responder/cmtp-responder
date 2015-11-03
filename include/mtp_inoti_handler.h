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

#ifndef _MTP_INOTI_HANDLER_H_
#define _MTP_INOTI_HANDLER_H_

#include "mtp_config.h"
#ifdef MTP_SUPPORT_OBJECTADDDELETE_EVENT
#include <sys/inotify.h>
#include <limits.h>
#include "mtp_datatype.h"

#define INOTI_EVENT_SIZE	(sizeof(struct inotify_event))
#define INOTI_BUF_LEN		(INOTI_EVENT_SIZE + NAME_MAX + 1)
#define INOTI_FOLDER_COUNT_MAX	(1024)

typedef struct {
	mtp_int32 wd;
	mtp_char *forlder_name;
} inoti_watches_t;

typedef struct _open_files_info {
	mtp_char *name;
	mtp_int32 wd;
	struct _open_files_info* previous;
	struct _open_files_info* next;
} open_files_info_t;

void *_thread_inoti(void *arg);
void _inoti_add_watch_for_fs_events(mtp_char *path);
mtp_bool _inoti_init_filesystem_evnts();
void _inoti_deinit_filesystem_events();
#endif /* MTP_SUPPORT_OBJECTADDDELETE_EVENT */

#endif /* _MTP_INOTI_HANDLER_H_ */
