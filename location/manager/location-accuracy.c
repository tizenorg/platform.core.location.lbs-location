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

#include "location-accuracy.h"
#include "location-log.h"

GType
location_accuracy_get_type(void)
{
	static volatile gsize type_volatile = 0;
	if (g_once_init_enter(&type_volatile)) {
		GType type = g_boxed_type_register_static(
		                 g_intern_static_string("LocationAccuracy"),
		                 (GBoxedCopyFunc) location_accuracy_copy,
		                 (GBoxedFreeFunc) location_accuracy_free);
		g_once_init_leave(&type_volatile, type);
	}
	return type_volatile;
}

EXPORT_API LocationAccuracy *
location_accuracy_new(LocationAccuracyLevel level,
                      gdouble horizontal_accuracy,
                      gdouble vertical_accuracy)
{
	LocationAccuracy *accuracy = g_slice_new0(LocationAccuracy);
	g_return_val_if_fail(accuracy, NULL);

	accuracy->level = level;
	accuracy->horizontal_accuracy = horizontal_accuracy;
	accuracy->vertical_accuracy = vertical_accuracy;

	return accuracy;
}

EXPORT_API void
location_accuracy_free(LocationAccuracy *accuracy)
{
	g_return_if_fail(accuracy);
	g_slice_free(LocationAccuracy, accuracy);
}

static int
comp_int(int a, int b)
{
	if (a < b) return -1;
	if (a == b) return 0;
	return 1;
}

static int
comp_double_reverse(double a, double b)
{
	if (a > b) return -1;
	else if (a == b) return 0;
	return 1;
}

EXPORT_API int
location_accuracy_level_compare(const LocationAccuracy *accuracy1, const LocationAccuracy *accuracy2)
{
	g_return_val_if_fail(accuracy1, -1);
	g_return_val_if_fail(accuracy2, 1);

	return comp_int(accuracy1->level, accuracy2->level);
}

EXPORT_API int
location_accuracy_compare(const LocationAccuracy *accuracy1, const LocationAccuracy *accuracy2)
{
	int ret = 0;
	ret = location_accuracy_level_compare(accuracy1, accuracy2);
	if (!ret) {
		ret = comp_double_reverse(accuracy1->horizontal_accuracy, accuracy2->horizontal_accuracy);
		if (!ret) return comp_double_reverse(accuracy1->vertical_accuracy, accuracy2->vertical_accuracy);
	}
	return ret;
}

EXPORT_API LocationAccuracy *
location_accuracy_copy(const LocationAccuracy *accuracy)
{
	g_return_val_if_fail(accuracy, NULL);
	return location_accuracy_new(accuracy->level,
	                             accuracy->horizontal_accuracy,
	                             accuracy->vertical_accuracy);
}
