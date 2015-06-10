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

#ifndef __LOCATION_SIGNALING_UTIL_H__
#define __LOCATION_SIGNALING_UTIL_H__

#include <location.h>
#include <location-ielement.h>

/**
 * @file location-signaling.h
 * @brief This file contains the signaling utils for LocationObject.
 */

G_BEGIN_DECLS

void enable_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL], gboolean *prev_enabled, gboolean enabled, LocationStatus status);

void position_velocity_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL],
                                 guint pos_interval,	guint vel_interval,	guint loc_interval,
                                 guint *pos_updated_timestamp, guint *vel_updated_timestamp, guint *loc_updated_timestamp,
                                 GList *prev_bound, LocationPosition *pos, LocationVelocity *vel, LocationAccuracy *acc);

void satellite_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL],
                         gboolean *prev_enabled, int interval, gboolean emit,
                         guint *updated_timestamp, LocationSatellite **pre_sat,
                         LocationSatellite *sat);

void location_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL],
                        gboolean enabled, GList *boundary_list,
                        LocationPosition *cur_pos, LocationVelocity *cur_vel, LocationAccuracy *cur_acc,
                        guint pos_interval, guint vel_interval, guint loc_interval,
                        gboolean *prev_enabled, guint *prev_pos_timestamp, guint *prev_vel_timestamp, guint *prev_loc_timestamp,
                        LocationPosition **prev_pos, LocationVelocity **prev_vel, LocationAccuracy **prev_acc);

void distance_based_position_signaling(LocationObject *obj, guint32 signals[LAST_SIGNAL],
                                       gboolean enabled,
                                       LocationPosition *cur_pos, LocationVelocity *cur_vel, LocationAccuracy *cur_acc,
                                       guint min_interval, gdouble min_distance, gboolean *prev_enabled, guint *prev_dist_timestamp,
                                       LocationPosition **prev_pos, LocationVelocity **prev_vel, LocationAccuracy **prev_acc);

G_END_DECLS

#endif
