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

#include "location-fused.h"
#include "location-marshal.h"
#include "location-ielement.h"
#include "location-signaling-util.h"
#include "location-common-util.h"
#include "location-privacy.h"

#define  _USE_MATH_DEFINES
#include <math.h>
#include <sensor.h>
#include <vconf-internal-location-keys.h>


/*
 * forward definitions
 */

typedef struct _LocationFusedPrivate {
	LocationFusedMod	*mod;
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
	guint				dist_updated_timestamp;
	guint				min_interval;
	gdouble				min_distance;
	LocationPosition	*pos;
	LocationVelocity	*vel;
	LocationAccuracy	*acc;
	GList				*boundary_list;
	guint				fused_timeout;
	guint				fused_balanced_interval;
	guint				fused_last_moving;
	LocationFusedMode	fused_mode;
	LocFusedAccelMotion	fused_motion;
	LocFusedCalibration fused_calibration_status;
	guint				fused_calibration_samples;
	sensor_h			sensor;
	sensor_listener_h	sensor_listener;
	gdouble				__s22a;
	gdouble				__g2;
} LocationFusedPrivate;

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
	PROP_SERVICE_STATUS,
	PROP_MAX
};

static guint32 signals[LAST_SIGNAL] = {0, };
static GParamSpec *properties[PROP_MAX] = {NULL, };

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), LOCATION_TYPE_FUSED, LocationFusedPrivate))

static void location_ielement_interface_init(LocationIElementInterface *iface);
static int location_fused_balanced_stop(LocationFused *self);

G_DEFINE_TYPE_WITH_CODE(LocationFused, location_fused, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(LOCATION_TYPE_IELEMENT, location_ielement_interface_init));

static void __reset_pos_data_from_priv(LocationFusedPrivate *priv)
{
	LOC_FUNC_LOG
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

static gboolean __get_started(gpointer self)
{
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, FALSE);

	return priv->is_started;
}

static int __set_started(gpointer self, gboolean started)
{
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, -1);

	if (priv->is_started != started) {
		g_mutex_lock(&priv->mutex);
		priv->is_started = started;
		g_mutex_unlock(&priv->mutex);
	}

	return 0;
}

static void fused_high_cb(gboolean enabled, LocationPosition *pos, LocationVelocity *vel, LocationAccuracy *acc, gpointer self)
{
	g_return_if_fail(self);
	g_return_if_fail(pos);
	g_return_if_fail(vel);
	g_return_if_fail(acc);

	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	location_signaling(self, signals, enabled, priv->boundary_list,
						pos, vel, acc, priv->pos_interval, priv->vel_interval, priv->loc_interval,
						&(priv->enabled), &(priv->pos_updated_timestamp), &(priv->vel_updated_timestamp),
						&(priv->loc_updated_timestamp), &(priv->pos), &(priv->vel), &(priv->acc));
}

static void fused_balanced_cb(gboolean enabled, LocationPosition *pos, LocationVelocity *vel, LocationAccuracy *acc, gpointer self)
{
	g_return_if_fail(self);
	g_return_if_fail(pos);
	g_return_if_fail(vel);
	g_return_if_fail(acc);

	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	location_signaling(self, signals, enabled, priv->boundary_list,
						pos, vel, acc, priv->pos_interval, priv->vel_interval, priv->loc_interval,
						&(priv->enabled), &(priv->pos_updated_timestamp), &(priv->vel_updated_timestamp),
						&(priv->loc_updated_timestamp), &(priv->pos), &(priv->vel), &(priv->acc));

	location_fused_balanced_stop(self);
}

static gboolean __fused_balanced_timeout_cb(void *data)
{
	LOC_FUNC_LOG
	LocationFused *self = (LocationFused *)data;
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, FALSE);
	g_return_val_if_fail(priv->mod->ops.start, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = priv->mod->ops.start(priv->mod->handler, priv->pos_interval, NULL, fused_balanced_cb, NULL, self);
//	if (ret != LOCATION_ERROR_NONE) __set_started(self, FALSE);
	LOC_IF_FAIL(ret, _E, "Fail to start : fused(balanced power) [%s]", err_msg(ret));

	return TRUE;
}

static void fused_nopower_cb(keynode_t * key, gpointer self)
{
	LOC_FUNC_LOG
	LOCATION_LOGD("gps_position_changed_cb ============== Passive invoke");
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);

	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;

//	LocModFusedOps ops = priv->mod->ops;
	int ret = priv->mod->ops.get_last_position(priv->mod->handler, &pos, &vel, &acc);
	LOC_COND_VOID(ret != LOCATION_ERROR_NONE, _E, "Fail to get position : fused [%s]", err_msg(ret));

	passive_signaling(self, signals, priv->pos_interval, priv->vel_interval, priv->loc_interval,
					&(priv->pos_updated_timestamp), &(priv->vel_updated_timestamp),
					&(priv->loc_updated_timestamp), priv->boundary_list, pos, vel, acc);

}

static double _G, _B1, _B2;
static double _u[TIME_SHIFT], _v[TIME_SHIFT];

static bool lp_filter_reset(double sampling_frequency)
{
	double CUTOFF_FREQUENCY = 4.0;
	LOCATION_LOGD("---------- MOVEMENT_THRESHOLD [%.4lf],  IMMOBILITY_THRESHOLD[%.4lf],  CALIBRATION_SAMPLES[%.1lf]----------- ", MOVEMENT_THRESHOLD, IMMOBILITY_THRESHOLD, CALIBRATION_SAMPLES);

	if (sampling_frequency) {
		double omegaC  = tan(M_PI * CUTOFF_FREQUENCY / sampling_frequency);
		double omegaC2 = SQUARE(omegaC);
		double _B0;

		_B0 = 1.0 / (1 + 2 * omegaC * cos(M_PI / 4.0) + omegaC2);
		_B1 = _B0 * 2*(omegaC2 - 1);
		_B2 = _B0 * (1 - 2 * omegaC * cos(M_PI / 4.0) + omegaC2);
		_G  = _B0 * omegaC2;

		_u[T1] = 0; _u[T2] = 0;
		_v[T1] = 0; _v[T2] = 0;
		return true;
	}

	return false;
}

static double lp_filter_process(double u)
{
	double v;

	v  = _G * (u + 2 * _u[T1] + _u[T2]) - _B1 * _v[T1] - _B2 * _v[T2];
	_u[T2] = _u[T1];
	_u[T1] =  u;
	_v[T2] = _v[T1];
	_v[T1] =  v;

	return v;
}

static void set_fused_motion(LocFusedAccelMotion motion, LocationFused *data)
{
	LocationFused *self = (LocationFused *) data;
	LocationFusedPrivate *priv = GET_PRIVATE(self);

	//LOCATION_LOGD("========= Accelerometer Motion [last: %d, new : %d] ", priv->fused_motion, motion);
	if (priv->fused_motion != motion) {
	//LOCATION_LOGD("========= UNDECIDED, MOVEMENT, IMMOBILITY, SLEEP ");
		priv->fused_motion = motion;
		if (motion == MOVEMENT) {
			LOCATION_LOGD("===========================================");
			LOCATION_LOGD("================= MOVEMENT  ===============");
			LOCATION_LOGD("===========================================");
			// change balanced interval
		} else if (motion == SLEEP) {
			LOCATION_LOGD("===========================================");
			LOCATION_LOGD("=================  SLEEP  =================");
			LOCATION_LOGD("===========================================");
			// change interval to max because sleep mode
		}
	}
}
static int cnt = 0;
static void __sensor_event_cb(sensor_h s, sensor_event_s *event, void *data)
{
	LocationFused *self = (LocationFused *)data;
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);
	g_return_if_fail(event);

	guint cur_time = (time_t)((event->timestamp / 1001000) % 100000);
//	LOCATION_LOGD("------t[%d]--- X[%.4lf], Y[%.4lf], Z[%.4lf]", cur_time, event->values[SPATIAL_X], event->values[SPATIAL_Y], event->values[SPATIAL_Z]);
	double s2a;
	double a2 = SQUARE(event->values[SPATIAL_X]) + SQUARE(event->values[SPATIAL_Y]) + SQUARE(event->values[SPATIAL_Z]);

	switch (priv->fused_calibration_status) {
	case COMPLETE:
		s2a = lp_filter_process((a2 - priv->__g2) / 3.0);
		priv->__s22a = SQUARE(s2a) * RMA_REPLACEMENT_RATE +  priv->__s22a * RMA_DECAY_RATE;

		cnt++;
		if (cnt > 10) {
			LOCATION_LOGD("[-COMPLETE] ----- s2a_lp_filter[%.4lf] __s22a[%.4lf], Threshold--> Move[%.2lf] Immobile[%.2lf]",s2a, priv->__s22a, MOVEMENT_THRESHOLD, IMMOBILITY_THRESHOLD);
			cnt = 0;
		}


		if (priv->__s22a > MOVEMENT_THRESHOLD) {
			set_fused_motion(MOVEMENT, self);

		} else if (priv->__s22a < IMMOBILITY_THRESHOLD) {
			switch (priv->fused_motion) {
			case UNDECIDED:
			case MOVEMENT:
				set_fused_motion(IMMOBILITY, self);
				priv->fused_last_moving = cur_time;
				break;

			case IMMOBILITY:
				if (cur_time - priv->fused_last_moving > IMMOBILITY_INTERVAL)
					set_fused_motion(SLEEP, self);
				break;

			default: // SLEEP
				break;
			}
		}

		break;

	case ONGOING:

		cnt++;
		if (cnt > 10) {
			LOCATION_LOGD("[ONGOING] ----- cali_sample[%d] __g2[%.4lf]", priv->fused_calibration_samples, priv->__g2);
			cnt = 0;
		}
		if (priv->fused_calibration_samples < CALIBRATION_SAMPLES) {
			priv->__g2 = priv->__g2 + a2;
			priv->fused_calibration_samples += 1;
		} else {
			priv->__g2 = priv->__g2 / CALIBRATION_SAMPLES;
			priv->fused_calibration_status = COMPLETE;
		}
		break;

	case UNINITIALIZED:
		priv->__g2 = a2;
		priv->fused_calibration_status = ONGOING;
		priv->fused_calibration_samples = 1;
		LOCATION_LOGD("[UNINITIALIZED] ----- cali_sample[%d] __g2[%.4lf]", priv->fused_calibration_samples, priv->__g2);
		break;
	}

}

static int __dynamic_interval_by_motion_detection(LocationFused *self)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	bool supported = false;
	int ret = LOCATION_ERROR_NONE;

	ret = sensor_is_supported(SENSOR_ACCELEROMETER, &supported);
	LOC_IF_FAIL(ret, _E, "Fail to sensor_is_supported [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));
	LOC_COND_RET(!supported, LOCATION_ERROR_NOT_SUPPORTED, _E, "Accelerometer is not supported [%s]", err_msg(LOCATION_ERROR_NOT_SUPPORTED));

	ret = sensor_get_default_sensor(SENSOR_ACCELEROMETER, &priv->sensor);
	LOC_IF_FAIL(ret, _E, "Fail to sensor_get_default_sensor [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

	ret = sensor_create_listener(priv->sensor, &priv->sensor_listener);
	LOC_IF_FAIL(ret, _E, "Fail to sensor_create_listener [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

	ret = sensor_listener_set_event_cb(priv->sensor_listener, 100, __sensor_event_cb, self);
	if (ret != SENSOR_ERROR_NONE) sensor_destroy_listener(priv->sensor_listener);
	LOC_IF_FAIL(ret, _E, "sensor event cb [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

	ret = sensor_listener_start(priv->sensor_listener);
	if (ret != SENSOR_ERROR_NONE) sensor_destroy_listener(priv->sensor_listener);
	LOC_IF_FAIL(ret, _E, "sensor listener start [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

	return LOCATION_ERROR_NONE;
}

static int location_fused_start(LocationFused *self)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.start, LOCATION_ERROR_NOT_AVAILABLE);

	LOC_COND_RET(__get_started(self) == TRUE, LOCATION_ERROR_NONE, _E, "__get_started : TRUE. Error[%s]", err_msg(LOCATION_ERROR_NONE));

	int ret = LOCATION_ERROR_NONE;

	if (!location_setting_get_int(VCONFKEY_LOCATION_ENABLED)) {
		ret = LOCATION_ERROR_SETTING_OFF;
	} else {
		LOCATION_LOGD("-----------------------------------------");

		__set_started(self, TRUE);

		// Fused algorithm - to select GPS or Sensor

		if (priv->fused_mode == LOCATION_FUSED_HIGH) {
			LOCATION_LOGD("------------------HIGH-------------------");

			ret = priv->mod->ops.start(priv->mod->handler, priv->pos_interval, NULL, fused_high_cb, NULL, self);
			if (ret != LOCATION_ERROR_NONE) __set_started(self, FALSE);
			LOC_IF_FAIL(ret, _E, "Fail to start : fused(high accuracy) [%s]", err_msg(ret));


		} else if (priv->fused_mode == LOCATION_FUSED_BALANCED) {
			LOCATION_LOGD("----------------BALANCE-------------------");

			if (priv->fused_timeout != 0)
				g_source_remove(priv->fused_timeout);

			priv->fused_timeout = g_timeout_add_seconds(priv->fused_balanced_interval, __fused_balanced_timeout_cb, self);

			ret = __dynamic_interval_by_motion_detection(self);
			LOC_IF_FAIL_LOG(ret, _E, "Accelerometer is not supported : Balanced power [%s]", err_msg(ret));

		} else if (priv->fused_mode == LOCATION_FUSED_NOPOWER) {
			LOCATION_LOGD("-----------------NO_POWER---------------------");

			// Temporary use WPS vconf, Need to add PASSIVE vconf
			g_signal_emit(self, signals[SERVICE_ENABLED], 0, LOCATION_STATUS_3D_FIX);
			ret = location_state_add_notify(VCONFKEY_LOCATION_RESTRICT, fused_nopower_cb, self);
			LOC_IF_FAIL(ret, _E, "Add vconf notify [%s]", err_msg(ret));
		}
	}

	LOCATION_LOGD("-----------------------------------------");
	if (priv->app_type != CPPAPP && priv->set_noti == FALSE) {
//		location_setting_add_notify(VCONFKEY_LOCATION_ENABLED, location_setting_gps_cb, self);
		priv->set_noti = TRUE;
	}

	return ret;
}

static int location_fused_stop(LocationFused *self)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.stop, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NONE;

	if (__get_started(self) == TRUE) {
		__set_started(self, FALSE);
		ret = priv->mod->ops.stop(priv->mod->handler);
		LOC_IF_FAIL_LOG(ret, _E, "Failed to stop [%s]", err_msg(ret));

		g_signal_emit(self, signals[SERVICE_DISABLED], 0, LOCATION_STATUS_NO_FIX);
	} else {
		LOCATION_LOGD("Fail to fused stop : already stopped [LOCATION_ERROR_NONE]");
		return LOCATION_ERROR_NONE;
	}

	if (priv->fused_timeout) g_source_remove(priv->fused_timeout);
	priv->fused_timeout = 0;

	if (priv->fused_mode == LOCATION_FUSED_BALANCED && priv->sensor_listener) {
		ret = sensor_listener_stop(priv->sensor_listener);
		LOC_IF_FAIL(ret, _E, "Fail to listener_stop [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

		ret = sensor_listener_unset_event_cb(priv->sensor_listener);
		LOC_IF_FAIL_LOG(ret, _E, "Fail to listener_unset_event_cb [%s]", err_msg(ret));

		ret = sensor_destroy_listener(priv->sensor_listener);
		LOC_IF_FAIL_LOG(ret, _E, "Fail to destroy_listener [%s]", err_msg(ret));

		priv->sensor_listener = NULL;
	}

	if (priv->app_type != CPPAPP && priv->set_noti == TRUE) {
//		location_setting_ignore_notify(VCONFKEY_LOCATION_ENABLED, location_setting_gps_cb);
		priv->set_noti = FALSE;
	}

	__reset_pos_data_from_priv(priv);

	return ret;
}

static int location_fused_balanced_stop(LocationFused *self)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod->ops.stop, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LOCATION_ERROR_NONE;

	if (__get_started(self) == TRUE) {
		ret = priv->mod->ops.stop(priv->mod->handler);
		LOC_IF_FAIL_LOG(ret, _E, "Failed to stop [%s]", err_msg(ret));
	} else {
		LOCATION_LOGD("Fail to fused stop : already stopped [LOCATION_ERROR_NONE]");
		return LOCATION_ERROR_NONE;
	}

	if (priv->app_type != CPPAPP && priv->set_noti == TRUE) {
//		location_setting_ignore_notify(VCONFKEY_LOCATION_ENABLED, location_setting_gps_cb);
		priv->set_noti = FALSE;
	}

	return ret;
}

static void location_fused_dispose(GObject *gobject)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);
	g_mutex_clear(&priv->mutex);
	int ret = LOCATION_ERROR_NONE;

	if (priv->fused_timeout) g_source_remove(priv->fused_timeout);
	priv->fused_timeout = 0;

	if (priv->fused_mode == LOCATION_FUSED_BALANCED && priv->sensor_listener) {
		ret = sensor_listener_stop(priv->sensor_listener);
		LOC_IF_FAIL_LOG(ret, _E, "Fail to listener_stop [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

		ret = sensor_listener_unset_event_cb(priv->sensor_listener);
		LOC_IF_FAIL_LOG(ret, _E, "Fail to listener_unset_cb [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

		ret = sensor_destroy_listener(priv->sensor_listener);
		LOC_IF_FAIL_LOG(ret, _E, "Fail to destroy_listener [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));
	}

	if (priv->app_type != CPPAPP && priv->set_noti == TRUE) {
//		location_setting_ignore_notify(VCONFKEY_LOCATION_ENABLED, location_setting_gps_cb);
		priv->set_noti = FALSE;
	}

	G_OBJECT_CLASS(location_fused_parent_class)->dispose(gobject);
}

static void location_fused_finalize(GObject *gobject)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(gobject);
	g_return_if_fail(priv);

	module_free(priv->mod, "fused");
	priv->mod = NULL;

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

	G_OBJECT_CLASS(location_fused_parent_class)->finalize(gobject);
}

static void location_fused_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);
	int ret = LOCATION_ERROR_NONE;

	switch (property_id) {
	case PROP_BOUNDARY: {
		GList *boundary_list = (GList *)g_list_copy(g_value_get_pointer(value));
		ret = set_prop_boundary(&priv->boundary_list, boundary_list);
		LOC_IF_FAIL_LOG(ret, _E, "Set boundary. Error[%s]", err_msg(ret));
		if (boundary_list) g_list_free(boundary_list);
		break;
	}
	case PROP_REMOVAL_BOUNDARY: {
		LocationBoundary *req_boundary = (LocationBoundary *) g_value_dup_boxed(value);
		ret = set_prop_removal_boundary(&priv->boundary_list, req_boundary);
		LOC_IF_FAIL_LOG(ret, _E, "Removal boundary. Error[%s]", err_msg(ret));
		break;
	}
	case PROP_POS_INTERVAL: {
		guint interval = g_value_get_uint(value);
		if (interval > 0) {
			if (interval < LOCATION_UPDATE_INTERVAL_MAX) priv->pos_interval = interval;
			else priv->pos_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
		} else {
			priv->pos_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;
		}
		break;
	}
	case PROP_VEL_INTERVAL: {
		guint interval = g_value_get_uint(value);
		if (interval > 0) {
			if (interval < LOCATION_UPDATE_INTERVAL_MAX) priv->vel_interval = interval;
			else priv->vel_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
		} else {
			priv->vel_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;
		}
		break;
	}
	case PROP_LOC_INTERVAL: {
		guint interval = g_value_get_uint(value);
		LOCATION_LOGD("Set prop>> PROP_LOC_INTERVAL: %u", interval);
		if (interval > 0) {
			if (interval < LOCATION_UPDATE_INTERVAL_MAX) priv->loc_interval = interval;
			else priv->loc_interval = (guint)LOCATION_UPDATE_INTERVAL_MAX;
		} else {
			priv->loc_interval = (guint)LOCATION_UPDATE_INTERVAL_DEFAULT;
		}
		break;
	}
	case PROP_MIN_INTERVAL: {
		guint interval = g_value_get_uint(value);
		LOCATION_LOGD("Set prop>> update-min-interval: %u", interval);
		if (interval > 0) {
			if (interval < LOCATION_MIN_INTERVAL_MAX) priv->min_interval = interval;
			else priv->min_interval = (guint)LOCATION_MIN_INTERVAL_MAX;
		} else {
			priv->min_interval = (guint)LOCATION_MIN_INTERVAL_DEFAULT;
		}
		break;
	}
	case PROP_MIN_DISTANCE: {
		gdouble distance = g_value_get_double(value);
		LOCATION_LOGD("Set prop>> update-min-distance: %u", distance);
		if (distance > 0) {
			if (distance < LOCATION_MIN_DISTANCE_MAX) priv->min_distance = distance;
			else priv->min_distance = (gdouble)LOCATION_MIN_DISTANCE_MAX;
		} else {
			priv->min_distance = (gdouble)LOCATION_MIN_DISTANCE_DEFAULT;
		}
		break;
	}
	case PROP_SERVICE_STATUS: {
		gint enabled = g_value_get_int(value);
		LOCATION_LOGD("Set prop>> PROP_SERVICE_STATUS: %u", enabled);
		priv->enabled = enabled;
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void location_fused_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(object);
	g_return_if_fail(priv);

	switch (property_id) {
		case PROP_METHOD_TYPE:
			g_value_set_int(value, LOCATION_METHOD_FUSED);
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
		case PROP_SERVICE_STATUS:
			g_value_set_int(value, priv->enabled);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
	}
}

static int location_fused_get_position(LocationFused *self, LocationPosition **position, LocationAccuracy **accuracy)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	LOC_COND_RET(__get_started(self) != TRUE, LOCATION_ERROR_NOT_AVAILABLE, _E, "Location is not started [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

	if (priv->pos) {
		*position = location_position_copy(priv->pos);
		if (priv->acc)
			*accuracy = location_accuracy_copy(priv->acc);
		else
			*accuracy = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);

		return LOCATION_ERROR_NONE;
	}

	return LOCATION_ERROR_NOT_AVAILABLE;
}

static int location_fused_get_position_ext(LocationFused *self, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	LOC_COND_RET(__get_started(self) != TRUE, LOCATION_ERROR_NOT_AVAILABLE, _E, "Location is not started [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

	if (priv->pos && priv->vel) {
		*position = location_position_copy(priv->pos);
		*velocity = location_velocity_copy(priv->vel);

		if (priv->acc)
			*accuracy = location_accuracy_copy(priv->acc);
		else
			*accuracy = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);

		return LOCATION_ERROR_NONE;
	}

	return LOCATION_ERROR_NOT_AVAILABLE;
}

static int location_fused_get_velocity(LocationFused *self, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
//	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	LOC_COND_RET(__get_started(self) != TRUE, LOCATION_ERROR_NOT_AVAILABLE, _E, "Location is not started [%s]", err_msg(LOCATION_ERROR_NOT_AVAILABLE));

	if (priv->vel) {
		*velocity = location_velocity_copy(priv->vel);

		if (priv->acc)
			*accuracy = location_accuracy_copy(priv->acc);
		else
			*accuracy = location_accuracy_new(LOCATION_ACCURACY_LEVEL_NONE, 0.0, 0.0);

		return LOCATION_ERROR_NONE;
	}

	return LOCATION_ERROR_NOT_AVAILABLE;
}

static int location_fused_get_last_position(LocationFused *self, LocationPosition **position, LocationAccuracy **accuracy)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	LocModFusedOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);

	LocationVelocity *_velocity = NULL;
	int ret = ops.get_last_position(priv->mod->handler, position, &_velocity, accuracy);
	if (_velocity) location_velocity_free(_velocity);

	return ret;
}

static int location_fused_get_last_position_ext(LocationFused *self, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	LocModFusedOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(ops.get_last_position, LOCATION_ERROR_NOT_AVAILABLE);

	return ops.get_last_position(priv->mod->handler, position, velocity, accuracy);
}

static int location_fused_get_last_velocity(LocationFused *self, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);
	setting_retval_if_fail(VCONFKEY_LOCATION_ENABLED);

	int ret = LOCATION_ERROR_NONE;
	LocationPosition *_position = NULL;

	LocModFusedOps ops = priv->mod->ops;
	g_return_val_if_fail(priv->mod->handler, LOCATION_ERROR_NOT_AVAILABLE);
	ret = ops.get_last_position(priv->mod->handler, &_position, velocity, accuracy);
	if (!_position) location_position_free(_position);

	return ret;
}

static int location_fused_set_option(LocationFused *self, const char *option)
{
	return LOCATION_ERROR_NONE;
}

static int location_fused_interval(LocationFused *self)
{
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(priv->mod, LOCATION_ERROR_NOT_AVAILABLE);

	return LOCATION_ERROR_NONE;
}

static int location_fused_accuracy(LocationFused *self, int mode)
{
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(priv, LOCATION_ERROR_NOT_AVAILABLE);

	priv->fused_mode = mode;

	return LOCATION_ERROR_NONE;
}

static void location_ielement_interface_init(LocationIElementInterface *iface)
{
	iface->start = (TYPE_START_FUNC)location_fused_start;
	iface->stop = (TYPE_STOP_FUNC)location_fused_stop;
	iface->get_position = (TYPE_GET_POSITION)location_fused_get_position;
	iface->get_position_ext = (TYPE_GET_POSITION_EXT)location_fused_get_position_ext;
	iface->get_velocity = (TYPE_GET_VELOCITY)location_fused_get_velocity;
	iface->get_last_position = (TYPE_GET_POSITION)location_fused_get_last_position;
	iface->get_last_position_ext = (TYPE_GET_POSITION_EXT)location_fused_get_last_position_ext;
	iface->get_last_velocity = (TYPE_GET_VELOCITY)location_fused_get_last_velocity;
	iface->set_option = (TYPE_SET_OPTION)location_fused_set_option;
	iface->fused_interval = (TYPE_SET_FUSED_INTERVAL) location_fused_interval;
	iface->fused_accuracy = (TYPE_SET_FUSED_ACCURACY) location_fused_accuracy;
}

static void location_fused_init(LocationFused *self)
{
	LOC_FUNC_LOG
	LocationFusedPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv);

	priv->mod = (LocationFusedMod *)module_new("fused");
	if (!priv->mod) LOCATION_LOGE("module loading failed");

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

	priv->fused_timeout = 0;
	priv->fused_balanced_interval = 20;
	priv->fused_motion = UNDECIDED;
	priv->fused_last_moving = 0;
	priv->__g2 = GRAVITY;
	priv->__s22a = IMMOBILITY_LEVEL;

	priv->fused_mode = LOCATION_FUSED_HIGH;
	priv->fused_calibration_status = UNINITIALIZED;
	priv->fused_calibration_samples = 0;
	lp_filter_reset(SAMPLING_FREQUENCY);

	priv->app_type = location_get_app_type(NULL);
	if (priv->app_type == 0) {
		LOCATION_LOGW("Fail to get app_type");
	}
}

static void location_fused_class_init(LocationFusedClass *klass)
{
	LOC_FUNC_LOG
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->set_property = location_fused_set_property;
	gobject_class->get_property = location_fused_get_property;

	gobject_class->dispose = location_fused_dispose;
	gobject_class->finalize = location_fused_finalize;

	g_type_class_add_private(klass, sizeof(LocationFusedPrivate));

	signals[SERVICE_ENABLED] = g_signal_new("service-enabled", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET(LocationFusedClass, enabled), NULL, NULL, location_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
	signals[SERVICE_DISABLED] = g_signal_new("service-disabled", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET(LocationFusedClass, disabled), NULL, NULL, location_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);

#if 0 /* TODO: STATUS_CHANGED will aggregate SERVICE_ENABLED and SERVICE_DISABLED */
	signals[STATUS_CHANGED] = g_signal_new("status-changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET(LocationFusedClass, status_changed), NULL, NULL, location_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
#endif

	signals[SERVICE_UPDATED] = g_signal_new("service-updated", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET(LocationFusedClass, service_updated), NULL, NULL, location_VOID__INT_POINTER_POINTER_POINTER, G_TYPE_NONE, 4, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
	signals[LOCATION_UPDATED] = g_signal_new("location-updated", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET(LocationFusedClass, location_updated), NULL, NULL, location_VOID__INT_POINTER_POINTER_POINTER, G_TYPE_NONE, 4, G_TYPE_INT, G_TYPE_POINTER,G_TYPE_POINTER, G_TYPE_POINTER);
	signals[ZONE_IN] = g_signal_new("zone-in", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET(LocationFusedClass, zone_in), NULL, NULL, location_VOID__POINTER_POINTER_POINTER,
						G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
	signals[ZONE_OUT] = g_signal_new("zone-out", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE, G_STRUCT_OFFSET(LocationFusedClass, zone_out), NULL, NULL, location_VOID__POINTER_POINTER_POINTER, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);

	properties[PROP_METHOD_TYPE] = g_param_spec_int("method", "method type", "location method type name", LOCATION_METHOD_FUSED, LOCATION_METHOD_FUSED, LOCATION_METHOD_FUSED, G_PARAM_READABLE);
	properties[PROP_IS_STARTED] = g_param_spec_boolean("is_started", "fused is started prop", "fused is started status", FALSE, G_PARAM_READWRITE);
	properties[PROP_LAST_POSITION] = g_param_spec_boxed("last-position", "fused last position prop", "fused last position data", LOCATION_TYPE_POSITION, G_PARAM_READABLE);
	properties[PROP_POS_INTERVAL] = g_param_spec_uint("pos-interval", "pos-int prop", "pos-int data", LOCATION_UPDATE_INTERVAL_MIN, LOCATION_UPDATE_INTERVAL_MAX, LOCATION_UPDATE_INTERVAL_DEFAULT, G_PARAM_READWRITE);
	properties[PROP_VEL_INTERVAL] = g_param_spec_uint("vel-interval", "vel-int prop", "vel-int data", LOCATION_UPDATE_INTERVAL_MIN, LOCATION_UPDATE_INTERVAL_MAX, LOCATION_UPDATE_INTERVAL_DEFAULT,G_PARAM_READWRITE);
	properties[PROP_LOC_INTERVAL] = g_param_spec_uint("loc-interval", "loc-int prop", "loc-int data", LOCATION_UPDATE_INTERVAL_MIN, LOCATION_UPDATE_INTERVAL_MAX, LOCATION_UPDATE_INTERVAL_DEFAULT, G_PARAM_READWRITE);
	properties[PROP_MIN_INTERVAL] = g_param_spec_uint("min-interval", "min-int prop", "min-int data", LOCATION_MIN_INTERVAL_MIN, LOCATION_MIN_INTERVAL_MAX, LOCATION_MIN_INTERVAL_DEFAULT, G_PARAM_READWRITE);
	properties[PROP_MIN_DISTANCE] = g_param_spec_double("min-distance", "dist prop", "dist data", LOCATION_MIN_DISTANCE_MIN, LOCATION_MIN_DISTANCE_MAX, LOCATION_MIN_DISTANCE_DEFAULT, G_PARAM_READWRITE);
	properties[PROP_BOUNDARY] = g_param_spec_pointer("boundary", "fused boundary prop", "fused boundary data", G_PARAM_READWRITE);
	properties[PROP_REMOVAL_BOUNDARY] = g_param_spec_boxed("removal-boundary", "fused removal boundary prop", "fused removal boundary data", LOCATION_TYPE_BOUNDARY, G_PARAM_READWRITE);

	/* Tizen 3.0 */
	properties[PROP_SERVICE_STATUS] = g_param_spec_int("service-status", "location service status prop", "location service status data", LOCATION_STATUS_NO_FIX, LOCATION_STATUS_3D_FIX, LOCATION_STATUS_NO_FIX, G_PARAM_READABLE);

	g_object_class_install_properties(gobject_class, PROP_MAX, properties);
}

