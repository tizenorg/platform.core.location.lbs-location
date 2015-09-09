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

#include <app_manager.h>
#include "location-setting.h"
#include "location-log.h"

#include "module-internal.h"

#include "location-gps.h"
#include "location-marshal.h"
#include "location-ielement.h"
#include "location-signaling-util.h"
#include "location-common-util.h"
#include "location-privacy.h"

#include <vconf-internal-location-keys.h>

typedef struct _LocationGpsPrivate {
	LocationGpsMod		*mod;
	GMutex				mutex;
	gboolean			is_started;
	guint				app_type;
	gboolean			set_noti;
	gboolean			enabled;
	gint				signal_type;
	guint				pos_updated_timestamp;
	guint				pos_interval;
	guint				vel_updated_timestamp;
	guint				vel_interval;
	guint				sat_updated_timestamp;
	guint				sat_interval;
	guint				loc_updated_timestamp;
	guint				loc_interval;
	guint				loc_timeout;
	guint				batch_interval;
	guint				batch_period;
	guint				dist_updated_timestamp;
	guint				min_interval;
	gdouble				min_distance;
	LocationPosition	*pos;
	LocationBatch		*batch;
	LocationVelocity	*vel;
	LocationAccuracy	*acc;
	LocationSatellite	*sat;
	GList				*boundary_list;
#ifdef TIZEN_PROFILE_MOBILE
	guint				pos_searching_timer;
	guint				vel_searching_timer;
#endif
} LocationGpsPrivate;

enum {
    PROP_0,
    PROP_METHOD_TYPE,
    PROP_IS_STARTED,
    PROP_LAST_POSITION,
    PROP_POS_INTERVAL,
    PROP_VEL_INTERVAL,
    PROP_SAT_INTERVAL,
    PROP_LOC_INTERVAL,
    PROP_BATCH_INTERVAL,
    PROP_BATCH_PERIOD,
    PROP_BOUNDARY,
    PROP_REMOVAL_BOUNDARY,
    PROP_NMEA,
    PROP_SATELLITE,
    PROP_MIN_INTERVAL,
    PROP_MIN_DISTANCE,
    PROP_MAX
};

static guint32 signals[LAST_SIGNAL] = {0, };
static GParamSpec *properties[PROP_MAX] = {NULL, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOCATION_TYPE_GPS, LocationGpsPrivate))

static void location_ielement_interface_init(LocationIElementInterface *iface);

G_DEFINE_TYPE_WITH_CODE(LocationGps, location_gps, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(LOCATION_TYPE_IELEMENT,
                                              location_ielement_interface_init));
#ifdef TIZEN_PROFILE_MOBILE
static gboolean
_location_timeout_cb(gpointer data)
{
	GObject *object = (GObject *)data;
	LocationGpsPrivate *priv = GET_PRIVATE(object);
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
	LocationGpsPrivate *priv = GET_PRIVATE(object);
	g_return_val_if_fail(priv, FALSE);

	if (priv->pos_interval == priv->vel_interval) {
		priv->signal_type |= POSITION_UPDATED;
		priv->signal_type |= VELOCITY_UPDATED;
	} else {
		priv->signal_type |= POSITION_UPDATED;
	}
	_location_timeout_cb(priv);

	return TRUE;
}

static gboolean
_velocity_timeout_cb(gpointer data)
{
	GObject *object = (GObject *)data;
	LocationGpsPrivate *priv = GET_PRIVATE(object);
	g_return_val_if_fail(priv, FALSE);

	if (priv->pos_interval != priv->vel_interval) {
		priv->signal_type |= VELOCITY_UPDATED;
		_location_timeout_cb(priv);
	}

	return TRUE;
}

#endif

static void
__reset_pos_data_from_priv(LocationGpsPrivate *priv)
{
	LOCATION_LOGD("__reset_pos_data_from_priv");
	g_return_if_fail(priv);

	if (priv->pos) {
		location_position_free(priv->pos);
		priv->pos = NULL;
	}

	if (priv->batch) {
		location_batch_free(priv->batch);
		priv->batch = NULL;
	}

	if (priv->vel) {
		location_velocity_free(priv->vel);
		priv->vel = NULL;
	}

	if (priv->sat) {
		location_satellite_free(priv->sat);
		priv->sat = NULL;
	}

	if (priv->acc) {
		location_accuracy_free(priv->acc);
		priv->acc = NULL;
	}
	priv->pos_updated_timestamp = 0;
	priv->vel_updated_timestamp = 0;
	priv->sat_updated_timestamp = 0;
	priv->loc_updated_timestamp = 0;

	priv->signal_type = 0;
}

static gboolean
__get_started(gpointer self)
{
	g_return_val_if_fail(self, FALSE);

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, FALSE);

	return priv->is_started;
}

static int
__set_started(gpointer self, gboolean started)
{
	g_return_val_if_fail(self, -1);

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, -1);

	if (priv->is_started != started) {
		g_mutex_lock(&priv->mutex);
		priv->is_started = started;
		g_mutex_unlock(&priv->mutex);
	}

	return 0;
}

static void
gps_status_cb(gboolean enabled,
              LocationStatus status,
              gpointer self)
{
	LOCATION_LOGD("gps_status_cb");
	g_return_if_fail(self);
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);
	if (!priv->enabled && enabled) {	/* Update satellite at searching status. */
#ifdef TIZEN_PROFILE_MOBILE
		if (priv->pos_searching_timer) g_source_remove(priv->pos_searching_timer);
		if (priv->vel_searching_timer) g_source_remove(priv->vel_searching_timer);
		priv->pos_searching_timer = 0;
		priv->vel_searching_timer = 0;
#endif
		return; /* Ignored: Support to get position at enabled callback */
	} else if (priv->enabled == TRUE && enabled == FALSE) {
		__set_started(self, FALSE);
		enable_signaling(self, signals, &(priv->enabled), enabled, status);
	}
}

static void
gps_location_cb(gboolean enabled,
                LocationPosition *pos,
                LocationVelocity *vel,
                LocationAccuracy *acc,
                gpointer self)
{
	g_return_if_fail(self);
	g_return_if_fail(pos);
	g_return_if_fail(vel);
	g_return_if_fail(acc);

	LocationGpsPrivate *priv = GET_PRIVATE(self);
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
	                   enabled,	/* previous status */
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
gps_batch_cb(gboolean enabled,
             guint num_of_location,
             gpointer self)
{
	g_return_if_fail(self);
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	if (priv->batch != NULL) {
		location_batch_free(priv->batch);
	}
	priv->batch = location_get_batch_file(num_of_location);

	g_signal_emit(self, signals[BATCH_UPDATED], 0, num_of_location);
}

static void
gps_satellite_cb(gboolean enabled,
                 LocationSatellite *sat,
                 gpointer self)
{
	g_return_if_fail(self);
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	satellite_signaling(self, signals, &(priv->enabled), priv->sat_interval, TRUE, &(priv->sat_updated_timestamp), &(priv->sat), sat);
}

#ifdef TIZEN_PROFILE_MOBILE
static void
location_setting_search_cb(keynode_t *key, gpointer self)
{
	LOCATION_LOGD("location_setting_search_cb");
	g_return_if_fail(key);
	g_return_if_fail(self);
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	if (location_setting_get_key_val(key) == VCONFKEY_LOCATION_GPS_SEARCHING) {
		if (!priv->pos_searching_timer) priv->pos_searching_timer = g_timeout_add(priv->pos_interval * 1000, _position_timeout_cb, self);
		if (!priv->vel_searching_timer) priv->vel_searching_timer = g_timeout_add(priv->vel_interval * 1000, _velocity_timeout_cb, self);
	} else {
		if (priv->pos_searching_timer) g_source_remove(priv->pos_searching_timer);
		if (priv->vel_searching_timer) g_source_remove(priv->vel_searching_timer);
		priv->pos_searching_timer = 0;
		priv->vel_searching_timer = 0;
	}
}
#endif

static void
location_setting_gps_cb(keynode_t *key,
                        gpointer self)
{
	LOCATION_LOGD("location_setting_gps_cb");
	g_return_if_fail(key);
	g_return_if_fail(self);
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);
	g_return_if_fail(priv->mod);
	g_return_if_fail(priv->mod->handler);

	int ret = LOCATION_ERROR_NONE;

	if (0 == location_setting_get_key_val(key) && priv->mod->ops.stop && __get_started(self)) {
		LOCATION_LOGD("location stopped by setting");
		__set_started(self, FALSE);
		ret = priv->mod->ops.stop(priv->mod->handler);
		if (ret == LOCATION_ERROR_NONE) {
			__reset_pos_data_from_priv(priv);
		} else {
			LOCATION_LOGI("Fail to stop[%d]", ret);
		}

	} else if (1 == location_setting_get_key_val(key) && priv->mod->ops.start && !__get_started(self)) {
		LOCATION_LOGD("location resumed by setting");
		__set_started(self, TRUE);
		ret = priv->mod->ops.start(priv->mod->handler, priv->pos_interval, gps_status_cb, gps_location_cb, gps_satellite_cb, self);
		if (ret != LOCATION_ERROR_NONE) {
			__set_started(self, FALSE);
			LOCATION_LOGI("Fail to start[%d]", ret);
		}
	}
}

static int
location_gps_start(LocationGps *self)
{
	LOCATION_LOGD("ENTER >>>");
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.start, LOCATION_ERROR_NOT_AVAILABLE);

	if (__get_started(self) == TRUE) return LOCATION_ERROR_NONE;

	int ret = LOCATION_ERROR_NONE;

	if (!location_setting_get_int(VCONFKEY_LOCATION_ENABLED)) {
		ret = LOCATION_ERROR_SETTING_OFF;
	} else if (location_setting_get_int(VCONFKEY_SETAPPL_PSMODE) == SETTING_PSMODE_WEARABLE_ENHANCED) {
		return LOCATION_ACCESS_DENIED;
	} else {
		__set_started(self, TRUE);
		ret = priv->mod->ops.start(priv->mod->handler, priv->pos_interval, gps_status_cb, gps_location_cb, gps_satellite_cb, self);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_LOGE("Fail to start gps. Error[%d]", ret);
			__set_started(self, FALSE);
			return ret;
		}
	}

	if (priv->app_type != CPPAPP && priv->set_noti == FALSE) {
		location_setting_add_notify(VCONFKEY_LOCATION_ENABLED, location_setting_gps_cb, self);

#ifdef TIZEN_PROFILE_MOBILE
		location_state_add_notify(VCONFKEY_LOCATION_GPS_STATE, location_setting_search_cb, self);
#endif
		priv->set_noti = TRUE;
	}

	LOCATION_LOGD("EXIT <<<");
	return ret;
}

static int
location_gps_stop(LocationGps *self)
{
	LOCATION_LOGD("location_gps_stop");
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.stop, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NONE;

	if (__get_started(self) == TRUE) {
		__set_started(self, FALSE);
		ret = priv->mod->ops.stop(priv->mod->handler);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_LOGE("Failed to stop. Error[%d]", ret);
		}
	} else {
		return LOCATION_ERROR_NONE;
	}

#ifdef TIZEN_PROFILE_MOBILE
	if (priv->pos_searching_timer) g_source_remove(priv->pos_searching_timer);
	if (priv->vel_searching_timer) g_source_remove(priv->vel_searching_timer);
	priv->pos_searching_timer = 0;
	priv->vel_searching_timer = 0;
#endif

	if (priv->app_type != CPPAPP && priv->set_noti == TRUE) {
		location_setting_ignore_notify(VCONFKEY_LOCATION_ENABLED, location_setting_gps_cb);
#ifdef TIZEN_PROFILE_MOBILE
		location_state_ignore_notify(VCONFKEY_LOCATION_GPS_STATE, location_setting_search_cb);
#endif
		priv->set_noti = FALSE;
	}

	__reset_pos_data_from_priv(priv);

	return ret;
}

static int
location_gps_start_batch(LocationGps *self)
{
	LOCATION_LOGD("location_gps_start_batch");
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.start_batch, LOCATION_ERROR_NOT_AVAILABLE);

	if (__get_started(self) == TRUE) return LOCATION_ERROR_NONE;

	int ret = LOCATION_ERROR_NONE;

	if (!location_setting_get_int(VCONFKEY_LOCATION_ENABLED)) {
		ret = LOCATION_ERROR_SETTING_OFF;
	} else {
		__set_started(self, TRUE);
		ret = priv->mod->ops.start_batch(priv->mod->handler, gps_batch_cb, priv->batch_interval, priv->batch_period, self);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_LOGE("Fail to start_batch. Error[%d]", ret);
			__set_started(self, FALSE);
			return ret;
		}
	}

	return ret;
}

static int
location_gps_stop_batch(LocationGps *self)
{
	LOCATION_LOGD("location_gps_stop_batch");
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.stop_batch, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NONE;

	if (__get_started(self) == TRUE) {
		__set_started(self, FALSE);
		ret = priv->mod->ops.stop_batch(priv->mod->handler);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_LOGE("Failed to stop_batch. Error[%d]", ret);
		}
	} else {
		return LOCATION_ERROR_NONE;
	}

	__reset_pos_data_from_priv(priv);

	return ret;
}

static void
location_gps_dispose(GObject *gobject)
{
	LOCATION_LOGD("location_gps_dispose");

	LocationGpsPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);
	g_mutex_clear(&priv->mutex);

#ifdef TIZEN_PROFILE_MOBILE
	if (priv->pos_searching_timer) g_source_remove(priv->pos_searching_timer);
	if (priv->vel_searching_timer) g_source_remove(priv->vel_searching_timer);
	priv->pos_searching_timer = 0;
	priv->vel_searching_timer = 0;
#endif
	if (priv->loc_timeout) g_source_remove(priv->loc_timeout);
	priv->loc_timeout = 0;

	if (priv->app_type != CPPAPP && priv->set_noti == TRUE) {
		location_setting_ignore_notify(VCONFKEY_LOCATION_ENABLED, location_setting_gps_cb);
#ifdef TIZEN_PROFILE_MOBILE
		location_state_ignore_notify(VCONFKEY_LOCATION_GPS_STATE, location_setting_search_cb);
#endif
		priv->set_noti = FALSE;
	}
}

static void
location_gps_finalize(GObject *gobject)
{
	LOCATION_LOGD("location_gps_finalize");
	LocationGpsPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);

	module_free(priv->mod, "gps");
	priv->mod = NULL;

	if (priv->boundary_list) {
		g_list_free_full(priv->boundary_list, free_boundary_list);
		priv->boundary_list = NULL;
	}

	if (priv->pos) {
		location_position_free(priv->pos);
		priv->pos = NULL;
	}

	if (priv->batch) {
		location_batch_free(priv->batch);
		priv->batch = NULL;
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
	G_OBJECT_CLASS(location_gps_parent_class)->finalize(gobject);
}

static void
location_gps_set_property(GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
	LocationGpsPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);
	g_return_if_fail(priv->mod);
	g_return_if_fail(priv->mod->handler);

	int ret = 0;
	switch (property_id) {
		case PROP_BOUNDARY: {
				GList *boundary_list = g_list_copy(g_value_get_pointer(value));
				ret = set_prop_boundary(&priv->boundary_list, boundary_list);
				if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Set boundary. Error[%d]", ret);
				if (boundary_list) g_list_free(boundary_list);
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
				/* We don't need to set interval when new one is same as the previous one */
				if (interval == priv->pos_interval) break;

				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->pos_interval = interval;
					else
						priv->pos_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
				} else {
					priv->pos_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;
				}

#ifdef TIZEN_PROFILE_MOBILE
				if (priv->pos_searching_timer) {
					g_source_remove(priv->pos_searching_timer);
					priv->pos_searching_timer = g_timeout_add(priv->pos_interval * 1000, _position_timeout_cb, object);
				}
#endif

				if (__get_started(object) == TRUE) {
					LOCATION_LOGD("[update_pos_interval]: update pos-interval while pos-tracking");
					g_return_if_fail(priv->mod->ops.set_position_update_interval);
					priv->mod->ops.set_position_update_interval(priv->mod->handler, priv->pos_interval);
				}

				break;
			}
		case PROP_VEL_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_VEL_INTERVAL: %u", interval);
				if (interval == priv->vel_interval) break;

				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->vel_interval = interval;
					else
						priv->vel_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
				} else
					priv->vel_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;

#ifdef TIZEN_PROFILE_MOBILE
				if (priv->vel_searching_timer) {
					g_source_remove(priv->vel_searching_timer);
					priv->vel_searching_timer = g_timeout_add(priv->vel_interval * 1000, _velocity_timeout_cb, object);
				}
#endif
				break;
			}
		case PROP_SAT_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_SAT_INTERVAL: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->sat_interval = interval;
					else
						priv->sat_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
				} else
					priv->sat_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;

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
		case PROP_BATCH_INTERVAL: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_BATCH_INTERVAL: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_UPDATE_INTERVAL_MAX)
						priv->batch_interval = interval;
					else
						priv->batch_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
				} else
					priv->batch_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;

				if (__get_started(object) == TRUE) {
					LOCATION_LOGD("[update_batch_interval]: update batch-interval while pos-tracking");
					g_return_if_fail(priv->mod->ops.set_position_update_interval);
					priv->mod->ops.set_position_update_interval(priv->mod->handler, priv->batch_interval);
				}
				break;
			}
		case PROP_BATCH_PERIOD: {
				guint interval = g_value_get_uint(value);
				LOCATION_LOGD("Set prop>> PROP_BATCH_PERIOD: %u", interval);
				if (interval > 0) {
					if (interval < LOCATION_BATCH_PERIOD_MAX)
						priv->batch_period = interval;
					else
						priv->batch_period = (guint)LOCATION_BATCH_PERIOD_MAX;
				} else
					priv->batch_period = (guint)LOCATION_BATCH_PERIOD_DEFAULT;

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

				break;
			}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

static void
location_gps_get_property(GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
	LocationGpsPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);
	g_return_if_fail(priv->mod);
	g_return_if_fail(priv->mod->handler);
	LocModGpsOps ops = priv->mod->ops;
	switch (property_id) {
		case PROP_METHOD_TYPE:
			g_value_set_int(value, LOCATION_METHOD_GPS);
			break;
		case PROP_IS_STARTED:
			g_value_set_boolean(value, __get_started(object));
			break;
		case PROP_LAST_POSITION:
			g_value_set_boxed(value, priv->pos);
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
		case PROP_BATCH_INTERVAL:
			g_value_set_uint(value, priv->batch_interval);
			break;
		case PROP_BATCH_PERIOD:
			g_value_set_uint(value, priv->batch_period);
			break;
		case PROP_MIN_INTERVAL:
			g_value_set_uint(value, priv->min_interval);
			break;
		case PROP_MIN_DISTANCE:
			g_value_set_double(value, priv->min_distance);
			break;
		case PROP_BOUNDARY:
			g_value_set_pointer(value, g_list_first(priv->boundary_list));
			break;
		case PROP_NMEA: {
				char *nmea_data = NULL;
				if (ops.get_nmea && LOCATION_ERROR_NONE == ops.get_nmea(priv->mod->handler, &nmea_data) && nmea_data) {
					LOCATION_SECLOG("Get prop>> Lastest nmea: \n%s", nmea_data);
					g_value_set_string(value, nmea_data);
					g_free(nmea_data);
				} else {
					LOCATION_LOGW("Get prop>> Lastest nmea: failed");
					g_value_set_string(value, NULL);
				}
				break;
			}
		case PROP_SATELLITE: {
				LocationSatellite *satellite = NULL;
				if (ops.get_satellite && priv->mod->handler && LOCATION_ERROR_NONE == ops.get_satellite(priv->mod->handler, &satellite) && satellite) {
					LOCATION_LOGD("Get prop>> Last sat: num_used(%d) num_view(%d)", satellite->num_of_sat_used, satellite->num_of_sat_inview);
					g_value_set_boxed(value, satellite);
					location_satellite_free(satellite);
				} else {
					LOCATION_LOGW("Get prop>> Last sat: failed");
					g_value_set_boxed(value, NULL);
				}
				break;
			}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

static int
location_gps_get_position(LocationGps *self,
                          LocationPosition **position,
                          LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

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
location_gps_get_position_ext(LocationGps *self,
                              LocationPosition **position,
                              LocationVelocity **velocity,
                              LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

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
location_gps_get_last_position(LocationGps *self,
                               LocationPosition **position,
                               LocationAccuracy **accuracy)
{
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	int ret = LOCATION_ERROR_NONE;
	LocationVelocity *_velocity = NULL;

	LocModGpsOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);
	ret = ops.get_last_position(priv->mod->handler, position, &_velocity, accuracy);
	if (_velocity) location_velocity_free(_velocity);

	return ret;
}

static int
location_gps_get_last_position_ext(LocationGps *self,
                                   LocationPosition **position,
                                   LocationVelocity **velocity,
                                   LocationAccuracy **accuracy)
{
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	LocModGpsOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);
	return ops.get_last_position(priv->mod->handler, position, velocity, accuracy);
}


static int
location_gps_get_velocity(LocationGps *self,
                          LocationVelocity **velocity,
                          LocationAccuracy **accuracy)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

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
location_gps_get_last_velocity(LocationGps *self,
                               LocationVelocity **velocity,
                               LocationAccuracy **accuracy)
{
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	int ret = LOCATION_ERROR_NONE;
	LocationPosition *_position = NULL;

	LocModGpsOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);
	ret = ops.get_last_position(priv->mod->handler, &_position, velocity, accuracy);
	if (_position) location_position_free(_position);

	return ret;
}

static gboolean __single_location_timeout_cb(void *data)
{
	LOCATION_LOGD("__single_location_timeout_cb");
	LocationGps *self = (LocationGps *)data;
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, FALSE);

	LocationPosition *pos = location_position_new(0, 0.0, 0.0, 0.0, LOCATION_STATUS_NO_FIX);
	LocationVelocity *vel = location_velocity_new(0, 0.0, 0.0, 0.0);
	LocationAccuracy *acc = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);

	if (priv->loc_timeout) g_source_remove(priv->loc_timeout);
	priv->loc_timeout = 0;

	g_signal_emit(self, signals[LOCATION_UPDATED], 0, LOCATION_ERROR_NOT_AVAILABLE, pos, vel, acc);
	location_gps_stop(self);

	return FALSE;
}

static void
gps_single_location_cb(gboolean enabled,
                       LocationPosition *pos,
                       LocationVelocity *vel,
                       LocationAccuracy *acc,
                       gpointer self)
{
	LOCATION_LOGD("gps_single_location_cb");
	g_return_if_fail(self);
	g_return_if_fail(pos);
	g_return_if_fail(vel);
	g_return_if_fail(acc);

	LocationGps *obj = (LocationGps *)self;
	LocationGpsPrivate *priv = GET_PRIVATE(obj);
	g_return_if_fail(priv);

	g_signal_emit(self, signals[LOCATION_UPDATED], 0, LOCATION_ERROR_NONE, pos, vel, acc);
	if (priv->loc_timeout) {
		g_source_remove(priv->loc_timeout);
		priv->loc_timeout = 0;
	}
	location_gps_stop(self);
}

static int
location_gps_request_single_location(LocationGps *self, int timeout)
{
	LOCATION_LOGD("location_gps_request_single_location");
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.start, LOCATION_ERROR_NOT_AVAILABLE);

	if (__get_started(self) == TRUE) return LOCATION_ERROR_NONE;

	int ret = LOCATION_ERROR_NONE;

	__set_started(self, TRUE);
	ret = priv->mod->ops.start(priv->mod->handler, priv->pos_interval, gps_status_cb, gps_single_location_cb, NULL, self);
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
location_gps_get_nmea(LocationGps *self, char **nmea_data)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.get_nmea, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	ret = priv->mod->ops.get_nmea(priv->mod->handler, nmea_data);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Failed to get_nmea. Error[%d]", ret);
	}

	return ret;
}

static int
location_gps_get_batch(LocationGps *self,
                       LocationBatch **batch)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	if (__get_started(self) != TRUE) {
		LOCATION_LOGE("location is not started");
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	if (priv->batch) {
		*batch = location_batch_copy(priv->batch);
		ret = LOCATION_ERROR_NONE;
	} else {
		LOCATION_LOGE("priv->batch is null");
		ret = LOCATION_ERROR_NOT_AVAILABLE;
	}

	return ret;
}

static int
location_gps_get_satellite(LocationGps *self,
                           LocationSatellite **satellite)
{
	int ret = LOCATION_ERROR_NOT_AVAILABLE;

	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	if (__get_started(self) != TRUE) {
		LOCATION_LOGE("location is not started");
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	if (priv->sat) {
		*satellite = location_satellite_copy(priv->sat);
		ret = LOCATION_ERROR_NONE;
	}

	return ret;
}

static int
location_gps_get_last_satellite(LocationGps *self,
                                LocationSatellite **satellite)
{
	return location_gps_get_satellite(self, satellite);
}

static int
location_gps_set_option(LocationGps *self, const char *option)
{
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.set_option, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NONE;

	ret = priv->mod->ops.set_option(priv->mod->handler, option);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Failed to set_option. Error[%d]", ret);
	}

	return ret;
}

static void
location_ielement_interface_init(LocationIElementInterface *iface)
{
	iface->start = (TYPE_START_FUNC)location_gps_start;
	iface->stop = (TYPE_STOP_FUNC)location_gps_stop;
	iface->get_position = (TYPE_GET_POSITION)location_gps_get_position;
	iface->get_position_ext = (TYPE_GET_POSITION_EXT)location_gps_get_position_ext;
	iface->get_last_position = (TYPE_GET_POSITION)location_gps_get_last_position;
	iface->get_last_position_ext = (TYPE_GET_POSITION_EXT)location_gps_get_last_position_ext;
	iface->get_velocity = (TYPE_GET_VELOCITY)location_gps_get_velocity;
	iface->get_last_velocity = (TYPE_GET_VELOCITY)location_gps_get_last_velocity;
	iface->get_satellite = (TYPE_GET_SATELLITE)location_gps_get_satellite;
	iface->get_last_satellite = (TYPE_GET_SATELLITE)location_gps_get_last_satellite;
	iface->set_option = (TYPE_SET_OPTION)location_gps_set_option;
	iface->get_batch = (TYPE_GET_BATCH)location_gps_get_batch;
	iface->start_batch = (TYPE_START_BATCH)location_gps_start_batch;
	iface->stop_batch = (TYPE_STOP_BATCH)location_gps_stop_batch;

	iface->request_single_location = (TYPE_REQUEST_SINGLE_LOCATION)location_gps_request_single_location;
	iface->get_nmea = (TYPE_GET_NMEA)location_gps_get_nmea;

}

static void
location_gps_init(LocationGps *self)
{
	LOCATION_LOGD("location_gps_init");
	LocationGpsPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	priv->mod = (LocationGpsMod *)module_new("gps");
	if (!priv->mod) LOCATION_LOGW("module loading failed");

	g_mutex_init(&priv->mutex);
	priv->is_started = FALSE;
	priv->set_noti = FALSE;
	priv->enabled = FALSE;
	priv->signal_type = 0;

	priv->pos_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
	priv->vel_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
	priv->sat_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
	priv->loc_interval = LOCATION_UPDATE_INTERVAL_DEFAULT;
	priv->batch_interval = LOCATION_UPDATE_INTERVAL_NONE;
	priv->batch_period = LOCATION_BATCH_PERIOD_DEFAULT;
	priv->min_interval = LOCATION_UPDATE_INTERVAL_NONE;

	priv->pos_updated_timestamp = 0;
	priv->vel_updated_timestamp = 0;
	priv->sat_updated_timestamp = 0;
	priv->loc_updated_timestamp = 0;

	priv->pos = NULL;
	priv->batch = NULL;
	priv->vel = NULL;
	priv->acc = NULL;
	priv->sat = NULL;
	priv->boundary_list = NULL;

#ifdef TIZEN_PROFILE_MOBILE
	priv->pos_searching_timer = 0;
	priv->vel_searching_timer = 0;
#endif
	priv->loc_timeout = 0;

	priv->app_type = location_get_app_type(NULL);
	if (priv->app_type == 0) {
		LOCATION_LOGW("Fail to get app_type");
	}
}

static void
location_gps_class_init(LocationGpsClass *klass)
{
	LOCATION_LOGD("location_gps_class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->set_property = location_gps_set_property;
	gobject_class->get_property = location_gps_get_property;

	gobject_class->dispose = location_gps_dispose;
	gobject_class->finalize = location_gps_finalize;

	g_type_class_add_private(klass, sizeof(LocationGpsPrivate));

	signals[SERVICE_ENABLED] = g_signal_new("service-enabled",
	                                        G_TYPE_FROM_CLASS(klass),
	                                        G_SIGNAL_RUN_FIRST |
	                                        G_SIGNAL_NO_RECURSE,
	                                        G_STRUCT_OFFSET(LocationGpsClass, enabled),
	                                        NULL, NULL,
	                                        location_VOID__UINT,
	                                        G_TYPE_NONE, 1,
	                                        G_TYPE_UINT);

	signals[SERVICE_DISABLED] = g_signal_new("service-disabled",
	                                         G_TYPE_FROM_CLASS(klass),
	                                         G_SIGNAL_RUN_FIRST |
	                                         G_SIGNAL_NO_RECURSE,
	                                         G_STRUCT_OFFSET(LocationGpsClass, disabled),
	                                         NULL, NULL,
	                                         location_VOID__UINT,
	                                         G_TYPE_NONE, 1,
	                                         G_TYPE_UINT);

	signals[SERVICE_UPDATED] = g_signal_new("service-updated",
	                                        G_TYPE_FROM_CLASS(klass),
	                                        G_SIGNAL_RUN_FIRST |
	                                        G_SIGNAL_NO_RECURSE,
	                                        G_STRUCT_OFFSET(LocationGpsClass, updated),
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
	                                         G_STRUCT_OFFSET(LocationGpsClass, location_updated),
	                                         NULL, NULL,
	                                         location_VOID__INT_POINTER_POINTER_POINTER,
	                                         G_TYPE_NONE, 4,
	                                         G_TYPE_INT,
	                                         G_TYPE_POINTER,
	                                         G_TYPE_POINTER,
	                                         G_TYPE_POINTER);

	signals[BATCH_UPDATED] = g_signal_new("batch-updated",
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_FIRST |
	                                      G_SIGNAL_NO_RECURSE,
	                                      G_STRUCT_OFFSET(LocationGpsClass, batch_updated),
	                                      NULL, NULL,
	                                      location_VOID__UINT,
	                                      G_TYPE_NONE, 1,
	                                      G_TYPE_UINT);

	signals[ZONE_IN] = g_signal_new("zone-in",
	                                G_TYPE_FROM_CLASS(klass),
	                                G_SIGNAL_RUN_FIRST |
	                                G_SIGNAL_NO_RECURSE,
	                                G_STRUCT_OFFSET(LocationGpsClass, zone_in),
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
	                                 G_STRUCT_OFFSET(LocationGpsClass, zone_out),
	                                 NULL, NULL,
	                                 location_VOID__POINTER_POINTER_POINTER,
	                                 G_TYPE_NONE, 3,
	                                 G_TYPE_POINTER,
	                                 G_TYPE_POINTER,
	                                 G_TYPE_POINTER);

	properties[PROP_METHOD_TYPE] = g_param_spec_int("method",
	                                                "method type",
	                                                "location method type name",
	                                                LOCATION_METHOD_GPS,
	                                                LOCATION_METHOD_GPS,
	                                                LOCATION_METHOD_GPS,
	                                                G_PARAM_READABLE);

	properties[PROP_IS_STARTED] = g_param_spec_boolean("is_started",
	                                                   "gps is started prop",
	                                                   "gps is started status",
	                                                   FALSE,
	                                                   G_PARAM_READWRITE);

	properties[PROP_LAST_POSITION] = g_param_spec_boxed("last-position",
	                                                    "gps last position prop",
	                                                    "gps last position data",
	                                                    LOCATION_TYPE_POSITION,
	                                                    G_PARAM_READABLE);

	properties[PROP_POS_INTERVAL] = g_param_spec_uint("pos-interval",
	                                                  "gps position interval prop",
	                                                  "gps position interval data",
	                                                  LOCATION_UPDATE_INTERVAL_MIN,
	                                                  LOCATION_UPDATE_INTERVAL_MAX,
	                                                  LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);

	properties[PROP_VEL_INTERVAL] = g_param_spec_uint("vel-interval",
	                                                  "gps velocity interval prop",
	                                                  "gps velocity interval data",
	                                                  LOCATION_UPDATE_INTERVAL_MIN,
	                                                  LOCATION_UPDATE_INTERVAL_MAX,
	                                                  LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                  G_PARAM_READWRITE);

	properties[PROP_SAT_INTERVAL] = g_param_spec_uint("sat-interval",
	                                                  "gps satellite interval prop",
	                                                  "gps satellite interval data",
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

	properties[PROP_BATCH_INTERVAL] = g_param_spec_uint("batch-interval",
	                                                    "gps batch interval interval prop",
	                                                    "gps batch interval interval data",
	                                                    LOCATION_UPDATE_INTERVAL_MIN,
	                                                    LOCATION_UPDATE_INTERVAL_MAX,
	                                                    LOCATION_UPDATE_INTERVAL_DEFAULT,
	                                                    G_PARAM_READWRITE);

	properties[PROP_BATCH_PERIOD] = g_param_spec_uint("batch-period",
	                                                  "gps batch period prop",
	                                                  "gps batch period data",
	                                                  LOCATION_BATCH_PERIOD_MIN,
	                                                  LOCATION_BATCH_PERIOD_MAX,
	                                                  LOCATION_BATCH_PERIOD_DEFAULT,
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
	                                                 "gps boundary prop",
	                                                 "gps boundary data",
	                                                 G_PARAM_READWRITE);

	properties[PROP_REMOVAL_BOUNDARY] = g_param_spec_boxed("removal-boundary",
	                                                       "gps removal boundary prop",
	                                                       "gps removal boundary data",
	                                                       LOCATION_TYPE_BOUNDARY,
	                                                       G_PARAM_READWRITE);


	properties[PROP_NMEA] = g_param_spec_string("nmea",
	                                            "gps NMEA name prop",
	                                            "gps NMEA",
	                                            NULL,
	                                            G_PARAM_READABLE);

	properties[PROP_SATELLITE] = g_param_spec_boxed("satellite",
	                                                "gps satellite prop",
	                                                "gps satellite data",
	                                                LOCATION_TYPE_SATELLITE,
	                                                G_PARAM_READABLE);

	g_object_class_install_properties(gobject_class,
	                                  PROP_MAX,
	                                  properties);
}
