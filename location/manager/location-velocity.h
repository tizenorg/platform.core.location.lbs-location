/*
 * libslp-location
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *			Genie Kim <daejins.kim@samsung.com>
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

#ifndef __LOCATION_VELOCITY_H_
#define __LOCATION_VELOCITY_H_

#include <location-types.h>

G_BEGIN_DECLS

GType location_velocity_get_type(void);
#define LOCATION_TYPE_VELOCITY		(location_velocity_get_type ())

/**
 * @file location-velocity.h
 * @brief This file contains the internal definitions and structures related to velocity information.
 */
/**
 * @addtogroup LocationAPI
 * @{
 * @defgroup LocationAPIVelocity Location Velocity
 * @breif This provides APIs related to Location Velocity
 * @addtogroup LocationAPIVelocity
 * @{
 */

/**
 * @brief This represents velocity information such as as speed, direction, climb.
 */
struct _LocationVelocity {
	guint timestamp;		/*/< Time stamp. */
	gdouble speed;			/*/< The speed over ground. (km/h) */
	gdouble direction;		/*/< The course made in degrees relative to true north. The value is always in the range [0.0, 360.0] degree. */
	gdouble climb;			/*/< The vertical speed. (km/h) */
};

/**
 * @brief	Create a new #LocationVelocity with given information.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] timestamp - Time stamp.
 * @param [in] speed - The speed over ground. (km/h)
 * @param [in] direction - The course made in degrees relative to true north. The value is always in the range [0.0, 360.0] degree.
 * @param [in] climb - The vertical speed. (km/h)
 * @return a new #LocationVelocity
 * @retval NULL if error occured
 */
LocationVelocity *location_velocity_new(guint timestamp, gdouble speed, gdouble direction,	gdouble climb);

/**
 * @brief	Free a #LocationVelocity.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] velocity - a #LocationVelocity.
 * @return None.
 */
void location_velocity_free(LocationVelocity *velocity);

/**
 * @brief	Compares two velocities for equality, returning TRUE if they are equal.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] velocity1 - a #LocationVelocity
 * @param [in] velocity2 - a #LocationVelocity
 * @return gboolean
 * @retval\n
 * TRUE - if equal\n
 * FALSE - if not equal\n
 */
gboolean location_velocity_equal(const LocationVelocity *velocity1, const LocationVelocity *velocity2);

/**
 * @brief	Makes a copy of #LocationVelocity
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] velocity - a #LocationVelocity
 * @return a new #LocationVelocity
 * @retval NULL if error occured
 */
LocationVelocity *location_velocity_copy(const LocationVelocity *velocity);

/**
 * @} @}
 */

G_END_DECLS

#endif
