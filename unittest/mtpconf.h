/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd All Rights Reserved
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
#ifndef __MTPCONF_H__
#define __MTPCONF_H__

#include <glib.h>

#ifdef USE_DLOG
#include <dlog.h>
#undef LOG_TAG
#define LOG_TAG "MTP_RESPONDER_TEST"
#define GLOGD(format, args...)	LOGD(format, ##args)
#else
#define GLOGD(format, args...)
#endif

typedef enum {
	ERROR_NONE = 0,
	ERROR_NOT_PERMITTED = -1,
	ERROR_OUT_OF_MEMORY = -2,
	ERROR_PERMISSION_DENIED = -3,
	ERROR_RESOURCE_BUSY = -4,
	ERROR_INVALID_OPERATION = -5,
	ERROR_INVALID_PARAMETER = -6,
	ERROR_NOT_SUPPORTED = -7,
	ERROR_OPERATION_FAILED = -8,
	ERROR_NOT_INITIALIZED = -9,
	ERROR_ALREADY_INITIALIZED = -10,
	ERROR_IN_PROGRESS = -11,
} error_e;
#endif /* __MTPCONF_H__ */

