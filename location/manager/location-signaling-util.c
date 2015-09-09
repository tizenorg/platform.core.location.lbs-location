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


void
enable_signaling(LocationObject *obj,
                 guint32 signals[LAST_SIGNAL],
                 gboolean *prev_enabled,
                 gboolean enabled,
                 LocationStatus status)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(prev_enabled);
	if (*prev_enabled == TRUE && enabled == FALSE) {
		*prev_enabled = FALSE;
		LOCATION_LOGD("Signal emit: SERVICE_DISABLED");
		g_signal_emit(obj, signals[SERVICE_DISABLED], 0, LOCATION_STATUS_NO_FIX);
	} else if (*prev_enabled == FALSE && enabled == TRUE) {
		*prev_enabled = TRUE;
		LOCATION_LOGD("Signal emit: SERVICE_ENABLED");
		g_signal_emit(obj, signals[SERVICE_ENABLED], 0, status);
	}
}

void
position_velocity_signaling(LocationObject *obj,
                            guint32 signals[LAST_SIGNAL],
                            guint pos_interval,
                            guint vel_interval,
                            guint loc_interval,
                            guint *pos_updated_timestamp,
                            guint *vel_updated_timestamp,
                            guint *loc_updated_timestamp,
                            GList *prev_bound,
                            LocationPosition *pos,
                            LocationVelocity *vel,
                            LocationAccuracy *acc)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(pos);

	int index = 0;
	int signal_type = 0;
	gboolean is_inside = FALSE;
	GList *boundary_list = prev_bound;
	LocationBoundaryPrivate *priv = NULL;

	if (!pos->timestamp) return;

	if (pos_interval > 0) {
		if (pos->timestamp - *pos_updated_timestamp >= pos_interval) {
			signal_type |= POSITION_UPDATED;
			*pos_updated_timestamp = pos->timestamp;
		}
	}

	if (vel_interval > 0) {
		if (vel->timestamp - *vel_updated_timestamp >= vel_interval) {
			signal_type |= VELOCITY_UPDATED;
			*vel_updated_timestamp = vel->timestamp;
		}
	}

	if (loc_interval > 0) {
		if (pos->timestamp - *loc_updated_timestamp >= loc_interval) {
			signal_type |= LOCATION_CHANGED;
			*loc_updated_timestamp = pos->timestamp;
		}
	}

	if (signal_type != 0) {
		g_signal_emit(obj, signals[SERVICE_UPDATED], 0, signal_type, pos, vel, acc);
	}

	if (boundary_list) {
		while ((priv = (LocationBoundaryPrivate *)g_list_nth_data(boundary_list, index)) != NULL) {
			is_inside = location_boundary_if_inside(priv->boundary, pos);
			if (is_inside) {
				if (priv->zone_status != ZONE_STATUS_IN) {
					LOCATION_LOGD("Signal emit: ZONE IN");
					g_signal_emit(obj, signals[ZONE_IN], 0, priv->boundary, pos, acc);
					priv->zone_status = ZONE_STATUS_IN;
				}
			} else {
				if (priv->zone_status != ZONE_STATUS_OUT) {
					LOCATION_LOGD("Signal emit : ZONE_OUT");
					g_signal_emit(obj, signals[ZONE_OUT], 0, priv->boundary, pos, acc);
					priv->zone_status = ZONE_STATUS_OUT;
				}
			}
			index++;
		}
	}
}

void
distance_based_position_signaling(LocationObject *obj,
                                  guint32 signals[LAST_SIGNAL],
                                  gboolean enabled,
                                  LocationPosition *cur_pos,
                                  LocationVelocity *cur_vel,
                                  LocationAccuracy *cur_acc,
                                  guint min_interval,
                                  gdouble min_distance,
                                  gboolean *prev_enabled,
                                  guint *prev_dist_timestamp,
                                  LocationPosition **prev_pos,	/* prev : keeping lastest info. */
                                  LocationVelocity **prev_vel,
                                  LocationAccuracy **prev_acc)
{
	if (!cur_pos->timestamp) {
		LOCATION_LOGE("Invalid location with timestamp, 0");
		return;
	}

	enable_signaling(obj, signals, prev_enabled, enabled, cur_pos->status);

	if (cur_pos->timestamp - *prev_dist_timestamp >= min_interval) {
		g_signal_emit(obj, signals[SERVICE_UPDATED], 0, DISTANCE_UPDATED, cur_pos, cur_vel, cur_acc);
		*prev_dist_timestamp = cur_pos->timestamp;

		if (*prev_pos) location_position_free(*prev_pos);
		*prev_pos = location_position_copy(cur_pos);

	} else {
		gulong distance;
		int ret = location_get_distance(*prev_pos, cur_pos, &distance);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_LOGE("Fail to get distance");
			return;
		}

		if (distance > min_distance) {
			g_signal_emit(obj, signals[SERVICE_UPDATED], 0, DISTANCE_UPDATED, cur_pos, cur_vel, cur_acc);
			*prev_dist_timestamp = cur_pos->timestamp;

			if (*prev_pos) location_position_free(*prev_pos);
			*prev_pos = location_position_copy(cur_pos);
		}
	}
}

void
location_signaling(LocationObject *obj,
                   guint32 signals[LAST_SIGNAL],
                   gboolean enabled,
                   GList *boundary_list,
                   LocationPosition *cur_pos,
                   LocationVelocity *cur_vel,
                   LocationAccuracy *cur_acc,
                   guint pos_interval,			/* interval : support an update interval */
                   guint vel_interval,
                   guint loc_interval,
                   gboolean *prev_enabled,
                   guint *prev_pos_timestamp,
                   guint *prev_vel_timestamp,
                   guint *prev_loc_timestamp,
                   LocationPosition **prev_pos,	/* prev : keeping lastest info. */
                   LocationVelocity **prev_vel,
                   LocationAccuracy **prev_acc)
{
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

	enable_signaling(obj, signals, prev_enabled, enabled, cur_pos->status);
	position_velocity_signaling(obj, signals, pos_interval, vel_interval, loc_interval, prev_pos_timestamp, prev_vel_timestamp, prev_loc_timestamp, boundary_list, cur_pos, cur_vel, cur_acc);
}

void
satellite_signaling(LocationObject *obj,
                    guint32 signals[LAST_SIGNAL],
                    gboolean *prev_enabled,
                    int interval,
                    gboolean emit,
                    guint *updated_timestamp,
                    LocationSatellite **prev_sat,
                    LocationSatellite *sat)
{
	g_return_if_fail(obj);
	g_return_if_fail(signals);
	g_return_if_fail(sat);

	if (!sat->timestamp) return;

	if (*prev_sat) location_satellite_free(*prev_sat);
	*prev_sat = location_satellite_copy(sat);

	if (emit && sat->timestamp - *updated_timestamp >= interval) {
		g_signal_emit(obj, signals[SERVICE_UPDATED], 0, SATELLITE_UPDATED, sat, NULL, NULL);
		*updated_timestamp = sat->timestamp;
	}
}

