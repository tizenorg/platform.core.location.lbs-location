/*
 * libslp-location
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *			 Genie Kim <daejins.kim@samsung.com>
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

#ifndef __LOCATION_BOUNDARY_H_
#define __LOCATION_BOUNDARY_H_

#include <location-types.h>
#include <location-position.h>

G_BEGIN_DECLS

GType location_boundary_get_type(void);
#define LOCATION_TYPE_BOUNDARY (location_boundary_get_type ())

/**
 * @file location-boundary.h
 * @brief This file contains the definitions, structures, and functions related to boundary information.
 */
/**
 * @addtogroup LocationAPI
 * @{
 * @defgroup LocationAPIBoundary Location Boundary
 * @breif This provides APIs related to Location Boundary
 * @addtogroup LocationAPIBoundary
 * @{
 */

/**
 * @brief
 * The type of the @location_boundary_foreach function of #LocationObject
 */
typedef void (*LocationBoundaryFunc)(LocationBoundary *boundary, gpointer user_data);

/**
 * @brief This represents used geographical type, and supports rectangle or circle area.
 */
typedef enum {
    LOCATION_BOUNDARY_NONE = 0,		/*/< Undefined geographical area type. */
    LOCATION_BOUNDARY_RECT,			/*/< Rectangular geographical area type. */
    LOCATION_BOUNDARY_CIRCLE,		/*/< Circle geographical area type. */
    LOCATION_BOUNDARY_POLYGON		/*/< Polygon geographical area type. */
} LocationBoundaryType;

/**
 * @brief This represents a rectangular geographical area.
 */
typedef struct {
	LocationPosition *left_top;		/*/< The left top position of rectangle. */
	LocationPosition *right_bottom;	/*/< The right bottom position of rectangle. */
} LocationRect;

/**
 * @brief This represents a circle geographical area with center geographic position and radius.
 */
typedef struct {
	LocationPosition *center;		/*/< The center position of a circle. */
	gdouble radius;					/*/< The radius of a circle. */
} LocationCircle;

/**
 * @brief This represents a polygon geographical area.
 */
typedef struct {
	GList *position_list;			/*/< The collection of positions */
} LocationPolygon;

/**
 * @brief This represents boundary information such as rectangular or circle area.
 */
struct _LocationBoundary {
	LocationBoundaryType type;		/*/< The boundary type of this information. */
	union {
		LocationRect rect;			/*/< The geographical information of a rectangle. */
		LocationCircle circle;		/*/< The geographical information of a circle. */
		LocationPolygon polygon;	/*/< The geographical information of a polygon. */
	};
};

/**
 * @brief	Create a rectangular type of new #LocationBoundary with given information.
 * @remarks	None.
 * @pre		#location_init should be called before.\n
 * @post	None.
 * @param [in] left_top - left top position.
 * @param [in] right_bottom - right bottom position.
 * @return a new #LocationBoundary
 * @retval NULL if error occured
 */
LocationBoundary *location_boundary_new_for_rect(LocationPosition *left_top, LocationPosition *right_bottom);

/**
 * @brief	Create a circle type of new #LocationBoundary with given information.
 * @remarks	None.
 * @pre		#location_init should be called before.\n
 * @post	None.
 * @param [in] center - center position.
 * @param [in] radius - radius.
 * @return a new #LocationBoundary
 * @retval NULL if error occured
 */
LocationBoundary *location_boundary_new_for_circle(LocationPosition *center, gdouble radius);

/**
 * @brief	Create a polygon type of new #LocationBoundary with given information.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	 None.
 * @param [in] position_list - the list of positions.
 * @return a new #LocationBoundary
 * @retval NULL if error occured
 */
LocationBoundary *location_boundary_new_for_polygon(GList *position_list);

/**
 * @brief	Free a #LocationBoundary.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	 None.
 * @param [in] boundary - a #LocationBoundary.
 * @return None.
 */
void location_boundary_free(LocationBoundary *boundary);

/**
 * @brief	Makes a copy of #LocationBoundary
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	 None.
 * @param [in] boundary - a #LocationBoundary
 * @return a new #LocationBoundary
 * @retval NULL if error occured
 */
LocationBoundary *location_boundary_copy(const LocationBoundary *boundary);

/**
 * @brief
 * Add Boundary on LocationFW.
 * You should call this fuction when you want to receive a crossing signal(zone-in/zone-out) from #LocationBoundary.
 * @remarks It supports multi-boundaries. \n
	 However a duplicated boundary would not be allowed.
 * @pre
 * #location_new should be called before.\n
 * @post None.
 * @param [in] obj - a #LocationObject
 * @param [in] boundary - a #LocationBoundary
 * @return int
 * @retval 0										Success
 * Please refer #LocationError for more information.
 * @par Example
 * @code
#include <location.h>

static void cb_zone_in (GObject *self, gpointer boundary, gpointer position, gpointer accuracy, gpointer user_data)
{
	g_printf ("[zone-in] position - lat: %f, long: %f", position->latitude, position->longitude);
}

static void cb_zone_out (GObject *self, gpointer boundary, gpointer position, gpointer accuracy, gpointer user_data)
{
	g_printf ("[zone-out] position - lat: %f, long: %f", position->latitude, position->longitude);
}

void location_test_boundary_add(LocationObject *loc)
{
	LocationPosition* rb = location_position_new (0, 37.300, -121.86, 0, LOCATION_STATUS_2D_FIX);
	LocationPosition* lt = location_position_new (0, 37.360, -121.92, 0, LOCATION_STATUS_2D_FIX);

	LoationBoundary *boundary = location_boundary_new_for_rect (lt, rb);

	ret = location_boundary_add(loc, boundary);

	g_signal_connect(loc, "zone-in", G_CALLBACK(cb_zone_in), NULL);
	g_siganl_connect(loc, "zone-out", G_CALLBACK(cb_zone_out), NULL);

	location_position_free(rb);
	location_position_free(lt);
}
 * @endcode
 */
int location_boundary_add(const LocationObject *obj, const LocationBoundary *boundary);

/**
 * @brief
 * Remove Boundary on LocationFW.
 * You should call this function when you don't want to receive a crossing signal(zone-in/zone-out) from #LocationBoundary any more.
 * @remarks It supports multi-boundaries.
 * @pre
 * #location_init should be called before.\n
 * @post None.
 * @param [in] obj - a #LocationObject
 * @param [in] boundary - a #LocationBoundary
 * @return int
 * @retval 0										Success
 *
 * Please refer #LocationError for more information.
 * @par Example
 * @code
#include <location.h>

void location_test_boundary_remove(LocationObject *loc)
{
	int ret = 0;
	LocationPosition* rb = location_position_new (0, 37.300, -121.86, 0, LOCATION_STATUS_2D_FIX);
	LocationPosition* lt = location_position_new (0, 37.360, -121.92, 0, LOCATION_STATUS_2D_FIX);

	LoationBoundary *boundary = location_boundary_new_for_rect (lt, rb);

	ret = location_boundary_remove(loc, boundary);

	location_position_free(rb);
	location_position_free(lt);

}
 * @endcode
 */
int location_boundary_remove(const LocationObject *obj, const LocationBoundary *boundary);

/**
 * @brief
 * Call a function for each element of a Boundary list.
 * @remarks None.
 * @pre
 * #location_init should be called before.\n
 * @post None.
 * @param [in] obj - a #LocationObject
 * @param [in] func - a #LocationBoundaryFunc
 * @param [in] user_data - a #void
 * @return int
 * @retval 0										Success
 *
 * Please refer #LocationError for more information.
 * @par Example
 * @code
#include <location.h>

static void remove_boundary(LocationBoundary *boundary, void *user_data)
{
	LocationBoundary *loc = (LocationBoundary *) user_data;
	if (loc == NULL || boundary == NULL) return;

	location_boundary_remove(loc, boundary);
}

void location_test_boundary_foreach(LocationObject *loc)
{
	int ret = location_boundary_foreach(loc, remove_boundary, loc);
}
 * @endcode
 */
int location_boundary_foreach(const LocationObject *obj, LocationBoundaryFunc func, gpointer user_data);


/**
 * @brief	Check if #LocationPosition is inside #LocationBoundary.
 * @remarks None.
 * @pre	 #location_init should be called before.\n
 * @post	 None.* @param [in] boundary - a #LocationBoundary
 * @param [in] position - a #LocationPosition
 * @return gboolean
 * @retval\n
 * TRUE - if inside\n
 * FALSE - if outside\n
 * @par Example
 * @code
#include <location.h>

void location_test_boundary_if_inside(LocationObject *loc, LocationBoundary *boundary)
{
	gboolean is_inside = FALSE;
	LocationPosition* position = location_position_new (0, 37.300, -121.86, 0, LOCATION_STATUS_2D_FIX);
	is_inside = location_boundary_if_inside(boundary, position);
	if (is_inside == TRUE) g_printf("The position is inside of the boundary\n");
	else g_printf("The position is outside of the boundary\n");

}
 * @endcode

 */
gboolean location_boundary_if_inside(LocationBoundary *boundary, LocationPosition *position);

/**
 * @brief Get bounding box of #LocationBoundary
 */
LocationBoundary *location_boundary_get_bounding_box(LocationBoundary *boundary);


/**
 * @brief Get the center position of #LocationBoundary
 */
LocationPosition *location_boundary_get_center_position(LocationBoundary *boundary);

/**
 * @} @}
 */

G_END_DECLS

#endif
