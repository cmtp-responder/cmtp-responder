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

#ifndef __MTPMAIN_H_INCLUDED
#define __MTPMAIN_H_INCLUDED

#include "mtp_store.h"

#define MTP_LOG_FILE			"/var/log/mtp.log"
#define MTP_LOG_MAX_SIZE		5 * 1024 * 1024 /*5MB*/

void _mtp_init(add_rem_store_t sel);
void _mtp_deinit(void);
void _features_supported_info(void);
void mtp_end_event(void);

#endif	/* _MTP_MAIN_H_ */
