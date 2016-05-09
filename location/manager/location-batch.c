/*
 * libslp-location
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *          Genie Kim <daejins.kim@samsung.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MAX_BATCH_ITEM		8
#define BATCH_SENTENCE_SIZE	256

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <tzplatform_config.h>
#include <sensor.h>
#include "location-batch.h"
#include "location-log.h"


EXPORT_API LocationBatch *
location_batch_new(int num_of_location)
{
	LocationBatch *batch = g_slice_new0(LocationBatch);
	g_return_val_if_fail(batch, NULL);

	batch->num_of_location = num_of_location;
	batch->batch_data = g_new0(LocationBatchDetail, batch->num_of_location);
	return batch;
}

EXPORT_API gboolean
location_set_batch_parse_details(LocationBatch *batch, char *location_info, int i)
{
	g_return_val_if_fail(batch, FALSE);
	g_return_val_if_fail(batch->batch_data, FALSE);

	char location[128] = {0,};
	char *last_location[MAX_BATCH_ITEM] = {0,};
	char *last = NULL;
	int index = 0;

	snprintf(location, sizeof(location), "%s", location_info);

	last_location[index] = (char *)strtok_r(location, ";", &last);
	while (last_location[index] != NULL) {
		switch (index) {
		case 0:
				batch->batch_data[i].timestamp = strtod(last_location[index], NULL);
				break;
		case 1:
				batch->batch_data[i].latitude = strtod(last_location[index], NULL);
				break;
		case 2:
				batch->batch_data[i].longitude = strtod(last_location[index], NULL);
				break;
		case 3:
				batch->batch_data[i].altitude = strtod(last_location[index], NULL);
				break;
		case 4:
				batch->batch_data[i].speed = strtod(last_location[index], NULL);
				break;
		case 5:
				batch->batch_data[i].direction = strtod(last_location[index], NULL);
				break;
		case 6:
				batch->batch_data[i].horizontal_accuracy = strtod(last_location[index], NULL);
				break;
		case 7:
				batch->batch_data[i].vertical_accuracy = strtod(last_location[index], NULL);
				break;
		default:
				break;
		}
		if (++index == MAX_BATCH_ITEM) break;
		last_location[index] = (char *)strtok_r(NULL, ";", &last);
	}

	return TRUE;
}

EXPORT_API LocationBatch *
location_get_batch_file(int num_of_location)
{
	LOCATION_LOGD("location_get_batch_file [num_of_location = %d]", num_of_location);
	LocationBatch *batch = location_batch_new(num_of_location);
	batch->num_of_location = num_of_location;

	const char *batch_path = tzplatform_mkpath(TZ_SYS_MEDIA, "lbs-server/location_batch.log");
	FILE *fd = fopen(batch_path, "r");

	if (fd != NULL) {
		char buf[BATCH_SENTENCE_SIZE] = { 0, };
		int i = 0;

		for (i = 0; i < num_of_location; i++) {
			if (fgets(buf, BATCH_SENTENCE_SIZE, fd) != NULL)
				location_set_batch_parse_details(batch, buf, i);
			else
				LOCATION_LOGE("Batch fgets failed");
		}

		if (fd != 0) {
			if (fclose(fd) != 0)
				LOCATION_LOGE("Batch fclose failed");
			fd = NULL;
		}
	} else {
		LOCATION_LOGE("Batch fopen failed. fd is NULL");
	}
	return batch;
}

#ifdef TIZEN_DEVICE
EXPORT_API gboolean location_set_sensor_batch(LocationBatch *batch, sensor_event_s *event)
{
	g_return_val_if_fail(batch, FALSE);
	g_return_val_if_fail(event, FALSE);
	g_return_val_if_fail(batch->num_of_location > event->values[4], FALSE);
	unsigned long long timestamp = event->timestamp;
	float latitude  = event->values[0];
	float longitude = event->values[1];
	float altitude  = event->values[2];
	float speed	 = event->values[3];
	int idx = (int)(event->values[4]);

	batch->batch_data[idx].timestamp = batch->start_time - (time_t)((timestamp / 1001000) % 100000);
	batch->batch_data[idx].latitude = latitude;
	batch->batch_data[idx].longitude = longitude;
	batch->batch_data[idx].altitude = altitude;
	batch->batch_data[idx].speed = speed;
	batch->batch_data[idx].direction = 0;
	batch->batch_data[idx].horizontal_accuracy = 0;
	batch->batch_data[idx].vertical_accuracy = 0;

	return TRUE;
}
#endif

EXPORT_API LocationBatch *
location_batch_copy(const LocationBatch *batch)
{
	g_return_val_if_fail(batch, NULL);
	LocationBatch *batch_dup = location_batch_new(batch->num_of_location);
	batch_dup->num_of_location = batch->num_of_location;
	int i = 0;
	for (i = 0 ; i < batch_dup->num_of_location; i++)
		location_set_batch_details(batch_dup, i,
									batch->batch_data[i].latitude,
									batch->batch_data[i].longitude,
									batch->batch_data[i].altitude,
									batch->batch_data[i].speed,
									batch->batch_data[i].direction,
									batch->batch_data[i].horizontal_accuracy,
									batch->batch_data[i].vertical_accuracy,
									batch->batch_data[i].timestamp);
	return batch_dup;
}

EXPORT_API gboolean
location_get_batch_details(const LocationBatch *batch,
							guint index,
							gdouble *latitude,
							gdouble *longitude,
							gdouble *altitude,
							gdouble *speed,
							gdouble *direction,
							gdouble *h_accuracy,
							gdouble *v_accuracy,
							guint *timestamp)
{
	g_return_val_if_fail(batch, FALSE);
	g_return_val_if_fail(latitude, FALSE);
	g_return_val_if_fail(longitude, FALSE);
	g_return_val_if_fail(altitude, FALSE);
	g_return_val_if_fail(speed, FALSE);
	g_return_val_if_fail(direction, FALSE);
	g_return_val_if_fail(h_accuracy, FALSE);
	g_return_val_if_fail(v_accuracy, FALSE);
	g_return_val_if_fail(timestamp, FALSE);
	g_return_val_if_fail(batch->batch_data, FALSE);

	*latitude	= batch->batch_data[index].latitude;
	*longitude	= batch->batch_data[index].longitude;
	*altitude	= batch->batch_data[index].altitude;
	*speed		= batch->batch_data[index].speed;
	*direction	= batch->batch_data[index].direction;
	*h_accuracy	= batch->batch_data[index].horizontal_accuracy;
	*v_accuracy	= batch->batch_data[index].vertical_accuracy;
	*timestamp	= batch->batch_data[index].timestamp;

	return TRUE;
}

EXPORT_API gboolean
location_set_batch_details(LocationBatch *batch,
							guint index,
							gdouble latitude,
							gdouble longitude,
							gdouble altitude,
							gdouble speed,
							gdouble direction,
							gdouble h_accuracy,
							gdouble v_accuracy,
							guint timestamp)
{
	g_return_val_if_fail(batch, FALSE);
	g_return_val_if_fail(batch->batch_data, FALSE);
	g_return_val_if_fail(index < batch->num_of_location, FALSE);

	batch->batch_data[index].latitude = latitude;
	batch->batch_data[index].longitude = longitude;
	batch->batch_data[index].altitude = altitude;
	batch->batch_data[index].speed = speed;
	batch->batch_data[index].direction = direction;
	batch->batch_data[index].horizontal_accuracy = h_accuracy;
	batch->batch_data[index].vertical_accuracy = v_accuracy;
	batch->batch_data[index].timestamp = timestamp;

	return TRUE;
}

EXPORT_API void
location_batch_free(LocationBatch *batch)
{
	g_return_if_fail(batch);
	g_free(batch->batch_data);
	g_slice_free(LocationBatch, batch);
}
