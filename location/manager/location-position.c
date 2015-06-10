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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "location-position.h"
#include "location-setting.h"
#include "location-log.h"

#define		DEG2RAD(x)	((x) * M_PI / 180)

GType
location_position_get_type(void)
{
	static volatile gsize type_volatile = 0;
	if (g_once_init_enter(&type_volatile)) {
		GType type = g_boxed_type_register_static(
		                 g_intern_static_string("LocationPosition"),
		                 (GBoxedCopyFunc) location_position_copy,
		                 (GBoxedFreeFunc) location_position_free);
		g_once_init_leave(&type_volatile, type);
	}
	return type_volatile;
}

EXPORT_API LocationPosition *
location_position_new(guint timestamp,
                      gdouble latitude,
                      gdouble longitude,
                      gdouble altitude,
                      LocationStatus status)
{
	if (latitude < -90 || latitude > 90) return NULL;
	if (longitude < -180 || longitude > 180) return NULL;

	LocationPosition *position = g_slice_new0(LocationPosition);
	g_return_val_if_fail(position, NULL);

	position->timestamp = timestamp;
	position->latitude = latitude;
	position->longitude = longitude;
	position->altitude = altitude;
	position->status = status;
	return position;
}

EXPORT_API void
location_position_free(LocationPosition *position)
{
	g_return_if_fail(position);
	g_slice_free(LocationPosition, position);
}

EXPORT_API gboolean
location_position_equal(const LocationPosition *position1, const LocationPosition *position2)
{
	g_return_val_if_fail(position1, FALSE);
	g_return_val_if_fail(position2, FALSE);

	if (position1->latitude == position2->latitude &&
	    position1->longitude == position2->longitude &&
	    position1->altitude == position2->altitude)
		return TRUE;
	return FALSE;
}

EXPORT_API LocationPosition *
location_position_copy(const LocationPosition *position)
{
	g_return_val_if_fail(position, NULL);

	LocationPosition *new_position = NULL;

	new_position = location_position_new(position->timestamp,
	                                     position->latitude,
	                                     position->longitude,
	                                     position->altitude,
	                                     position->status);

	return new_position;

}

/* Vincenty formula. WGS-84 */
EXPORT_API int
location_get_distance(const LocationPosition *pos1, const LocationPosition *pos2, gulong *distance)
{
	g_return_val_if_fail(pos1, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(pos2, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(distance, LOCATION_ERROR_PARAMETER);

	*distance = 0;

	const double a = 6378137.0, b = 6356752.314245, f = 1 / 298.257223563;
	double delta_lon = DEG2RAD(pos2->longitude - pos1->longitude);
	double u_1 = atan((1 - f) * tan(DEG2RAD(pos1->latitude)));
	double u_2 = atan((1 - f) * tan(DEG2RAD(pos2->latitude)));

	double lambdaP, iter_limit = 100.0;
	double lambda = delta_lon;

	double sin_sigma, sin_alpha, cos_sigma, sigma, sq_cos_alpha, cos_2sigma, C;
	double sq_u, cal1, cal2, delta_sigma, cal_dist;
	double sin_lambda, cos_lambda;

	double sin_u1 = sin(u_1);
	double cos_u1 = cos(u_1);
	double sin_u2 = sin(u_2);
	double cos_u2 = cos(u_2);

	do {
		sin_lambda = sin(lambda);
		cos_lambda = cos(lambda);

		sin_sigma = sqrt((cos_u2 * sin_lambda) * (cos_u2 * sin_lambda) + \
		                 (cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda) * \
		                 (cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda));

		if (sin_sigma == 0)
			return LOCATION_ERROR_NONE;	/* co-incident points */

		cos_sigma = sin_u1 * sin_u2 + cos_u1 * cos_u2 * cos_lambda;
		sigma = atan2(sin_sigma, cos_sigma);

		sin_alpha = cos_u1 * cos_u2 * sin_lambda / sin_sigma;
		sq_cos_alpha = 1.0 - sin_alpha * sin_alpha;
		cos_2sigma = cos_sigma - 2.0 * sin_u1 * sin_u2 / sq_cos_alpha;

		if (isnan(cos_2sigma))
			cos_2sigma = 0;

		C = f / 16.0 * sq_cos_alpha * (4.0 + f * (4.0 - 3.0 * sq_cos_alpha));

		lambdaP = lambda;
		lambda = delta_lon + (1.0 - C) * f * sin_alpha * \
		         (sigma + C * sin_sigma * (cos_2sigma + C * cos_sigma * (-1.0 + 2.0 * cos_2sigma * cos_2sigma)));

	} while (abs(lambda - lambdaP) > 1e-12 && --iter_limit > 0);

	if (iter_limit == 0) return LOCATION_ERROR_UNKNOWN;

	sq_u = sq_cos_alpha * (a * a - b * b) / (b * b);

	cal1 = 1.0 + sq_u / 16384.0 * (4096.0 + sq_u * (-768.0 + sq_u * (320.0 - 175.0 * sq_u)));
	cal2 = sq_u / 1024.0 * (256.0 + sq_u * (-128.0 + sq_u * (74.0 - 47.0 * sq_u)));

	delta_sigma = cal2 * sin_sigma * (cos_2sigma + cal2 / 4.0 * (cos_sigma * (-1.0 + 2.0 * cos_2sigma * cos_2sigma) - \
	                                                             cal2 / 6.0 * cos_2sigma * (-3.0 + 4.0 * sin_sigma * sin_sigma) * (-3.0 + 4.0 * cos_2sigma * cos_2sigma)));
	cal_dist = b * cal1 * (sigma - delta_sigma);

	*distance = (gulong) cal_dist;

	return LOCATION_ERROR_NONE;

}

EXPORT_API void
location_last_position_a2i(char *position, int *lat, int *lon)
{
	char *d_lat, *d_lon;
	char latitude[HALF_KEY_LENGTH];
	char longitude[HALF_KEY_LENGTH];

	memcpy(latitude, position + 1, HALF_KEY_LENGTH - 1);
	memcpy(longitude, position + HALF_KEY_LENGTH + 1, HALF_KEY_LENGTH - 1);
	latitude[HALF_KEY_LENGTH - 1] = '\0';
	longitude[HALF_KEY_LENGTH - 1] = '\0';
	d_lat = position;
	d_lon = position + HALF_KEY_LENGTH;

	*lat = atoi(latitude);
	*lon = atoi(longitude);

	if (*d_lat == 'S') {
		*lat = *lat * -1;
	}
	if (*d_lon == 'W') {
		*lon = *lon * -1;
	}
}
