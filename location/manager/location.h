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


#ifndef __LOCATION_H__
#define __LOCATION_H__


#include <glib.h>
#include <location-types.h>
#include <location-position.h>
#include <location-batch.h>
#include <location-velocity.h>
#include <location-accuracy.h>
#include <location-boundary.h>
#include <location-satellite.h>

G_BEGIN_DECLS

/**
 * @file location.h
 * @brief This file contains the Location API and related structure and enumeration.
 */
/**
 * @defgroup LocationFW LocationFW
 * @brief This is a Location Framework for providing location based services.
 * @addtogroup LocationFW
 * @{
 * @defgroup LocationAPI Location API
 * @brief This sub module provides the Location API.
 * @addtogroup LocationAPI
 * @{
 */

/**
 * @brief
 * Initialize location sub module.
 * @remarks None.
 * This API should be called before any other Location APIs.
 * @pre None.
 * @post None.
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see None.
 */
int location_init(void);

/**
 * @brief
 * Create a new #LocationObject by using given #LocationMethod.
 * @remarks
 * Returned object is necessary for other APIs.
 * @pre
 * #location_init should be called before.
 * @post None.
 * @param [in]
 * method - Location method to be used.
 * @return a new #LocationObject
 * @retval NULL			if error occured
 * @see location_free
 */
LocationObject *location_new(LocationMethod method);

/**
 * @brief
 * Free memory of given #LocationObject.
 * @remarks None.
 * @pre
 * #location_init should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new.
 * @return int
 * @retval 0							Success.
 *
 * Please refer #LocationError for more information.
 * @see location_new
 * @par Example
 * @code
#include <location.h>
int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	location_init ();
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc)
		return -1;

	// ... Implement your code

	location_free (loc);
	// You must free LocationObject when you don't need to use anymore.
	return 0;
}
 * @endcode
 */
int location_free(LocationObject *obj);

/**
 * @brief
 * Start the location service by using given #LocationObject.
 * @remarks
 * If you want to recieve signals, you should use this API.
 * @pre
 * #location_init should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @return int
 * @retval 0							Success.
 *
 * Please refer #LocationError for more information.
 * @see location_stop
 * @par Example
 * @code
#include <location.h>
static GMainLoop *loop = NULL;

static void cb_service_enabled (GObject *self, guint status, gpointer userdata)
{
	LocationObject *loc = (LocationObject*)userdata;
	LocationAccuracy *acc = NULL;
	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAddress *addr = NULL;
	LocationMethod method;

	g_object_get(loc, "method", &method, NULL);
	g_debug("Get property>> method:%d", method);

	if (LOCATION_ERROR_NONE == location_get_position (loc, &pos, &acc)) {
		g_debug ("SYNC>> Current position> time: %d, lat: %f, long: %f, alt: %f, status: %d",
			pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
			acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_position_free(pos);
		location_accuracy_free(acc);
	} else g_warning ("SYNC>> Current position> failed");

	if (LOCATION_ERROR_NONE == location_get_velocity (loc, &vel, &acc)) {
		g_debug ("SYNC>> Current velocity> time: %d, speed: %f, direction:%f, climb:%f",
				vel->timestamp, vel->speed, vel->direction, vel->climb);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
				acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_velocity_free(vel);
		location_accuracy_free(acc);
	} else g_warning ("SYNC>> Current velocity> failed");

	if (LOCATION_ERROR_NONE == location_get_address(loc, &addr, &acc)) {
		g_debug ("SYNC>> Current address> %s %s %s %s %s %s %s",
				addr->building_number, addr->street, addr->district, addr->city, addr->state, addr->postal_code, addr->country_code);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)", acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_address_free(addr);
		location_accuracy_free(acc);
	} else g_warning ("SYNC>> Current address> failed");
}

static void
cb_service_updated (GObject *self,
		guint type,
		gpointer data,
		gpointer accuracy,
		gpointer userdata)
{
	LocationAccuracy *acc = (LocationAccuracy*) accuracy;
	switch (type) {
		case POSITION_UPDATED: {
			LocationPosition *pos = (LocationPosition*) data;
			g_debug ("ASYNC>> Current position> time: %d, lat: %f, long: %f, alt: %f, status: %d",
				pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
			g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
				acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		}
			break;
		case VELOCITY_UPDATED: {
			LocationVelocity *vel = (LocationVelocity*) data;
			g_debug ("ASYNC>> Current velocity> time: %d, speed: %f, direction:%f, climb:%f",
					vel->timestamp, vel->speed, vel->direction, vel->climb);
			g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
					acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		}
			break;
		case SATELLITE_UPDATED: {
			int idx = 0;
			guint prn;
			gboolean used;
			guint elevation;
			guint azimuth;
			gint snr;

			LocationSatellite *sat = (LocationSatellite *)data;
			g_debug ("ASYNC>> Current Satellite> time: %d, satellite in view = %d, satellite in used = %d", sat->timestamp, sat->num_of_sat_inview, sat->num_of_sat_used);
			g_debug ("\tinview satellite information = ");
			for (idx=0; idx<sat->num_of_sat_inview; idx++) {
				location_satellite_get_satellite_details(sat, idx, &prn, &used, &elevation, &azimuth, &snr);
				g_debug ("\t\t[%02d] used: %d, prn: %d, elevation: %d, azimuth: %d, snr: %d", idx, used, prn, elevation, azimuth, snr);
			}
		}
			break;

		default:
			g_warning ("ASYNC>> Undefined update type");
			break;
	}
}


int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	int interval = 6;	//seconds
	location_init ();

	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc)
		return -1;

	g_object_set(loc, "update-interval", interval, NULL);

	g_signal_connect (loc, "service-enabled", G_CALLBACK(cb_service_enabled), loc);
	g_signal_connect (loc, "service-updated", G_CALLBACK(cb_service_updated), loc);

	location_start(loc);
	loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (loop); // GMainLoop is needed for receiving signals.

	// ...
	return 0;
}
 * @endcode
 */
int location_start(LocationObject *obj);

/**
 * @brief
 * Stop the location service by using given #LocationObject.
 * @remarks
 * After call this API, you can not recieve signals.
 * @pre
 * #location_init should be called before.\n
 * #location_start should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see location_start
 * @par Example
 * @code
#include <location.h>
static GMainLoop *loop = NULL;

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	location_init ();
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc)
		return -1;

	location_start(loc);
	loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (loop);

	// ....

	location_stop (loc);
	// you can not receive signals anymore.
	return 0;
}
 * @endcode
 */
int location_stop(LocationObject *obj);

int location_request_single_location(LocationObject *obj, int timeout);

/**
 * @brief
 * Check wheither a method is available.
 * @remarks
 * @pre
 * #location_init should be called before.\n
 * @post None.
 * @param [in] method - a #LocationMethod
 * @return int
 * @retval True		Supported
 *		False		Not supported
 * @par Example
 #include <location.h>
static GMainLoop *loop = NULL;

int main (int argc, char *argv[])
{
	gboolean is_supported = FALSE;

	// ....

	is_supported = location_is_supported_method(LOCATION_METHOD_HYBRID);
	if(is_supported == TRUE)
		g_printf("Hybrid Method is supported.\n");
	else
		g_printf("Hybrid Method is not supported.\n");

	return 0;
}* @code
 * @endcode
 */
gboolean location_is_supported_method(LocationMethod method);

int location_is_enabled_method(LocationMethod method, int *is_enabled);

int location_enable_method(const LocationMethod method, const int enable);


/**
 * @brief
 * Get current position information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre
 * #location_init should be called before.\n
 * #location_start should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @param [out]
 * position - a new #LocationPosition
 * @param [out]
 * accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see location_get_velocity
 * @par Example
 * @code
#include <location.h>
static GMainLoop *loop = NULL;

static void cb_service_enabled (GObject *self, guint status, gpointer userdata)
{
	g_debug("cb_service_enabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);

	LocationObject *loc = (LocationObject*)userdata;
	LocationAccuracy *acc = NULL;
	LocationPosition *pos = NULL;

	// This function works properly after service is enabled.
	if (LOCATION_ERROR_NONE == location_get_position (loc, &pos, &acc)) {
		g_debug ("SYNC>> Current position> time: %d, lat: %f, long: %f, alt: %f, status: %d",
			pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
			acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_position_free(pos);
		location_accuracy_free(acc);
	} else g_warning ("SYNC>> Current position> failed");
}

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	gulong handler_id = 0;

	location_init ();
	loop = g_main_loop_new (NULL, TRUE);
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	handler_id = g_signal_connect (loc, "service-enabled", G_CALLBACK(cb_service_enabled), loc);
	location_start (loc);
	g_main_loop_run (loop);

	g_signal_handler_disconnect(loc, handler_id);
	location_stop (loc);
	location_free (loc);

	return 0;
}
 * @endcode
 */
int location_get_position(LocationObject *obj, LocationPosition **position, LocationAccuracy **accuracy);

/**
 * @brief
 * Get current position & velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre
 * #location_init should be called before.\n
 * #location_start should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] position - a new #LocationPosition
 * @param [out] velocity - a new #LocationVelocity
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see location_get_position
 * @par Example
 * @code
#include <location.h>
static GMainLoop *loop = NULL;

static void cb_service_enabled (GObject *self, guint status, gpointer userdata)
{
	g_debug("cb_service_enabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);

	LocationObject *loc = (LocationObject*)userdata;

	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;

	// This function works properly after service is enabled.
	if (LOCATION_ERROR_NONE == location_get_position_ext (loc, &pos, &vel, &acc)) {
		g_debug ("SYNC>> Current position> time: %d, lat: %f, long: %f, alt: %f, status: %d, speed: %f, direction: %f",
			pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status, vel->speed, vel->direction);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
			acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_position_free(pos);
		location_accuracy_free(acc);
	} else g_warning ("SYNC>> Current position_ext > failed");
}

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	gulong handler_id = 0;

	location_init ();
	loop = g_main_loop_new (NULL, TRUE);
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	handler_id = g_signal_connect (loc, "service-enabled", G_CALLBACK(cb_service_enabled), loc);
	location_start (loc);
	g_main_loop_run (loop);

	g_signal_handler_disconnect(loc, handler_id);
	location_stop (loc);
	location_free (loc);

	return 0;
}
 * @endcode
 */
int location_get_position_ext(LocationObject *obj, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);

/**
 * @brief
 * Get last position information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @param [out] position - a new #LocationPosition
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see location_get_position
 * @par Example
 * @code
#include <location.h>

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	int ret = 0;
	LocationPosition *last_pos = NULL;
	LocationAccuracy *last_acc = NULL;

	location_init ();
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	if (LOCATION_ERROR_NONE == location_get_last_position (loc, &last_pos, &last_acc)) {
		g_debug ("SYNC>> Last position> time: %d, lat: %f, long: %f, alt: %f, status: %d",
			last_pos->timestamp, last_pos->latitude, last_pos->longitude, last_pos->altitude, last_pos->status);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
			last_acc->level, last_acc->horizontal_accuracy, last_acc->vertical_accuracy);
		location_position_free(last_pos);
		location_accuracy_free(last_acc);
	} else g_warning ("SYNC>> Last position> failed");

	location_free (loc);

	return 0;
}
 * @endcode
 */
int location_get_last_position(LocationObject *obj, LocationPosition **position, LocationAccuracy **accuracy);

/**
 * @brief
 * Get last position & velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] position - a new #LocationPosition
 * @param [out] velocity - a new #LocationVelocity
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see location_get_position_ext
 * @par Example
 * @code
#include <location.h>

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	int ret = 0;
	LocationPosition *last_pos = NULL;
	LocationVelocity *last_vel = NULL;
	LocationAccuracy *last_acc = NULL;

	location_init ();
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	if (LOCATION_ERROR_NONE == location_get_last_position_ext (loc, &last_pos, &last_vel, &last_acc)) {
		g_debug ("SYNC>> Last position> time: %d, lat: %f, long: %f, alt: %f, status: %d, speed: %f, direction: %f",
			last_pos->timestamp, last_pos->latitude, last_pos->longitude, last_pos->altitude, last_pos->status, last_vel->speed, last_vel->direction);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
			last_acc->level, last_acc->horizontal_accuracy, last_acc->vertical_accuracy);
		location_position_free(last_pos);
		location_velocity_free(last_vel);
		location_accuracy_free(last_acc);
	} else g_warning ("SYNC>> Last position_ext > failed");

	location_free (loc);

	return 0;
}
 * @endcode
 */
int location_get_last_position_ext(LocationObject *obj, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
/**
 * @brief
 * Get last satellite information.
 * @remarks This API is not implemented now. \n
 *	Out parameters are should be freed.
 * @pre
 * #location_init should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @param [out] satellite - a new #LocationSatellite
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 * @see location_get_last_satellite
 * @par Example
 * @code
#include <location.h>

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	int ret = 0, idx = 0;
	LocationSatellite *sat = NULL;
	guint prn;
	gboolean used;
	guint elevation;
	guint azimuth;
	gint snr;

	location_init ();
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	if (LOCATION_ERROR_NONE == location_get_satellite (loc, &sat)) {
		g_debug ("SYNC>> Current Sattelite> satellite in view = %d, satellite in used = %d", sat->num_of_sat_inview, sat->num_of_sat_used);
		g_debug ("\tinview satellite information = ");
		for (idx=0; idx<sat->num_of_sat_inview; idx++) {
			location_satellite_get_satellite_details(sat, idx, &prn, &used, &elevation, &azimuth, &snr);
			g_debug ("\t\t[%02d] used: %d, prn: %d, elevation: %d, azimuth: %d, snr: %d", idx, used, prn, elevation, azimuth, snr);
		}
		location_satellite_free (sat);
	} else g_warning ("SYNC>> Current satellite> failed");

	location_free (loc);

	return 0;
}
 * @endcode
 */
int location_get_satellite(LocationObject *obj, LocationSatellite **satellite);


int location_start_batch(LocationObject *obj);

int location_stop_batch(LocationObject *obj);

int location_get_batch(LocationObject *obj, LocationBatch **batch);


/**
 * @brief
 * Get last satellite information.
 * @remarks This API is not implemented now. \n
 *	Out parameters are should be freed.
 * @pre
 * #location_init should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @param [out]
 * satellite - a new #LocationSatellite
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @par Example
 * @code
#include <location.h>

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	int ret = 0, idx = 0;
	LocationSatellite *last_sat = NULL;
	guint prn;
	gboolean used;
	guint elevation;
	guint azimuth;
	gint snr;

	location_init ();
	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	if (LOCATION_ERROR_NONE == location_get_last_satellite (loc, &last_sat)) {
		g_debug ("SYNC>> Last Sattelite> satellite in view = %d, satellite in used = %d", last_sat->num_of_sat_inview, last_sat->num_of_sat_used);
		g_debug ("\tinview satellite information = ");
		for (idx=0; idx<last_sat->num_of_sat_inview; idx++) {
			location_satellite_get_satellite_details(last_sat, idx, &prn, &used, &elevation, &azimuth, &snr);
			g_debug ("\t\t[%02d] used: %d, prn: %d, elevation: %d, azimuth: %d, snr: %d", idx, used, prn, elevation, azimuth, snr);
		}
		location_satellite_free (last_sat);
	} else g_warning ("SYNC>> Last satellite> failed");

	location_free (loc);

	return 0;
}
 * @endcode
 */
int location_get_last_satellite(LocationObject *obj, LocationSatellite **satellite);

/**
 * @brief
 * Get current velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre
 * #location_init should be called before.\n
 * #location_start should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @param [out]
 * velocity - a new #LocationVelocity
 * @param [out]
 * accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see location_get_position
 * @par Example
 * @code
#include <location.h>
static GMainLoop *loop = NULL;

static void cb_service_enabled (GObject *self, guint status, gpointer userdata)
{
	g_debug("cb_service_enabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);

	LocationObject *loc = (LocationObject*)userdata;
	LocationAccuracy *acc = NULL;
	LocationVelocity *vel = NULL;

	if (LOCATION_ERROR_NONE == location_get_velocity (loc, &vel, &acc)) {
		g_debug ("SYNC>> Current velocity> time: %d, speed: %f, direction:%f, climb:%f",
			vel->timestamp, vel->speed, vel->direction, vel->climb);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
			acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_velocity_free(vel);
		location_accuracy_free(acc);
	} else g_warning ("SYNC>> Current velocity> failed");
}

int main (int argc, char *argv[])
{
	LocationObject *loc = NULL;
	gulong hander_id = 0;
	location_init ();

	loop = g_main_loop_new (NULL, TRUE);

	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	handler_id = g_signal_connect (loc, "service-enabled", G_CALLBACK(cb_service_enabled), loc);
	location_start (loc);
	g_main_loop_run (loop);

	g_signal_handler_disconnect(loc, handler_id);
	location_stop (loc);
	location_free (loc);

	return 0;
}

 * @endcode
 */
int location_get_velocity(LocationObject *obj, LocationVelocity **velocity, LocationAccuracy **accuracy);

/**
 * @brief
 * Get last velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre
 * #location_init should be called before.
 * @post None.
 * @param [in]
 * obj - a #LocationObject created by #location_new
 * @param [out]
 * velocity - a new #LocationVelocity
 * @param [out]
 * accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 * @see location_get_position
 * @par Example
 * @code
#include <location.h>

int main (int argc, char *argv[])
{
	int ret = 0;
	LocationObject *loc = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;
	gulong hander_id = 0;

	location_init ();

	loc = location_new (LOCATION_METHOD_GPS);
	if(!loc){
		g_debug("location_new failed");
		return -1;
	}

	ret = location_get_last_velocity (loc, &vel, &acc);
	if (ret == LOCATION_ERROR_NONE) {
		g_debug ("SYNC>> Last velocity> time: %d, speed: %f, direction:%f, climb:%f",
			vel->timestamp, vel->speed, vel->direction, vel->climb);
		g_debug ("\tAccuracy level %d (%.0f meters %.0f meters)",
			acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_velocity_free(vel);
		location_accuracy_free(acc);
	}

	location_free (loc);
	return 0;
}
 * @endcode
 */
int location_get_last_velocity(LocationObject *obj, LocationVelocity **velocity, LocationAccuracy **accuracy);

/**
 * @brief
 * Get the accessibility state of an application
 * @remarks None.
 * @pre
 * #location_init should be called before.\n
 * @post None.
 * @param [out] state - a #LocationAccessState
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 */
int location_get_accessibility_state(LocationAccessState *state);

/**
 * @brief
 * Send command to the server.
 * @pre
 * #location_init should be called before.\n
 * Calling application must have glib or ecore main loop.\n
 * Calling application must have an active data connection.
 * @post None.
 * @param [in] cmd - a #char
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 */
int location_send_command(const char *cmd);

/**
 * @brief
 * Set option of server.
 * @pre
 * #location_init should be called before.\n
 * Calling application must have glib or ecore main loop.\n
 * Calling application must have an active data connection.
 * @post None.
 * @param [in]	obj - a #LocationObject created by #location_new
 * @param [in]	option - a #char
 * @return int
 * @retval 0							Success
 *
 * Please refer #LocationError for more information.
 */
int location_set_option(LocationObject *obj, const char *option);

int location_add_setting_notify(LocationMethod method, LocationSettingCb callback, void *user_data);

int location_ignore_setting_notify(LocationMethod method, LocationSettingCb callback);

int location_get_nmea(LocationObject *obj, char **nmea_data);


/**
 * @} @}
 */

G_END_DECLS

#endif /* __LOCATION_H__ */
