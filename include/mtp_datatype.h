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

#ifndef _MTP_DATATYPES_H_
#define _MTP_DATATYPES_H_

#include <stdio.h>
#include <stdlib.h>

#ifndef _MTP_USE_OWNTYPES
#define _MTP_USE_OWNTYPES
#endif
#ifdef _MTP_USE_OWNTYPES

#define INVALID_FILE		(0)

typedef unsigned char mtp_byte;
typedef unsigned char mtp_bool;
typedef unsigned char mtp_uchar;
typedef char mtp_char ;

typedef unsigned short mtp_uint16;
typedef unsigned short mtp_word;
typedef unsigned short mtp_wchar;
typedef short mtp_int16;

typedef unsigned int mtp_uint32;
typedef int mtp_htimer;
typedef int mtp_int32;

typedef unsigned long mtp_ulong ;
typedef unsigned long mtp_dword ;

typedef unsigned long long  mtp_uint64;
typedef long long mtp_int64;

#endif

#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif

typedef enum {
	MTP_ERROR_NONE = 0,
	MTP_ERROR_GENERAL = -1,
	MTP_ERROR_INVALID_STORE = -2,
	MTP_ERROR_STORE_READ_ONLY = -3,
	MTP_ERROR_STORE_FULL = -4,
	MTP_ERROR_INVALID_DATASET = -5,
	MTP_ERROR_INVALID_OBJECTHANDLE = -6,
	MTP_ERROR_INVALID_OBJ_PROP_CODE = -7,
	MTP_ERROR_INVALID_OBJECT_PROP_FORMAT = -8,
	MTP_ERROR_INVALID_OBJ_PROP_VALUE = -9,
	MTP_ERROR_INVALID_PARENT = -10,
	MTP_ERROR_ACCESS_DENIED = -11,
	MTP_ERROR_STORE_NOT_AVAILABLE = -12,
	MTP_ERROR_INVALID_PARAM = -13,
	MTP_ERROR_INVALID_OBJECT_INFO = -14,
	MTP_ERROR_OBJECT_WRITE_PROTECTED = -15,
	MTP_ERROR_PARTIAL_DELETION = -16,
	MTP_ERROR_NO_SPEC_BY_FORMAT = -17,
	MTP_ERROR_OPERATION_NOT_SUPPORTED = -19,
	MTP_ERROR_MAX = -20
} mtp_err_t;

#define WCHAR_SIZ	2

/**
 * \brief The CharMode enumeration.
 *
 * The CharMode enumerates character type by which string is written.
 * CHAR_TYPE means that one character consists of 8-bits.
 * WCHAR_TYPE means that one character consists of 16-bits.
 */
typedef enum _char_mode {
	CHAR_TYPE = 0,
	WCHAR_TYPE
} char_mode_t;

#endif /* _MTP_DATATYPES_H_ */
