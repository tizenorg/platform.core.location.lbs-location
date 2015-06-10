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

#ifndef __LOCATION_BATCH_H_
#define __LOCATION_BATCH_H_

#include <location-types.h>

G_BEGIN_DECLS

GType location_batch_get_type(void);
#define LOCATION_TYPE_BATCH (location_batch_get_type ())

/**
 * @file location-batch.h
 * @brief This file contains the internal definitions and structures related to batch information.
 */
/**
 * @addtogroup LocationAPI
 * @{
 * @defgroup LocationAPIBatch Location Batch
 * @breif This provides APIs related to Location Batch
 * @addtogroup LocationAPIBatch
 * @{
 */

/**
 * TBD
 */
typedef struct {
	guint timestamp;		/*/< Time stamp. */
	guint level;			/*/< Level. */
	gdouble latitude;		/*/< Latitude data. */
	gdouble longitude;		/*/< Longitude data. */
	gdouble altitude;		/*/< Altitude data. */
	gdouble horizontal_accuracy;	/*/< Horizontal accuracy data. */
	gdouble vertical_accuracy;		/*/< Vertical accuracy data. */
	gdouble speed;			/*/< The speed over ground. (km/h) */
	gdouble direction;		/*/< The course made in degrees relative to true north. The value is always in the range [0.0, 360.0] degree. */
	gdouble climb;			/*/< The vertical speed. (km/h) */
} LocationBatchDetail;

/**
 * TBD
 */
struct _LocationBatch {
	guint num_of_location;	/*/< The number of location batch. */
	LocationBatchDetail *batch_data;
};

LocationBatch *location_batch_new(int num_of_location);

gboolean location_set_batch_parse_details(LocationBatch *batch, char *location_info, int i);

gboolean location_get_batch_details(const LocationBatch *batch, guint index, gdouble *latitude, gdouble *longitude, gdouble *altitude, gdouble *speed, gdouble *direction, gdouble *h_accuracy, gdouble *v_accuracy, guint *timestamp);

gboolean location_set_batch_details(LocationBatch *batch, guint index, gdouble latitude, gdouble longitude, gdouble altitude, gdouble speed, gdouble direction, gdouble h_accuracy, gdouble v_accuracy, guint timestamp);

LocationBatch *location_batch_copy(const LocationBatch *batch);

LocationBatch *location_get_batch_file(int num_of_location);

void location_batch_free(LocationBatch *batch);

/**
 * @} @}
 */

G_END_DECLS

#endif
