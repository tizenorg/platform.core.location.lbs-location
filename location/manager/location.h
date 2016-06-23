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


#ifndef __LOCATION_H__
#define __LOCATION_H__


#include <glib.h>
#include <location-types.h>
#include <location-position.h>
#include <location-batch.h>
#include <location-velocity.h>
#include <location-accuracy.h>
#include <location-boundary.h>
#include <location-satellite.h>

G_BEGIN_DECLS

/**
 * @file location.h
 * @brief This file contains the Location API and related structure and enumeration.
 */
/**
 * @defgroup LocationFW LocationFW
 * @brief This is a Location Framework for providing location based services.
 * @addtogroup LocationFW
 * @{
 * @defgroup LocationAPI Location API
 * @brief This sub module provides the Location API.
 * @addtogroup LocationAPI
 * @{
 */

/**
 * @brief Initialize location sub module.
 * @remarks None. This API should be called before any other Location APIs.
 * @pre None.
 * @post None.
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 * @see None.
 */
int location_init(void);

/**
 * @brief Create a new #LocationObject by using given #LocationMethod.
 * @remarks Returned object is necessary for other APIs.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] method - Location method to be used.
 * @return a new #LocationObject
 * @retval NULL			if error occured
 * @see location_free
 */
LocationObject *location_new(LocationMethod method);

/**
 * @brief Free memory of given #LocationObject.
 * @remarks None.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new.
 * @return int
 * @retval 0							Success.
 * Please refer #LocationError for more information.
 */
int location_free(LocationObject *obj);

/**
 * @brief Start the location service by using given #LocationObject.
 * @remarks If you want to recieve signals, you should use this API.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @return int
 * @retval 0							Success.
 * Please refer #LocationError for more information.
 */
int location_start(LocationObject *obj);

/**
 * @brief Stop the location service by using given #LocationObject.
 * @remarks After call this API, you can not recieve signals.
 * @pre #location_init should be called before.\n
 * #location_start should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_stop(LocationObject *obj);

int location_request_single_location(LocationObject *obj, int timeout);

/**
 * @brief Check wheither a method is available.
 * @remarks
 * @pre #location_init should be called before.\n
 * @post None.
 * @param [in] method - a #LocationMethod
 * @return int
 * @retval True		Supported
 *		False		Not supported
 */
gboolean location_is_supported_method(LocationMethod method);

int location_is_enabled_method(LocationMethod method, int *is_enabled);

int location_enable_method(const LocationMethod method, const int enable);


/**
 * @brief Get current position information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.\n #location_start should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] position - a new #LocationPosition
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_position(LocationObject *obj, LocationPosition **position, LocationAccuracy **accuracy);

/**
 * @brief Get current position & velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.\n #location_start should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] position - a new #LocationPosition
 * @param [out] velocity - a new #LocationVelocity
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_position_ext(LocationObject *obj, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);

/**
 * @brief Get last position information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] position - a new #LocationPosition
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_last_position(LocationObject *obj, LocationPosition **position, LocationAccuracy **accuracy);

/**
 * @brief Get last position & velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] position - a new #LocationPosition
 * @param [out] velocity - a new #LocationVelocity
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_last_position_ext(LocationObject *obj, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy);
/**
 * @brief Get last satellite information.
 * @remarks This API is not implemented now. \n Out parameters are should be freed.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] satellite - a new #LocationSatellite
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_satellite(LocationObject *obj, LocationSatellite **satellite);


int location_start_batch(LocationObject *obj);

int location_stop_batch(LocationObject *obj);

int location_get_batch(LocationObject *obj, LocationBatch **batch);


/**
 * @brief Get last satellite information.
 * @remarks This API is not implemented now. \n Out parameters are should be freed.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] satellite - a new #LocationSatellite
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_last_satellite(LocationObject *obj, LocationSatellite **satellite);

/**
 * @brief Get current velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.\n #location_start should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] velocity - a new #LocationVelocity
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_velocity(LocationObject *obj, LocationVelocity **velocity, LocationAccuracy **accuracy);

/**
 * @brief Get last velocity information with estimate of the accuracy.
 * @remarks Out parameters are should be freed.
 * @pre #location_init should be called before.
 * @post None.
 * @param [in] obj - a #LocationObject created by #location_new
 * @param [out] velocity - a new #LocationVelocity
 * @param [out] accuracy - a new #LocationAccuracy
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_last_velocity(LocationObject *obj, LocationVelocity **velocity, LocationAccuracy **accuracy);

/**
 * @brief Get the accessibility state of an application
 * @remarks None.
 * @pre #location_init should be called before.\n
 * @post None.
 * @param [out] state - a #LocationAccessState
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_get_accessibility_state(LocationAccessState *state);

/**
 * @brief Send command to the server.
 * @pre #location_init should be called before.\n
 * Calling application must have glib or ecore main loop.\n
 * Calling application must have an active data connection.
 * @post None.
 * @param [in] cmd - a #char
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_send_command(const char *cmd);

/**
 * @brief Set option of server.
 * @pre #location_init should be called before.\n
 * Calling application must have glib or ecore main loop.\n
 * Calling application must have an active data connection.
 * @post None.
 * @param [in]	obj - a #LocationObject created by #location_new
 * @param [in]	option - a #char
 * @return int
 * @retval 0							Success
 * Please refer #LocationError for more information.
 */
int location_set_option(LocationObject *obj, const char *option);

int location_add_setting_notify(LocationMethod method, LocationSettingCb callback, void *user_data);

int location_ignore_setting_notify(LocationMethod method, LocationSettingCb callback);

int location_get_nmea(LocationObject *obj, char **nmea_data);


/* Tizen 3.0 */

int location_get_service_state(LocationObject *obj, int *state);
int location_enable_mock(const LocationMethod method, const int enable);
int location_set_mock_method_enabled(const LocationMethod method, const int enable);
int location_set_mock_location(LocationObject *obj, const LocationPosition *position, const LocationVelocity *velocity, const LocationAccuracy *accuracy);
int location_clear_mock_location(LocationObject *obj);

int location_enable_restriction(const int enable);
int location_enable_fused_interval(LocationObject *obj);
int location_fused_accuracy_mode(LocationObject *obj, int fused_mode);

/**
 * @} @}
 */

G_END_DECLS

#endif /* __LOCATION_H__ */
