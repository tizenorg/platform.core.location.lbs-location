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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "location-setting.h"
#include "location-log.h"

#include "module-internal.h"

#include "location-hybrid.h"
#include "location-marshal.h"
#include "location-ielement.h"
#include "location-signaling-util.h"
#include "location-common-util.h"

#include "location-gps.h"
#include "location-wps.h"

typedef struct _LocationHybridPrivate {
	gboolean		gps_enabled;
	gboolean		wps_enabled;
	gint			signal_type;
	guint			pos_updated_timestamp;
	guint			pos_interval;
	guint			vel_updated_timestamp;
	guint			vel_interval;
	guint			sat_updated_timestamp;
	guint			sat_interval;
	guint			dist_updated_timestamp;
	guint			min_interval;
	gdouble			min_distance;
	guint			loc_updated_timestamp;
	guint			loc_interval;
	LocationObject	*gps;
	LocationObject	*wps;
	gboolean		enabled;
	LocationMethod	current_method;
	LocationPosition *pos;
	LocationVelocity *vel;
	LocationAccuracy *acc;
	LocationSatellite *sat;
	GList			*boundary_list;
	gboolean		set_noti;
	guint			pos_timer;
	guint			vel_timer;
} LocationHybridPrivate;

enum {
    PROP_0,
    PROP_METHOD_TYPE,
    PROP_LAST_POSITION,
    PROP_POS_INTERVAL,
    PROP_VEL_INTERVAL,
    PROP_SAT_INTERVAL,
    PROP_LOC_INTERVAL,
    PROP_BOUNDARY,
    PROP_REMOVAL_BOUNDARY,
    PROP_MIN_INTERVAL,
    PROP_MIN_DISTANCE,
    PROP_MAX
};

static guint32 signals[LAST_SIGNAL] = {0, };
static GParamSpec *properties[PROP_MAX] = {NULL, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOCATION_TYPE_HYBRID, LocationHybridPrivate))

static void location_ielement_interface_init(LocationIElementInterface *iface);

G_DEFINE_TYPE_WITH_CODE(LocationHybrid, location_hybrid, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(LOCATION_TYPE_IELEMENT,
                                              location_ielement_interface_init));

static LocationMethod
hybrid_get_current_method(LocationHybridPrivate *priv)
{
	g_return_val_if_fail(priv, LOCATION_METHOD_NONE);
	LOCATION_LOGW("Current Method [%d]\n", priv->current_method);
	return priv->current_method;
}

static gboolean
hybrid_set_current_method(LocationHybridPrivate *priv, GType g_type)
{
	g_return_val_if_fail(priv, FALSE);

	if (g_type == LOCATION_TYPE_GPS) {
		priv->current_method = LOCATION_METHOD_GPS;
	} else if (g_type == LOCATION_TYPE_WPS) {
		priv->current_method = LOCATION_METHOD_WPS;
	} else if (g_type == LOCATION_TYPE_HYBRID) {
		priv->current_method = LOCATION_METHOD_HYBRID;
	} else
		return FALSE;

	return TRUE;
}


static int
hybrid_get_update_method(LocationHybridPrivate *priv)
{
	if (!priv->gps && !priv->wps) return -1;

	if (priv->gps_enabled) {
		hybrid_set_current_method(priv, LOCATION_TYPE_GPS);
	} else if (priv->wps_enabled) {
		hybrid_set_current_method(priv, LOCATION_TYPE_WPS);
	} else {
		hybrid_set_current_method(priv, LOCATION_TYPE_HYBRID);
	}

	return 0;
}

#if 0
static LocationObject *
hybrid_get_current_object(LocationHybridPrivate *priv)
{
	LocationMethod method = hybrid_get_current_method(priv);

	LocationObject *obj = NULL;
	switch (method) {
		case LOCATION_METHOD_GPS:
			obj = priv->gps;
			break;
		case LOCATION_METHOD_WPS:
			obj = priv->wps;
			break;
		default:
			break;
	}

	return obj;
}
#endif

static gboolean	/* True : Receive more accurate info. False : Receive less accurate info */
hybrid_compare_g_type_method(LocationHybridPrivate *priv, GType g_type)
{
	if (g_type == LOCATION_TYPE_GPS) {
		hybrid_set_current_method(priv, LOCATION_TYPE_GPS);
		return TRUE;
	} else if (g_type == LOCATION_TYPE_WPS && hybrid_get_current_method(priv) == LOCATION_METHOD_WPS) {
		hybrid_set_current_method(priv, LOCATION_TYPE_WPS);
		return TRUE;
	}

	return FALSE;
}

static gboolean
_location_timeout_cb(gpointer data)
{
	GObject *object = (GObject *)data;
	if (!object) return FALSE;
	LocationHybridPrivate *priv = GET_PRIVATE(object);
	g_return_val_if_fail(priv, FALSE);

	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;

	if (priv->pos) {
		pos = location_position_copy(priv->pos);
	} else {
		pos = location_position_new(0, 0.0, 0.0, 0.0, LOCATION_STATUS_NO_FIX);
	}

	if (priv->vel) {
		vel = location_velocity_copy(priv->vel);
	} else {
		vel = location_velocity_new(0, 0.0, 0.0, 0.0);
	}

	if (priv->acc) {
		acc = location_accuracy_copy(priv->acc);
	} else {
		acc = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);
	}

	g_signal_emit(object, signals[SERVICE_UPDATED], 0, priv->signal_type, pos, vel, acc);
	priv->signal_type = 0;

	location_position_free(pos);
	location_velocity_free(vel);
	location_accuracy_free(acc);

	return TRUE;
}

static gboolean
_position_timeout_cb(gpointer data)
{
	GObject *object = (GObject *)data;
	if (!object) return FALSE;
	LocationHybridPrivate *priv = GET_PRIVATE(object);
	g_return_val_if_fail(priv, FALSE);

	if (priv->pos_interval == priv->vel_interval) {
		priv->signal_type |= POSITION_UPDATED;
		priv->signal_type |= VELOCITY_UPDATED;
	} else {
		priv->signal_type |= POSITION_UPDATED;
	}
	_location_timeout_cb(object);

	return TRUE;
}

static gboolean
_velocity_timeout_cb(gpointer data)
{
	GObject *object = (GObject *)data;
	LocationHybridPrivate *priv = GET_PRIVATE(object);
	g_return_val_if_fail(priv, FALSE);

	if (priv->pos_interval != priv->vel_interval) {
		priv->signal_type |= VELOCITY_UPDATED;
		_location_timeout_cb(object);
	}

	return TRUE;
}

static void
location_hybrid_gps_cb(keynode_t *key,
                       gpointer self)
{
	LOCATION_LOGD("location_hybrid_gps_cb");
	g_return_if_fail(key);
	g_return_if_fail(self);
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);
	g_return_if_fail(priv->wps);

	gboolean wps_started = FALSE;
	int ret = LOCATION_ERROR_NONE;
	int onoff = 0;

	onoff = location_setting_get_key_val(key);
	if (0 == onoff) {
		/* restart WPS when GPS stopped by setting */
		g_object_get(priv->wps, "is_started", &wps_started, NULL);
		if (wps_started == FALSE && 1 == location_setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED)) {
			LOCATION_LOGD("GPS stoped by setting, so restart WPS");
			ret = location_start(priv->wps);
			if (ret != LOCATION_ERROR_NONE) {
				LOCATION_LOGW("Fail hyhrid/wps location_start : [%d]", ret);
				return;
			}
		}
	} else if (1 == onoff) {
		LOCATION_LOGD("Hybrid GPS resumed by setting");

	} else {
		LOCATION_LOGD("Invalid Value[%d]", onoff);
	}

}

static void
hybrid_location_updated(GObject *obj,
                        guint error,
                        gpointer position,
                        gpointer velocity,
                        gpointer accuracy,
                        gpointer self)
{
	LocationPosition *pos = (LocationPosition *)position;
	LocationVelocity *vel = (LocationVelocity *)velocity;
	LocationAccuracy *acc = (LocationAccuracy *)accuracy;

	LocationHybridPrivate *priv = GET_PRIVATE((LocationHybrid *)self);
	g_return_if_fail(priv);

	g_signal_emit(self, signals[LOCATION_UPDATED], 0, error, pos, vel, acc);
}

static void
hybrid_service_updated(GObject *obj,
                       gint type,
                       gpointer data,
                       gpointer velocity,
                       gpointer accuracy,
                       gpointer self)
{
	LOCATION_LOGD("hybrid_service_updated");
	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;
	LocationSatellite *sat = NULL;
	gboolean wps_started = FALSE;
	int ret = LOCATION_ERROR_NONE;

	/* To discard invalid data in a hybrid */
	if (type == SATELLITE_UPDATED) {
		sat = (LocationSatellite *)data;
		if (!sat->timestamp) return;
	} else {
		pos = (LocationPosition *)data;
		vel = (LocationVelocity *)velocity;
		acc = (LocationAccuracy *)accuracy;
		if (!pos->timestamp) return;
		if (!vel->timestamp) return;
	}

	LocationHybridPrivate *priv = GET_PRIVATE((LocationHybrid *)self);
	g_return_if_fail(priv);

	GType g_type = G_TYPE_FROM_INSTANCE(obj);
	if (g_type == LOCATION_TYPE_GPS) {
		if (type == SATELLITE_UPDATED) {
			satellite_signaling(self, signals, &(priv->enabled), priv->sat_interval, TRUE, &(priv->sat_updated_timestamp), &(priv->sat), sat);
			return;
		} else if (location_setting_get_int(VCONFKEY_LOCATION_GPS_STATE) == VCONFKEY_LOCATION_GPS_SEARCHING) {
			LOCATION_LOGD("Searching GPS");

			/* restart WPS when GPS not available */
			if (priv->wps) g_object_get(priv->wps, "is_started", &wps_started, NULL);
			if (priv->wps && wps_started == FALSE) {
				LOCATION_LOGD("Starting WPS");
				ret = location_start(priv->wps);
				if (ret != LOCATION_ERROR_NONE) {
					LOCATION_LOGW("Fail hyhrid location_start : [%d]", ret);
					return;
				}
			}
			return;
		}

	} else if (g_type == LOCATION_TYPE_WPS && location_setting_get_int(VCONFKEY_LOCATION_WPS_STATE) == VCONFKEY_LOCATION_WPS_SEARCHING) {
		LOCATION_LOGD("Searching WPS");
		return;
	} else if (g_type == LOCATION_TYPE_WPS) {
		LOCATION_LOGD("g_type is LOCATION_TYPE_WPS");
	}

	if (hybrid_compare_g_type_method(priv, g_type)) {

		if (priv->pos) location_position_free(priv->pos);
		if (priv->vel) location_velocity_free(priv->vel);
		if (priv->acc) location_accuracy_free(priv->acc);

		if (pos) priv->pos = location_position_copy(pos);
		if (vel) priv->vel = location_velocity_copy(vel);
		if (acc) priv->acc = location_accuracy_copy(acc);

		if (!priv->enabled && pos) {
			enable_signaling(self, signals, &(priv->enabled), TRUE, pos->status);
		}

		if (type == DISTANCE_UPDATED) {
			distance_based_position_signaling(self, signals, priv->enabled, pos, vel, acc,
			                                  priv->min_interval, priv->min_distance, &(priv->enabled),
			                                  &(priv->dist_updated_timestamp), &(priv->pos), &(priv->vel), &(priv->acc));
		} else {
			position_velocity_signaling(self, signals, priv->pos_interval, priv->vel_interval, priv->loc_interval,
			                            &(priv->pos_updated_timestamp), &(priv->vel_updated_timestamp), &(priv->loc_updated_timestamp),
			                            priv->boundary_list, pos, vel, acc);
		}

		/* if receive GPS position then stop WPS.. */
		if (priv->wps) g_object_get(priv->wps, "is_started", &wps_started, NULL);
		if (LOCATION_TYPE_GPS == g_type && wps_started == TRUE) {
			ret = location_stop(priv->wps);
			if (ret != LOCATION_ERROR_NONE) {
				LOCATION_LOGW("Fail hybrid location_stop : [%d]", ret);
				return;
			}
		}
	} else if (g_type == LOCATION_TYPE_WPS && location_setting_get_int(VCONFKEY_LOCATION_GPS_STATE) == VCONFKEY_LOCATION_GPS_SEARCHING) {
		hybrid_set_current_method(priv, g_type);

		if (priv->pos) location_position_free(priv->pos);
		if (priv->vel) location_velocity_free(priv->vel);
		if (priv->acc) location_accuracy_free(priv->acc);

		if (pos) priv->pos = location_position_copy(pos);
		if (vel) priv->vel = location_velocity_copy(vel);
		if (acc) priv->acc = location_accuracy_copy(acc);

		if (!priv->enabled && pos) {
			enable_signaling(self, signals, &(priv->enabled), TRUE, pos->status);
		}

		if (type == DISTANCE_UPDATED) {
			distance_based_position_signaling(self, signals, priv->enabled, pos, vel, acc,
											  priv->min_interval, priv->min_distance, &(priv->enabled),
											  &(priv->dist_updated_timestamp), &(priv->pos), &(priv->vel), &(priv->acc));
		} else {
			LOCATION_LOGD("position_velocity_signaling");
			position_velocity_signaling(self, signals, priv->pos_interval, priv->vel_interval, priv->loc_interval,
										&(priv->pos_updated_timestamp), &(priv->vel_updated_timestamp), &(priv->loc_updated_timestamp),
										priv->boundary_list, pos, vel, acc);
		}
	}
}

static void
hybrid_service_enabled(GObject *obj,
                       guint status,
                       gpointer self)
{
	LOCATION_LOGD("hybrid_service_enabled");
	LocationHybridPrivate *priv = GET_PRIVATE((LocationHybrid *)self);
	g_return_if_fail(priv);
	GType g_type = G_TYPE_FROM_INSTANCE(obj);
	if (g_type == LOCATION_TYPE_GPS) {
		priv->gps_enabled = TRUE;
	} else if (g_type == LOCATION_TYPE_WPS) {
		priv->wps_enabled = TRUE;
	} else {
		LOCATION_LOGW("Undefined GType enabled");
		return;
	}
	hybrid_get_update_method(priv);
}

static void
hybrid_service_disabled(GObject *obj,
                        guint status,
                        gpointer self)
{
	LOCATION_LOGD("hybrid_service_disabled");
	LocationHybridPrivate *priv = GET_PRIVATE((LocationHybrid *)self);
	g_return_if_fail(priv);
	GType g_type = G_TYPE_FROM_INSTANCE(obj);
	if (g_type == LOCATION_TYPE_GPS) {
		priv->gps_enabled = FALSE;
	} else if (g_type == LOCATION_TYPE_WPS) {
		priv->wps_enabled = FALSE;
	} else {
		LOCATION_LOGW("Undefined GType disabled");
		return;
	}
	hybrid_get_update_method(priv);
	if (!priv->gps_enabled && !priv->wps_enabled)
		enable_signaling(self, signals, &(priv->enabled), FALSE, status);

}

static int
location_hybrid_start(LocationHybrid *self)
{
	LOCATION_LOGD("location_hybrid_start");

	int ret_gps = LOCATION_ERROR_NONE;
	int ret_wps = LOCATION_ERROR_NONE;
	gboolean gps_started = FALSE;
	gboolean wps_started = FALSE;

	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	if (!priv->gps && !priv->wps) {
		LOCATION_LOGE("GPS and WPS Object are not created.");
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	if (priv->gps) g_object_get(priv->gps, "is_started", &gps_started, NULL);
	if (priv->wps) g_object_get(priv->wps, "is_started", &wps_started, NULL);

	if ((gps_started == TRUE) || (wps_started == TRUE)) {
		LOCATION_LOGD("Already started");
		return LOCATION_ERROR_NONE;
	}

	if (priv->gps) ret_gps = location_start(priv->gps);
	if (priv->wps) ret_wps = location_start(priv->wps);

	if ((ret_gps != LOCATION_ERROR_NONE) && (ret_wps != LOCATION_ERROR_NONE)) {
		LOCATION_LOGD("ret_gps = %d, ret_wps = %d", ret_gps, ret_wps);
		if (ret_gps == LOCATION_ERROR_SECURITY_DENIED || ret_wps == LOCATION_ERROR_SECURITY_DENIED) {
			return LOCATION_ERROR_SECURITY_DENIED;
		} else if (ret_gps == LOCATION_ERROR_SETTING_OFF || ret_wps == LOCATION_ERROR_SETTING_OFF) {
			return LOCATION_ERROR_SETTING_OFF;
		} else if (ret_gps == LOCATION_ERROR_NOT_ALLOWED || ret_wps == LOCATION_ERROR_NOT_ALLOWED) {
			return LOCATION_ERROR_NOT_ALLOWED;
		} else {
			return LOCATION_ERROR_NOT_AVAILABLE;
		}
	}

	if (priv->set_noti == FALSE) {
		location_setting_add_notify(VCONFKEY_LOCATION_ENABLED, location_hybrid_gps_cb, self);
		priv->set_noti = TRUE;
	}

	return LOCATION_ERROR_NONE;
}

static int
location_hybrid_stop(LocationHybrid *self)
{
	LOCATION_LOGD("location_hybrid_stop");

	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	LOCATION_LOGD("location_hybrid_stop started......!!!");

	int ret_gps = LOCATION_ERROR_NOT_AVAILABLE;
	int ret_wps = LOCATION_ERROR_NOT_AVAILABLE;
	gboolean gps_started = FALSE;
	gboolean wps_started = FALSE;

	g_object_get(priv->gps, "is_started", &gps_started, NULL);
	if (priv->wps) g_object_get(priv->wps, "is_started", &wps_started, NULL);

	if ((gps_started == FALSE) && (wps_started == FALSE)) {
		return LOCATION_ERROR_NONE;
	}

	if (priv->gps) ret_gps = location_stop(priv->gps);
	if (priv->wps) ret_wps = location_stop(priv->wps);

	if (ret_gps != LOCATION_ERROR_NONE && ret_wps != LOCATION_ERROR_NONE)
		return LOCATION_ERROR_NOT_AVAILABLE;

	if (priv->pos_timer) g_source_remove(priv->pos_timer);
	if (priv->vel_timer) g_source_remove(priv->vel_timer);
	priv->pos_timer = 0;
	priv->vel_timer = 0;

	if (priv->set_noti == TRUE) {
		location_setting_ignore_notify(VCONFKEY_LOCATION_ENABLED, location_hybrid_gps_cb);
		priv->set_noti = FALSE;
	}

	return LOCATION_ERROR_NONE;
}

static void
location_hybrid_dispose(GObject *gobject)
{
	LOCATION_LOGD("location_hybrid_dispose");
	LocationHybridPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);

	if (priv->pos_timer) g_source_remove(priv->pos_timer);
	if (priv->vel_timer) g_source_remove(priv->vel_timer);
	priv->pos_timer = 0;
	priv->vel_timer = 0;

	if (priv->set_noti == TRUE) {
		location_setting_ignore_notify(VCONFKEY_LOCATION_ENABLED, location_hybrid_gps_cb);
		priv->set_noti = FALSE;
	}

	G_OBJECT_CLASS(location_hybrid_parent_class)->dispose(gobject);
}

static void
location_hybrid_finalize(GObject *gobject)
{
	LOCATION_LOGD("location_hybrid_finalize");
	LocationHybridPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);

	if (priv->gps) {
		g_signal_handlers_disconnect_by_func(priv->gps, G_CALLBACK(hybrid_service_enabled), gobject);
		g_signal_handlers_disconnect_by_func(priv->gps, G_CALLBACK(hybrid_service_disabled), gobject);
		g_signal_handlers_disconnect_by_func(priv->gps, G_CALLBACK(hybrid_service_updated), gobject);
		g_signal_handlers_disconnect_by_func(priv->gps, G_CALLBACK(hybrid_location_updated), gobject);
		location_free(priv->gps);
	}
	if (priv->wps) {
		g_signal_handlers_disconnect_by_func(priv->wps, G_CALLBACK(hybrid_service_enabled), gobject);
		g_signal_handlers_disconnect_by_func(priv->wps, G_CALLBACK(hybrid_service_disabled), gobject);
		g_signal_handlers_disconnect_by_func(priv->wps, G_CALLBACK(hybrid_service_updated), gobject);
		g_signal_handlers_disconnect_by_func(priv->wps, G_CALLBACK(hybrid_location_updated), gobject);
		location_free(priv->wps);
	}

	if (priv->boundary_list) {
		g_list_free_full(priv->boundary_list, free_boundary_list);
		priv->boundary_list = NULL;
	}

	if (priv->pos) {
		location_position_free(priv->pos);
		priv->pos = NULL;
	}

	if (priv->vel) {
		location_velocity_free(priv->vel);
		priv->vel = NULL;
	}

	if (priv->acc) {
		location_accuracy_free(priv->acc);
		priv->acc = NULL;
	}

	if (priv->sat) {
		location_satellite_free(priv->sat);
		priv->sat = NULL;
	}

	G_OBJECT_CLASS(location_hybrid_parent_class)->finalize(gobject);
}

static void
location_hybrid_set_property(GObject *object,
                             guint property_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
	LocationHybridPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);
	if (!priv->gps && !priv->wps) {
		LOCATION_LOGW("Set property is not available now");
		return;
	}

	int ret = 0;
	switch (property_id) {
		case PROP_BOUNDARY: {
				GList *boundary_list = (GList *)g_list_copy(g_value_get_pointer(value));
				ret = set_prop_boundary(&priv->boundary_list, boundary_list);
				if (ret != 0) LOCATION_LOGD("Set boundary. Error[%d]", ret);
				break;
			}
		case PROP_REMOVAL_BOUNDARY: {
				LocationBoundary *req_boundary = (LocationBoundary *) g_value_dup_boxed(value);
				ret = set_prop_removal_boundary(&priv->boundary_list, req_boundary);
				if (ret != 0) LOCATION_LOGD("Removal boundary. Error[%d]", ret);
				break;
			}
		case PROP_POS_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_POS_INTERVAL: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->pos_interval = interval;
					else
						priv->pos_interval = (guint) LOCATION_UPDATE_INTERVAL_MAX;
				} else {
					priv->pos_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
				}

				if (priv->pos_timer) {
					g_source_remove(priv->pos_timer);
					priv->pos_timer = g_timeout_add(priv->pos_interval * 1000, _position_timeout_cb, object);
				}

				if (priv->gps) g_object_set(priv->gps, "pos-interval", priv->pos_interval, NULL);
				if (priv->wps) g_object_set(priv->wps, "pos-interval", priv->pos_interval, NULL);

				break;
			}
		case PROP_VEL_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_VEL_INTERVAL: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->vel_interval = interval;
					else
						priv->vel_interval = (guint) LOCATION_UPDATE_INTERVAL_MAX;

				} else
					priv->vel_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;

				if (priv->vel_timer) {
					g_source_remove(priv->vel_timer);
					priv->vel_timer = g_timeout_add(priv->vel_interval * 1000, _velocity_timeout_cb, object);
				}

				if (priv->gps) g_object_set(priv->gps, "vel-interval", priv->vel_interval, NULL);
				if (priv->wps) g_object_set(priv->wps, "vel-interval", priv->vel_interval, NULL);

				break;
			}
		case PROP_SAT_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_SAT_INTERVAL: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->sat_interval = interval;
					else
						priv->sat_interval = (guint) LOCATION_UPDATE_INTERVAL_MAX;

				} else
					priv->sat_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;

				break;
			}
		case PROP_LOC_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_LOC_INTERVAL: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->loc_interval = interval;
					else
						priv->loc_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
				} else
					priv->loc_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;

				if (priv->gps) g_object_set(priv->gps, "loc-interval", priv->loc_interval, NULL);
				if (priv->wps) g_object_set(priv->wps, "loc-interval", priv->loc_interval, NULL);

				break;
			}
		case PROP_MIN_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_MIN_INTERVAL: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_MIN_INTERVAL_MAX)
						priv->min_interval = interval;
					else
						priv->min_interval = (guint)LOCATION_MIN_INTERVAL_MAX;
				} else
					priv->min_interval = (guint)LOCATION_MIN_INTERVAL_DEFAULT;

				if (priv->gps) g_object_set(priv->gps, "min-interval", priv->min_interval, NULL);
				if (priv->wps) g_object_set(priv->wps, "min-interval", priv->min_interval, NULL);

				break;
			}
		case PROP_MIN_DISTANCE: {
				gdouble distance = g_value_get_double(value);
				LOCATION_LOGD("Set prop>> PROP_MIN_DISTANCE: %u", distance);
				if (distance > 0) {
					if (distance < LOCATION_MIN_DISTANCE_MAX)
						priv->min_distance = distance;
					else
						priv->min_distance = (gdouble)LOCATION_MIN_DISTANCE_MAX;
				} else
					priv->min_distance = (gdouble)LOCATION_MIN_DISTANCE_DEFAULT;

				if (priv->gps) g_object_set(priv->gps, "min-distance", priv->min_distance, NULL);
				if (priv->wps) g_object_set(priv->wps, "min-distance", priv->min_distance, NULL);

				break;
			}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

static void
location_hybrid_get_property(GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
	LocationHybridPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);
	if (!priv->gps && !priv->wps) {
		LOCATION_LOGW("Get property is not available now");
		return;
	}

	LOCATION_LOGW("Get Propery ID[%d]", property_id);

	switch (property_id) {
		case PROP_METHOD_TYPE:
			g_value_set_int(value, hybrid_get_current_method(priv));
			break;
		case PROP_LAST_POSITION:
			g_value_set_boxed(value, priv->pos);
			break;
		case PROP_BOUNDARY:
			g_value_set_pointer(value, g_list_first(priv->boundary_list));
			break;
		case PROP_POS_INTERVAL:
			g_value_set_uint(value, priv->pos_interval);
			break;
		case PROP_VEL_INTERVAL:
			g_value_set_uint(value, priv->vel_interval);
			break;
		case PROP_SAT_INTERVAL:
			g_value_set_uint(value, priv->sat_interval);
			break;
		case PROP_LOC_INTERVAL:
			g_value_set_uint(value, priv->loc_interval);
			break;
		case PROP_MIN_INTERVAL:
			g_value_set_uint(value, priv->min_interval);
			break;
		case PROP_MIN_DISTANCE:
			g_value_set_double(value, priv->min_distance);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

static int
location_hybrid_get_position(LocationHybrid *self,
                             LocationPosition **position,
                             LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;
	LOCATION_LOGD("location_hybrid_get_position");
	if (!location_setting_get_int(VCONFKEY_LOCATION_ENABLED) && !location_setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED)) {
		return LOCATION_ERROR_SETTING_OFF;
	}

	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	if (priv->pos) {
		*position = location_position_copy(priv->pos);
		ret = LOCATION_ERROR_NONE;
	}

	if (priv->acc) {
		*accuracy = location_accuracy_copy(priv->acc);
	}

	return ret;
}

static int
location_hybrid_get_position_ext(LocationHybrid *self,
                                 LocationPosition **position,
                                 LocationVelocity **velocity,
                                 LocationAccuracy **accuracy)
{
	LOCATION_LOGD("location_hybrid_get_position_ext");
	if (!location_setting_get_int(VCONFKEY_LOCATION_ENABLED) && !location_setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED)) {
		return LOCATION_ERROR_SETTING_OFF;
	}

	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	if (priv->pos && priv->vel) {
		*position = location_position_copy(priv->pos);
		*velocity = location_velocity_copy(priv->vel);
	} else {
		LOCATION_LOGE("There is invalid data.");
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	if (priv->acc) {
		*accuracy = location_accuracy_copy(priv->acc);
	} else {
		*accuracy = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);
	}

	return LOCATION_ERROR_NONE;
}


static int
location_hybrid_get_last_position(LocationHybrid *self,
                                  LocationPosition **position,
                                  LocationAccuracy **accuracy)
{
	LOCATION_LOGD("location_hybrid_get_last_position");

	int ret = LOCATION_ERROR_NONE;
	LocationPosition *gps_pos = NULL, *wps_pos = NULL;
	LocationAccuracy *gps_acc = NULL, *wps_acc = NULL;
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	if (priv->gps) location_get_last_position(priv->gps, &gps_pos, &gps_acc);
	if (priv->wps) location_get_last_position(priv->wps, &wps_pos, &wps_acc);

	if (gps_pos && wps_pos) {
		if (wps_pos->timestamp > gps_pos->timestamp) {
			*position = wps_pos;
			*accuracy = wps_acc;
			location_position_free(gps_pos);
			location_accuracy_free(gps_acc);
		} else {
			*position = gps_pos;
			*accuracy = gps_acc;
			location_position_free(wps_pos);
			location_accuracy_free(wps_acc);
		}
	} else if (gps_pos) {
		*position = gps_pos;
		*accuracy = gps_acc;
	} else if (wps_pos) {
		*position = wps_pos;
		*accuracy = wps_acc;
	} else {
		ret = LOCATION_ERROR_NOT_AVAILABLE;
	}

	return ret;
}

static int
location_hybrid_get_last_position_ext(LocationHybrid *self,
                                      LocationPosition **position,
                                      LocationVelocity **velocity,
                                      LocationAccuracy **accuracy)
{
	LOCATION_LOGD("location_hybrid_get_last_position_ext");

	int ret = LOCATION_ERROR_NONE;
	LocationPosition *gps_pos = NULL, *wps_pos = NULL;
	LocationVelocity *gps_vel = NULL, *wps_vel = NULL;
	LocationAccuracy *gps_acc = NULL, *wps_acc = NULL;
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	if (priv->gps) location_get_last_position_ext(priv->gps, &gps_pos, &gps_vel, &gps_acc);
	if (priv->wps) location_get_last_position_ext(priv->wps, &wps_pos, &wps_vel, &wps_acc);

	if (gps_pos && wps_pos && gps_vel && wps_vel) {
		if (wps_pos->timestamp > gps_pos->timestamp) {
			*position = wps_pos;
			*velocity = wps_vel;
			*accuracy = wps_acc;
			location_position_free(gps_pos);
			location_velocity_free(gps_vel);
			location_accuracy_free(gps_acc);
		} else {
			*position = gps_pos;
			*velocity = gps_vel;
			*accuracy = gps_acc;
			location_position_free(wps_pos);
			location_velocity_free(wps_vel);
			location_accuracy_free(wps_acc);
		}
	} else if (gps_pos && gps_vel) {
		*position = gps_pos;
		*velocity = gps_vel;
		*accuracy = gps_acc;
	} else if (wps_pos && wps_vel) {
		*position = wps_pos;
		*velocity = wps_vel;
		*accuracy = wps_acc;
	} else {
		ret = LOCATION_ERROR_NOT_AVAILABLE;
	}

	return ret;
}

static int
location_hybrid_get_velocity(LocationHybrid *self,
                             LocationVelocity **velocity,
                             LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;
	LOCATION_LOGD("location_hybrid_get_velocity");
	if (!location_setting_get_int(VCONFKEY_LOCATION_ENABLED) && !location_setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED)) {
		return LOCATION_ERROR_SETTING_OFF;
	}

	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	if (priv->vel) {
		*velocity = location_velocity_copy(priv->vel);
		ret = LOCATION_ERROR_NONE;
	}

	if (priv->acc) {
		*accuracy = location_accuracy_copy(priv->acc);
	}

	return ret;
}

static int
location_hybrid_get_last_velocity(LocationHybrid *self,
                                  LocationVelocity **velocity,
                                  LocationAccuracy **accuracy)
{
	LOCATION_LOGD("location_hybrid_get_last_velocity");

	int ret = LOCATION_ERROR_NONE;
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	LocationVelocity *gps_vel = NULL, *wps_vel = NULL;
	LocationAccuracy *gps_acc = NULL, *wps_acc = NULL;

	if (priv->gps) location_get_last_velocity(priv->gps, &gps_vel, &gps_acc);
	if (priv->wps) location_get_last_velocity(priv->wps, &wps_vel, &wps_acc);

	if (gps_vel && wps_vel) {
		if (wps_vel->timestamp > gps_vel->timestamp) {
			*velocity = wps_vel;
			*accuracy = wps_acc;
			location_velocity_free(gps_vel);
			location_accuracy_free(gps_acc);
		} else {
			*velocity = gps_vel;
			*accuracy = gps_acc;
			location_velocity_free(wps_vel);
			location_accuracy_free(wps_acc);
		}
	} else if (gps_vel) {
		*velocity = gps_vel;
		*accuracy = gps_acc;
	} else if (wps_vel) {
		*velocity = wps_vel;
		*accuracy = wps_acc;
	} else {
		*velocity = NULL;
		*accuracy = NULL;
		ret = LOCATION_ERROR_NOT_AVAILABLE;
	}

	return ret;
}

static int
location_hybrid_get_satellite(LocationHybrid *self,
                              LocationSatellite **satellite)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;
	LOCATION_LOGD("location_hybrid_get_satellite");
	if (!location_setting_get_int(VCONFKEY_LOCATION_ENABLED) && !location_setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED)) {
		return LOCATION_ERROR_SETTING_OFF;
	}

	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	if (priv->sat) {
		*satellite = location_satellite_copy(priv->sat);
		ret = LOCATION_ERROR_NONE;
	}

	return ret;
}

static int
location_hybrid_get_last_satellite(LocationHybrid *self,
                                   LocationSatellite **satellite)
{
	LOCATION_LOGD("location_hybrid_get_last_satellite");

	int ret = LOCATION_ERROR_NONE;
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	if (priv->gps) {
		ret = location_get_last_satellite(priv->gps, satellite);
	} else {
		*satellite = NULL;
		ret = LOCATION_ERROR_NOT_AVAILABLE;
	}

	return ret;
}

static int
location_hybrid_set_option(LocationHybrid *self, const char *option)
{
	LOCATION_LOGD("location_hybrid_set_option");
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	int ret_gps = LOCATION_ERROR_NOT_AVAILABLE;
	int ret_wps = LOCATION_ERROR_NOT_AVAILABLE;

	if (priv->gps) ret_gps = location_set_option(priv->gps, option);
	if (priv->wps) ret_wps = location_set_option(priv->wps, option);

	if (ret_gps != LOCATION_ERROR_NONE && ret_wps != LOCATION_ERROR_NONE)
		return LOCATION_ERROR_NOT_AVAILABLE;

	return LOCATION_ERROR_NONE;
}

static int
location_hybrid_request_single_location(LocationHybrid *self, int timeout)
{
	LOCATION_LOGD("location_hybrid_request_single_location");
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NONE;

	if (priv->gps) {
		ret = location_request_single_location(priv->gps, timeout);
	} else {
		ret = location_request_single_location(priv->wps, timeout);
	}

	return ret;
}

static int
location_hybrid_get_nmea(LocationHybrid *self,
                         char **nmea_data)
{
	LOCATION_LOGD("location_hybrid_get_nmea");
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	if (priv->gps) ret = location_get_nmea(priv->gps, nmea_data);

	if (ret != LOCATION_ERROR_NONE)
		return LOCATION_ERROR_NOT_AVAILABLE;

	return LOCATION_ERROR_NONE;
}


static void
location_ielement_interface_init(LocationIElementInterface *iface)
{
	iface->start = (TYPE_START_FUNC)location_hybrid_start;
	iface->stop = (TYPE_STOP_FUNC)location_hybrid_stop;
	iface->get_position = (TYPE_GET_POSITION)location_hybrid_get_position;
	iface->get_position_ext = (TYPE_GET_POSITION_EXT)location_hybrid_get_position_ext;
	iface->get_last_position = (TYPE_GET_POSITION)location_hybrid_get_last_position;
	iface->get_last_position_ext = (TYPE_GET_POSITION_EXT)location_hybrid_get_last_position_ext;
	iface->get_velocity = (TYPE_GET_VELOCITY)location_hybrid_get_velocity;
	iface->get_last_velocity = (TYPE_GET_VELOCITY)location_hybrid_get_last_velocity;
	iface->get_satellite = (TYPE_GET_SATELLITE)location_hybrid_get_satellite;
	iface->get_last_satellite = (TYPE_GET_SATELLITE)location_hybrid_get_last_satellite;
	iface->set_option = (TYPE_SET_OPTION)location_hybrid_set_option;
	iface->request_single_location = (TYPE_REQUEST_SINGLE_LOCATION)location_hybrid_request_single_location;
	iface->get_nmea = (TYPE_GET_NMEA)location_hybrid_get_nmea;
}

static void
location_hybrid_init(LocationHybrid *self)
{
	LOCATION_LOGD("location_hybrid_init");
	LocationHybridPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	priv->pos_interval = LOCATION_UPDATE_INTERVAL_NONE;
	priv->vel_interval = LOCATION_UPDATE_INTERVAL_NONE;
	priv->sat_interval = LOCATION_UPDATE_INTERVAL_NONE;
	priv->loc_interval = LOCATION_UPDATE_INTERVAL_NONE;
	priv->min_interval = LOCATION_UPDATE_INTERVAL_NONE;

	priv->pos_updated_timestamp = 0;
	priv->vel_updated_timestamp = 0;
	priv->sat_updated_timestamp = 0;
	priv->loc_updated_timestamp = 0;

	priv->gps_enabled = FALSE;
	priv->wps_enabled = FALSE;
	priv->gps = NULL;
	priv->wps = NULL;

	priv->set_noti = FALSE;
	priv->signal_type = 0;
	priv->pos_timer = 0;
	priv->vel_timer = 0;

	if (location_is_supported_method(LOCATION_METHOD_GPS)) priv->gps = location_new(LOCATION_METHOD_GPS);
	if (location_is_supported_method(LOCATION_METHOD_WPS)) priv->wps = location_new(LOCATION_METHOD_WPS);

	if (priv->gps) {
		g_signal_connect(priv->gps, "service-enabled", G_CALLBACK(hybrid_service_enabled), self);
		g_signal_connect(priv->gps, "service-disabled", G_CALLBACK(hybrid_service_disabled), self);
		g_signal_connect(priv->gps, "service-updated", G_CALLBACK(hybrid_service_updated), self);
		g_signal_connect(priv->gps, "location-updated", G_CALLBACK(hybrid_location_updated), self);
	}
	if (priv->wps) {
		g_signal_connect(priv->wps, "service-enabled", G_CALLBACK(hybrid_service_enabled), self);
		g_signal_connect(priv->wps, "service-disabled", G_CALLBACK(hybrid_service_disabled), self);
		g_signal_connect(priv->wps, "service-updated", G_CALLBACK(hybrid_service_updated), self);
		g_signal_connect(priv->wps, "location-updated", G_CALLBACK(hybrid_location_updated), self);
	}

	hybrid_set_current_method(priv, LOCATION_TYPE_HYBRID);
	priv->enabled = FALSE;

	priv->pos = NULL;
	priv->vel = NULL;
	priv->acc = NULL;
	priv->sat = NULL;

	priv->boundary_list = NULL;
}

static void
location_hybrid_class_init(LocationHybridClass *klass)
{
	LOCATION_LOGD("location_hybrid_class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->set_property = location_hybrid_set_property;
	gobject_class->get_property = location_hybrid_get_property;

	gobject_class->dispose = location_hybrid_dispose;
	gobject_class->finalize = location_hybrid_finalize;

	g_type_class_add_private(klass, sizeof(LocationHybridPrivate));

	signals[SERVICE_ENABLED] = g_signal_new("service-enabled",
	                                        G_TYPE_FROM_CLASS(klass),
	                                        G_SIGNAL_RUN_FIRST |
	                                        G_SIGNAL_NO_RECURSE,
	                                        G_STRUCT_OFFSET(LocationHybridClass, enabled),
	                                        NULL, NULL,
	                                        location_VOID__UINT,
	                                        G_TYPE_NONE, 1,
	                                        G_TYPE_UINT);

	signals[SERVICE_DISABLED] = g_signal_new("service-disabled",
	                                         G_TYPE_FROM_CLASS(klass),
	                                         G_SIGNAL_RUN_FIRST |
	                                         G_SIGNAL_NO_RECURSE,
	                                         G_STRUCT_OFFSET(LocationHybridClass, disabled),
	                                         NULL, NULL,
	                                         location_VOID__UINT,
	                                         G_TYPE_NONE, 1,
	                                         G_TYPE_UINT);

	signals[SERVICE_UPDATED] = g_signal_new("service-updated",
	                                        G_TYPE_FROM_CLASS(klass),
	                                        G_SIGNAL_RUN_FIRST |
	                                        G_SIGNAL_NO_RECURSE,
	                                        G_STRUCT_OFFSET(LocationHybridClass, updated),
	                                        NULL, NULL,
	                                        location_VOID__INT_POINTER_POINTER_POINTER,
	                                        G_TYPE_NONE, 4,
	                                        G_TYPE_INT,
	                                        G_TYPE_POINTER,
	                                        G_TYPE_POINTER,
	                                        G_TYPE_POINTER);

	signals[LOCATION_UPDATED] = g_signal_new("location-updated",
	                                         G_TYPE_FROM_CLASS(klass),
	                                         G_SIGNAL_RUN_FIRST |
	                                         G_SIGNAL_NO_RECURSE,
	                                         G_STRUCT_OFFSET(LocationHybridClass, location_updated),
	                                         NULL, NULL,
	                                         location_VOID__INT_POINTER_POINTER_POINTER,
	                                         G_TYPE_NONE, 4,
	                                         G_TYPE_INT,
	                                         G_TYPE_POINTER,
	                                         G_TYPE_POINTER,
	                                         G_TYPE_POINTER);

	signals[ZONE_IN] = g_signal_new("zone-in",
	                                G_TYPE_FROM_CLASS(klass),
	                                G_SIGNAL_RUN_FIRST |
	                                G_SIGNAL_NO_RECURSE,
	                                G_STRUCT_OFFSET(LocationHybridClass, zone_in),
	                                NULL, NULL,
	                                location_VOID__POINTER_POINTER_POINTER,
	                                G_TYPE_NONE, 3,
	                                G_TYPE_POINTER,
	                                G_TYPE_POINTER,
	                                G_TYPE_POINTER);

	signals[ZONE_OUT] = g_signal_new("zone-out",
	                                 G_TYPE_FROM_CLASS(klass),
	                                 G_SIGNAL_RUN_FIRST |
	                                 G_SIGNAL_NO_RECURSE,
	                                 G_STRUCT_OFFSET(LocationHybridClass, zone_out),
	                                 NULL, NULL,
	                                 location_VOID__POINTER_POINTER_POINTER,
	                                 G_TYPE_NONE, 3,
	                                 G_TYPE_POINTER,
	                                 G_TYPE_POINTER,
	                                 G_TYPE_POINTER);

	properties[PROP_METHOD_TYPE] = g_param_spec_int("method",
	                                                "method type",
	                                                "location method type name",
	                                                LOCATION_METHOD_HYBRID,
	                                                LOCATION_METHOD_HYBRID,
	                                                LOCATION_METHOD_HYBRID,
	                                                G_PARAM_READABLE);

	properties[PROP_LAST_POSITION] = g_param_spec_boxed("last-position",
	                                                    "hybrid last position prop",
	                                                    "hybrid last position data",
	                                                    LOCATION_TYPE_POSITION,
	                                                    G_PARAM_READABLE);

	properties[PROP_POS_INTERVAL] = g_param_spec_uint("pos-interval",
	                                                  "position interval prop",
	                                                  "position interval data",
	                                                  LOCATION_UPDATE_INTERVAL_MIN,
	                                                  LOCATION_UPDATE_INTERVAL_MAX,
	                                                  LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);
	properties[PROP_VEL_INTERVAL] = g_param_spec_uint("vel-interval",
	                                                  "velocity interval prop",
	                                                  "velocity interval data",
	                                                  LOCATION_UPDATE_INTERVAL_MIN,
	                                                  LOCATION_UPDATE_INTERVAL_MAX,
	                                                  LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);
	properties[PROP_SAT_INTERVAL] = g_param_spec_uint("sat-interval",
	                                                  "satellite interval prop",
	                                                  "satellite interval data",
	                                                  LOCATION_UPDATE_INTERVAL_MIN,
	                                                  LOCATION_UPDATE_INTERVAL_MAX,
	                                                  LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);

	properties[PROP_LOC_INTERVAL] = g_param_spec_uint("loc-interval",
	                                                  "gps location interval prop",
	                                                  "gps location interval data",
	                                                  LOCATION_UPDATE_INTERVAL_MIN,
	                                                  LOCATION_UPDATE_INTERVAL_MAX,
	                                                  LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);

	properties[PROP_MIN_INTERVAL] = g_param_spec_uint("min-interval",
	                                                  "gps distance-based interval prop",
	                                                  "gps distance-based interval data",
	                                                  LOCATION_MIN_INTERVAL_MIN,
	                                                  LOCATION_MIN_INTERVAL_MAX,
	                                                  LOCATION_MIN_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);

	properties[PROP_MIN_DISTANCE] = g_param_spec_double("min-distance",
	                                                    "gps distance-based distance prop",
	                                                    "gps distance-based distance data",
	                                                    LOCATION_MIN_DISTANCE_MIN,
	                                                    LOCATION_MIN_DISTANCE_MAX,
	                                                    LOCATION_MIN_DISTANCE_DEFAULT,
	                                                    G_PARAM_READWRITE);

	properties[PROP_BOUNDARY] = g_param_spec_pointer("boundary",
	                                                 "hybrid boundary prop",
	                                                 "hybrid boundary data",
	                                                 G_PARAM_READWRITE);

	properties[PROP_REMOVAL_BOUNDARY] = g_param_spec_boxed("removal-boundary",
	                                                       "hybrid removal boundary prop",
	                                                       "hybrid removal boundary data",
	                                                       LOCATION_TYPE_BOUNDARY,
	                                                       G_PARAM_READWRITE);

	g_object_class_install_properties(gobject_class,
	                                  PROP_MAX,
	                                  properties);
}
