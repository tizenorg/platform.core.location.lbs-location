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

#include "location-log.h"
#include "location-ielement.h"

static void
location_ielement_base_init(gpointer g_class)
{
	static gboolean is_initialized = FALSE;

	if (is_initialized) {
		/* add properties and signals to the interface here */

		is_initialized = TRUE;
	}
}

GType
location_ielement_get_type(void)
{
	static GType iface_type = 0;

	if (iface_type == 0) {
		static const GTypeInfo info = {
			sizeof(LocationIElementInterface),
			location_ielement_base_init, /* base_init */
			NULL /* base_finalize */
		};

		iface_type = g_type_register_static(G_TYPE_INTERFACE, "LocationIElement", &info, 0);
	}

	return iface_type;
}

int
location_ielement_start(LocationIElement *self)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->start, LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_IELEMENT_GET_INTERFACE(self)->start(self);
}

int
location_ielement_stop(LocationIElement *self)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->stop, LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_IELEMENT_GET_INTERFACE(self)->stop(self);
}

int
location_ielement_start_batch(LocationIElement *self)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->start_batch, LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_IELEMENT_GET_INTERFACE(self)->start_batch(self);
}

int
location_ielement_stop_batch(LocationIElement *self)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->stop_batch, LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_IELEMENT_GET_INTERFACE(self)->stop_batch(self);
}

int
location_ielement_request_single_location(LocationIElement *self, int timeout)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->request_single_location, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->request_single_location(self, timeout);
}

int
location_ielement_get_position(LocationIElement *self,
                               LocationPosition **position,
                               LocationAccuracy **accuracy)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_position, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_position(self, position, accuracy);
}

int
location_ielement_get_position_ext(LocationIElement *self,
                                   LocationPosition **position,
                                   LocationVelocity **velocity,
                                   LocationAccuracy **accuracy)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_position_ext, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_position_ext(self, position, velocity, accuracy);
}


int
location_ielement_get_last_position(LocationIElement *self,
                                    LocationPosition **position,
                                    LocationAccuracy **accuracy)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_position, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_position(self, position, accuracy);
}

int
location_ielement_get_last_position_ext(LocationIElement *self,
                                        LocationPosition **position,
                                        LocationVelocity **velocity,
                                        LocationAccuracy **accuracy)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_position_ext, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_position_ext(self, position, velocity, accuracy);
}


int
location_ielement_get_batch(LocationIElement *self, LocationBatch **batch)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(batch, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_batch, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_batch(self, batch);
}

int
location_ielement_get_satellite(LocationIElement *self,
                                LocationSatellite **satellite)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(satellite, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_satellite, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_satellite(self, satellite);
}

int
location_ielement_get_last_satellite(LocationIElement *self,
                                     LocationSatellite **satellite)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(satellite, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_satellite, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_satellite(self, satellite);
}

int
location_ielement_get_velocity(LocationIElement *self,
                               LocationVelocity **velocity,
                               LocationAccuracy **accuracy)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_velocity, LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_velocity(self, velocity, accuracy);
}

int
location_ielement_get_last_velocity(LocationIElement *self,
                                    LocationVelocity **velocity,
                                    LocationAccuracy **accuracy)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_velocity, LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_last_velocity(self, velocity, accuracy);
}

int
location_ielement_set_option(LocationIElement *self, const char *option)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_IELEMENT_GET_INTERFACE(self)->set_option(self, option);
}

int
location_ielement_get_nmea(LocationIElement *self,
                           char **nmea)
{
	g_return_val_if_fail(LOCATION_IS_IELEMENT(self), LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(nmea, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self), LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(LOCATION_IELEMENT_GET_INTERFACE(self)->get_nmea, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_IELEMENT_GET_INTERFACE(self)->get_nmea(self, nmea);
}
