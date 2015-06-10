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

#ifndef __LOCATION_ACCURACY_H_
#define __LOCATION_ACCURACY_H_

#include <location-types.h>

G_BEGIN_DECLS

GType location_accuracy_get_type(void);
#define LOCATION_TYPE_ACCURACY (location_accuracy_get_type())

/**
 * @file location-accuracy.h
 * @brief This file contains the definitions, structures, and functions related to accuracy information.
 */

/**
 * @addtogroup LocationAPI
 * @{
 * @defgroup LocationAPIAccuracy Location Accuracy
 * @breif This provides APIs related to Location Accuracy.
 * @addtogroup LocationAPIAccuracy
 * @{
 */

/**
 * @brief This represents the approximate accuracy level of given information.
 */
typedef enum {
    LOCATION_ACCURACY_LEVEL_NONE = 0,	/*/< Invalid data. */
    LOCATION_ACCURACY_LEVEL_COUNTRY,	/*/< Country accuracy level-> */
    LOCATION_ACCURACY_LEVEL_REGION,		/*/< Regional accuracy level-> */
    LOCATION_ACCURACY_LEVEL_LOCALITY,	/*/< Local accuracy level-> */
    LOCATION_ACCURACY_LEVEL_POSTALCODE,	/*/< Postal accuracy level-> */
    LOCATION_ACCURACY_LEVEL_STREET,		/*/< Street accuracy level-> */
    LOCATION_ACCURACY_LEVEL_DETAILED,	/*/< Detailed accuracy level-> */
} LocationAccuracyLevel;

/**
 * @brief This represents location accuracy information such as accuracy level, horizontal and vertical accuracy.
 */
struct _LocationAccuracy {
	LocationAccuracyLevel level;		/*/< The accuracy level of the location information. */
	gdouble horizontal_accuracy;		/*/< The horizontal position uncertainty of the location in meters. */
	gdouble vertical_accuracy;			/*/< The vertical position uncertainty of the location in meters. */
};

/**
 * @brief	Create a new #LocationAccuracy with given information.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] level - The accuracy level of the location information.
 * @param [in] horizontal_accuracy - The horizontal position uncertainty of the location in meters.
 * @param [in] vertical_accuracy - The vertical position uncertainty of the location in meters.
 * @return a new #LocationAccuracy
 * @retval NULL if error occured
 */
LocationAccuracy *location_accuracy_new(LocationAccuracyLevel level, gdouble horizontal_accuracy, gdouble vertical_accuracy);

/**
 * @brief	Free a #LocationAccuracy.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] accuracy - a #LocationAccuracy.
 * @return None.
 */
void location_accuracy_free(LocationAccuracy *accuracy);

/**
 * @brief	Compare for two accuracys.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] accuracy1 - a #LocationAccuracy
 * @param [in] accuracy2 - another #LocationAccuracy
 * @return integer
 * @retval\n
 * 0 - if the accuracy match\n
 * positive value - if accuracy of accuracy1 is better than accuracy of accuracy2\n
 * negative value - if accuracy of accuracy1 is worse than accuracy of accuracy2
 */
int location_accuracy_compare(const LocationAccuracy *accuracy1, const LocationAccuracy *accuracy2);

/**
 * @brief	Compare for two accuracys' level.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] accuracy1 - a #LocationAccuracy
 * @param [in] accuracy2 - another #LocationAccuracy
 * @return integer
 * @retval\n
 * 0 - if the accuracy match\n
 * positive value - if accuracy1's level is better than accuracy2's level\n
 * negative value - if accuracy1's level is worse than accuracy2's level
 */
int location_accuracy_level_compare(const LocationAccuracy *accuracy1, const LocationAccuracy *accuracy2);

/**
 * @brief	Makes a copy of #LocationAccuracy
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	None.
 * @param [in] accuracy - a #LocationAccuracy
 * @return a new #LocationAccuracy
 * @retval NULL if error occured
 */
LocationAccuracy *location_accuracy_copy(const LocationAccuracy *accuracy);

/**
 * @} @}
 */

G_END_DECLS

#endif
