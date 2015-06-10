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

#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <glib/gprintf.h>
#include <location-map-service.h>
#include "location-api-test-util.h"
#include "location-geocode.h"

#define STR_MAX	128
LocationMapObject *map_obj = NULL;
static GMainLoop *g_mainloop = NULL;

#define LOCATION_API_TEST_JSON_FILE "/opt/data/location-api-test.json"

static gpointer GmainThread(gpointer data)
{
	g_mainloop = g_main_loop_new(NULL, FALSE);
	g_printf("\n...Entering GMain Loop to Receive Notifications....\n");
	g_main_loop_run(g_mainloop);
	g_main_loop_unref(g_mainloop);
	g_mainloop = NULL;
	return NULL;
}


static void GetLocationError(char str[STR_MAX], int ret)
{
	switch (ret) {
		case LOCATION_ERROR_NONE:
			g_utf8_strncpy(str, "LOCATION_ERROR_NONE", STR_MAX);
			break;
		case LOCATION_ERROR_NOT_ALLOWED:
			g_utf8_strncpy(str, "LOCATION_ERROR_NOT_ALLOWED", STR_MAX);
			break;
		case LOCATION_ERROR_NOT_AVAILABLE:
			g_utf8_strncpy(str, "LOCATION_ERROR_NOT_AVAILABLE", STR_MAX);
			break;
		case LOCATION_ERROR_NETWORK_FAILED:
			g_utf8_strncpy(str, "LOCATION_ERROR_NETWORK_FAILED", STR_MAX);
			break;
		case LOCATION_ERROR_NETWORK_NOT_CONNECTED:
			g_utf8_strncpy(str, "LOCATION_ERROR_NETWORK_NOT_CONNECTED", STR_MAX);
			break;
		case LOCATION_ERROR_CONFIGURATION:
			g_utf8_strncpy(str, "LOCATION_ERROR_CONFIGURATION", STR_MAX);
			break;
		case LOCATION_ERROR_PARAMETER:
			g_utf8_strncpy(str, "LOCATION_ERROR_PARAMETER", STR_MAX);
			break;
		case LOCATION_ERROR_UNKNOWN:
			g_utf8_strncpy(str, "LOCATION_ERROR_UNKNOWN", STR_MAX);
			break;
		case LOCATION_ERROR_NOT_SUPPORTED:
			g_utf8_strncpy(str, "LOCATION_ERROR_UNKNOWN", STR_MAX);
			break;
		case LOCATION_ERROR_SETTING_OFF:
			g_utf8_strncpy(str, "LOCATION_ERROR_SETTING_OFF", STR_MAX);
			break;
		case LOCATION_ERROR_SECURITY_DENIED:
			g_utf8_strncpy(str, "LOCATION_ERROR_SECURITY_DENIED", STR_MAX);
			break;
		default:
			g_utf8_strncpy(str, "Error: undefined error code", STR_MAX);
	}
}

static void GetAccuracyLevel(char str[STR_MAX], LocationAccuracyLevel acc_level)
{
	switch (acc_level) {
		case LOCATION_ACCURACY_LEVEL_NONE:
			g_utf8_strncpy(str, "LOCATION_ACCURACY_LEVEL_NONE", STR_MAX);
			break;
		case LOCATION_ACCURACY_LEVEL_COUNTRY:
			g_utf8_strncpy(str, "LOCATION_ACCURACY_LEVEL_COUNTRY", STR_MAX);
			break;
		case LOCATION_ACCURACY_LEVEL_REGION:
			g_utf8_strncpy(str, "LOCATION_ACCURACY_LEVEL_REGION", STR_MAX);
			break;
		case LOCATION_ACCURACY_LEVEL_LOCALITY:
			g_utf8_strncpy(str, "LOCATION_ACCURACY_LEVEL_LOCALITY", STR_MAX);
			break;
		case LOCATION_ACCURACY_LEVEL_POSTALCODE:
			g_utf8_strncpy(str, "LOCATION_ACCURACY_LEVEL_POSTALCODE", STR_MAX);
			break;
		case LOCATION_ACCURACY_LEVEL_STREET:
			g_utf8_strncpy(str, "LOCATION_ACCURACY_LEVEL_STREET", STR_MAX);
			break;
		case LOCATION_ACCURACY_LEVEL_DETAILED:
			g_utf8_strncpy(str, "LOCATION_ACCURACY_LEVEL_DETAILED", STR_MAX);
			break;
		default:
			g_utf8_strncpy(str, "Error: undefined accuracy level", STR_MAX);
	}
}

static void SelectOpt(char *buf)
{
	int iLen = 0;
	char *str = NULL;
	str = fgets(buf, 255, stdin);
	if (NULL == str) {
		g_printf(" fgets return NULL. \n");
	}
	iLen = g_utf8_strlen(buf, -1);
	buf[iLen - 1] = '\0';
}

static int PromptInt()
{
	char buf[255];
	int ret;
	char *str = NULL;
	str = fgets(buf, 255, stdin);
	buf[strlen(buf) - 1] = '\0';
	ret = g_ascii_strtoll(buf, NULL, 10);
	return ret;
}

static double PromptDB()
{
	char buf[255];
	double ret;
	char *str = NULL;
	str = fgets(buf, 255, stdin);
	if (NULL == str) {
		g_printf(" fgets return NULL. \n");
	}
	buf[strlen(buf) - 1] = '\0';
	ret = g_ascii_strtod(buf, NULL);
	return ret;
}

static void PrintPos(gpointer data, gpointer user_data)
{
	LocationPosition *pos = (LocationPosition *)data;
	if (pos) {
		g_printf("time: [%d], latitude: [%f], longitude: [%f], altitude: [%f]\n", pos->timestamp, pos->latitude, pos->longitude, pos->altitude);
		location_position_free(pos);
	}
}

static void PrintAcc(gpointer data, gpointer user_data)
{
	LocationAccuracy *acc = (LocationAccuracy *)data;
	if (acc) {
		g_printf("level: [%d], horizontal_accuracy: [%f], vertical_accuracy: [%f]\n", acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_accuracy_free(acc);
	}
}


static void PrintProperty(LocationObject *loc)
{
	if (!loc) return;
}

static void _print_property(gpointer data, gpointer user_data)
{
	LocationLandmark *landmark = (LocationLandmark *)user_data;
	gpointer key = (gpointer)data;
	gpointer value = NULL;

	if (key) {
		value = (gpointer)location_landmark_get_property(landmark, key);
		g_printf(", [%s:%s]", (gchar *) key, (gchar *) value);
	}
}

static void
__plugin_print_poi_list(gpointer data, gpointer user_data)
{
	int i = 0;
	LocationLandmark *landmark = (LocationLandmark *)data;
	LocationPosition *pos = location_landmark_get_position(landmark);
	LocationAddress *addr = location_landmark_get_address(landmark);

	g_printf("[ID:%d], [NAME:%s], phone[%s],pos[%f:%f], addr[%s:%s:%s:%s:%s]",
	         location_landmark_get_id(landmark),
	         location_landmark_get_name(landmark),
	         location_landmark_get_phone_number(landmark),
	         pos->latitude, pos->longitude,
	         addr->building_number, addr->city, addr->country_code, addr->district,
	         addr->postal_code);
	g_printf("Supplier details : [NAME: %s],[LINK: %s],[ID:%s],[TYPE: %s]",
	         landmark_linkobject_get_name(location_landmark_get_supplier(landmark)),
	         landmark_linkobject_get_link_string(location_landmark_get_supplier(landmark)),
	         landmark_linkobject_get_id(location_landmark_get_supplier(landmark)),
	         landmark_linkobject_get_type(location_landmark_get_supplier(landmark)));
	g_printf("Related  details : [NAME: %s],[LINK: %s],[ID:%s],[TYPE: %s]",
	         landmark_linkobject_get_name(location_landmark_get_related(landmark)),
	         landmark_linkobject_get_link_string(location_landmark_get_related(landmark)),
	         landmark_linkobject_get_id(location_landmark_get_related(landmark)),
	         landmark_linkobject_get_type(location_landmark_get_related(landmark)));
	g_printf("Rating details : [AVG: %f],[COUNT:%d]",
	         landmark_rating_get_average(location_landmark_get_rating(landmark)),
	         landmark_rating_get_count(location_landmark_get_rating(landmark)));
	for (i = 0; i < g_list_length(location_landmark_get_editorial(landmark)); i++) {
		LandmarkEditorial *editorial = g_list_nth_data(location_landmark_get_editorial(landmark), i);
		g_printf("Editorials : [Desc: %s] [LANG: %s]",
		         landmark_editorial_get_description(editorial), landmark_editorial_get_language(editorial));
	}
	GList *key_list = location_landmark_get_property_key(landmark);
	if (key_list) {
		g_list_foreach(key_list, _print_property, landmark);
	}

	g_printf("\n");
}


static void cb_poi(LocationError error, guint req_id, GList *landmark_list, gchar *error_code, gchar *error_msg, gpointer userdata)
{
	g_printf("\n===== __location_POI_cb ======\n");
	if (error != LOCATION_ERROR_NONE) {
		g_printf("Failed :%d\n", error);
		return;
	}
	g_printf("Success, poi_list[0x%x] user_data[0x%x] req_id[%d]\n", (unsigned int)landmark_list, (unsigned int)userdata, req_id);
	g_list_foreach(landmark_list, __plugin_print_poi_list, NULL);

	if (error_code && error_msg) {
		g_printf("__location_POI_cb: error_code[%s], error_msg[%s]\n", error_code, error_msg);
	}
}
static void Print_step_info(const LocationRouteStep *step)
{
	const LocationPosition *start = location_route_step_get_start_point(step);
	gdouble start_lat = 0;
	gdouble start_lon = 0;
	if (start) {
		start_lat = start->latitude;
		start_lon = start->longitude;
	} else {
		g_printf("Step start position NULL\n");
	}
	const LocationPosition *end = location_route_step_get_end_point(step);
	gdouble end_lat = 0;
	gdouble end_lon = 0;
	if (end) {
		end_lat = end->latitude;
		end_lon = end->longitude;
	} else {
		g_printf("Step end position NULL\n");
	}
	const gchar *inst = location_route_step_get_instruction(step);

	g_printf("Step: start(%f/%f), end(%f/%f), instruction(%s)\n", start_lat, start_lon, end_lat, end_lon, inst);
}

static void Print_form_of_way(LocationRoadElement *road_element)
{
	g_printf("+++Print form of way begin \n");

	FormOfWay form = location_road_element_get_form_of_way(road_element);

	switch (form) {
		case FOW_UNDEFINED:
			g_printf("Indicates that the road or road type is undefined/unknown\n");
			break;

		case FOW_MOTORWAY:
			g_printf("Identifies a road as a motorway \n");
			break;

		case FOW_MULTI_CARRIAGEWAY:
			g_printf("Identifies a road as a multi-lane carriageway \n");
			break;

		case FOW_SINGLE_CARRIAGEWAY:
			g_printf("Identifies a road as a single carriageway \n");
			break;

		case FOW_ROUNDABOUT:
			g_printf("Identifies a road feature as a roundabout/rotary \n");
			break;

		case FOW_SPECIAL_TRAFFIC_FIGURE:
			g_printf("Identifies a road features as a special traffic figure \n");
			break;

		case FOW_SLIPROAD:
			g_printf(" Identifies a road as a slip road \n");
			break;

		case FOW_PEDESTRIAN_ZONE:
			g_printf("Identifies an area or road section as a pedestrian zone \n");
			break;

		case FOW_PEDESTRIAN_WALKWAY:
			g_printf(" Identifies a pedestrian walkway \n");
			break;

		case FOW_SERVICE_ACCESS_PARKING:
			g_printf(" Identifies access to a parking facility \n");
			break;

		case FOW_SERVICE_ACCESS_OTHER:
			g_printf(" Identifies access to an unspecified service or facility \n");
			break;

		case FOW_SERVICE_ROAD:
			g_printf(" Identifies a road as a service road \n");
			break;

		case FOW_ETA_PARKING_PLACE:
			g_printf(" Identifies a parking facility \n");
			break;

		case FOW_ETA_PARKING_BUILDING:
			g_printf("  Identifies a parking house \n");
			break;

		case FOW_ETA_UNSTRUCTURED_TRAFFIC_SQUARE:
			g_printf(" Identifies an unstructured traffic square \n");
			break;

		case FOW_ROAD_FOR_AUTHORITIES:
			g_printf(" Identifies a road restricted for authorized access only \n");
			break;

		default:
			g_warning("ASYNC>> Undefined Form Of Way type");
			break;
	}
	g_printf("---Print form of way end\n");
}

static void Print_transit_type(LocationRouteTransitElement *element)
{
	g_printf("+++Print transit type begin \n");

	TransitType ttype = location_route_transit_element_get_transit_type(element);
	switch (ttype) {
		case TRANSIT_TYPE_BUS_PUBLIC:
			g_printf("Indicates transit type is bus public\n");
			break;

		case TRANSIT_TYPE_BUS_TOURISTIC:
			g_printf("Indicates transit type is bus touristic\n");
			break;

		case TRANSIT_TYPE_BUS_INTERCITY:
			g_printf("Indicates transit type is bus intercity \n");
			break;

		case TRANSIT_TYPE_BUS_EXPRESS:
			g_printf("Indicates transit type is bus express \n");
			break;

		case TRANSIT_TYPE_RAIL_METRO:
			g_printf("Indicates transit type is rail metro \n");
			break;

		case TRANSIT_TYPE_RAIL_LIGHT:
			g_printf("Indicates transit type is rail light \n");
			break;

		case TRANSIT_TYPE_RAIL_REGIONAL:
			g_printf("Indicates transit type is rail regional \n");
			break;

		case TRANSIT_TYPE_TRAIN_REGIONAL:
			g_printf("Indicates transit type is train  \n");
			break;

		case TRANSIT_TYPE_TRAIN_INTERCITY:
			g_printf("Indicates transit type is bus regional \n");
			break;

		case TRANSIT_TYPE_TRAIN_HIGH_SPEED:
			g_printf("Indicates transit type is high speed \n");
			break;

		case TRANSIT_TYPE_MONORAIL:
			g_printf("Indicates transit type is monorall \n");
			break;

		case TRANSIT_TYPE_AERIAL:
			g_printf("Indicates transit type is aerial \n");
			break;

		case TRANSIT_TYPE_INCLINED:
			g_printf("Indicates transit type is inclined \n");
			break;

		case TRANSIT_TYPE_WATER:
			g_printf("Indicates transit type is water \n");
			break;

		case TRANSIT_TYPE_AIRLINE:
			g_printf("Indicates transit type is airline \n");
			break;

		case TRANSIT_TYPE_RESERVED:
			g_printf("Indicates reserved for future usage \n");
			break;

		case TRANSIT_TYPE_COUNT:
			g_printf("Indicates transit type's count \n");
			break;

		default:
			g_warning("ASYNC>> Undefined Transit Type \n");
			break;
	}
	g_printf("---Print transit type end\n");
}

static void _print_elem(gpointer data, gpointer user_data)
{
	LocationRouteStep *step = (LocationRouteStep *) user_data;
	LocationRouteElement *elem = (LocationRouteElement *) data;

	if (location_route_element_get_is_transit_element(elem)) {
		LocationRouteTransitElement *transit_element = location_route_element_get_route_transit_element(elem);


		Print_transit_type(transit_element);
		gchar *t_dest = location_route_transit_element_get_transit_destination(transit_element);
		gchar *t_line = location_route_transit_element_get_transit_line_name(transit_element);
		gchar *official = location_route_transit_element_get_system_official_name(transit_element);
		gchar *short_name = location_route_transit_element_get_system_short_name(transit_element);

		gchar *type_name = location_route_transit_element_get_transit_type_name(transit_element);
		guint d_t = location_route_transit_element_get_transit_departure_time(transit_element);
		guint a_t = location_route_transit_element_get_transit_arrival_time(transit_element);
		LocationRouteTransitStop *d_s = location_route_transit_element_get_transit_departure_station(transit_element);
		gchar *d_s_name = location_route_transit_get_station_name(d_s);
		gint d_s_level = location_route_transit_get_platform_level(d_s);
		LocationPosition *d_s_plat_pos = location_route_transit_get_platform_coordinates(d_s);
		if (d_s_plat_pos) {
			g_printf("d_s_plat_pos: time: %d, lat: %f, long: %f, alt: %f, status: %d\n",
			         d_s_plat_pos->timestamp, d_s_plat_pos->latitude, d_s_plat_pos->longitude, d_s_plat_pos->altitude, d_s_plat_pos->status);
			/*location_position_free (d_s_plat_pos); */
		}

		LocationPosition *d_s_egr_pos = location_route_transit_get_egress_coordinates(d_s);
		if (d_s_egr_pos) {
			g_printf("d_s_egr_pos: time: %d, lat: %f, long: %f, alt: %f, status: %d\n",
			         d_s_egr_pos->timestamp, d_s_egr_pos->latitude, d_s_egr_pos->longitude, d_s_egr_pos->altitude, d_s_egr_pos->status);
			/*location_position_free (d_s_egr_pos); */
		}

		g_printf("Transit: d_s_name(%s), d_s_level(%d)\n", d_s_name, d_s_level);

		g_printf("Transit: t_dest(%s), t_line(%s), official(%s), short(%s), type_name(%s), d_t(%lld), a_t(%lld)\n",
		         t_dest, t_line, official, short_name, type_name, d_t, a_t);

		LocationRouteTransitStop *a_s = location_route_transit_element_get_transit_arrival_station(transit_element);
		gchar *a_s_name = location_route_transit_get_station_name(a_s);
		gint a_s_level = location_route_transit_get_platform_level(a_s);
		if (a_s) {
			g_printf("Transit: a_s_name(%s), a_s_level(%d)  \n", a_s_name, a_s_level);
		}
	} else {
		LocationRoadElement *road_element = location_route_element_get_road_element(elem);

		Print_form_of_way(road_element);

		gboolean is_plural = location_road_element_is_plural(road_element);
		if (is_plural) {
			g_printf(" The road element is plural \n");
		} else {
			g_printf(" The road element is not plural \n");
		}
		gchar *road_name = location_road_element_get_road_name(road_element);
		gchar *route_name = location_road_element_get_route_name(road_element);
		gfloat speed_l = location_road_element_get_speed_limit(road_element);
		guint speed = location_road_element_get_average_speed_m_s(road_element);
		guint nums = location_road_element_get_number_of_lanes(road_element);
		guint s_time = location_road_element_get_start_time(road_element);
		guint t_time = location_road_element_get_travel_time(road_element);

		g_printf("RoadElement: road_name(%s), route_name(%s), speed_l(%f), speed(%d), nums(%d), s_time(%lld), t_time(%lld)\n",
		         road_name, route_name, speed_l, speed, nums, s_time, t_time);

		gboolean is_pedestrain = location_road_element_is_pedestrian(road_element);
		if (is_pedestrain) {
			g_printf(" The road is allowed only for pedestrians \n");
		} else {
			g_printf(" The road is not only for pedestrians \n");
		}

		gboolean is_valid = location_road_element_is_valid(road_element);
		if (is_valid) {
			g_printf(" This road element is valid \n");
		} else {
			g_printf(" This road element is invalid \n");
		}
	}

}

static void Print_road_element(LocationRouteStep *step)
{
	g_printf("+++PrintRoadElement begin\n");
	GList *route_elem_list = location_route_step_get_route_elment_list(step);
	if (route_elem_list) {
		g_list_foreach(route_elem_list, _print_elem, step);
	}
	g_printf("---Print Road Element end\n");
}

static void print_route_maneuver(LocationRouteManeuver *maneuver)
{
	g_printf("+++print route maneuver begin\n");

	TrafficDirection dirct = TRAFFIC_DIR_LEFT;

	guint dist_start = location_route_maneuver_get_distance_from_start(maneuver);
	guint dist_prev = location_route_maneuver_get_distance_from_previous_maneuver(maneuver);
	g_printf("maneuver: dist_start(%d), dist_prev(%d)\n", dist_start, dist_prev);

	gchar *next_road_name = location_route_maneuver_get_next_road_name(maneuver);
	gchar *action = location_route_maneuver_get_action(maneuver);
	gchar *turn = location_route_maneuver_get_turn(maneuver);
	g_printf("maneuver: next_road_name(%s), action(%s), turn(%s)\n", next_road_name, action, turn);

	dirct = location_route_maneuver_get_traffic_direction(maneuver);
	if (TRAFFIC_DIR_LEFT == dirct) {
		g_printf("Traffic is left sided. \n");
	} else if (TRAFFIC_DIR_RIGHT == dirct) {
		g_printf("Traffic is right sided. \n");
	}

	guint angle = location_route_maneuver_get_angle(maneuver);
	guint start_angle = location_route_maneuver_get_start_angle(maneuver);
	guint s_time = location_route_maneuver_get_start_time(maneuver);
	g_printf("maneuver: angle(%d), start_angle(%d), s_time(%d)\n", angle, start_angle, s_time);

	gboolean is_s_sliproad = location_route_maneuver_is_starts_from_sliproad(maneuver);
	gboolean is_n_sliproad = location_route_maneuver_is_next_is_sliproad(maneuver);
	gboolean is_round = location_route_maneuver_is_counter_roundabout(maneuver);

	if (is_n_sliproad) {
		g_printf("Maneuver starts on a sliproad. \n");
	}
	if (is_s_sliproad) {
		g_printf("Next road is sliproad. \n");
	}
	if (is_round) {
		g_printf("This maneuver is a roundabout. \n");
	}

	g_printf("---print route maneuver  end\n");
}

static void cb_print_route_step(gpointer data, gpointer user_data)
{
	g_printf("+++Step begin\n");
	LocationRouteStep *step = (LocationRouteStep *)data;

	Print_step_info(step);
	g_printf("after Print Step Info\n");

	/*****************NLP start now *********************/
	g_printf("*****************NLP step start now *********************\n");
	Print_road_element(step);

	LocationRouteManeuver *maneuver = location_route_step_get_maneuver(step);
	print_route_maneuver(maneuver);

	g_printf("---Step end\n");
}

static void print_pos_nokia(gpointer data, gpointer user_data)
{
	g_printf("+++print pos nokia begin. \n");

	LocationPosition *pos = (LocationPosition *)data;

	if (pos) {
		g_printf("time: %d, lat: %f, long: %f, alt: %f, status: %d",
		         pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
		/*location_position_free (pos); */
	}

	g_printf("---print pos nokia end\n");
}

static void print_acc_nokia(gpointer data, gpointer user_data)
{
	g_printf("+++print acc nokia begin. \n");

	LocationAccuracy *acc = (LocationAccuracy *)data;

	if (acc) {
		g_printf("\tAccuracy level %d (%.0f meters %.0f meters)", acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		/*location_accuracy_free (acc); */
	}

	g_printf("---print acc nokia end. \n");
}

static void cb_position_from_address_nokia(LocationError error, GList *pos_list, GList *acc_list, gpointer userdata)
{
	g_printf("+++cb position from address nokia. \n");

	if (error != LOCATION_ERROR_NONE) {
		g_debug("cb_position_from_address failed: error=%d\n", error);
		return;
	}

	g_list_foreach(pos_list, print_pos_nokia, NULL);
	g_list_foreach(pos_list, print_acc_nokia, NULL);
	g_printf("---cb position from address nokia end. \n");
}

static void cb_print_route_segment(gpointer data, gpointer user_data)
{
	g_printf("++Segment begin\n");
	LocationRouteSegment *seg = (LocationRouteSegment *)data;
	gdouble seg_dist = location_route_segment_get_distance(seg);
	glong seg_duration = location_route_segment_get_duration(seg);
	const LocationPosition *start = location_route_segment_get_start_point(seg);
	gdouble start_lat = 0;
	gdouble start_lon = 0;
	if (start) {
		start_lat = start->latitude;
		start_lon = start->longitude;
	} else {
		g_printf("Segment start position NULL\n");
	}
	const LocationPosition *end = location_route_segment_get_end_point(seg);
	gdouble end_lat = 0;
	gdouble end_lon = 0;
	if (end) {
		end_lat = end->latitude;
		end_lon = end->longitude;
	} else {
		g_printf("Segment end postion NULL\n");
	}
	g_printf("Segment info: Distance[%f], Duration[%ld], start(%f/%f), end(%f/%f)\n", seg_dist, seg_duration,
	         start_lat, start_lon, end_lat, end_lon);

	GList *step_list = location_route_segment_get_route_step(seg);
	GList *tmp_list = (GList *)step_list;
	if (tmp_list) {
		g_list_foreach(tmp_list, cb_print_route_step, NULL);
	}
	g_printf("--Segment end\n");
}


static void cb_print_route_list(gpointer data, gpointer user_data)
{
	g_printf("+Route begin\n");
	LocationRoute *route = (LocationRoute *)data;

	const LocationPosition *start = location_route_get_origin(route);
	gdouble start_lat = 0;
	gdouble start_lon = 0;
	if (start) {
		start_lat = start->latitude;
		start_lon = start->longitude;
	} else {
		g_printf("Route start position NULL\n");
	}
	const LocationPosition *end = location_route_get_destination(route);
	gdouble end_lat = 0;
	gdouble end_lon = 0;
	if (end) {
		end_lat = end->latitude;
		end_lon = end->longitude;
	} else {
		g_printf("Route end position NULL\n");
	}
	g_printf("Route: start(%f/%f), end(%f/%f)\n", start_lat, start_lon, end_lat, end_lon);

	gdouble distance = location_route_get_total_distance(route);
	const gchar *dis_unit = location_route_get_distance_unit(route);
	glong duration = location_route_get_total_duration(route);
	const LocationBoundary *bound = location_route_get_bounding_box(route);
	if (bound && bound->type == LOCATION_BOUNDARY_RECT) {
		g_printf("RECT left top[%f-%f], right bottom[%f-%f]\n", bound->rect.left_top->latitude, bound->rect.left_top->longitude,
		         bound->rect.right_bottom->latitude, bound->rect.right_bottom->longitude);
	} else {
		g_printf("route boundary not exist, or not RECT\n");
	}
	g_printf("Distance[%f], Distance unit[%s], Duration[%ld]\n", distance, dis_unit, duration);

	GList *seg_list = location_route_get_route_segment(route);
	if (seg_list) {
		g_list_foreach(seg_list, cb_print_route_segment, NULL);
	}

	g_printf("-Route end\n");
}

static void cb_route(LocationError error, guint req_id, GList *route_list, gchar *error_code, gchar *error_msg, gpointer userdata)
{
	g_printf("\n===== cb_route ======\n");
	if (error != LOCATION_ERROR_NONE) {
		g_printf("Failed :%d\n", error);
		return;
	}

	g_printf("Success, poi_list[0x%x] user_data[0x%x] req_id[%d]\n", (unsigned int)route_list, (unsigned int)userdata, req_id);

	if (route_list)
		g_list_foreach(route_list, cb_print_route_list, NULL);

	if (error_code && error_msg) {
		g_printf("cb_route: error_code[%s], error_msg[%s]\n", error_code, error_msg);
	}
}

static void cb_address(LocationError error, LocationAddress *addr, LocationAccuracy *acc, gpointer userdata)
{
	if (error != LOCATION_ERROR_NONE) {
		g_printf("cb_address failed: error=%d\n", error);
		return;
	}
	char str[STR_MAX];
	g_printf("userdata[0x%x] building number: [%s], street: [%s], state: [%s], country code: [%s], city: [%s], district: [%s], postal code: [%s]\n",
	         (unsigned int)userdata, addr->building_number, addr->street, addr->state, addr->country_code, addr->city, addr->district, addr->postal_code);
	GetAccuracyLevel(str, acc->level);
	g_printf("level: [%s], horizontal_accuracy: [%f], vertical_accuracy: [%f]\n", str, acc->horizontal_accuracy, acc->vertical_accuracy);
}

static void cb_position(LocationError error,
                        GList *pos_list,
                        GList *acc_list,
                        gpointer userdata)
{
	if (error != LOCATION_ERROR_NONE) {
		g_printf("cb_position failed: error=%d\n", error);
		return;
	}

	g_list_foreach(pos_list, PrintPos, NULL);
	g_list_foreach(acc_list, PrintAcc, NULL);
}


typedef struct {
	LocationObject *obj;
	LocationPosition *pos;
	LocationAddress *addr;
	gchar *str_addr;
	LocationPositionCB pos_cb;
	LocationAddressCB addr_cb;
	gpointer user_data;
} IdleData;

static gboolean idle_position_from_address_async(gpointer data)
{
	IdleData *idle_data = (IdleData *)data;
	guint req_id = 0;
	char str[STR_MAX];
	LocationGeocodePreference *pref = location_geocode_pref_new();
	int ret = location_map_get_position_from_address_async(idle_data->obj, idle_data->addr, pref, idle_data->pos_cb, idle_data->user_data, &req_id);
	GetLocationError(str, ret);
	if (ret != LOCATION_ERROR_NONE) {
		g_printf("Fail location_get_position_from_address_async Error[%s]\n", str);
	} else {
		g_printf("location_get_position_from_address_async, req_id %d\n", req_id);
	}
	location_address_free(idle_data->addr);
	g_free(idle_data);
	location_geocode_pref_free(pref);
	return FALSE;
}

static gboolean idle_position_from_freefromed_address_async(gpointer data)
{
	IdleData *idle_data = (IdleData *)data;
	char str[STR_MAX];
	guint req_id = 0;
	LocationGeocodePreference *pref = location_geocode_pref_new();
	int ret = location_map_get_position_from_freeformed_address_async(idle_data->obj, idle_data->str_addr, pref, idle_data->pos_cb, idle_data->user_data, &req_id);
	GetLocationError(str, ret);
	if (ret != LOCATION_ERROR_NONE) {
		g_printf("Fail location_get_position_from_freeformed_address_async Error[%s]\n", str);
	} else {
		g_printf("location_get_position_from_freeformed_address_async, req_id %d\n", req_id);
	}
	g_free(idle_data->str_addr);
	g_free(idle_data);
	location_geocode_pref_free(pref);
	return FALSE;
}

static gboolean idle_address_from_position_async(gpointer data)
{
	IdleData *idle_data = (IdleData *)data;
	char str[STR_MAX];
	guint reqid = 0;
	int ret = location_map_get_address_from_position_async(idle_data->obj, idle_data->pos, idle_data->addr_cb, idle_data->user_data, &reqid);
	GetLocationError(str, ret);
	if (ret != LOCATION_ERROR_NONE) {
		g_printf("Fail location_map_get_address_from_position_async Error[%s]\n", str);
	} else {
		g_printf("location_map_get_address_from_position_async, req_id %d\n", reqid);
	}
	location_position_free(idle_data->pos);
	g_free(idle_data);
	return FALSE;
}
static gboolean idle_location_map_get_position_from_address_async(gpointer data)
{
	g_printf("+++idle location map get position from address async begin\n");
	IdleData *idle_data = (IdleData *)data;
	guint req_id = 0;
	char str[STR_MAX];
	LocationGeocodePreference *pref = location_geocode_pref_new();
	LocationError err = location_map_get_position_from_address_async(idle_data->obj, idle_data->addr, pref, idle_data->pos_cb, idle_data->user_data, &req_id);

	if (err != LOCATION_ERROR_NONE) {
		g_printf("Fail location_get_position_from_address_async Error[%s]\n", str);
	} else {
		g_printf("location_get_position_from_address_async, req_id %d\n", req_id);
	}
	g_free(idle_data);
	location_geocode_pref_free(pref);
	g_printf("---idle location map get position from address async  end\n");
	return FALSE;
}

void _print_keys(gpointer data, gpointer user_data)
{
	g_printf(" %s ", (gchar *) data);
}

static void print_map_service_keys(LocationObject *obj, int type)
{
	GList *key = NULL;

	location_map_get_provider_capability_key(obj, type, &key);

	if (key) {
		g_list_foreach(key, (GFunc)_print_keys, NULL);
	}

	g_list_free_full(key, g_free);
}

void _print_provider(gpointer data, gpointer user_data)
{
	gchar *provider = (gchar *)data;

	g_printf("[%s] is supported\n", provider);
}

static void print_menu()
{
	g_printf("\n================================= Location API Test =================================\n");
	g_printf("q.   Exit\n");
	g_printf("1.   location_init\n");
	g_printf("2.   location_map_new for default \n");
	g_printf("2a.  location_map_new for decarta \n");
	g_printf("2b.  location_map_new for osm \n");
	g_printf("2c.   location_map_new for nlp \n");
	g_printf("3.   location_map_free\n");
	g_printf("4.   location_map_get_address_from_position_async\n");
	g_printf("5.   location_map_get_position_from_address_async\n");
	g_printf("6.   location_map_get_position_from_freeformed_address_async\n");
	g_printf("7.   location_map_search_poi_with_keyword\n");
	g_printf("7a.  location_map_search_poi_with_category\n");
	g_printf("7b.  location_map_search_poi_with_poi_name\n");
	g_printf("8.   location_map_search_poi_by_area_with_keyword\n");
	g_printf("8a.  location_map_search_poi_by_area_with_category\n");
	g_printf("8b.  location_map_search_poi_by_area_with_poi_name\n");
	g_printf("9.   location_map_search_poi_by_addr_with_keyword\n");
	g_printf("9a.  location_map_search_poi_by_addr_with_category\n");
	g_printf("9b.  location_map_search_poi_by_addr_with_poi_name\n");
	g_printf("10.   location_map_search_poi_by_freeformed_address_with_keyword\n");
	g_printf("10a.  location_map_search_poi_by_freeformed_address_with_category\n");
	g_printf("10b.  location_map_search_poi_by_freeformed_address_with_poi_name\n");
	g_printf("11.  location_map_cancel_poi_request\n");
	g_printf("12.  location_map_request_route\n");
	g_printf("12a. location_map_request_route for a long distance\n");
	g_printf("13.  location_map_cancel_route_request\n");
	g_printf("14.  location_map_is_supported_provider_capability\n");
	g_printf("15.  location_map_get_provider_capability_key\n");
	g_printf("16.  location_map_get_supported_providers \n");
	g_printf("17.  location_map_get_default_provider \n");
	g_printf("18.  location_map_set_provider \n");
	g_printf("19.  location_map_cancel_geocode_request. \n");
	g_printf("20.  location_map_get_map_version. \n");
	g_printf("21.  location_map_updatea_map_version[TRUE]. \n");
	g_printf("22.  location_map_updatea_map_version[FALSE]. \n");
	g_printf("23.  location_map_is_latest_map_data \n");
	g_printf("99.  change map provider to default\n");
	g_printf("99a. change map provider to decarta\n");
	g_printf("99b. change map provider to osm\n");

	g_printf("==================================== Property ====================================\n");
	PrintProperty(map_obj);
	g_printf("\n==================================================================================\n");
}

int main(int argc, char **argv)
{
	char strOpt[255];
	int ret;
	char str[STR_MAX];
	GError *gerr = NULL;
	guint req_id = 0;

	/* If application is executed by AUL, this is not needed. */
	g_setenv("PKG_NAME", "com.samsung.map-service-test", 1);

#if !GLIB_CHECK_VERSION (2, 35, 0)
	g_type_init();
#endif

#if !GLIB_CHECK_VERSION (2, 31, 0)
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
#endif

	GThread *g_main;
	g_main = g_thread_create(GmainThread, NULL, TRUE, &gerr);
	if (!g_main) {
		g_debug("Error create gmain thread: Err domain[%d] Err code[%d] Err msg[%s]",
		        gerr->domain, gerr->code, gerr->message);
		g_error_free(gerr);
		return 0;
	}

	g_printf("--- Start LBS Test App ---\n");
	while (1) {
		print_menu();
		g_printf("Select option: ");
		SelectOpt(strOpt);
		g_printf("======================================================================================\n");
		if (0 == g_strcmp0("x", strOpt)) {

		} else if (0 == g_strcmp0("1", strOpt)) {
			ret = location_init();
			GetLocationError(str, ret);
			g_printf("location_init: returned value [%s]\n", str);
		} else if (0 == g_strcmp0("2", strOpt)) {
			if (map_obj) {
				g_printf("Location object already existed: [0x%x]", (unsigned int)map_obj);
				continue;
			}
			map_obj = location_map_new(NULL);

			LocationMapPref *mapPref = location_map_pref_new();
			location_map_pref_set_maps_key(mapPref, "SpMLqvWrvuFmp27kV4e6/Md_LeHoPn0b4LPOxB60p4A");
			location_map_set_service_pref(map_obj, mapPref);

			if (map_obj) g_printf("Success\n");
			else             g_printf("Failed\n");
		} else if (0 == g_strcmp0("2a", strOpt)) {
			if (map_obj) {
				g_printf("Location object already existed: [0x%x]", (unsigned int)map_obj);
				continue;
			}
			map_obj = location_map_new("decarta");
			if (map_obj) g_printf("Success\n");
			else             g_printf("Failed\n");
		} else if (0 == g_strcmp0("2b", strOpt)) {
			if (map_obj) {
				g_printf("Location object already existed: [0x%x]", (unsigned int)map_obj);
				continue;
			}
			map_obj = location_map_new("osm");
			if (map_obj) g_printf("Success\n");
			else             g_printf("Failed\n");
		} else if (0 == g_strcmp0("2c", strOpt)) {
			if (map_obj) {
				g_printf("Location object already existed: [0x%x]", (unsigned int)map_obj);
				continue;
			}
			map_obj = location_map_new("nlp");

			LocationMapPref *mapPref = location_map_pref_new();
			location_map_pref_set_maps_key(mapPref, "SpMLqvWrvuFmp27kV4e6/Md_LeHoPn0b4LPOxB60p4A");
			location_map_set_service_pref(map_obj, mapPref);

			if (map_obj) {
				g_printf("Success\n");
			} else {
				g_printf("Failed\n");
			}
		} else if (0 == g_strcmp0("3", strOpt)) {
			ret = location_map_free(map_obj);
			map_obj = NULL;
			GetLocationError(str, ret);
			g_printf("location_map_free: returned value [%s]\n", str);
		} else if (0 == g_strcmp0("4", strOpt)) {
			IdleData *data = g_new0(IdleData, 1);
			data->obj = map_obj;
			data->addr_cb = cb_address;
			data->user_data = map_obj;

			g_printf("[0].San jose [1].Suwon HQ [*].Custom\n");
			g_printf("Select Position: ");
			int opt = PromptInt();
			if (opt == 0)       data->pos = location_position_new(0, 37.335276, -121.890059, 0, LOCATION_STATUS_2D_FIX);
			else if (opt == 1) data->pos = location_position_new(0, 37.257809, 127.056383, 0, LOCATION_STATUS_2D_FIX);
			else {
				g_printf("Input latitude: ");
				gdouble lat = PromptDB();
				g_printf("Input longitude: ");
				gdouble lon = PromptDB();
				data->pos = location_position_new(0, lat, lon, 0, LOCATION_STATUS_2D_FIX);
			}
			g_idle_add((GSourceFunc)idle_address_from_position_async, data);
		} else if (0 == g_strcmp0("5", strOpt)) {
			IdleData *data = g_new0(IdleData, 1);
			data->obj = map_obj;
			data->pos_cb = cb_position;
			data->user_data = map_obj;

			g_printf("[0].San jose [1].수원 삼성  [2].Suwon HQ [*].Custom\n");
			g_printf("Select Address: ");
			int opt = PromptInt();
			char *ret_str = NULL;
			if (opt == 0)      data->addr = location_address_new("1", "Post Street", NULL, "san jose", "ca", NULL, "95113", NULL, NULL, NULL);
			else if (opt == 1) data->addr = location_address_new(NULL, "삼성전자", "매탄3동", "수원시 영>통구", "경기도",  NULL, NULL, NULL, NULL, NULL);
			else if (opt == 2) data->addr = location_address_new(NULL, "Samsung Electro-Mechanics Co. LTD", "Maetan 3-dong", "Suwon Si Yeongtong-gu", "Gyeonggi-do", NULL, NULL, NULL, NULL, NULL);
			else {
				char building_number[255], street[255], state[255], country_code[255], city[255], district[255], postal_code[255];
				g_printf("Input building number: ");
				ret_str = fgets(building_number, 255, stdin);
				building_number[strlen(building_number) - 1] = '\0';
				g_printf("Input street: ");
				ret_str = fgets(street, 255, stdin);
				street[strlen(street) - 1] = '\0';
				g_printf("Input state: ");
				ret_str = fgets(state, 255, stdin);
				state[strlen(state) - 1] = '\0';
				g_printf("Input country code: ");
				ret_str = fgets(country_code, 255, stdin);
				country_code[strlen(country_code) - 1] = '\0';
				g_printf("Input city: ");
				ret_str = fgets(city, 255, stdin);
				city[strlen(city) - 1] = '\0';
				g_printf("Input district: ");
				ret_str = fgets(district, 255, stdin);
				district[strlen(district) - 1] = '\0';
				g_printf("Input postal code: ");
				ret_str = fgets(postal_code, 255, stdin);
				postal_code[strlen(postal_code) - 1] = '\0';
				data->addr = location_address_new(building_number, street, district, city, state, country_code, postal_code, NULL, NULL, NULL);
			}
			g_idle_add((GSourceFunc)idle_position_from_address_async, data);
		} else if (0 == g_strcmp0("6", strOpt)) {
			IdleData *data = g_new0(IdleData, 1);
			data->obj = map_obj;
			data->pos_cb = cb_position;
			data->user_data = map_obj;
			g_printf("[0].San jose [1].수원 삼성  [2].Suwon HQ [*].Custom\n");
			g_printf("Select Address: ");
			int opt = PromptInt();
			char *ret_str = NULL;
			if (opt == 0) {
				data->str_addr = g_strdup("4 N 2nd Street 95113");
			} else if (opt == 1) {
				data->str_addr = g_strdup("경기도 수원시 영통구 매탄 3동 삼성전자");
			} else if (opt == 2) {
				data->str_addr = g_strdup("Samsung Electronics Co. LTD Maetan 3-dong, Suwon Si Yeongtong-gu, Gyeonggi-Do (Seoul 443-742 Korea), Rep of KOREA");
			} else {
				char buf[255];
				g_printf("Input freeform address: ");
				ret_str = fgets(buf, 255, stdin);
				if (NULL == ret_str) {
					g_printf(" fgets return NULL \n");
				}
				buf[strlen(buf) - 1] = '\0';
				data->str_addr = g_strdup(buf);
			}
			g_idle_add((GSourceFunc)idle_position_from_freefromed_address_async, data);

		} else if (0 == g_strcmp0("7", strOpt)) {
			g_printf("location_map_search_poi\n");

			LocationPOIFilter *filter = location_poi_filter_new();
			location_poi_filter_set(filter, "KEYWORD", "pizza");

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 2);
			location_poi_pref_set_sort_by(pref, "Distance");
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_DESC);

			LocationPosition *position = location_position_new(0, 37.771008, -122.41175, 0, LOCATION_STATUS_2D_FIX);

			ret = location_map_search_poi(map_obj, filter, position, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI. Error[%s]\n", str);
			} else {
				g_printf("Seach POI success, req_id %d\n", req_id);
			}

			location_poi_filter_free(filter);
			location_poi_pref_free(pref);
			location_position_free(position);
		} else if (0 == g_strcmp0("7a", strOpt)) {
			g_printf("location_map_search_poi_with_category\n");
			LocationPOIFilter *filter = location_poi_filter_new();
			location_poi_filter_set(filter, "CATEGORY", "eat_drink");

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 5);
			location_poi_pref_set_sort_by(pref, "Distance");
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);

			LocationPosition *position = location_position_new(0, 37.771008, -122.41175, 0, LOCATION_STATUS_2D_FIX);

			ret = location_map_search_poi(map_obj, filter, position, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI. Error[%s]\n", str);
			} else {
				g_printf("Seach POI success, req_id %d\n", req_id);
			}

			location_poi_filter_free(filter);
			g_printf("location_search_poi_by_freeformed_address_with_poi_name\n");
			location_poi_pref_free(pref);
			location_position_free(position);

		} else if (0 == g_strcmp0("7b", strOpt)) {
			g_printf("location_map_search_poi_with_poi_name\n");

			LocationPOIFilter *filter = location_poi_filter_new();
			location_poi_filter_set(filter, "POIName", "cafe");

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item);
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_DESC);

			LocationPosition *position = location_position_new(0, 37.771008, -122.41175, 0, LOCATION_STATUS_2D_FIX);

			ret = location_map_search_poi(map_obj, filter, position, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI. Error[%s]\n", str);
			} else {
				g_printf("Seach POI success, req_id %d\n", req_id);
			}

			location_poi_filter_free(filter);
			location_poi_pref_free(pref);
			location_position_free(position);

		} else if (0 == g_strcmp0("8", strOpt)) {
			g_printf("location_map_search_poi_by_area_with_keyword\n");

			/* only circle supported by decarta */
			LocationPosition *center = location_position_new(0, 37.336723, -121.889555, 0, LOCATION_STATUS_2D_FIX);
			gdouble radius = 400;
			LocationBoundary *bound = location_boundary_new_for_circle(center, radius);

			LocationPOIFilter *filter = location_poi_filter_new();
			gchar *key = g_strdup("KEYWORD");
			gchar *value = g_strdup("cafe");
			location_poi_filter_set(filter, key, value); /* same with Type ?  CATEGORY, KEYWORD, POIName */

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item); /* can't set const char* directly !! */
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);  /*LOCATION_POI_PREF_SO_ASC */


			ret = location_map_search_poi_by_area(map_obj, filter, bound, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by area. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by area sucess, req_id %d\n", req_id);
			}

			g_free(key);
			g_free(value);
			g_free(item);
			location_position_free(center);
			location_boundary_free(bound);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);
		} else if (0 == g_strcmp0("8a", strOpt)) {
			g_printf("location_map_search_poi_by_area_with_category\n");

			/* only circle supported by decarta */
			LocationPosition *center = location_position_new(0, 37.336723, -121.889555, 0, LOCATION_STATUS_2D_FIX);
			gdouble radius = 400;
			LocationBoundary *bound = location_boundary_new_for_circle(center, radius);

			LocationPOIFilter *filter = location_poi_filter_new();
			gchar *key = g_strdup("CATEGORY");
			gchar *value = g_strdup("restaurant");
			location_poi_filter_set(filter, key, value);

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 5);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item);
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);

			ret = location_map_search_poi_by_area(map_obj, filter, bound, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by area. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by area sucess, req_id %d\n", req_id);
			}

			g_free(key);
			g_free(value);
			g_free(item);
			location_position_free(center);
			location_boundary_free(bound);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);

		} else if (0 == g_strcmp0("8b", strOpt)) {
			g_printf("location_map_search_poi_by_area_with_poi_name\n");

			/* only circle supported by decarta */
			LocationPosition *center = location_position_new(0, 37.336723, -121.889555, 0, LOCATION_STATUS_2D_FIX);
			gdouble radius = 400;
			LocationBoundary *bound = location_boundary_new_for_circle(center, radius);

			LocationPOIFilter *filter = location_poi_filter_new();
			gchar *key = g_strdup("POIName");
			gchar *value = g_strdup("cafe");
			location_poi_filter_set(filter, key, value); /* same with Type ?  CATEGORY, KEYWORD, POIName */

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item); /* can't set const char* directly !! */
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);  /*LOCATION_POI_PREF_SO_ASC */


			ret = location_map_search_poi_by_area(map_obj, filter, bound, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by area. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by area sucess, req_id %d\n", req_id);
			}

			g_free(key);
			g_free(value);
			g_free(item);
			location_position_free(center);
			location_boundary_free(bound);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);

		} else if (0 == g_strcmp0("9", strOpt)) {
			g_printf("location_map_search_poi_by_address_with_keyword\n");

			LocationAddress *addr = location_address_new("51", "N SAN PEDRO ST", NULL, "SAN JOSE", "SANTA CLARA", "CA", "95110", NULL, NULL, NULL);

			LocationPOIFilter *filter = location_poi_filter_new();
			gchar *key = g_strdup("KEYWORD");
			gchar *value = g_strdup("cafe");
			location_poi_filter_set(filter, key, value);

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item);
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);

			ret = location_map_search_poi_by_address(map_obj, filter, addr, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by address. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by address sucess, req_id %d\n", req_id);
			}

			g_free(key);
			g_free(value);
			g_free(item);

			location_address_free(addr);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);
		} else if (0 == g_strcmp0("9a", strOpt)) {
			g_printf("location_search_poi_by_address_with_category\n");

			LocationAddress *addr = location_address_new("51", "N SAN PEDRO ST", NULL, "SAN JOSE", "SANTA CLARA", "CA", "95110", NULL, NULL, NULL);

			LocationPOIFilter *filter = location_poi_filter_new();
			location_poi_filter_set(filter, "CATEGORY", "restaurant");

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			location_poi_pref_set_sort_by(pref, "Distance");
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);

			ret = location_map_search_poi_by_address(map_obj, filter, addr, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by address. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by address sucess, req_id %d\n", req_id);
			}

			location_address_free(addr);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);
		} else if (0 == g_strcmp0("9b", strOpt)) {
			g_printf("location_map_search_poi_by_address_with_poi_name\n");

			LocationAddress *addr = location_address_new("51", "N SAN PEDRO ST", NULL, "SAN JOSE", "SANTA CLARA", "CA", "95110", NULL, NULL, NULL);

			LocationPOIFilter *filter = location_poi_filter_new();
			gchar *key = g_strdup("POIName");
			gchar *value = g_strdup("cafe");
			location_poi_filter_set(filter, key, value);

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item);
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);

			ret = location_map_search_poi_by_address(map_obj, filter, addr, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by address. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by address sucess, req_id %d\n", req_id);
			}

			g_free(key);
			g_free(value);
			g_free(item);

			location_address_free(addr);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);

		} else if (0 == g_strcmp0("10", strOpt)) {
			g_printf("location_map_search_poi_by_freeformed_address_with_keyword\n");

			gchar *addr = g_strdup("North Second St.");

			LocationPOIFilter *filter = location_poi_filter_new();
			location_poi_filter_set(filter, "KEYWORD", "station");

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item); /* can't set const char* directly !! */
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);  /*LOCATION_POI_PREF_SO_ASC */

			ret = location_map_search_poi_by_freeformed_address(map_obj, filter, addr, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by address. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by address success, req_id %d\n", req_id);
			}
			g_free(item);

			g_free(addr);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);
		} else if (0 == g_strcmp0("10a", strOpt)) {
			g_printf("location_map_search_poi_by_freeformed_address_with_category\n");

			gchar *addr = g_strdup("North Second St.");

			LocationPOIFilter *filter = location_poi_filter_new();
			gchar *key = g_strdup("CATEGORY");
			gchar *value = g_strdup("restaurant");
			location_poi_filter_set(filter, key, value);

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			gchar *item = g_strdup("Distance");
			location_poi_pref_set_sort_by(pref, item);
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);

			ret = location_map_search_poi_by_freeformed_address(map_obj, filter, addr, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by address. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by address success, req_id %d\n", req_id);
			}

			g_free(key);
			g_free(value);
			g_free(item);

			g_free(addr);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);
		} else if (0 == g_strcmp0("10b", strOpt)) {
			g_printf("location_map_search_poi_by_freeformed_address_with_poi_name\n");

			gchar *addr = g_strdup("North Second St.");

			LocationPOIFilter *filter = location_poi_filter_new();
			location_poi_filter_set(filter, "POIName", "pizza");

			LocationPOIPreference *pref = location_poi_pref_new();
			location_poi_pref_set_max_result(pref, 10);
			location_poi_pref_set_sort_by(pref, "Distance");
			location_poi_pref_set_sort_order(pref, LOCATION_POI_PREF_SO_ASC);

			ret = location_map_search_poi_by_freeformed_address(map_obj, filter, addr, pref, cb_poi, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search POI by address. Error[%s]\n", str);
			} else {
				g_printf("Seach POI by address success, req_id %d\n", req_id);
			}

			g_free(addr);
			location_poi_filter_free(filter);
			location_poi_pref_free(pref);

		} else if (0 == g_strcmp0("11", strOpt)) {
			int req_id;
			int len = 0;

			g_printf("Input ReqID : ");
			len = scanf("%d", &req_id);

			ret = location_map_cancel_poi_request(map_obj, req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to cancel POI request. Error[%s]\n", str);
			} else
				g_printf("location_map_cancel_poi_request, req_id %d\n", req_id);
		} else if (0 == g_strcmp0("12", strOpt)) {
			g_printf("location_map_request_route\n\n");

			LocationPosition *origin = location_position_new(0, 37.564263, 126.974676, 0, LOCATION_STATUS_2D_FIX);	/* Seoul city hall */
			LocationPosition *destination = location_position_new(0, 37.557120, 126.992410, 0, LOCATION_STATUS_2D_FIX);	/* NamSan */

			GList *waypoint = NULL;
			LocationPosition *via_pos = location_position_new(0, 37.560950, 126.986240, 0, LOCATION_STATUS_2D_FIX); /* Wangsimli */
			waypoint = g_list_append(waypoint, (gpointer)via_pos);

			LocationRoutePreference *pref = location_route_pref_new();
			location_route_pref_set_route_type(pref, "FASTEST");

			LocationRoadElementPenalty *penalty = NULL;
			penalty = location_route_element_penalty_new();

			location_route_element_penalty_set_id(penalty, "Penalty1");
			location_route_element_penalty_set_direction(penalty, DIR_BOTH);
			location_route_element_penalty_set_penalty(penalty, 8);
			location_route_element_penalty_set_speed(penalty, 90);
			location_route_element_penalty_set_validity_start_time(penalty, 146);
			location_route_element_penalty_set_validity_end_time(penalty, 244);

			LocationRouteOptions *options = location_route_options_new();
			if (NULL == options) {
				g_printf("\n ERROR: options is NULL \n");
			}
			GList *penalty_list = NULL;
			penalty_list = g_list_append(penalty_list, penalty);
			if (NULL == penalty_list) {
				g_printf("\n ERROR: penalty_list is NULL \n");
			}
			location_route_options_set_road_element_penalty(options, penalty_list);
			location_route_options_set_start_direction(options, 45);
			location_route_options_set_walk_time_multiplier(options, (gfloat)10);
			location_route_options_set_minimum_change_time(options, 30);
			location_route_options_set_transit_type_allowed(options, TRANSIT_TYPE_RAIL_LIGHT, TRUE);
			location_route_options_set_maximum_changes(options, 2);
			location_route_options_set_departure_time(options, 1373282862);
			/*location_route_options_set_arrival_time(options, 244); */

			location_route_pref_set_options(pref, options);
			location_route_options_free(options);

			LocationMapPref *mapPref = location_map_pref_new();
			location_map_pref_set_maps_key(mapPref, "SpMLqvWrvuFmp27kV4e6/Md_LeHoPn0b4LPOxB60p4A");
			location_map_set_service_pref(map_obj, mapPref);

			ret = location_map_request_route(map_obj, origin, destination, waypoint, pref, cb_route, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search route by address. Error[%s]\n", str);
			} else {
				g_printf("Search Route successfully, req_id %d\n", req_id);
			}
		} else if (0 == g_strcmp0("12a", strOpt)) {
			g_printf("location_map_request_route for a long distance\n");

			LocationPosition *origin = location_position_new(0, 29.783449, -95.373688, 0, LOCATION_STATUS_2D_FIX);
			LocationPosition *destination = location_position_new(0, 39.749962, -104.984665, 0, LOCATION_STATUS_2D_FIX);

			GList *waypoint = NULL;

			LocationRoutePreference *pref = location_route_pref_new();
			location_route_pref_set_route_type(pref, "FASTEST");

			LocationMapPref *mapPref = location_map_pref_new();
			location_map_pref_set_maps_key(mapPref, "SpMLqvWrvuFmp27kV4e6/Md_LeHoPn0b4LPOxB60p4A");
			location_map_set_service_pref(map_obj, mapPref);

			ret = location_map_request_route(map_obj, origin, destination, waypoint, pref, cb_route, NULL, &req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to search route by address. Error[%s]\n", str);
			} else {
				g_printf("Search Route successfully, req_id %d\n", req_id);
			}


		} else if (0 == g_strcmp0("13", strOpt)) {
			g_printf("location_map_cancel_route_request\n");

			int req_id;
			int len;
			g_printf("Input ReqID : ");
			len = scanf("%d", &req_id);

			ret = location_map_cancel_route_request(map_obj, req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to cancel route request. Error[%s]\n", str);
			} else {
				g_printf("location_map_cancel_route_request, req_id %d\n", req_id);
			}

		} else if (0 == g_strcmp0("14", strOpt)) {
			int idx = 0;
			for (idx = 0; idx < MAP_SERVICE_TYPE_MAX; idx++) {
				g_printf("[%d:%d], ", idx, location_map_is_supported_provider_capability(map_obj, idx));
			}
		} else if (0 == g_strcmp0("15", strOpt)) {
			int idx = 0;
			for (idx = 0; idx < MAP_SERVICE_TYPE_MAX; idx++) {
				g_printf("[%d:", idx);
				print_map_service_keys(map_obj, idx);
				g_printf("]\n");
			}
		} else if (0 == g_strcmp0("16", strOpt)) {
			GList *list = location_map_get_supported_providers(map_obj);
			if (list) {
				g_list_foreach(list, _print_provider, NULL);
				g_list_free_full(list, g_free);
			} else {
				g_printf("Fail to get supported_providers\n");
			}
		} else if (0 == g_strcmp0("17", strOpt)) {
			gchar *default_provider = location_map_get_default_provider(map_obj);
			if (default_provider) {
				g_printf("Defaut : [%s]\n", default_provider);
				g_free(default_provider);
			} else {
				g_printf("Fail to get default provider\n");
			}
		} else if (0 == g_strcmp0("18", strOpt)) {
			g_printf("[0].default [1].decarta [2].osm\n");
			g_printf("Select provider: ");
			int opt = PromptInt();
			gboolean ret = FALSE;
			switch (opt) {
				case 0:
					ret = location_map_set_provider(map_obj, NULL);
					break;
				case 1:
					ret = location_map_set_provider(map_obj, "decarta");
					break;
				case 2:
					ret = location_map_set_provider(map_obj, "osm");
					break;
				default:
					break;
			}

			if (ret == TRUE) {
				g_printf("Success to set provider [%d]\n", opt);
			} else {
				g_printf("Fail to set provider [%d]\n", opt);
			}
		} else if (0 == g_strcmp0("19", strOpt)) {
			int req_id;
			int len = 0;

			g_printf("Input ReqID : ");
			len = scanf("%d", &req_id);

			ret = location_map_cancel_geocode_request(map_obj, req_id);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to cancel Geocode request. Error[%s]\n", str);
			} else
				g_printf("location_map_cancel_geocode_request, req_id %d\n", req_id);
		} else if (0 == g_strcmp0("20", strOpt)) {
			g_printf("Get map version\n");
			gchar *version = NULL;
			ret = location_map_get_map_version(map_obj, &version);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to get map version[%d]\n", ret);
			} else {
				g_printf("Version : %s\n", version);
			}
			if (version) g_free(version);
		} else if (0 == g_strcmp0("21", strOpt)) {
			ret = location_map_update_map_data(map_obj, TRUE);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to update map data[%d]\n", ret);
			} else {
				g_printf("Success to updata map data. [TRUE] \n");
			}
		} else if (0 == g_strcmp0("22", strOpt)) {
			ret = location_map_update_map_data(map_obj, FALSE);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to update map data[%d]\n", ret);
			} else {
				g_printf("Success to updata map data. [FALSE] \n");
			}
		} else if (0 == g_strcmp0("23", strOpt)) {
			gboolean is_lastest_map_data_flag = FALSE;
			ret = location_map_is_latest_map_data(map_obj, &is_lastest_map_data_flag);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to check the latest map data. Error Code [%d]\n", ret);
			} else {
				g_printf("Success to check the latest map data. [%s]\n", is_lastest_map_data_flag ? "TRUE" : "FALSE");
			}
		} else if (0 == g_strcmp0("99", strOpt)) {
			if (map_obj) {
				g_object_set(map_obj, "provider", NULL, NULL);
			}
		} else if (0 == g_strcmp0("99a", strOpt)) {
			if (map_obj) {
				g_object_set(map_obj, "provider", "decarta", NULL);
			}
		} else if (0 == g_strcmp0("99b", strOpt)) {
			if (map_obj) {
				g_object_set(map_obj, "provider", "osm",  NULL);
			}
		} else if (0 == g_strcmp0("q", strOpt)) {
			g_main_loop_quit(g_mainloop);
			break;
		}
	}
	g_thread_join(g_main);
	g_printf("\n--- Exit LBS Test App ---\n");
	return 1;
}
