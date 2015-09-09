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

#include "location-wps.h"
#include "location-marshal.h"
#include "location-ielement.h"
#include "location-signaling-util.h"
#include "location-common-util.h"
#include "location-privacy.h"

#include <vconf-internal-location-keys.h>

/*
 * forward definitions
 */

typedef struct _LocationWpsPrivate {
	LocationWpsMod		*mod;
	GMutex				mutex;
	gboolean			is_started;
	guint				app_type;
	gboolean			set_noti;
	gboolean			enabled;
	guint				pos_updated_timestamp;
	guint				pos_interval;
	guint				vel_updated_timestamp;
	guint				vel_interval;
	guint				loc_updated_timestamp;
	guint				loc_interval;
	guint				loc_timeout;
	guint				dist_updated_timestamp;
	guint				min_interval;
	gdouble				min_distance;
	LocationPosition	*pos;
	LocationVelocity	*vel;
	LocationAccuracy	*acc;
	GList				*boundary_list;
} LocationWpsPrivate;

enum {
    PROP_0,
    PROP_METHOD_TYPE,
    PROP_IS_STARTED,
    PROP_LAST_POSITION,
    PROP_POS_INTERVAL,
    PROP_VEL_INTERVAL,
    PROP_LOC_INTERVAL,
    PROP_BOUNDARY,
    PROP_REMOVAL_BOUNDARY,
    PROP_MIN_INTERVAL,
    PROP_MIN_DISTANCE,
    PROP_MAX
};

static guint32 signals[LAST_SIGNAL] = {0, };
static GParamSpec *properties[PROP_MAX] = {NULL, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOCATION_TYPE_WPS, LocationWpsPrivate))

static void location_ielement_interface_init(LocationIElementInterface *iface);

G_DEFINE_TYPE_WITH_CODE(LocationWps, location_wps, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(LOCATION_TYPE_IELEMENT,
                                              location_ielement_interface_init));

static void
__reset_pos_data_from_priv(LocationWpsPrivate *priv)
{
	LOCATION_LOGD("__reset_pos_data_from_priv");
	g_return_if_fail(priv);

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
	priv->pos_updated_timestamp = 0;
	priv->vel_updated_timestamp = 0;
}

static gboolean
__get_started(gpointer self)
{
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, FALSE);

	return priv->is_started;
}

static int
__set_started(gpointer self, gboolean started)
{
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, -1);

	if (priv->is_started != started) {
		g_mutex_lock(&priv->mutex);
		priv->is_started = started;
		g_mutex_unlock(&priv->mutex);
	}

	return 0;
}

static void
wps_status_cb(gboolean enabled,
              LocationStatus status,
              gpointer self)
{
	LOCATION_LOGD("wps_status_cb");
	g_return_if_fail(self);
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);
	if (priv->enabled == TRUE && enabled == FALSE) {
		__set_started(self, FALSE);
		enable_signaling(self, signals, &(priv->enabled), enabled, status);
	}
}

static void
wps_location_cb(gboolean enabled,
                LocationPosition *pos,
                LocationVelocity *vel,
                LocationAccuracy *acc,
                gpointer self)
{
	g_return_if_fail(self);
	g_return_if_fail(pos);
	g_return_if_fail(vel);
	g_return_if_fail(acc);
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	if (priv->min_interval != LOCATION_UPDATE_INTERVAL_NONE) {
		distance_based_position_signaling(self,
		                                  signals,
		                                  enabled,
		                                  pos,
		                                  vel,
		                                  acc,
		                                  priv->min_interval,
		                                  priv->min_distance,
		                                  &(priv->enabled),
		                                  &(priv->dist_updated_timestamp),
		                                  &(priv->pos),
		                                  &(priv->vel),
		                                  &(priv->acc));
	}
	location_signaling(self,
	                   signals,
	                   enabled,
	                   priv->boundary_list,
	                   pos,
	                   vel,
	                   acc,
	                   priv->pos_interval,
	                   priv->vel_interval,
	                   priv->loc_interval,
	                   &(priv->enabled),
	                   &(priv->pos_updated_timestamp),
	                   &(priv->vel_updated_timestamp),
	                   &(priv->loc_updated_timestamp),
	                   &(priv->pos),
	                   &(priv->vel),
	                   &(priv->acc));
}

static void
location_setting_wps_cb(keynode_t *key,
                        gpointer self)
{
	LOCATION_LOGD("location_setting_wps_cb");
	g_return_if_fail(key);
	g_return_if_fail(self);
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);
	g_return_if_fail(priv->mod);
	g_return_if_fail(priv->mod->handler);

	int ret = LOCATION_ERROR_NONE;

	if (location_setting_get_key_val(key) == 0) {
		if (priv->mod->ops.stop && __get_started(self)) {
			__set_started(self, FALSE);
			ret = priv->mod->ops.stop(priv->mod->handler);
			if (ret == LOCATION_ERROR_NONE) {
				__reset_pos_data_from_priv(priv);
			}
		}
	} else {
		if (1 == location_setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED) && priv->mod->ops.start && !__get_started(self)) {
			LOCATION_LOGD("location resumed by setting");
			__set_started(self, TRUE);
			ret = priv->mod->ops.start(priv->mod->handler, wps_status_cb, wps_location_cb, NULL, self);
			if (ret != LOCATION_ERROR_NONE) {
				__set_started(self, FALSE);
				LOCATION_LOGD("Fail to start. Error[%d]", ret);
			}
		}
	}

}

static int
location_wps_start(LocationWps *self)
{
	LOCATION_LOGD("location_wps_start");
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.start, LOCATION_ERROR_NOT_AVAILABLE);

	if (__get_started(self) == TRUE) return LOCATION_ERROR_NONE;

	int ret = LOCATION_ERROR_NONE;

	if (!location_setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED)) {
		ret = LOCATION_ERROR_SETTING_OFF;
	} else {
		__set_started(self, TRUE);
		ret = priv->mod->ops.start(priv->mod->handler, wps_status_cb, wps_location_cb, NULL, self);
		if (ret != LOCATION_ERROR_NONE) {
			__set_started(self, FALSE);
			LOCATION_LOGE("Fail to start wps. Error[%d]", ret);
			return ret;
		}
	}

	if (priv->app_type != CPPAPP && priv->set_noti == FALSE) {
		location_setting_add_notify(VCONFKEY_LOCATION_NETWORK_ENABLED, location_setting_wps_cb, self);
		priv->set_noti = TRUE;
	}

	return ret;
}

static int
location_wps_stop(LocationWps *self)
{
	LOCATION_LOGD("location_wps_stop");
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.stop, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NONE;

	if (__get_started(self) == TRUE) {
		__set_started(self, FALSE);
		ret = priv->mod->ops.stop(priv->mod->handler);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_LOGD("Failed to stop. Error[%d]", ret);
		}
	}

	if (priv->app_type != CPPAPP && priv->set_noti == TRUE) {
		location_setting_ignore_notify(VCONFKEY_LOCATION_NETWORK_ENABLED, location_setting_wps_cb);
		priv->set_noti = FALSE;
	}

	__reset_pos_data_from_priv(priv);

	return ret;
}

static void
location_wps_dispose(GObject *gobject)
{
	LOCATION_LOGD("location_wps_dispose");

	LocationWpsPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);

	g_mutex_clear(&priv->mutex);
	if (priv->app_type != CPPAPP && priv->set_noti == TRUE) {
		location_setting_ignore_notify(VCONFKEY_LOCATION_NETWORK_ENABLED, location_setting_wps_cb);
		priv->set_noti = FALSE;

	}

	G_OBJECT_CLASS(location_wps_parent_class)->dispose(gobject);
}

static void
location_wps_finalize(GObject *gobject)
{
	LOCATION_LOGD("location_wps_finalize");
	LocationWpsPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);
	module_free(priv->mod, "wps");

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
	G_OBJECT_CLASS(location_wps_parent_class)->finalize(gobject);
}

static void
location_wps_set_property(GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
	LocationWpsPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);
	int ret = 0;

	switch (property_id) {
		case PROP_BOUNDARY: {
				GList *boundary_list = (GList *)g_list_copy(g_value_get_pointer(value));
				ret = set_prop_boundary(&priv->boundary_list, boundary_list);
				if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Set boundary. Error[%d]", ret);
				if (boundary_list) g_list_free(boundary_list);
				break;
			}
		case PROP_REMOVAL_BOUNDARY: {
				LocationBoundary *req_boundary = (LocationBoundary *) g_value_dup_boxed(value);
				ret = set_prop_removal_boundary(&priv->boundary_list, req_boundary);
				if (ret != 0) LOCATION_LOGD("Set removal boundary. Error[%d]", ret);
				break;
			}
		case PROP_POS_INTERVAL: {
				guint interval = g_value_get_uint(value);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->pos_interval = interval;
					else
						priv->pos_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
				} else
					priv->pos_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;

				break;
			}
		case PROP_VEL_INTERVAL: {
				guint interval = g_value_get_uint(value);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->vel_interval = interval;
					else
						priv->vel_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
				} else {
					priv->vel_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;
				}
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

				break;
			}
		case PROP_MIN_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> update-min-interval: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_MIN_INTERVAL_MAX)
						priv->min_interval = interval;
					else
						priv->min_interval = (guint)LOCATION_MIN_INTERVAL_MAX;
				} else
					priv->min_interval = (guint)LOCATION_MIN_INTERVAL_DEFAULT;

				break;
			}
		case PROP_MIN_DISTANCE: {
				gdouble distance = g_value_get_double(value);
				LOCATION_LOGD("Set prop>> update-min-distance: %u", distance);
				if (distance > 0) {
					if (distance < LOCATION_MIN_DISTANCE_MAX)
						priv->min_distance = distance;
					else
						priv->min_distance = (gdouble)LOCATION_MIN_DISTANCE_MAX;
				} else
					priv->min_distance = (gdouble)LOCATION_MIN_DISTANCE_DEFAULT;

				break;
			}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

static void
location_wps_get_property(GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
	LocationWpsPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);

	switch (property_id) {
		case PROP_METHOD_TYPE:
			g_value_set_int(value, LOCATION_METHOD_WPS);
			break;
		case PROP_IS_STARTED:
			g_value_set_boolean(value, __get_started(object));
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
location_wps_get_position(LocationWps *self,
                          LocationPosition **position,
                          LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_NETWORK_ENABLED);

	if (__get_started(self) != TRUE) {
		LOCATION_LOGE("location is not started");
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	if (priv->pos) {
		*position = location_position_copy(priv->pos);
		if (priv->acc) *accuracy = location_accuracy_copy(priv->acc);
		else *accuracy = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);
		ret = LOCATION_ERROR_NONE;
	}

	return ret;
}

static int
location_wps_get_position_ext(LocationWps *self,
                              LocationPosition **position,
                              LocationVelocity **velocity,
                              LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_NETWORK_ENABLED);

	if (__get_started(self) != TRUE) {
		LOCATION_LOGE("location is not started");
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	if (priv->pos && priv->vel) {
		*position = location_position_copy(priv->pos);
		*velocity = location_velocity_copy(priv->vel);
		if (priv->acc) *accuracy = location_accuracy_copy(priv->acc);
		else *accuracy = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);
		ret = LOCATION_ERROR_NONE;
	}

	return ret;
}


static int
location_wps_get_last_position(LocationWps *self,
                               LocationPosition **position,
                               LocationAccuracy **accuracy)
{
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_NETWORK_ENABLED);

	int ret = LOCATION_ERROR_NONE;
	LocationVelocity *_velocity = NULL;

	LocModWpsOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);

	ret = ops.get_last_position(priv->mod->handler, position, &_velocity, accuracy);
	if (_velocity) location_velocity_free(_velocity);

	return ret;
}

static int
location_wps_get_last_position_ext(LocationWps *self,
                                   LocationPosition **position,
                                   LocationVelocity **velocity,
                                   LocationAccuracy **accuracy)
{
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_NETWORK_ENABLED);

	LocModWpsOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);

	return ops.get_last_position(priv->mod->handler, position, velocity, accuracy);
}


static int
location_wps_get_velocity(LocationWps *self,
                          LocationVelocity **velocity,
                          LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_NETWORK_ENABLED);

	if (__get_started(self) != TRUE) {
		LOCATION_LOGE("location is not started");
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	if (priv->vel) {
		*velocity = location_velocity_copy(priv->vel);
		if (priv->acc) *accuracy = location_accuracy_copy(priv->acc);
		else *accuracy = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);
		ret = LOCATION_ERROR_NONE;
	}

	return ret;
}

static int
location_wps_get_last_velocity(LocationWps *self,
                               LocationVelocity **velocity,
                               LocationAccuracy **accuracy)
{
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_NETWORK_ENABLED);

	int ret = LOCATION_ERROR_NONE;
	LocationPosition *_position = NULL;

	LocModWpsOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	ret = ops.get_last_position(priv->mod->handler, &_position, velocity, accuracy);
	if (!_position) location_position_free(_position);

	return ret;
}

static gboolean __single_location_timeout_cb(void *data)
{
	LOCATION_LOGD("__single_location_timeout_cb");
	LocationWps *self = (LocationWps *)data;
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, FALSE);

	LocationPosition *pos = location_position_new(0, 0.0, 0.0, 0.0, LOCATION_STATUS_NO_FIX);
	LocationVelocity *vel = location_velocity_new(0, 0.0, 0.0, 0.0);
	LocationAccuracy *acc = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);

	if (priv->loc_timeout) g_source_remove(priv->loc_timeout);
	priv->loc_timeout = 0;

	g_signal_emit(self, signals[LOCATION_UPDATED], 0, LOCATION_ERROR_NOT_AVAILABLE, pos, vel, acc);
	location_wps_stop(self);

	return FALSE;
}


static void
wps_single_location_cb(gboolean enabled,
                       LocationPosition *pos,
                       LocationVelocity *vel,
                       LocationAccuracy *acc,
                       gpointer self)
{
	LOCATION_LOGD("wps_single_location_cb");
	g_return_if_fail(self);
	g_return_if_fail(pos);
	g_return_if_fail(vel);
	g_return_if_fail(acc);

	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	g_signal_emit(self, signals[LOCATION_UPDATED], 0, LOCATION_ERROR_NONE, pos, vel, acc);
	if (priv->loc_timeout) {
		g_source_remove(priv->loc_timeout);
		priv->loc_timeout = 0;
	}
	location_wps_stop(self);
}

static int
location_wps_request_single_location(LocationWps *self, int timeout)
{
	LOCATION_LOGD("location_wps_request_single_location");
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.start, LOCATION_ERROR_NOT_AVAILABLE);

	if (__get_started(self) == TRUE) return LOCATION_ERROR_NONE;

	int ret = LOCATION_ERROR_NONE;

	__set_started(self, TRUE);
	ret = priv->mod->ops.start(priv->mod->handler, wps_status_cb, wps_single_location_cb, NULL, self);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Fail to start request single. Error[%d]", ret);
		__set_started(self, FALSE);
		return ret;
	} else {
		if (priv->loc_timeout != 0) {
			g_source_remove(priv->loc_timeout);
		}
		priv->loc_timeout = g_timeout_add_seconds(timeout, __single_location_timeout_cb, self);
	}

	return ret;
}

static int
location_wps_get_satellite(LocationWps *self,
                           LocationSatellite **satellite)
{
	return LOCATION_ERROR_NOT_SUPPORTED;
}

static int
location_wps_get_last_satellite(LocationWps *self,
                                LocationSatellite **satellite)
{
	return LOCATION_ERROR_NOT_SUPPORTED;
}

static int
location_wps_set_option(LocationWps *self, const char *option)
{
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	return LOCATION_ERROR_NONE;
}

static int
location_wps_get_nmea(LocationWps *self,
                      char **nmea_data)
{
	return LOCATION_ERROR_NOT_SUPPORTED;
}

static void
location_ielement_interface_init(LocationIElementInterface *iface)
{
	iface->start = (TYPE_START_FUNC)location_wps_start;
	iface->stop = (TYPE_STOP_FUNC)location_wps_stop;
	iface->get_position = (TYPE_GET_POSITION)location_wps_get_position;
	iface->get_position_ext = (TYPE_GET_POSITION_EXT)location_wps_get_position_ext;
	iface->get_last_position = (TYPE_GET_POSITION)location_wps_get_last_position;
	iface->get_last_position_ext = (TYPE_GET_POSITION_EXT)location_wps_get_last_position_ext;
	iface->get_velocity = (TYPE_GET_VELOCITY)location_wps_get_velocity;
	iface->get_last_velocity = (TYPE_GET_VELOCITY)location_wps_get_last_velocity;
	iface->get_satellite = (TYPE_GET_SATELLITE)location_wps_get_satellite;
	iface->get_last_satellite = (TYPE_GET_SATELLITE)location_wps_get_last_satellite;
	iface->set_option = (TYPE_SET_OPTION)location_wps_set_option;
	iface->request_single_location = (TYPE_REQUEST_SINGLE_LOCATION)location_wps_request_single_location;
	iface->get_nmea = (TYPE_GET_NMEA)location_wps_get_nmea;
}

static void
location_wps_init(LocationWps *self)
{
	LOCATION_LOGD("location_wps_init");
	LocationWpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	priv->mod = (LocationWpsMod *)module_new("wps");
	if (!priv->mod) LOCATION_LOGW("module loading failed");

	g_mutex_init(&priv->mutex);
	priv->is_started = FALSE;
	priv->set_noti = FALSE;
	priv->enabled = FALSE;

	priv->pos_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
	priv->vel_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
	priv->loc_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
	priv->min_interval = LOCATION_UPDATE_INTERVAL_NONE;

	priv->pos_updated_timestamp = 0;
	priv->vel_updated_timestamp = 0;
	priv->loc_updated_timestamp = 0;

	priv->pos = NULL;
	priv->vel = NULL;
	priv->acc = NULL;
	priv->boundary_list = NULL;

	priv->loc_timeout = 0;

	priv->app_type = location_get_app_type(NULL);
	if (priv->app_type == 0) {
		LOCATION_LOGW("Fail to get app_type");
	}
}

static void
location_wps_class_init(LocationWpsClass *klass)
{
	LOCATION_LOGD("location_wps_class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->set_property = location_wps_set_property;
	gobject_class->get_property = location_wps_get_property;

	gobject_class->dispose = location_wps_dispose;
	gobject_class->finalize = location_wps_finalize;

	g_type_class_add_private(klass, sizeof(LocationWpsPrivate));

	signals[SERVICE_ENABLED] = g_signal_new("service-enabled",
	                                        G_TYPE_FROM_CLASS(klass),
	                                        G_SIGNAL_RUN_FIRST |
	                                        G_SIGNAL_NO_RECURSE,
	                                        G_STRUCT_OFFSET(LocationWpsClass, enabled),
	                                        NULL, NULL,
	                                        location_VOID__UINT,
	                                        G_TYPE_NONE, 1,
	                                        G_TYPE_UINT);

	signals[SERVICE_DISABLED] = g_signal_new("service-disabled",
	                                         G_TYPE_FROM_CLASS(klass),
	                                         G_SIGNAL_RUN_FIRST |
	                                         G_SIGNAL_NO_RECURSE,
	                                         G_STRUCT_OFFSET(LocationWpsClass, disabled),
	                                         NULL, NULL,
	                                         location_VOID__UINT,
	                                         G_TYPE_NONE, 1,
	                                         G_TYPE_UINT);

	signals[SERVICE_UPDATED] = g_signal_new("service-updated",
	                                        G_TYPE_FROM_CLASS(klass),
	                                        G_SIGNAL_RUN_FIRST |
	                                        G_SIGNAL_NO_RECURSE,
	                                        G_STRUCT_OFFSET(LocationWpsClass, updated),
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
	                                         G_STRUCT_OFFSET(LocationWpsClass, location_updated),
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
	                                G_STRUCT_OFFSET(LocationWpsClass, zone_in),
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
	                                 G_STRUCT_OFFSET(LocationWpsClass, zone_out),
	                                 NULL, NULL,
	                                 location_VOID__POINTER_POINTER_POINTER,
	                                 G_TYPE_NONE, 3,
	                                 G_TYPE_POINTER,
	                                 G_TYPE_POINTER,
	                                 G_TYPE_POINTER);

	properties[PROP_METHOD_TYPE] = g_param_spec_int("method",
	                                                "method type",
	                                                "location method type name",
	                                                LOCATION_METHOD_WPS,
	                                                LOCATION_METHOD_WPS,
	                                                LOCATION_METHOD_WPS,
	                                                G_PARAM_READABLE);

	properties[PROP_IS_STARTED] = g_param_spec_boolean("is_started",
	                                                   "wps is started prop",
	                                                   "wps is started status",
	                                                   FALSE,
	                                                   G_PARAM_READWRITE);

	properties[PROP_LAST_POSITION] = g_param_spec_boxed("last-position",
	                                                    "wps last position prop",
	                                                    "wps last position data",
	                                                    LOCATION_TYPE_POSITION,
	                                                    G_PARAM_READABLE);

	properties[PROP_POS_INTERVAL] = g_param_spec_uint("pos-interval",
	                                                  "wps position interval prop",
	                                                  "wps position interval data",
	                                                  LOCATION_UPDATE_INTERVAL_MIN,
	                                                  LOCATION_UPDATE_INTERVAL_MAX,
	                                                  LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);

	properties[PROP_VEL_INTERVAL] = g_param_spec_uint("vel-interval",
	                                                  "wps velocity interval prop",
	                                                  "wps velocity interval data",
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
	                                                  "wps distance-based interval prop",
	                                                  "wps distance-based interval data",
	                                                  LOCATION_MIN_INTERVAL_MIN,
	                                                  LOCATION_MIN_INTERVAL_MAX,
	                                                  LOCATION_MIN_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);

	properties[PROP_MIN_DISTANCE] = g_param_spec_double("min-distance",
	                                                    "wps distance-based distance prop",
	                                                    "wps distance-based distance data",
	                                                    LOCATION_MIN_DISTANCE_MIN,
	                                                    LOCATION_MIN_DISTANCE_MAX,
	                                                    LOCATION_MIN_DISTANCE_DEFAULT,
	                                                    G_PARAM_READWRITE);

	properties[PROP_BOUNDARY] = g_param_spec_pointer("boundary",
	                                                 "wps boundary prop",
	                                                 "wps boundary data",
	                                                 G_PARAM_READWRITE);

	properties[PROP_REMOVAL_BOUNDARY] = g_param_spec_boxed("removal-boundary",
	                                                       "wps removal boundary prop",
	                                                       "wps removal boundary data",
	                                                       LOCATION_TYPE_BOUNDARY,
	                                                       G_PARAM_READWRITE);

	g_object_class_install_properties(gobject_class,
	                                  PROP_MAX,
	                                  properties);
}

