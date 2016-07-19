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

#ifndef __LOCATION_MODULE_H__
#define __LOCATION_MODULE_H__

#include <gmodule.h>
#include <location-types.h>
#include <location-position.h>
#include <location-batch.h>
#include <location-velocity.h>
#include <location-accuracy.h>
#include <location-satellite.h>

G_BEGIN_DECLS

/**
 * @file location-module.h
 * @brief This file contains the structure and enumeration for location plug-in development.
 */

/**
 * @addtogroup LocationFW
 * @{
 * @defgroup LocationModules Location Modules
 * @brief  This sub module provides the definitions and structrues for 3rd party plugin modules.
 * @addtogroup LocationModules
 * @{
 */


/**
* @brief This represents a enabled/disabled callback function for a plug-in.
*/
typedef void (*LocModStatusCB)(gboolean enabled, LocationStatus status, gpointer userdata);

/**
 * @brief This represents a position callback function for a plug-in.
 */
typedef void (*LocModPositionExtCB)(gboolean enabled, LocationPosition *position, LocationVelocity *velocity, LocationAccuracy *accuracy, gpointer userdata);

/**
 * @brief This represents a batch callback function for a plug-in.
 */
typedef void (*LocModBatchExtCB)(gboolean enabled, guint num_of_location, gpointer userdata);

/**
 * @brief This represents a velocity callback function for a plug-in.
 */
typedef void (*LocModSatelliteCB)(gboolean enabled, LocationSatellite *satellite, gpointer userdata);

/**
 * @brief This represents APIs declared in a GPS plug-in for location GPS modules.
 */
typedef struct {
	int (*start)(gpointer handle, guint pos_update_interval, LocModStatusCB status_cb, LocModPositionExtCB pos_ext_cb, LocModSatelliteCB sat_cb, gpointer userdata);
	int (*start_batch)(gpointer handle, LocModBatchExtCB batch_ext_cb, guint batch_interval, guint batch_period, gpointer userdata);
	int (*stop)(gpointer handle);
	int (*stop_batch)(gpointer handle);
	int (*get_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	int (*get_last_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	int (*get_nmea)(gpointer handle, gchar **nmea_data);
	int (*get_satellite)(gpointer handle, LocationSatellite **satellite);
	int (*get_last_satellite)(gpointer handle, LocationSatellite **satellite);
	int (*set_option)(gpointer handle, const char *option);
	int (*set_position_update_interval)(gpointer handle, guint interval);
} LocModGpsOps;

/**
 * @brief This represents APIs declared in a WPS plug-in for location WPS modules.
 */
typedef struct {
	int (*start)(gpointer handle, LocModStatusCB status_cb, LocModPositionExtCB pos_ext_cb, gpointer userdata);
	int (*stop)(gpointer handle);
	int (*get_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	int (*get_last_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	int (*set_option)(gpointer handle, const char *option);
} LocModWpsOps;

typedef struct {
	int (*start)(gpointer handle, LocModStatusCB status_cb, LocModPositionExtCB pos_ext_cb, gpointer userdata);
	int (*stop)(gpointer handle);
	int (*get_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	int (*get_last_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	int (*set_option)(gpointer handle, const char *option);
	/* int (*set_position_update_interval)(gpointer handle, guint interval); */
	int (*set_mock_location)(gpointer handle, LocationPosition *position, LocationVelocity *velocity, LocationAccuracy *accuracy, LocModStatusCB status_cb, gpointer userdata);
	int (*clear_mock_location)(gpointer handle, LocModStatusCB status_cb, gpointer userdata);
} LocModMockOps;

typedef struct {
	int (*get_last_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	int (*get_last_wps_position)(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
	/* int (*set_option)(gpointer handle, const char *option); */
} LocModPassiveOps;

/**
 * @brief This is used for exported APIs in a plug-in for a location framework.
 */
#define LOCATION_MODULE_API __attribute__((visibility("default"))) G_MODULE_EXPORT

/**
 * @} @}
 */
G_END_DECLS

#endif
