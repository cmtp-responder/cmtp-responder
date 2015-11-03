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

#include <mtp_thread.h>

/*
 * FUNCTIONS
 */
mtp_bool _util_thread_create(pthread_t *tid, const mtp_char *tname,
		mtp_int32 thread_state, thread_func_t thread_func, void *arg)
{
	int error = 0;
	pthread_attr_t attr;

	retv_if(tname == NULL, FALSE);
	retv_if(thread_func == NULL, FALSE);

	error = pthread_attr_init(&attr);
	if (error != 0) {
		ERR("pthread_attr_init Fail [%d], errno [%d]\n", error, errno);
		return FALSE;
	}

	if (thread_state == PTHREAD_CREATE_JOINABLE) {
		error = pthread_attr_setdetachstate(&attr,
				PTHREAD_CREATE_JOINABLE);
		if (error != 0) {
			ERR("pthread_attr_setdetachstate Fail [%d], errno [%d]\n", error, errno);
			pthread_attr_destroy(&attr);
			return FALSE;
		}
	}

	error = pthread_create(tid, &attr, thread_func, arg);
	if (error != 0) {
		ERR( "[%s] Thread creation Fail errno [%d]\n", tname, errno);
		pthread_attr_destroy(&attr);
		return FALSE;
	}

	error = pthread_attr_destroy(&attr);
	if (error != 0) {
		ERR("pthread_attr_destroy Fail [%d] errno [%d]\n", error, errno);
	}

	return TRUE;
}

mtp_bool _util_thread_join(pthread_t tid, void **data)
{
	mtp_int32 res = 0;

	res = pthread_join(tid, data);
	if (res != 0) {
		ERR("pthread_join Fail res = [%d] for thread [%u] errno [%d]\n",
				res, tid, errno);
		return FALSE;
	}

	return TRUE;
}

mtp_bool _util_thread_cancel(pthread_t tid)
{
	mtp_int32 res;

	res = pthread_cancel(tid);
	if (res != 0) {
		ERR("pthread_cancel  Fail [%d] errno [%d]\n", tid, errno);
		return FALSE;
	}

	return TRUE;
}

void _util_thread_exit(void *val_ptr)
{
	pthread_exit(val_ptr);
	return;
}
