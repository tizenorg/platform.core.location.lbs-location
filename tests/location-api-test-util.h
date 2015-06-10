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


#ifndef __LOCATION_API_TEST_UTIL_H__
#define __LOCATION_API_TEST_UTIL_H__

#include "location.h"

G_BEGIN_DECLS

void json_util_init(const char *file_file);
char *json_util_result_zone_test(int polygon_index, int marker_index);
char *json_util_get_marker_name(int polygon_index, int marker_index);
int json_util_get_marker_position_count(int polygon_index);
LocationPosition *json_util_get_marker_position(int polygon_index, int marker_index);

int json_util_get_polygon_count(void);
char *json_util_get_polygon_name(int polygon_index);
LocationBoundary *json_util_get_polygon_boundary(int polygon_index);

G_END_DECLS

#endif /* __LOCATION_H__ */
