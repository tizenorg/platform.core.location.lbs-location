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

#ifndef __LOCATION_SATELLITE_H_
#define __LOCATION_SATELLITE_H_

#include <location-types.h>

G_BEGIN_DECLS

GType location_satellite_get_type(void);
#define LOCATION_TYPE_SATELLITE (location_satellite_get_type ())

/**
 * @file location-satellite.h
 * @brief This file contains the internal definitions and structures related to satellite information.
 */
/**
 * @addtogroup LocationAPI
 * @{
 * @defgroup LocationAPISatellite Location Satellite
 * @breif This provides APIs related to Location Satellite
 * @addtogroup LocationAPISatellite
 * @{
 */

/**
 * @brief This represents one satellite information in view.
 */
typedef struct {
	guint prn;			/*/< The PRNs(Pseudo-Random Noise code) of a satellite. */
	gboolean used;		/*/< TRUE if currently used satellite. */
	guint elevation;	/*/< The elevation of a satellite. */
	guint azimuth;		/*/< The degree from true north 000 to 359 of a satellite. */
	gint snr;			/*/< The signal-to-noise ratio, dB of a satellite. */
} LocationSatelliteDetail;

/**
 * @brief This represents position information such as number of satellites in used or in view.
 */
struct _LocationSatellite {
	guint timestamp;							/*/< Time stamp. */
	guint num_of_sat_inview;					/*/< The number of satellites in view. */
	guint num_of_sat_used;						/*/< The number of satellites in used. */
	LocationSatelliteDetail *sat_inview;		/*/< The information of satellites in view . */
};

/**
 * @brief	Create a new #LocationSatellite with given number of #LocationSatelliteDetail.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] num_of_sat_inview - number of #LocationSatelliteDetail.
 * @return a new #LocationSatellite
 * @retval NULL if error occured
 */
LocationSatellite *location_satellite_new(int num_of_sat_inview);

/**
 * @brief	Free a #LocationSatellite.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] satellite - a #LocationSatellite.
 * @return None.
 */
void location_satellite_free(LocationSatellite *satellite);

/**
 * @brief	Makes a copy of #LocationSatellite
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] satellite - a #LocationSatellite
 * @return a new #LocationSatellite
 * @retval NULL if error occured
 */
LocationSatellite *location_satellite_copy(const LocationSatellite *satellite);

/**
 * @brief	Get elements of #LocationSatelliteDetail with given index in #LocationSatellite.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in]	satellite - a #LocationSatellite
 * @param [in]	index - index of #LocationSatelliteDetail in #LocationSatellite
 * @param [out] prn - The PRNs(Pseudo-Random Noise code) of a #LocationSatellite
 * @param [out] used - TRUE if a #LocationSatellite in used.
 * @param [out] elevation - The elevation of a #LocationSatellite
 * @param [out] azimuth - The degree from true north 000 to 359 of a#LocationSatellite
 * @param [out] snr - The signal-to-noise ratio, dB of #LocationSatellite
 * @return gboolean
 * @retval\n
 * TRUE - if success\n
 * FALSE - if error occured\n
 */
gboolean location_satellite_get_satellite_details(const LocationSatellite *satellite, guint index, guint *prn, gboolean *used, guint *elevation, guint *azimuth, gint *snr);

/**
 * @brief	Set elements of #LocationSatelliteDetail with given index in #LocationSatellite.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in]  satellite - a #LocationSatellite
 * @param [in]  index - index of #LocationSatelliteDetail in #LocationSatellite
 * @param [in]  prn - The PRNs(Pseudo-Random Noise code) of a #LocationSatellite
 * @param [in]  used - TRUE if a #LocationSatellite in used.
 * @param [in]  elevation - The elevation of a #LocationSatellite
 * @param [in]  azimuth - The degree from true north 000 to 359 of a#LocationSatellite
 * @param [in]  snr - The signal-to-noise ratio, dB of #LocationSatellite
 * @return gboolean
 * @retval\n
 * TRUE - if success\n
 * FALSE - if error occured\n
 */
gboolean location_satellite_set_satellite_details(LocationSatellite *satellite, guint index, guint prn,	gboolean used, guint elevation, guint azimuth, gint snr);

/**
 * @} @}
 */

G_END_DECLS

#endif
