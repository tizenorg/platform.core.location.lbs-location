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

#include "location-velocity.h"
#include "location-log.h"

GType
location_velocity_get_type(void)
{
	static volatile gsize type_volatile = 0;
	if (g_once_init_enter(&type_volatile)) {
		GType type = g_boxed_type_register_static(
		                 g_intern_static_string("LocationVelocity"),
		                 (GBoxedCopyFunc) location_velocity_copy,
		                 (GBoxedFreeFunc) location_velocity_free);
		g_once_init_leave(&type_volatile, type);
	}
	return type_volatile;
}

EXPORT_API LocationVelocity *
location_velocity_new(guint timestamp,
                      gdouble speed,
                      gdouble direction,
                      gdouble climb)
{
	LocationVelocity *velocity = g_slice_new0(LocationVelocity);
	g_return_val_if_fail(velocity, NULL);

	velocity->timestamp = timestamp;
	velocity->speed = speed;
	velocity->direction = direction;
	velocity->climb = climb;
	return velocity;
}

EXPORT_API void
location_velocity_free(LocationVelocity *velocity)
{
	g_return_if_fail(velocity);
	g_slice_free(LocationVelocity, velocity);
}

EXPORT_API gboolean
location_velocity_equal(const LocationVelocity *velocity1, const LocationVelocity *velocity2)
{
	g_return_val_if_fail(velocity1, FALSE);
	g_return_val_if_fail(velocity2, FALSE);

	if (velocity1->timestamp == velocity2->timestamp &&
	    velocity1->speed == velocity2->speed &&
	    velocity1->direction == velocity2->direction &&
	    velocity1->climb == velocity2->climb)
		return TRUE;
	return FALSE;
}

EXPORT_API LocationVelocity *
location_velocity_copy(const LocationVelocity *velocity)
{
	g_return_val_if_fail(velocity, NULL);

	LocationVelocity *new_velocity = NULL;

	new_velocity = location_velocity_new(velocity->timestamp,
	                                     velocity->speed,
	                                     velocity->direction,
	                                     velocity->climb);

	return new_velocity;
}
