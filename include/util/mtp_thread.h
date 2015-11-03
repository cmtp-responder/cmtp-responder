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

#ifndef _MTP_THREAD_H_
#define _MTP_THREAD_H_

#include <pthread.h>
#include "mtp_datatype.h"
#include "mtp_util.h"

typedef void *(*thread_func_t) (void *pArg);

#define UTIL_LOCK_MUTEX(mut)\
	do {\
		int lock_ret = 0;\
		DBG("Thread [%d] trying to lock the Mutex \n", syscall(__NR_gettid));\
		lock_ret = pthread_mutex_lock(mut);\
		if(lock_ret != 0) {\
			if(lock_ret == EDEADLK) {\
				DBG("Mutex is already locked by the same thread");\
			} else {\
				ERR("Error locking mutex. Error = %d \n", lock_ret);\
			}\
		} else {\
			DBG("Mutex locked by thread [%d] \n", syscall(__NR_gettid));\
		}\
	} while (0);\


#define UTIL_UNLOCK_MUTEX(mut)\
	do {\
		int unlock_ret = 0;\
		unlock_ret = pthread_mutex_unlock(mut);\
		if (unlock_ret != 0) {\
			ERR("Error unlocking mutex. Error = %d \n", unlock_ret);\
		} else {\
			DBG("Mutex unlocked by thread [%d] \n", syscall(__NR_gettid));\
		}\
	} while (0);\

mtp_bool _util_thread_create(pthread_t *tid, const mtp_char *tname,
		mtp_int32 thread_state, thread_func_t thread_func, void *arg);
mtp_bool _util_thread_join(pthread_t tid, void **data);
mtp_bool _util_thread_cancel(pthread_t tid);
void _util_thread_exit(void *val_ptr);

#endif /*_MTP_THREAD_H_*/
