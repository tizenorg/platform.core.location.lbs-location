/*
 * libslp-location
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *		  Genie Kim <daejins.kim@samsung.com>
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

#ifndef __LOCATION_POSITION_H_
#define __LOCATION_POSITION_H_

#include <location-types.h>

#define MAX_KEY_LENGTH		16
#define HALF_KEY_LENGTH		8

G_BEGIN_DECLS

GType location_position_get_type(void);
#define LOCATION_TYPE_POSITION (location_position_get_type ())

/**
 * @file location-position.h
 * @brief This file contains the internal definitions and structures related to position information.
 */
/**
 * @addtogroup LocationAPI
 * @{
 * @defgroup LocationAPIPosition Location Position
 * @breif This provides APIs related to Location Position
 * @addtogroup LocationAPIPosition
 * @{
 *
 */

/**
 * @brief This represents the various fix states.
 */
typedef enum {
    LOCATION_STATUS_NO_FIX = 0,	/*/< No fix status. */
    LOCATION_STATUS_2D_FIX,		/*/< 2D fix status (latitude/longitude/speed/direction). */
    LOCATION_STATUS_3D_FIX		/*/< 3D fix status (altitude/climb as well). */
} LocationStatus;

/**
 * @brief This represents position information such as latitude-longitude-altitude values and timestamp.
 */
struct _LocationPosition {
	guint timestamp;		/*/< Time stamp. */
	gdouble latitude;		/*/< Latitude data. */
	gdouble longitude;		/*/< Longitude data. */
	gdouble altitude;		/*/< Altitude data. */
	LocationStatus status;	/*/< Fix states. */
};

/**
 * @brief This represents last known position information such as latitude-longitude values and accuracy.
 */
struct _LocationLastPosition {
	LocationMethod method;			/*/< Location Method. */
	guint timestamp;				/*/< Time stamp. */
	gdouble latitude;				/*/< Latitude data. */
	gdouble longitude;				/*/< Longitude data. */
	gdouble altitude;				/*/< Altitude data. */
	gdouble horizontal_accuracy;	/*/< Horizontal accuracy data. */
	gdouble vertical_accuracy;		/*/< Vertical accuracy data. */
};

/**
 * @brief   Create a new #LocationPosition with given information.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in]  timestamp - Time stamp.
 * @param [in]  latitude - Latitude data.
 * @param [in]  longitude - Longitude data.
 * @param [in]  altitude - Altitude data.
 * @param [in]  status - a #LocationStatus.
 * @return a new #LocationPosition
 * @retval NULL if error occured
 */
LocationPosition *location_position_new(guint timestamp, gdouble latitude, gdouble longitude, gdouble altitude, LocationStatus status);

/**
 * @brief   Free a #LocationPosition.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] position - a #LocationPosition.
 * @return None.
 */
void location_position_free(LocationPosition *position);

/**
 * @brief   Compares two positions for equality, returning TRUE if they are equal.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in]  position1 - a #LocationPosition
 * @param [in]  position2 - a #LocationPosition
 * @return gboolean
 * @retval\n
 * TRUE - if equal\n
 * FALSE - if not equal\n
 */
gboolean location_position_equal(const LocationPosition *position1, const LocationPosition *position2);

/**
 * @brief   Makes a copy of #LocationPosition
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in]  position - a #LocationPosition
 * @return a new #LocationPosition
 * @retval NULL if error occured
 */
LocationPosition *location_position_copy(const LocationPosition *position);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/* Vincenty Inverse Solution of Geodesics on the Ellipsoid (c) Chris Veness 2002-2010			 */
/*																								*/
/* from: Vincenty inverse formula - T Vincenty, "Direct and Inverse Solutions of Geodesics on the */
/*	   Ellipsoid with application of nested equations", Survey Review, vol XXII no 176, 1975	*/
/*	   http://www.ngs.noaa.gov/PUBS_LIB/inverse.pdf											 */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/**
 * @brief   Gets the approximate distance between two points. A distance is defined using the WGS84 ellipsoid.
 * @remarks Uses meters as a unit of measurement for a distance.
 * @pre	 None.
 * @post	None.
 * @param [in]  pos1 - a #LocationPosition (decimal degree)
 * @param [in]  pos2 - a #LocationPosition (decimal degree)
 * @param [out]  distance - a #gulong (meters)
 * @return int
 * @retval 0							  Success.
 *
 * Please refer #LocationError for more information.
 */
int location_get_distance(const LocationPosition *pos1, const LocationPosition *pos2, gulong *distance);

/**
 * @brief   Change position string to latitude and longitude integer.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in]  position - string of last position.
 * @param [in]  lat - latitude.
 * @param [in]  lon - longitude.
 * @return None.
 */
void location_last_position_a2i(char *position, int *lat, int *lon);

/**
 * @} @}
 */

G_END_DECLS

#endif
