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
#include <glib.h>
#include <glib-object.h>
#include <malloc.h>
#include <vconf.h>
#include "mtp_init.h"
#include "mtp_config.h"
#include "mtp_thread.h"
#include "mtp_support.h"
#include "mtp_device.h"
#include "mtp_event_handler.h"
#include "mtp_cmd_handler.h"
#include "mtp_inoti_handler.h"
#include "mtp_transport.h"
#include "mtp_util.h"
#include "mtp_media_info.h"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
extern pthread_t g_eh_thrd;
extern pthread_mutex_t g_cmd_inoti_mutex;
extern mtp_bool g_is_sync_estab;

mtp_mgr_t g_mtp_mgr;
mtp_config_t g_conf;

/*
 * STATIC VARIABLES AND FUNCTIONS
 */
static GMainLoop *g_mainloop = NULL;
static mtp_mgr_t *g_mgr = &g_mtp_mgr;
static void __read_mtp_conf(void);
static void __init_mtp_info(void);
static void __mtp_exit(void);
/*
 * FUNCTIONS
 */

/*
 * static void __mtp_exit(void)
 * This function send MTP stopped state to event handler thread and MTP UI
 * @param[in]		None.
 * @param[out]		None.
 * @return		None.
 */
static void __mtp_exit(void)
{
	long cur_time;

	DBG("## Terminate all threads");
	if (g_eh_thrd) {
		_eh_send_event_req_to_eh_thread(EVENT_USB_REMOVED, 0, 0, NULL);
		if (_util_thread_join(g_eh_thrd, NULL) == FALSE) {
			ERR("_util_thread_join() Fail");
		}
		g_eh_thrd = 0;
	}

	if (g_is_sync_estab) {
		time(&cur_time);
		vconf_set_int(VCONFKEY_MTP_SYNC_TIME_INT, (int)cur_time);
	}

	DBG("## Terminate main loop");

	g_main_loop_quit(g_mainloop);

	return;
}

void _mtp_init(add_rem_store_t sel)
{
	mtp_char wmpinfopath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char *device_name = NULL;
	mtp_char *sync_partner = NULL;
	mtp_bool ret = 0;
	int vconf_ret = 0;
	mtp_int32 error = 0;

	DBG("Initialization start!");

	__read_mtp_conf();

	if (g_conf.mmap_threshold) {
		if (!mallopt(M_MMAP_THRESHOLD, g_conf.mmap_threshold))
			ERR("mallopt(M_MMAP_THRESHOLD) Fail");

		if (!mallopt(M_TRIM_THRESHOLD, g_conf.mmap_threshold * 2))
			ERR("mallopt(M_TRIM_THRESHOLD) Fail");
	}

	__init_mtp_info();

	_transport_init_status_info();
	_transport_set_mtp_operation_state(MTP_STATE_INITIALIZING);

	device_name = vconf_get_str(VCONFKEY_SETAPPL_DEVICE_NAME_STR);
	if (device_name != NULL) {
		_device_set_device_name(device_name);
		g_free(device_name);
	} else {
		_device_set_device_name(MTP_DEV_PROPERTY_FRIENDLYNAME);
	}

	sync_partner = vconf_get_str(VCONFKEY_MTP_SYNC_PARTNER_STR);
	if (sync_partner != NULL && strlen(sync_partner) > 0) {
		_device_set_sync_partner(sync_partner);
		g_free(sync_partner);
	} else {
		_device_set_sync_partner(MTP_DEV_PROPERTY_SYNCPARTNER);
	}


	g_mgr->ftemp_st.filepath = g_strdup(MTP_TEMP_FILE_DEFAULT);
	if (g_mgr->ftemp_st.filepath == NULL) {
		ERR("g_strdup() Fail");
		goto MTP_INIT_FAIL;
	}

	if (g_mgr->ftemp_st.temp_buff == NULL) {
		/* Allocate memory for temporary */
		g_mgr->ftemp_st.temp_buff = (mtp_char *)g_malloc(g_conf.write_file_size);
		if (g_mgr->ftemp_st.temp_buff == NULL) {
			ERR("memory allocation fail");
			goto MTP_INIT_FAIL;
		}
	}

	/* Internal Storage */
	if (access(MTP_STORE_PATH_CHAR, F_OK) < 0) {
		if (FALSE == _util_dir_create(MTP_STORE_PATH_CHAR, &error)) {
			ERR("Cannot make directory!! [%s]\n",
					MTP_STORE_PATH_CHAR);
			goto MTP_INIT_FAIL;
		}
	}
	/* External Storage */
	if (MTP_PHONE_MMC_INSERTED == _util_get_local_mmc_status()) {
		char ext_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
		_util_get_external_path(ext_path);
		if (access(ext_path, F_OK) < 0) {
			if (FALSE == _util_dir_create(ext_path, &error)) {
				ERR("Cannot make directory!! [%s]\n",
						ext_path);
				goto MTP_INIT_FAIL;
			}
		}
	}
#ifndef MTP_SUPPORT_HIDE_WMPINFO_XML
	/* Update WMPInfo.xml for preventing frequent saving */
	ret = _util_create_path(wmpinfopath, sizeof(wmpinfopath),
			MTP_STORE_PATH_CHAR, MTP_FILE_NAME_WMPINFO_XML);
	if (FALSE == ret) {
		ERR("szWMPInfoPath is too long");
		goto MTP_INIT_FAIL;
	}
#endif /*MTP_SUPPORT_HIDE_WMPINFO_XML*/

	/* Set mtpdeviceinfo */
	_init_mtp_device();

	_features_supported_info();

	/* Install storage */
	_device_install_storage(sel);

#ifdef MTP_SUPPORT_OBJECTADDDELETE_EVENT
	_inoti_init_filesystem_evnts();
#endif /*MTP_SUPPORT_OBJECTADDDELETE_EVENT*/

	vconf_ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
			_handle_mmc_notification, NULL);
	if (vconf_ret < 0) {
		ERR("vconf_notify_key_changed(%s) Fail", VCONFKEY_SYSMAN_MMC_STATUS);
		goto MTP_INIT_FAIL;
	}

	return;

MTP_INIT_FAIL:
	/* Set MTP state to stopped */
	_transport_set_mtp_operation_state(MTP_STATE_STOPPED);
	mtp_end_event();

	return;
}

void _mtp_deinit(void)
{
	_cmd_hdlr_reset_cmd(&g_mgr->hdlr);

	/* initialize MTP_USE_FILE_BUFFER*/
	if (g_mgr->ftemp_st.temp_buff != NULL) {
		g_free(g_mgr->ftemp_st.temp_buff);
		g_mgr->ftemp_st.temp_buff = NULL;
	}

#ifdef MTP_SUPPORT_OBJECTADDDELETE_EVENT
	_inoti_deinit_filesystem_events();
#endif /*MTP_SUPPORT_OBJECTADDDELETE_EVENT*/

	vconf_ignore_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
			_handle_mmc_notification);

	return;
}

static void __print_mtp_conf(void)
{
	if (g_conf.is_init == false) {
		ERR("g_conf is not initialized");
		return;
	}

	DBG("MMAP_THRESHOLD : %d\n", g_conf.mmap_threshold);
	DBG("INIT_RX_IPC_SIZE : %d\n", g_conf.init_rx_ipc_size);
	DBG("INIT_TX_IPC_SIZE : %d\n", g_conf.init_tx_ipc_size);
	DBG("MAX_RX_IPC_SIZE : %d\n", g_conf.max_rx_ipc_size);
	DBG("MAX_TX_IPC_SIZE : %d\n", g_conf.max_tx_ipc_size);
	DBG("READ_USB_SIZE : %d\n", g_conf.read_usb_size);
	DBG("WRITE_USB_SIZE : %d\n", g_conf.write_usb_size);
	DBG("READ_FILE_SIZE : %d\n", g_conf.read_file_size);
	DBG("WRITE_FILE_SIZE : %d\n", g_conf.write_file_size);
	DBG("MAX_IO_BUF_SIZE : %d\n\n", g_conf.max_io_buf_size);

	DBG("SUPPORT_PTHEAD_SHCED : %s\n", g_conf.support_pthread_sched ? "Support" : "Not support");
	DBG("INHERITSCHED : %c\n", g_conf.inheritsched);
	DBG("SCHEDPOLICY : %c\n", g_conf.schedpolicy);
	DBG("FILE_SCHEDPARAM: %d\n", g_conf.file_schedparam);
	DBG("USB_SCHEDPARAM: %d\n\n", g_conf.usb_schedparam);
}

static void __read_mtp_conf(void)
{
	FILE *fp;
	char buf[256];
	char *token;
	char *saveptr = NULL;

	g_conf.mmap_threshold = MTP_MMAP_THRESHOLD;

	g_conf.read_usb_size = MTP_READ_USB_SIZE;
	g_conf.write_usb_size = MTP_WRITE_USB_SIZE;

	g_conf.read_file_size = MTP_READ_FILE_SIZE;
	g_conf.write_file_size = MTP_WRITE_FILE_SIZE;

	g_conf.init_rx_ipc_size = MTP_INIT_RX_IPC_SIZE;
	g_conf.init_tx_ipc_size = MTP_INIT_TX_IPC_SIZE;

	g_conf.max_rx_ipc_size = MTP_MAX_RX_IPC_SIZE;
	g_conf.max_tx_ipc_size = MTP_MAX_TX_IPC_SIZE;

	g_conf.max_io_buf_size = MTP_MAX_IO_BUF_SIZE;
	g_conf.read_file_delay = MTP_READ_FILE_DELAY;

	if (MTP_SUPPORT_PTHREAD_SCHED) {
		g_conf.support_pthread_sched = MTP_SUPPORT_PTHREAD_SCHED;
		g_conf.inheritsched = MTP_INHERITSCHED;
		g_conf.schedpolicy = MTP_SCHEDPOLICY;
		g_conf.file_schedparam = MTP_FILE_SCHEDPARAM;
		g_conf.usb_schedparam = MTP_USB_SCHEDPARAM;
	}

	fp = fopen(MTP_CONFIG_FILE_PATH, "r");
	if (fp == NULL) {
		DBG("Default configuration is used");
		g_conf.is_init = true;

		__print_mtp_conf();
		return;
	}

	while (fgets(buf, sizeof(buf), fp)) {
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		token = strrchr(buf, '\n');
		if (token == NULL) {
			ERR("g_conf is too long");
			break;
		}
		*token = '\0';

		token = strtok_r(buf, "=", &saveptr);
		if (token == NULL) {
			continue;
		}

		if (strcasecmp(token, "mmap_threshold") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.mmap_threshold = atoi(token);

		} else if (strcasecmp(token, "init_rx_ipc_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.init_rx_ipc_size = atoi(token);

		} else if (strcasecmp(token, "init_tx_ipc_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.init_tx_ipc_size = atoi(token);

		} else if (strcasecmp(token, "max_rx_ipc_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.max_rx_ipc_size = atoi(token);

		} else if (strcasecmp(token, "max_tx_ipc_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.max_tx_ipc_size = atoi(token);

		} else if (strcasecmp(token, "read_usb_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.read_usb_size = atoi(token);

		} else if (strcasecmp(token, "write_usb_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.write_usb_size = atoi(token);

		} else if (strcasecmp(token, "read_file_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.read_file_size = atoi(token);

		} else if (strcasecmp(token, "write_file_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.write_file_size = atoi(token);

		} else if (strcasecmp(token, "max_io_buf_size") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.max_io_buf_size = atoi(token);

		} else if (strcasecmp(token, "read_file_delay") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.read_file_delay = atoi(token);

		} else if (strcasecmp(token, "support_pthread_sched") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.support_pthread_sched = atoi(token) ? true : false;

		} else if (strcasecmp(token, "inheritsched") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.inheritsched = *token;

		} else if (strcasecmp(token, "schedpolicy") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.schedpolicy = *token;

		} else if (strcasecmp(token, "file_schedparam") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.file_schedparam = atoi(token);

		} else if (strcasecmp(token, "usb_schedparam") == 0) {
			token = strtok_r(NULL, "=", &saveptr);
			g_conf.usb_schedparam = atoi(token);

		} else {
			ERR("Unknown option : %s\n", buf);
		}
	}
	fclose(fp);
	g_conf.is_init = true;

	__print_mtp_conf();
	return;
}

void __init_mtp_info(void)
{
	/* initialize struct one time*/
	memset(&g_mgr->ftemp_st, 0, sizeof(g_mgr->ftemp_st));
	memset(&g_mgr->hdlr, 0, sizeof(g_mgr->hdlr));
	memset(&g_mgr->meta_info, 0, sizeof(g_mgr->meta_info));

	return ;
}

void _features_supported_info(void)
{
	DBG("***********************************************************");
	DBG("### MTP Information ###");
	DBG("### 1. Solution		: SLP");
	DBG("### 2. MTP Version		: 1.0");
	DBG("### 3. DB Limitation       : Reference(%d)\n", MTP_MAX_REFDB_ROWCNT);

	DBG("***********************************************************");
	DBG("### Extension ###");
	if (_get_oma_drm_status() == TRUE) {
		DBG("### 2. OMADRM		: [ON]");
	} else {
		DBG("### 2. OMADRM		: [OFF]");
	}

	DBG("***********************************************************");
	DBG("### Feature ###");

#ifdef MTP_SUPPORT_ALBUM_ART
	DBG("### 2. MTP_SUPPORT_ALBUM_ART	: [ON]");
#else /* MTP_SUPPORT_ALBUM_ART */
	DBG("### 2. MTP_SUPPORT_ALBUM_ART	: [OFF]");
#endif /* MTP_SUPPORT_ALBUM_ART */

#ifdef MTP_SUPPORT_SET_PROTECTION
	DBG("### 3. MTP_SUPPORT_SET_PROTECTION	: [ON]");
#else /* MTP_SUPPORT_SET_PROTECTION */
	DBG("### 3. MTP_SUPPORT_SET_PROTECTION	: [OFF]");
#endif /* MTP_SUPPORT_SET_PROTECTION */
	DBG("***********************************************************");
	return;
}

/*
 * void mtp_end_event(void)
 * This function terminates mtp.
 * It must not be called in gthr_mtp_event thread.
 * It makes dead lock state if it is called in gthr_mtp_event thread.
 */
void mtp_end_event(void)
{
	__mtp_exit();
}

static inline int _main_init()
{
	pthread_mutexattr_t mutex_attr;

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	if (0 != pthread_mutex_init(&g_cmd_inoti_mutex, &mutex_attr)) {
		ERR("pthread_mutex_init() Fail");
		_util_print_error();
		pthread_mutexattr_destroy(&mutex_attr);
		return MTP_ERROR_GENERAL;
	}
	pthread_mutexattr_destroy(&mutex_attr);

	if (_eh_handle_usb_events(USB_INSERTED) == FALSE) {
		ERR("_eh_handle_usb_events() Fail");
		return MTP_ERROR_GENERAL;
	}

	g_mainloop = g_main_loop_new(NULL, FALSE);
	if (g_mainloop == NULL) {
		ERR("g_mainloop is NULL");
		return MTP_ERROR_GENERAL;
	}

	return MTP_ERROR_NONE;
}

int main(int argc, char *argv[])
{
	mtp_int32 ret;

	ret = media_content_connect();
	if (MEDIA_CONTENT_ERROR_NONE != ret) {
		ERR("media_content_connect() Fail(%d)", ret);
		return MTP_ERROR_GENERAL;
	}

	if (_eh_register_notification_callbacks() == FALSE) {
		ERR("_eh_register_notification_callbacks() Fail");
		return MTP_ERROR_GENERAL;
	}

	ret = _main_init();
	if (MTP_ERROR_NONE != ret) {
		ERR("_main_init() Fail(%d)", ret);
		_eh_deregister_notification_callbacks();
		media_content_disconnect();
		return MTP_ERROR_GENERAL;
	}
	DBG("MTP UID = [%u] and GID = [%u]\n", getuid(), getgid());

	g_main_loop_run(g_mainloop);

	_eh_deregister_notification_callbacks();
	media_content_disconnect();

	return MTP_ERROR_NONE;
}
