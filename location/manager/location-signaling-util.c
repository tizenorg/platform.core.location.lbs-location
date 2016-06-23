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

#include "location-signaling-util.h"
#include "location-common-util.h"
#include "location-log.h"
#include "location-position.h"
#include "vconf.h"
//#include "vconf-internal-location-keys.h"

void
enable_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL], gboolean *prev_enabled, gboolean enabled, LocationStatus status)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(prev_enabled);

	if (*prev_enabled == TRUE && enabled == FALSE) {
		*prev_enabled = FALSE;
		LOCATION_LOGD("Signal emit: SERVICE_DISABLED, status = %d", status);
		g_signal_emit(obj, signals[SERVICE_DISABLED], 0, LOCATION_STATUS_NO_FIX);
		/* g_signal_emit(obj, signals[STATUS_CHANGED], 0, LOCATION_STATUS_NO_FIX); */
	} else if (*prev_enabled == FALSE && enabled == TRUE) {
		*prev_enabled = TRUE;
		LOCATION_LOGD("Signal emit: SERVICE_ENABLED, status = %d", status);
		g_signal_emit(obj, signals[SERVICE_ENABLED], 0, status);
		/* g_signal_emit(obj, signals[STATUS_CHANGED], 0, status); */
	}
#if 0
	} else if (*prev_enabled != enabled) {
		LOCATION_LOGD("Signal emit: prev_enabled = %d, enabled = %d, status = %d", *prev_enabled, enabled, status);
		/* g_signal_emit(obj, signals[STATUS_CHANGED], 0, status); */
	}
#endif
}

void
position_velocity_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL],
			guint pos_interval, guint vel_interval, guint loc_interval,
			guint *pos_last_timestamp, guint *vel_last_timestamp, guint *loc_last_timestamp,
			GList *prev_bound, LocationPosition *cur_pos, LocationVelocity *cur_vel, LocationAccuracy *cur_acc)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(cur_pos);
	g_return_if_fail(cur_vel);
	g_return_if_fail(cur_acc);

	int index = 0;
	int signal_type = 0;
	gboolean is_inside = FALSE;
	GList *boundary_list = prev_bound;
	LocationBoundaryPrivate *priv = NULL;

	if (cur_pos && !cur_pos->timestamp) {
		LOCATION_LOGW("Invalid location with timestamp, 0");
		return;
	}

	if (pos_interval > 0) {
		if (cur_pos->timestamp - *pos_last_timestamp >= pos_interval) {
			signal_type |= POSITION_UPDATED;
			*pos_last_timestamp = cur_pos->timestamp;
		}
	}

	if (vel_interval > 0) {
		if (cur_vel && (cur_vel->timestamp - *vel_last_timestamp >= vel_interval)) {
			signal_type |= VELOCITY_UPDATED;
			*vel_last_timestamp = cur_vel->timestamp;
		}
	}

	if (loc_interval > 0) {
		if (cur_pos->timestamp - *loc_last_timestamp >= loc_interval) {
			signal_type |= LOCATION_CHANGED;
			*loc_last_timestamp = cur_pos->timestamp;
		}
	}

	if (signal_type != 0) {
		g_signal_emit(obj, signals[SERVICE_UPDATED], 0, signal_type, cur_pos, cur_vel, cur_acc);

		LOCATION_LOGW("=================== Passive : vconf updated ======================");
		int ret = vconf_set_int(VCONFKEY_LOCATION_RESTRICT, 0);
		LOC_IF_FAIL_LOG(ret, _E, "vconf set failed!!");
	}

	if (boundary_list) {
		while ((priv = (LocationBoundaryPrivate *)g_list_nth_data(boundary_list, index)) != NULL) {
			is_inside = location_boundary_if_inside(priv->boundary, cur_pos);
			if (is_inside) {
				if (priv->zone_status != ZONE_STATUS_IN) {
					LOCATION_LOGD("Signal emit: ZONE IN");
					g_signal_emit(obj, signals[ZONE_IN], 0, priv->boundary, cur_pos, cur_acc);
					priv->zone_status = ZONE_STATUS_IN;
				}
			} else {
				if (priv->zone_status != ZONE_STATUS_OUT) {
					LOCATION_LOGD("Signal emit : ZONE_OUT");
					g_signal_emit(obj, signals[ZONE_OUT], 0, priv->boundary, cur_pos, cur_acc);
					priv->zone_status = ZONE_STATUS_OUT;
				}
			}
			index++;
		}
	}
}

void
passive_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL],
				guint pos_interval, guint vel_interval, guint loc_interval,
				guint *pos_updated_timestamp, guint *vel_updated_timestamp, guint *loc_updated_timestamp,
				GList *prev_bound, LocationPosition *cur_pos, LocationVelocity *cur_vel, LocationAccuracy *cur_acc)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(cur_pos);

	int index = 0;
	int signal_type = 0;
	gboolean is_inside = FALSE;
	GList *boundary_list = prev_bound;
	LocationBoundaryPrivate *priv = NULL;

	if (cur_pos && !cur_pos->timestamp) {
		LOCATION_LOGW("Invalid location with timestamp, 0");
		return;
	}

	if (pos_interval > 0) {
		if (cur_pos->timestamp - *pos_updated_timestamp >= pos_interval) {
			signal_type |= POSITION_UPDATED;
			*pos_updated_timestamp = cur_pos->timestamp;
		}
	}

	if (vel_interval > 0) {
		if (cur_vel && (cur_vel->timestamp - *vel_updated_timestamp >= vel_interval)) {
			signal_type |= VELOCITY_UPDATED;
			*vel_updated_timestamp = cur_vel->timestamp;
		}
	}

	if (loc_interval > 0) {
		if (cur_pos->timestamp - *loc_updated_timestamp >= loc_interval) {
			signal_type |= LOCATION_CHANGED;
			*loc_updated_timestamp = cur_pos->timestamp;
		}
	}

	if (signal_type != 0)
		g_signal_emit(obj, signals[SERVICE_UPDATED], 0, signal_type, cur_pos, cur_vel, cur_acc);

	if (boundary_list) {
		while ((priv = (LocationBoundaryPrivate *)g_list_nth_data(boundary_list, index)) != NULL) {
			is_inside = location_boundary_if_inside(priv->boundary, cur_pos);
			if (is_inside) {
				if (priv->zone_status != ZONE_STATUS_IN) {
					LOCATION_LOGD("Signal emit: ZONE IN");
					g_signal_emit(obj, signals[ZONE_IN], 0, priv->boundary, cur_pos, cur_acc);
					priv->zone_status = ZONE_STATUS_IN;
				}
			} else {
				if (priv->zone_status != ZONE_STATUS_OUT) {
					LOCATION_LOGD("Signal emit : ZONE_OUT");
					g_signal_emit(obj, signals[ZONE_OUT], 0, priv->boundary, cur_pos, cur_acc);
					priv->zone_status = ZONE_STATUS_OUT;
				}
			}
			index++;
		}
	}
}

void
distance_based_position_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL], gboolean enabled,
				LocationPosition *cur_pos, LocationVelocity *cur_vel, LocationAccuracy *cur_acc,
				guint min_interval, gdouble min_distance, gboolean *prev_enabled, guint *prev_dist_timestamp,
				LocationPosition **prev_pos, LocationVelocity **prev_vel, LocationAccuracy **prev_acc)
{
	LOC_FUNC_LOG
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(cur_pos);

	if (!cur_pos->timestamp) {
		LOCATION_LOGE("Invalid location with timestamp, 0");
		return;
	}

	enable_signaling(obj, signals, prev_enabled, enabled, cur_pos->status);

	if (cur_pos->timestamp - *prev_dist_timestamp >= min_interval) {
		g_signal_emit(obj, signals[SERVICE_UPDATED], 0, DISTANCE_UPDATED, cur_pos, cur_vel, cur_acc);
		*prev_dist_timestamp = cur_pos->timestamp;

		if (*prev_pos) location_position_free(*prev_pos);
		if (*prev_vel) location_velocity_free(*prev_vel);
		if (*prev_acc) location_accuracy_free(*prev_acc);

		*prev_pos = location_position_copy(cur_pos);
		*prev_vel = location_velocity_copy(cur_vel);
		*prev_acc = location_accuracy_copy(cur_acc);

	} else {
		gulong distance;
		int ret = location_get_distance(*prev_pos, cur_pos, &distance);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_LOGE("Fail to get distance");
			return;
		}

		if (distance >= min_distance) {
			g_signal_emit(obj, signals[SERVICE_UPDATED], 0, DISTANCE_UPDATED, cur_pos, cur_vel, cur_acc);
			*prev_dist_timestamp = cur_pos->timestamp;

			if (*prev_pos) location_position_free(*prev_pos);
			if (*prev_vel) location_velocity_free(*prev_vel);
			if (*prev_acc) location_accuracy_free(*prev_acc);

			*prev_pos = location_position_copy(cur_pos);
			*prev_vel = location_velocity_copy(cur_vel);
			*prev_acc = location_accuracy_copy(cur_acc);
		}
	}
}

void
location_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL], gboolean enabled, GList *boundary_list,
					LocationPosition *cur_pos, LocationVelocity *cur_vel, LocationAccuracy *cur_acc,
					guint pos_interval, guint vel_interval, guint loc_interval, gboolean *prev_enabled,
					guint *prev_pos_timestamp, guint *prev_vel_timestamp, guint *prev_loc_timestamp,
					LocationPosition **prev_pos, LocationVelocity **prev_vel, LocationAccuracy **prev_acc)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(cur_pos);
	g_return_if_fail(cur_vel);
	g_return_if_fail(cur_acc);

	if (!cur_pos->timestamp) {
		LOCATION_LOGD("Invalid location with timestamp, 0");
		return;
	}

	if (*prev_pos) location_position_free(*prev_pos);
	if (*prev_vel) location_velocity_free(*prev_vel);
	if (*prev_acc) location_accuracy_free(*prev_acc);

	*prev_pos = location_position_copy(cur_pos);
	*prev_vel = location_velocity_copy(cur_vel);
	*prev_acc = location_accuracy_copy(cur_acc);

	LOCATION_LOGD("cur_pos->status = %d", cur_pos->status);
	enable_signaling(obj, signals, prev_enabled, enabled, cur_pos->status);
	position_velocity_signaling(obj, signals, pos_interval, vel_interval, loc_interval, prev_pos_timestamp, prev_vel_timestamp, prev_loc_timestamp, boundary_list, cur_pos, cur_vel, cur_acc);
}

void
satellite_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL], gboolean *prev_enabled, int interval,
					gboolean emit, guint *last_timestamp, LocationSatellite **prev_sat, LocationSatellite *sat)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(sat);

	if (!sat->timestamp) return;

	if (*prev_sat) location_satellite_free(*prev_sat);
	*prev_sat = location_satellite_copy(sat);

	if (emit && sat->timestamp - *last_timestamp >= interval) {
		g_signal_emit(obj, signals[SERVICE_UPDATED], 0, SATELLITE_UPDATED, sat, NULL, NULL);
		*last_timestamp = sat->timestamp;
	}
}

