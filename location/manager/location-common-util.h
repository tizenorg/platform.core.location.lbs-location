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

#ifndef __LOCATION_COMMON_UTIL_H__
#define __LOCATION_COMMON_UTIL_H__

#include <location.h>
#include <location-ielement.h>
#include <location-boundary.h>

/**
 * @file location-common-util.h
 * @brief This file contains the common utils for LocationObject.
 */

G_BEGIN_DECLS

#define CAPP	1
#define WEBAPP	2
#define CPPAPP	3

typedef enum {
    LOCATION_APP_OFF = 0,
    LOCATION_APP_ON,
    LOCATION_APP_NOT_FOUND
} LOCATION_APP_STATE;

typedef enum {
    ZONE_STATUS_NONE = 0,
    ZONE_STATUS_IN,
    ZONE_STATUS_OUT,
} ZoneStatus;

typedef struct _LocationBoundaryPrivate {
	LocationBoundary *boundary;
	ZoneStatus zone_status;
} LocationBoundaryPrivate;

int location_application_get_authority(void);
int location_application_set_authority(int auth);

int set_prop_boundary(GList **prev_boundary_list, GList *new_boundary_list);
int set_prop_removal_boundary(GList **prev_boundary_list, LocationBoundary *boundary);
void free_boundary_list(gpointer data);

int location_get_app_type(char *app_id);


G_END_DECLS

#endif
