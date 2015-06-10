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
#include <json-glib.h>
#include "location.h"

JsonParser *parser;
JsonNode *root;

static int _get_polygon_position_count(int polygon_index)
{
	if (parser == NULL || root == NULL) return 0;

	JsonObject *polygon_obj = json_array_get_object_element(json_node_get_array(root), polygon_index);
	if (polygon_obj == NULL) return 0;

	JsonArray *positions = json_object_get_array_member(polygon_obj, "positions");
	if (positions == NULL) return 0;

	return json_array_get_length(positions);
}

static LocationPosition *_get_position_from_polygon(int polygon_index, int pos_index)
{
	double latitude = 0.0;
	double longitude = 0.0;
	LocationPosition *position = NULL;
	if (parser == NULL || root == NULL) return NULL;

	JsonObject *polygon_obj = json_array_get_object_element(json_node_get_array(root), polygon_index);
	JsonArray *pos_array = json_object_get_array_member(polygon_obj, "positions");
	JsonObject *pos = json_array_get_object_element(pos_array, pos_index);

	latitude = json_object_get_double_member(pos, "latitude");
	longitude = json_object_get_double_member(pos, "longitude");

	if (latitude == 0.0 || longitude == 0.0) return NULL;

	position = location_position_new(0, latitude, longitude, 0.0, LOCATION_STATUS_2D_FIX);

	return position;
}

static LocationPosition *_get_marker_position_from_polygon(int polygon_index, int pos_index)
{
	double latitude = 0.0;
	double longitude = 0.0;
	LocationPosition *position = NULL;
	if (parser == NULL || root == NULL) return NULL;

	JsonObject *polygon_obj = json_array_get_object_element(json_node_get_array(root), polygon_index);
	JsonArray *pos_array = json_object_get_array_member(polygon_obj, "marker_position");
	JsonObject *pos = json_array_get_object_element(pos_array, pos_index);

	latitude = json_object_get_double_member(pos, "latitude");
	longitude = json_object_get_double_member(pos, "longitude");

	if (latitude == 0.0 || longitude == 0.0) return NULL;

	position = location_position_new(0, latitude, longitude, 0.0, LOCATION_STATUS_2D_FIX);

	return position;
}

static void _free_position_list(gpointer data)
{
	if (data == NULL) return;

	LocationPosition *position = (LocationPosition *) data;

	location_position_free(position);
}


LocationBoundary *json_util_get_polygon_boundary(int polygon_index)
{
	if (parser == NULL || root == NULL) {
		g_printf("invalid param parser[%d], root[%d]\n", parser, root);
		return NULL;
	}
	GList *position_list = NULL;
	LocationBoundary *boundary = NULL;
	int index = 0;
	int pos_count = _get_polygon_position_count(polygon_index);
	if (pos_count == 0) return NULL;

	for (index = 0; index < pos_count; index++) {
		position_list = g_list_append(position_list, _get_position_from_polygon(polygon_index, index));
	}

	boundary = location_boundary_new_for_polygon(position_list);

	g_list_free_full(position_list, (GDestroyNotify)_free_position_list);

	return boundary;
}


/* Polygon boundary */
int json_util_get_polygon_count(void)
{
	JsonArray *array = json_node_get_array(root);

	return json_array_get_length(array);
}

char *json_util_get_polygon_name(int polygon_index)
{
	char *name = NULL;
	JsonObject *polygon_obj = json_array_get_object_element(json_node_get_array(root), polygon_index);

	name = (char *)json_object_get_string_member(polygon_obj, "name");
	if (name == NULL) return NULL;

	return g_strdup(name);
}

/* Test Marker */
char *json_util_get_marker_name(int polygon_index, int pos_index)
{
	char *result = NULL;
	if (parser == NULL || root == NULL) return NULL;

	JsonObject *polygon_obj = json_array_get_object_element(json_node_get_array(root), polygon_index);
	JsonArray *pos_array = json_object_get_array_member(polygon_obj, "marker_position");
	JsonObject *pos = json_array_get_object_element(pos_array, pos_index);

	result = (char *)json_object_get_string_member(pos, "where");
	if (result == NULL) return NULL;

	return g_strdup(result);
}

int json_util_get_marker_position_count(int polygon_index)
{
	if (parser == NULL || root == NULL) return 0;

	JsonObject *polygon_obj = json_array_get_object_element(json_node_get_array(root), polygon_index);
	JsonArray *marker_position = json_object_get_array_member(polygon_obj, "marker_position");

	return json_array_get_length(marker_position);
}

LocationPosition *json_util_get_marker_position(int polygon_index, int marker_index)
{
	if (parser == NULL || root == NULL) return NULL;

	LocationPosition *position = NULL;
	position = _get_marker_position_from_polygon(polygon_index, marker_index);

	return position;
}

char *json_util_result_zone_test(int polygon_index, int marker_index)
{
	if (parser == NULL || root == NULL) return NULL;

	char *result = NULL;
	JsonObject *polygon_obj = json_array_get_object_element(json_node_get_array(root), polygon_index);
	JsonArray *pos_array = json_object_get_array_member(polygon_obj, "marker_position");
	JsonObject *pos = json_array_get_object_element(pos_array, marker_index);

	result = (char *)json_object_get_string_member(pos, "result");
	if (result == NULL) return NULL;

	return g_strdup(result);
}

void json_util_init(const char *file_name)
{
	g_print("Enter init_json_parser");
	GError *error;
	gboolean ret = FALSE;
	if (parser != NULL) return;

	parser = json_parser_new();

	error = NULL;
	ret = json_parser_load_from_file(parser, file_name, &error);
	if (ret == FALSE) {
		g_print("Unable to parser[%s]:[%s]\n", file_name, error->message);
		return ;
	}

	root = json_parser_get_root(parser);
}
