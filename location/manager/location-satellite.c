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

#include "location-satellite.h"
#include "location-log.h"

GType
location_satellite_get_type(void)
{
	static volatile gsize type_volatile = 0;
	if (g_once_init_enter(&type_volatile)) {
		GType type = g_boxed_type_register_static(
		                 g_intern_static_string("LocationSatellite"),
		                 (GBoxedCopyFunc) location_satellite_copy,
		                 (GBoxedFreeFunc) location_satellite_free);
		g_once_init_leave(&type_volatile, type);
	}
	return type_volatile;
}

static void
update_num_of_used(LocationSatellite *satellite)
{
	g_return_if_fail(satellite);
	satellite->num_of_sat_used = 0;
	if (satellite->num_of_sat_inview > 0 && satellite->sat_inview) {
		int i = 0;
		for (i = 0 ; i < satellite->num_of_sat_inview ; i++)
			if (satellite->sat_inview[i].used)(satellite->num_of_sat_used)++;
	}
}

EXPORT_API LocationSatellite *
location_satellite_new(int num_of_sat_inview)
{
	LocationSatellite *satellite = g_slice_new0(LocationSatellite);
	g_return_val_if_fail(satellite, NULL);

	satellite->num_of_sat_inview = num_of_sat_inview;
	satellite->num_of_sat_used = 0;
	satellite->sat_inview = g_new0(LocationSatelliteDetail, satellite->num_of_sat_inview);
	return satellite;
}

EXPORT_API void
location_satellite_free(LocationSatellite *satellite)
{
	g_return_if_fail(satellite);
	g_free(satellite->sat_inview);
	g_slice_free(LocationSatellite, satellite);
}

EXPORT_API LocationSatellite *
location_satellite_copy(const LocationSatellite *satellite)
{
	g_return_val_if_fail(satellite, NULL);
	LocationSatellite *satellite_dup = location_satellite_new(satellite->num_of_sat_inview);
	satellite_dup->timestamp = satellite->timestamp;
	satellite_dup->num_of_sat_used = satellite->num_of_sat_used;
	int i = 0;
	for (i = 0 ; i < satellite_dup->num_of_sat_inview ; i++)
		location_satellite_set_satellite_details(satellite_dup, i,
		                                         satellite->sat_inview[i].prn,
		                                         satellite->sat_inview[i].used,
		                                         satellite->sat_inview[i].elevation,
		                                         satellite->sat_inview[i].azimuth,
		                                         satellite->sat_inview[i].snr);
	return satellite_dup;
}

EXPORT_API gboolean
location_satellite_get_satellite_details(const LocationSatellite *satellite,
                                         guint index,
                                         guint *prn,
                                         gboolean *used,
                                         guint *elevation,
                                         guint *azimuth,
                                         gint *snr)
{
	g_return_val_if_fail(satellite, FALSE);
	g_return_val_if_fail(prn, FALSE);
	g_return_val_if_fail(used, FALSE);
	g_return_val_if_fail(elevation, FALSE);
	g_return_val_if_fail(azimuth, FALSE);
	g_return_val_if_fail(snr, FALSE);
	g_return_val_if_fail(satellite->sat_inview, FALSE);
	g_return_val_if_fail(index < satellite->num_of_sat_inview, FALSE);

	*prn = satellite->sat_inview[index].prn;
	*used = satellite->sat_inview[index].used;
	*elevation = satellite->sat_inview[index].elevation;
	*azimuth = satellite->sat_inview[index].azimuth;
	*snr = satellite->sat_inview[index].snr;

	return TRUE;
}

EXPORT_API gboolean
location_satellite_set_satellite_details(LocationSatellite *satellite,
                                         guint index,
                                         guint prn,
                                         gboolean used,
                                         guint elevation,
                                         guint azimuth,
                                         gint snr)
{
	g_return_val_if_fail(satellite, FALSE);
	g_return_val_if_fail(satellite->sat_inview, FALSE);
	g_return_val_if_fail(index < satellite->num_of_sat_inview, FALSE);

	satellite->sat_inview[index].prn = prn;
	satellite->sat_inview[index].used = used;
	satellite->sat_inview[index].elevation = elevation;
	satellite->sat_inview[index].azimuth = azimuth;
	satellite->sat_inview[index].snr = snr;
	update_num_of_used(satellite);

	return TRUE;
}
