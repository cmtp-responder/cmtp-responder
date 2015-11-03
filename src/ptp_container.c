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
#include "ptp_container.h"
#include "ptp_datacodes.h"
#include "mtp_transport.h"
#include "mtp_support.h"
#include "mtp_util.h"

/*
 * FUNCTIONS
 */
void _hdlr_init_cmd_container(cmd_container_t *cntr)
{
	cntr->tid = 0;
	cntr->type = CONTAINER_UNDEFINED;
	cntr->code = PTP_OPCODE_UNDEFINED;
	cntr->len = sizeof(header_container_t);
	cntr->no_param = 0;
	return;
}

mtp_uint32 _hdlr_get_param_cmd_container(cmd_container_t *cntr,
		mtp_uint32 index)
{
	if (index < cntr->no_param) {
		return cntr->params[index];
	}
	return 0;
}

void _hdlr_copy_cmd_container_unknown_params(cmd_container_t *src,
		cmd_container_t *dst)
{
	mtp_uint16 ii;

	dst->tid = src->tid;
	dst->type = src->type;
	dst->code = src->code;
	dst->len = src->len;
	dst->no_param =
		(src->len - sizeof(header_container_t)) / sizeof(mtp_uint32);

	for (ii = 0; ii < dst->no_param; ii++) {
		dst->params[ii] = src->params[ii];
	}
	return;
}

void _hdlr_copy_cmd_container(cmd_container_t *src, cmd_container_t *dst)
{
	mtp_uint16 ii;

	dst->tid = src->tid;
	dst->type = src->type;
	dst->code = src->code;
	dst->len = src->len;
	dst->no_param = src->no_param;

	for (ii = 0; ii < dst->no_param; ii++) {
		dst->params[ii] = src->params[ii];
	}

	return;
}

mtp_bool _hdlr_add_param_resp_container(resp_blk_t *dst, mtp_uint32 num,
		mtp_uint32 *params)
{
	mtp_uint16 ii;

	retvm_if(num > MAX_MTP_PARAMS, FALSE, "num(%d) exceed", num);
	retvm_if(num != 0 && params == NULL, FALSE, "num = %d, params = %p", num, params);

	dst->no_param = num;
	dst->len = sizeof(header_container_t) + sizeof(mtp_uint32) * (dst->no_param);

	for (ii = 0; ii < dst->no_param; ii++) {
		dst->params[ii] = params[ii];
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(&(dst->params[ii]),
				sizeof(dst->params[ii]));
#endif /* __BIG_ENDIAN__ */
	}
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(&(dst->no_param), sizeof(dst->no_param));
	_util_conv_byte_order(&(dst->len), sizeof(dst->len));
#endif /* __BIG_ENDIAN__ */

	return TRUE;
}

mtp_bool _hdlr_validate_cmd_container(mtp_uchar *blk, mtp_uint32 size)
{
	if (size < sizeof(header_container_t) || size > sizeof(cmd_container_t))
		return FALSE;

	cmd_container_t *ptr = NULL;

	ptr = (cmd_container_t *)blk;

	if (ptr->len != size || ptr->type != CONTAINER_CMD_BLK) {
		ERR("size = [%d] length[%d] type[%d]\n", size, ptr->len,
				ptr->type);
		return FALSE;
	}

	return TRUE;
}

void _hdlr_init_data_container(data_container_t *dst, mtp_uint16 code,
		mtp_uint32 trans_id)
{
	_hdlr_init_cmd_container((cmd_container_t *)dst);
	dst->type = CONTAINER_DATA_BLK;
	dst->code = code;
	dst->tid = trans_id;
	dst->data = NULL;
#ifdef __BIG_ENDIAN__
	_hdlr_conv_data_container_byte_order(dst);
#endif /* __BIG_ENDIAN__ */
	return;
}

mtp_uchar *_hdlr_alloc_buf_data_container(data_container_t *dst,
		mtp_uint32 bufsz, mtp_uint64 pkt_size)
{
	mtp_uint32 blk_len;
	header_container_t *header = NULL;

	pkt_size = (pkt_size + sizeof(header_container_t) > 0xFFFFFFFF) ?
		0xFFFFFFFF : pkt_size + sizeof(header_container_t);


	blk_len = bufsz + sizeof(header_container_t);
	dst->data = (mtp_uchar *)g_malloc(blk_len);
	if (dst->data == NULL) {
		ERR("g_malloc() Fail");
		return NULL;
	}

	memset(dst->data, 0, blk_len);
	dst->len = bufsz + sizeof(header_container_t);
	header = (header_container_t *)dst->data;
	header->len = pkt_size;
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(&(header->len), sizeof(header->len));
#endif /* __BIG_ENDIAN__ */
	header->type = dst->type;
	header->code = dst->code;
	header->tid = dst->tid;
	return (dst->data + sizeof(header_container_t));
}

mtp_bool _hdlr_send_data_container(data_container_t *dst)
{
	mtp_uint32 sent;

	sent = _transport_send_pkt_to_tx_mq(dst->data, dst->len);

	if (sent != dst->len)
		return FALSE;

	return TRUE;
}

mtp_bool _hdlr_send_bulk_data(mtp_uchar *dst, mtp_uint32 len)
{
	mtp_uint32 sent = 0;

	sent = _transport_send_bulk_pkt_to_tx_mq(dst, len);
	if (sent != len)
		return FALSE;

	return TRUE;
}

mtp_bool _hdlr_rcv_data_container(data_container_t *dst, mtp_uint32 size)
{
	mtp_uint32 blk_size;
	mtp_uint32 bytes_rcvd;
	mtp_uint16 exp_code;
	mtp_uint32 exp_tid;
	header_container_t *header = NULL;

	g_free(dst->data);
	dst->data = NULL;

	/* Allocated space for data + header */
	/* Also allocate extra space in case chip writes DWORDS */

	blk_size = size + sizeof(header_container_t) + sizeof(mtp_uint32);
	dst->data = (mtp_uchar *)g_malloc(blk_size);
	if (dst->data == NULL) {
		ERR("g_malloc() Fail");
		return FALSE;
	}
	bytes_rcvd = 0;

	_transport_rcv_temp_file_data(dst->data, blk_size, &bytes_rcvd);
	exp_code = dst->code;
	exp_tid = dst->tid;
	header = (header_container_t *)dst->data;

#ifdef __BIG_ENDIAN__
	_hdlr_conv_data_container_byte_order((data_container_t *)dst->data);
#endif /* __BIG_ENDIAN__ */

	/* Copy the header from the data block to the structure */
	dst->len = header->len;
	dst->type = header->type;
	dst->code = header->code;
	dst->tid = header->tid;
	if (dst->len != bytes_rcvd || dst->type != CONTAINER_DATA_BLK ||
			dst->code != exp_code || dst->tid != exp_tid) {
		ERR("HEADER FAILURE");
		ERR("HEADER length[%d], Type[%d], Code[%d], tid[%d]\n",
				dst->len, dst->type, dst->code, dst->tid);
		ERR("EXPECTED length[%d], Type[%d], Code[%d], tid[%d]\n",
				bytes_rcvd, CONTAINER_DATA_BLK, exp_code, exp_tid);
		return FALSE;
	}

	return TRUE;
}

mtp_bool _hdlr_rcv_file_in_data_container(data_container_t *dst,
		mtp_char *filepath, mtp_uint32 path_len)
{
	mtp_uint32 blk_size;
	mtp_uint64 bytes_rcvd;
	mtp_uint16 exp_code;
	mtp_uint32 exp_tid;
	header_container_t *header = NULL;

	g_free(dst->data);
	dst->data = NULL;

	/* Allocated space for data + header */
	/* Also allocate extra space in case chip writes DWORDS */

	blk_size = sizeof(header_container_t) + sizeof(mtp_uint32);
	dst->data = (mtp_uchar *)g_malloc((mtp_uint32) blk_size);
	if (dst->data == NULL) {
		ERR("g_malloc() Fail");
		return FALSE;
	}

	bytes_rcvd = 0;
	_transport_rcv_temp_file_info(dst->data, filepath, &bytes_rcvd,
			path_len);
	exp_code = dst->code;
	exp_tid = dst->tid;
	header = (header_container_t *)dst->data;

#ifdef __BIG_ENDIAN__
	_hdlr_conv_data_container_byte_order((data_container_t *)dst->data);
#endif /* __BIG_ENDIAN__ */

	/* Copy the header from the data block to the structure */
	dst->len = header->len;
	dst->type = header->type;
	dst->code = header->code;
	dst->tid = header->tid;

	if ((dst->len != bytes_rcvd && bytes_rcvd < MTP_FILESIZE_4GB) ||
			dst->type != CONTAINER_DATA_BLK ||
			dst->code != exp_code || dst->tid != exp_tid) {
		ERR("HEADER FAILURE");
		ERR("HEADER length[%d], Type[%d], Code[%d], tid[%d]\n",
				dst->len, dst->type, dst->code, dst->tid);
		ERR("EXPECTED length[%d], Type[%d], Code[%d], tid[%d]\n",
				bytes_rcvd, CONTAINER_DATA_BLK, exp_code, exp_tid);
		return FALSE;
	}

	return TRUE;
}

mtp_uint32 _hdlr_get_payload_size(data_container_t *dst)
{
	if (dst->data == NULL) {
		ERR("Payload data is NULL");
		return 0;
	}

	return (dst->len - sizeof(header_container_t));
}

mtp_uchar *_hdlr_get_payload_data(data_container_t *dst)
{
	if (dst->data == NULL) {
		ERR("Payload data is NULL");
		return NULL;
	}

	return (dst->data + sizeof(header_container_t));
}

void _hdlr_resp_container_init(cmd_container_t *dst, mtp_uint16 resp_code,
		mtp_uint32 tid)
{
	_hdlr_init_cmd_container(dst);
	dst->type = CONTAINER_RESP_BLK;
	dst->code = resp_code;
	dst->tid = tid;
#ifdef __BIG_ENDIAN__
	_hdlr_conv_cmd_container_byte_order(dst);
#endif /* __BIG_ENDIAN__ */
	return;
}

mtp_bool _hdlr_send_resp_container(cmd_container_t *dst)
{
	mtp_uint32 sent = 0;

#ifdef __BIG_ENDIAN__
	resp_blk_t resp_blk;

	_hdlr_copy_cmd_container(dst, &resp_blk);
	_hdlr_conv_cmd_container_byte_order(&resp_blk);
	sent = _transport_send_pkt_to_tx_mq((mtp_uchar *)&resp_blk, dst->len);
#else /* __BIG_ENDIAN__ */

	sent = _transport_send_pkt_to_tx_mq((mtp_uchar *)dst, dst->len);
#endif
	if (sent != dst->len) {
		ERR("_transport_send_pkt_to_tx_mq() Fail: dst->len(%u), sent(%u)",
			dst->len, sent);
		return FALSE;
	}

	return TRUE;
}

void _hdlr_init_event_container(cmd_container_t *dst, mtp_uint16 code,
		mtp_uint32 tid, mtp_uint32 param1, mtp_uint32 param2)
{
	dst->type = CONTAINER_EVENT_BLK;
	dst->code = code;
	dst->tid = tid;
	dst->no_param = 1;
	dst->params[0] = param1;
	dst->len = sizeof(header_container_t) + sizeof(mtp_uint32) * 1;
#ifdef __BIG_ENDIAN__
	_hdlr_conv_cmd_container_byte_order(dst);
#endif /* __BIG_ENDIAN__ */
	return;
}

void _hdlr_init_event_container_with_param(cmd_container_t *dst,
		mtp_uint16 code, mtp_uint32 tid, mtp_uint32 param1, mtp_uint32 param2)
{
	dst->type = CONTAINER_EVENT_BLK;
	dst->code = code;
	dst->tid = tid;
	dst->no_param = 2;
	dst->params[0] = param1;
	dst->params[1] = param2;
	dst->len = sizeof(header_container_t) + sizeof(mtp_uint32) * 3;
#ifdef __BIG_ENDIAN__
	_hdlr_conv_cmd_container_byte_order(dst);
#endif /* __BIG_ENDIAN__ */
	return;
}
mtp_bool _hdlr_send_event_container(cmd_container_t *dst)
{
	mtp_uint32 sent = 0;
	mtp_err_t retval;

	retval = _transport_send_event((mtp_uchar *)dst, dst->len, &sent);
	return (retval == MTP_ERROR_NONE && sent == dst->len) ?
		TRUE : FALSE;
}

void _hdlr_conv_cmd_container_byte_order(cmd_container_t *dst)
{
#ifdef __BIG_ENDIAN__
	mtp_uchar idx;

	_util_conv_byte_order(&(dst->code), sizeof(dst->code));
	_util_conv_byte_order(&(dst->len), sizeof(dst->len));
	_util_conv_byte_order(&(dst->no_param), sizeof(dst->NumParams));
	_util_conv_byte_order(&(dst->tid), sizeof(dst->tid));
	_util_conv_byte_order(&(dst->type), sizeof(dst->Type));

	for (idx = 0; idx < dst->no_param; idx++) {
		_util_conv_byte_order(&(dst->params[idx]),
				sizeof(dst->params[idx]));
	}
#endif /* __BIG_ENDIAN__ */
	return;
}

void _hdlr_conv_data_container_byte_order(data_container_t *dst)
{
#ifdef __BIG_ENDIAN__
	mtp_uchar idx;

	_util_conv_byte_order(&(dst->code), sizeof(dst->code));
	_util_conv_byte_order(&(dst->len), sizeof(dst->len));
	_util_conv_byte_order(&(dst->no_param), sizeof(dst->NumParams));
	_util_conv_byte_order(&(dst->tid), sizeof(dst->tid));
	_util_conv_byte_order(&(dst->type), sizeof(dst->Type));
	for (idx = 0; idx < dst->no_param; idx++) {
		_util_conv_byte_order(&(dst->params[idx]),
				sizeof(dst->params[idx]));
	}
#endif /* __BIG_ENDIAN__ */
	return;
}
