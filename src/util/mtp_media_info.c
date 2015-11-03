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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <metadata_extractor.h>
#include "mtp_media_info.h"
#include "mtp_util.h"
#include "mtp_support.h"
#include "mtp_fs.h"

static bool __fill_media_id_cb(media_info_h media, void *user_data)
{
	media_info_h *media_id = (media_info_h *)user_data;
	DBG("INTO MEdia id retrieval callback");
	media_info_clone(media_id, media);

	return FALSE;
}

static void __scan_folder_cb(media_content_error_e err, void *user_data)
{
	if (err != MEDIA_CONTENT_ERROR_NONE) {
		ERR("Scan folder callback returns error = [%d]\n", err);
	}

	return;
}

mtp_bool _util_get_audio_metadata(const mtp_char *filepath,
		comp_audio_meta_t *audio_data)
{
	char *temp = NULL;
	audio_meta_h audio;
	filter_h filter = NULL;
	media_info_h media_item = NULL;
	mtp_int32 ret = MEDIA_CONTENT_ERROR_NONE;
	mtp_char condition[MEDIA_PATH_COND_MAX_LEN + 1];

	retv_if(filepath == NULL, FALSE);
	retv_if(audio_data == NULL, FALSE);

	g_snprintf(condition, MEDIA_PATH_COND_MAX_LEN + 1, "%s\"%s\"",
			MEDIA_PATH_COND, filepath);

	ret = media_filter_create(&filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("Fail to create filter ");
		return FALSE;
	}

	ret = media_filter_set_condition(filter, condition,
			MEDIA_CONTENT_COLLATE_DEFAULT);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("Failed to set condition ");
		media_filter_destroy(filter);
		return FALSE;
	}

	ret = media_info_foreach_media_from_db(filter, __fill_media_id_cb,
			&media_item);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("media_info_foreach_media_from_db Fail");
		media_filter_destroy(filter);
		return FALSE;
	}

	media_filter_destroy(filter);

	if (media_item == NULL) {
		ERR("File entry not found in db");
		return FALSE;
	}

	ret = media_info_get_audio(media_item, &audio);
	if (ret != MEDIA_CONTENT_ERROR_NONE || audio == NULL) {
		ERR("media_info_get_audio Fail or Audio is NULL");
		media_info_destroy(media_item);
		return FALSE;
	}

	ret = audio_meta_get_album(audio, &(audio_data->commonmeta.album));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_ALBUM Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_artist(audio, &(audio_data->commonmeta.artist));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_ARTIST Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_bit_rate(audio,
			&(audio_data->commonmeta.audio_bitrate));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_AUDIO_BITRATE Fail");
		goto ERROR_EXIT;
	}

	ret =audio_meta_get_composer(audio, &(audio_data->commonmeta.author));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_AUTHOR Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_copyright(audio,
			&(audio_data->commonmeta.copyright));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_COPYRIGHT Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_duration(audio,
			&(audio_data->commonmeta.duration));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_DURATION Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_genre(audio, &(audio_data->commonmeta.genre));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_GENRE Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_recorded_date(audio,
			&(audio_data->commonmeta.year));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_RECDATE Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_track_num(audio, &temp);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_TRACK_NUM Fail");
		goto ERROR_EXIT;
	}

	if (NULL != temp) {
		audio_data->audiometa.track = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = audio_meta_get_channel(audio,
			&(audio_data->commonmeta.num_channel));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_NUM_CHANNEL Fail");
		goto ERROR_EXIT;
	}

	ret = audio_meta_get_sample_rate(audio,
			&(audio_data->commonmeta.sample_rate));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_SAMPLE_RATE Fail");
		goto ERROR_EXIT;
	}

	audio_data->commonmeta.description = g_strdup("Unknown");
	audio_data->commonmeta.audio_codec = 0;

	audio_meta_destroy(audio);
	media_info_destroy(media_item);
	return TRUE;

ERROR_EXIT:

	audio_meta_destroy(audio);
	media_info_destroy(media_item);
	return FALSE;
}

mtp_bool _util_get_video_metadata(mtp_char *filepath,
		comp_video_meta_t *video_data)
{
	filter_h filter = NULL;
	video_meta_h video;
	media_info_h media_item = NULL;
	mtp_int32 ret = MEDIA_CONTENT_ERROR_NONE;
	mtp_char condition[MEDIA_PATH_COND_MAX_LEN + 1];

	retv_if(filepath == NULL, FALSE);
	retv_if(video_data == NULL, FALSE);

	g_snprintf(condition, sizeof(condition), "%s\"%s\"", MEDIA_PATH_COND, filepath);

	ret = media_filter_create(&filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("Fail to create filter ");
		return FALSE;
	}

	ret = media_filter_set_condition(filter, condition,
			MEDIA_CONTENT_COLLATE_DEFAULT);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("Failed to set condition ");
		media_filter_destroy(filter);
		return FALSE;
	}

	ret = media_info_foreach_media_from_db(filter, __fill_media_id_cb,
			&media_item);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("media_info_foreach_media_from_db Fail");
		media_filter_destroy(filter);
		return FALSE;
	}

	media_filter_destroy(filter);

	if (media_item == NULL) {
		ERR("File entry not found in db");
		return FALSE;
	}

	ret = media_info_get_video(media_item, &video);
	if (ret != MEDIA_CONTENT_ERROR_NONE || video == NULL) {
		ERR("media_info_get_audio Fail or video is NULL");
		media_info_destroy(media_item);
		return FALSE;
	}

	ret = video_meta_get_album(video, &(video_data->commonmeta.album));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_ALBUM Fail");
		goto ERROR_EXIT;
	}

	ret = video_meta_get_artist(video, &(video_data->commonmeta.artist));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_ARTIST Fail");
		goto ERROR_EXIT;
	}

	/* AUDIO BITRATE */
	video_data->commonmeta.audio_bitrate = 0;

	ret = video_meta_get_composer(video, &(video_data->commonmeta.author));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_AUTHOR Fail");
		goto ERROR_EXIT;
	}

	ret = video_meta_get_copyright(video,
			&(video_data->commonmeta.copyright));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_COPYRIGHT Fail");
		goto ERROR_EXIT;
	}

	/* Description */
	video_data->commonmeta.description = g_strdup("Unknown");

	ret = video_meta_get_duration(video,
			&(video_data->commonmeta.duration));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_DURATION Fail");
		goto ERROR_EXIT;
	}

	ret = video_meta_get_genre(video, &(video_data->commonmeta.genre));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_GENRE Fail");
		goto ERROR_EXIT;
	}

	ret = video_meta_get_recorded_date(video,
			&(video_data->commonmeta.year));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_REC_DATE Fail");
		goto ERROR_EXIT;
	}

	/* METADATA_AUDIO_CHANNELS */
	video_data->commonmeta.num_channel = 0;

	/* METADAT_RATING */
	video_data->commonmeta.rating = 0;

	/* METADATA_SAMPLE_RATE */
	video_data->commonmeta.sample_rate = 0;

	ret = video_meta_get_track_num(video, &(video_data->videometa.track));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_TRACK_NUM Fail");
		goto ERROR_EXIT;
	}

	ret = video_meta_get_bit_rate(video, &(video_data->videometa.video_br));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_VIDEO_BITRATE Fail");
		goto ERROR_EXIT;
	}

	/* VIDEO_FPS */
	video_data->videometa.video_fps = 0;
	video_data->commonmeta.audio_codec = 0;
	video_data->videometa.video_br = 0;

	ret = video_meta_get_height(video, &(video_data->videometa.video_h));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_HEIGHT Fail");
		goto ERROR_EXIT;
	}

	ret = video_meta_get_width(video, &(video_data->videometa.video_w));
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("METADATA_WIDTH Fail");
		goto ERROR_EXIT;
	}

	video_meta_destroy(video);
	media_info_destroy(media_item);
	return TRUE;
ERROR_EXIT:
	video_meta_destroy(video);
	media_info_destroy(media_item);

	return FALSE;
}

static media_info_h __util_find_media_info(mtp_char *condition)
{
	int ret;
	filter_h filter = NULL;
	media_info_h media_item = NULL;

	ret = media_filter_create(&filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("Fail to create filter ");
		return NULL;
	}

	do {
		ret = media_filter_set_condition(filter, condition, MEDIA_CONTENT_COLLATE_DEFAULT);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			ERR("media_filter_set_condition() Fail");
			break;
		}

		ret = media_filter_set_offset(filter, 0, 1);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			ERR("media_filter_set_offset() Fail");
			break;
		}

		ret = media_info_foreach_media_from_db(filter, __fill_media_id_cb, &media_item);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			ERR("media_info_foreach_media_from_db() Fail");
			break;
		}
	}while (0);

	media_filter_destroy(filter);

	return media_item;
}


static mtp_bool __util_get_height_width(image_meta_h image, int *h, int *w)
{
	int ret;

	ret = image_meta_get_height(image, h);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("image_meta_get_height() Fail(%d)", ret);
		return FALSE;
	}

	ret = image_meta_get_width(image, w);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("image_meta_get_width() Fail(%d)", ret);
		return FALSE;
	}

	return TRUE;
}

mtp_bool _util_get_image_ht_wt(const mtp_char *filepath,
		image_meta_t *image_data)
{
	mtp_int32 ret;
	image_meta_h image;
	media_info_h media_item;
	mtp_char condition[MEDIA_PATH_COND_MAX_LEN + 1];

	retv_if(filepath == NULL, FALSE);
	retv_if(image_data == NULL, FALSE);

	g_snprintf(condition, sizeof(condition), "%s\"%s\"", MEDIA_PATH_COND, filepath);

	media_item = __util_find_media_info(condition);
	if (media_item == NULL) {
		ERR("File entry not found in db");
		return FALSE;
	}

	ret = media_info_get_image(media_item, &image);
	if (ret != MEDIA_CONTENT_ERROR_NONE || image == NULL) {
		ERR("media_info_get_image() Fail(%d) or image(%p) is NULL", ret, image);
		media_info_destroy(media_item);
		return FALSE;
	}

	ret = __util_get_height_width(image, &image_data->ht, &image_data->wt);
	if (FALSE == ret)
		ERR("__util_get_height_width() Fail");

	media_info_destroy(media_item);
	image_meta_destroy(image);

	return ret;
}

mtp_bool _util_get_audio_meta_from_extractor(const mtp_char *filepath,
		comp_audio_meta_t *audio_data)
{
	mtp_int32 ret = 0;
	mtp_char *temp = NULL;
	metadata_extractor_h metadata = NULL;

	retv_if(filepath == NULL, FALSE);
	retv_if(audio_data == NULL, FALSE);

	ret = metadata_extractor_create(&metadata);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("metadata extractor create Fail");
		return FALSE;
	}

	ret = metadata_extractor_set_path(metadata, filepath);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("metadata extractor set path Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_ALBUM,
			&(audio_data->commonmeta.album));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_ALBUM Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_ARTIST,
			&(audio_data->commonmeta.artist));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_ARTIST Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_AUDIO_BITRATE,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_AUDIO_BITRATE Fail");
		goto ERROR_EXIT;
	}

	if (NULL != temp) {
		audio_data->commonmeta.audio_bitrate = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_AUTHOR,
			&(audio_data->commonmeta.author));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_AUTHOR Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_COPYRIGHT,
			&(audio_data->commonmeta.copyright));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_COPYRIGHT Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_DESCRIPTION,
			&(audio_data->commonmeta.description));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_DESCRIPTION Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_DURATION,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_DURATION Fail");
		goto ERROR_EXIT;
	}

	if (NULL != temp) {
		audio_data->commonmeta.duration = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_GENRE,
			&(audio_data->commonmeta.genre));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_GENRE Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_RECDATE,
			&(audio_data->commonmeta.year));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_RECDATE Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_TRACK_NUM,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_TRACK_NUM Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		audio_data->audiometa.track = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_RATING,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_RATING Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		audio_data->commonmeta.rating = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata,
			METADATA_AUDIO_SAMPLERATE, &temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_SAMPLERATE Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		audio_data->commonmeta.sample_rate = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_AUDIO_CHANNELS,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_CHANNELS Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		audio_data->commonmeta.num_channel = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	audio_data->commonmeta.audio_codec = 0;

	metadata_extractor_destroy(metadata);
	return TRUE;

ERROR_EXIT:
	metadata_extractor_destroy(metadata);
	return FALSE;
}

mtp_bool _util_get_video_meta_from_extractor(const mtp_char *filepath,
		comp_video_meta_t *video_data)
{
	mtp_int32 ret = 0;
	mtp_char *temp = NULL;
	metadata_extractor_h metadata = NULL;

	retv_if(filepath == NULL, FALSE);
	retv_if(video_data == NULL, FALSE);

	ret = metadata_extractor_create(&metadata);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("metadata extractor create Fail");
		return FALSE;
	}

	ret = metadata_extractor_set_path(metadata, filepath);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("metadata extractor set path Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_ALBUM,
			&(video_data->commonmeta.album));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_ALBUM Fail");
		goto ERROR_EXIT;

	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_ARTIST,
			&(video_data->commonmeta.artist));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_ARTIST Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_AUDIO_BITRATE,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_AUDIO_BITRATE Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		video_data->commonmeta.audio_bitrate = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_AUTHOR,
			&(video_data->commonmeta.author));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_AUTHOR Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_COPYRIGHT,
			&(video_data->commonmeta.copyright));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_COPYRIGHT Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_DESCRIPTION,
			&(video_data->commonmeta.description));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_DESCRIPTION Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_DURATION,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_DURATION Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		video_data->commonmeta.duration = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_GENRE,
			&(video_data->commonmeta.genre));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_GENRE Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_RECDATE,
			&(video_data->commonmeta.year));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_RECDATE Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_AUDIO_CHANNELS,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_AUDIO_CHANNELS Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		video_data->commonmeta.num_channel = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_RATING,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_RATING Fail");
		goto ERROR_EXIT;
	}

	if (NULL != temp) {
		video_data->commonmeta.rating = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata,
			METADATA_AUDIO_SAMPLERATE, &temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_AUDIO_SAMPLERATE Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		video_data->commonmeta.sample_rate = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_TRACK_NUM,
			&(video_data->videometa.track));
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_TRACK_NUM Fail");
		goto ERROR_EXIT;
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_VIDEO_BITRATE,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_VIDEO_BITRATE Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		video_data->videometa.video_br = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_VIDEO_FPS,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_VIDEO_FPS Fail");
		goto ERROR_EXIT;
	}
	video_data->videometa.video_fps = atoi(temp);
	MTP_PAL_SAFE_FREE(temp);

	ret = metadata_extractor_get_metadata(metadata, METADATA_VIDEO_HEIGHT,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_VIDEO_HEIGHT Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		video_data->videometa.video_h = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	ret = metadata_extractor_get_metadata(metadata, METADATA_VIDEO_WIDTH,
			&temp);
	if (ret != METADATA_EXTRACTOR_ERROR_NONE) {
		ERR("METADATA_VIDEO_WIDTH Fail");
		goto ERROR_EXIT;
	}
	if (NULL != temp) {
		video_data->videometa.video_w = atoi(temp);
		MTP_PAL_SAFE_FREE(temp);
	}

	metadata_extractor_destroy(metadata);
	return TRUE;

ERROR_EXIT:
	metadata_extractor_destroy(metadata);
	return FALSE;

}

void _util_flush_db(void)
{
	_util_add_file_to_db(NULL);
	_util_delete_file_from_db(NULL);
}

void _util_delete_file_from_db(const mtp_char *filepath)
{
	int ret;

	ret_if(NULL == filepath);

	ret = media_info_delete_from_db(filepath);
	if (MEDIA_CONTENT_ERROR_NONE != ret)
		ERR("media_info_delete_from_db() Fail(%d)", ret);

	return;
}

void _util_add_file_to_db(const mtp_char *filepath)
{
	mtp_int32 ret;
	media_info_h info = NULL;

	ret_if(NULL == filepath);

	ret = media_info_insert_to_db(filepath, &info);
	if (MEDIA_CONTENT_ERROR_NONE != ret)
		ERR("media_info_insert_to_db() Fail(%d)", ret);

	if (info)
		media_info_destroy(info);

	return;
}

void _util_scan_folder_contents_in_db(const mtp_char *filepath)
{
	mtp_int32 ret;

	ret_if(filepath == NULL);

	ret = media_content_scan_folder(filepath, true, __scan_folder_cb, NULL);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("media_content_scan_folder Fail : %d\n", ret);
	}

	return;
}

void _util_free_common_meta(common_meta_t *metadata)
{
	MTP_PAL_SAFE_FREE(metadata->album);
	MTP_PAL_SAFE_FREE(metadata->artist);
	MTP_PAL_SAFE_FREE(metadata->author);
	MTP_PAL_SAFE_FREE(metadata->copyright);
	MTP_PAL_SAFE_FREE(metadata->description);
	MTP_PAL_SAFE_FREE(metadata->genre);
	MTP_PAL_SAFE_FREE(metadata->year);

	return;
}
void _util_free_video_meta(video_meta_t *video)
{
	MTP_PAL_SAFE_FREE(video->track);
	return;
}
