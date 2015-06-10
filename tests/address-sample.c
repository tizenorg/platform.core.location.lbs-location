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

#include <location.h>
#include <location-geocode.h>
#include <location-map-service.h>

static GMainLoop *loop = NULL;

static gboolean
exit_program(gpointer data)
{
	g_main_loop_quit(loop);
	g_debug("Quit g_main_loop");
	return FALSE;
}

static void
print_pos(gpointer data, gpointer user_data)
{
	LocationPosition *pos = (LocationPosition *)data;

	if (pos) {
		g_debug("time: %d, lat: %f, long: %f, alt: %f, status: %d",
		        pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
		location_position_free(pos);
	}
}

static void
print_acc(gpointer data, gpointer user_data)
{
	LocationAccuracy *acc = (LocationAccuracy *)data;

	if (acc) {
		g_debug("\tAccuracy level %d (%.0f meters %.0f meters)", acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_accuracy_free(acc);
	}
}
static void
cb_address(LocationError error,
           LocationAddress *addr,
           LocationAccuracy *acc,
           gpointer userdata)
{
	if (error != LOCATION_ERROR_NONE) {
		g_debug("cb_address failed: error=%d\n", error);
		return;
	}
	g_debug("ASYNC>> location_map_get_address_async> %s %s %s %s %s %s %s",
	        addr->building_number, addr->street, addr->district, addr->city, addr->state, addr->postal_code, addr->country_code);
	g_debug("\tAccuracy level %d (%.0f meters %.0f meters)", acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
}

static void
cb_service_disabled(GObject *self,
                    guint status,
                    gpointer userdata)
{
	g_debug("cb_service_disabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);
}

static void
cb_position_from_address(LocationError error, GList *pos_list, GList *acc_list, gpointer userdata)
{
	if (error != LOCATION_ERROR_NONE) {
		g_debug("cb_position_from_address failed: error=%d\n", error);
		return;
	}

	g_list_foreach(pos_list, print_pos, NULL);
	g_list_foreach(acc_list, print_acc, NULL);
}

static void
cb_position_from_freeformed_address(LocationError error, GList *pos_list, GList *acc_list, gpointer userdata)
{
	if (error != LOCATION_ERROR_NONE) {
		g_debug("cb_position_from_freeformed_address failed: error=%d\n", error);
		return;
	}

	g_list_foreach(pos_list, print_pos, NULL);
	g_list_foreach(acc_list, print_acc, NULL);
}

static void
cb_address_from_position(LocationError error, LocationAddress *addr, LocationAccuracy *acc, gpointer userdata)
{
	if (error != LOCATION_ERROR_NONE) {
		g_debug("cb_address_from_position failed: error=%d\n", error);
		return;
	}
	g_debug("ASYNC>> location_map_get_address_from_position_async> %s %s %s %s %s %s %s",
	        addr->building_number, addr->street, addr->district, addr->city, addr->state, addr->postal_code, addr->country_code);
	g_debug("\tAccuracy level %d (%.0f meters %.0f meters)", acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
}

static void
cb_service_enabled(GObject *self,
                   guint status,
                   gpointer userdata)
{
	g_debug("cb_service_enabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);
}

static gboolean
async_request(gpointer loc)
{
	guint req_id = 0;
	LocationGeocodePreference *pref = location_geocode_pref_new();
	location_geocode_pref_set_max_result(pref, 25);

	LocationAddress *addr = location_address_new("1", "Post Street", NULL, "san jose", "ca", NULL, "95113", NULL, NULL, NULL);
	LocationError err = location_map_get_position_from_address_async(loc, addr, pref, cb_position_from_address, loc, &req_id);
	if (LOCATION_ERROR_NONE == err)
		g_debug("location_map_get_position_from_address_async() success");
	else g_warning("location_map_get_position_from_address_async() failed> error code:%d", err);
	location_address_free(addr);

	gchar *addr_str = g_strdup("4 N 2nd Street 95113");
	err = location_map_get_position_from_freeformed_address_async(loc, addr_str, pref, cb_position_from_freeformed_address, loc, &req_id);
	if (LOCATION_ERROR_NONE == err)
		g_debug("location_map_get_position_from_freeformed_address_async() success");
	else g_warning("location_map_get_position_from_freeformed_address_async() failed> error code:%d", err);
	g_free(addr_str);

	LocationPosition *pos = location_position_new(0, 37.3322, -121.8720, 0, LOCATION_STATUS_2D_FIX);
	err = location_map_get_address_from_position_async(loc, pos, cb_address_from_position, loc, &req_id);
	if (LOCATION_ERROR_NONE == err)
		g_debug("location_map_get_address_from_position_async() success");
	else g_warning("location_map_get_address_from_position_async() failed> error code:%d", err);
	location_position_free(pos);
	location_geocode_pref_free(pref);
	return FALSE;
}

int
main(int argc, char *argv[])
{
	LocationMapObject *loc = NULL;

	/* If application is executed by AUL, this is not needed. */
	g_setenv("PKG_NAME", "com.samsung.address-sample", 1);

#if !GLIB_CHECK_VERSION (2, 35, 0)
	g_type_init();
#endif

	location_init();
	loop = g_main_loop_new(NULL, TRUE);

	loc = location_map_new(NULL);
	if (!loc) {
		g_warning("location_map_new failed");
		return -1;
	}

	LocationPosition *pos = NULL;
	LocationAccuracy *acc = NULL;
	GList *pos_list = NULL;
	GList *acc_list = NULL;
	LocationAddress *addr = NULL;

	addr = location_address_new("1", "Post Street", NULL, "san jose", "ca", NULL, "95113", NULL, NULL, NULL);
	LocationError err = location_map_get_position_from_address(loc, addr, &pos_list, &acc_list);
	if (LOCATION_ERROR_NONE == err) {
		g_list_foreach(pos_list, print_pos, NULL);
		g_list_foreach(acc_list, print_acc, NULL);
	} else g_warning("SYNC>>>location_map_get_position_from_address() failed> error code:%d", err);
	location_address_free(addr);

	char *addr_str = g_strdup("4 N 2nd Street 95113");
	err = location_map_get_position_from_freeformed_address(loc, addr_str, &pos_list, &acc_list);
	if (LOCATION_ERROR_NONE == err) {
		g_list_foreach(pos_list, print_pos, NULL);
		g_list_foreach(acc_list, print_acc, NULL);
	} else g_warning("SYNC>> location_map_get_position_from_freeformed_address() failed> error code:%d", err);
	g_free(addr_str);

	pos = location_position_new(0, 37.3322, -121.8720, 0, LOCATION_STATUS_2D_FIX);
	err = location_map_get_address_from_position(loc, pos, &addr, &acc);
	if (LOCATION_ERROR_NONE == err) {
		g_debug("SYNC>> location_map_get_address_from_position() success> %s %s %s %s %s %s %s",
		        addr->building_number, addr->street, addr->district, addr->city, addr->state, addr->postal_code, addr->country_code);
		g_debug("\tAccuracy level %d (%.0f meters %.0f meters)", acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_address_free(addr);
		location_accuracy_free(acc);
	} else g_warning("SYNC>> location_map_get_address_from_position() failed> error code:%d", err);
	location_position_free(pos);

	g_signal_connect(loc, "service-enabled", G_CALLBACK(cb_service_enabled), loc);
	g_signal_connect(loc, "service-disabled", G_CALLBACK(cb_service_disabled), loc);

	g_timeout_add_seconds(3, async_request, loc);

	g_timeout_add_seconds(60, exit_program, NULL);
	g_main_loop_run(loop);

	location_map_free(loc);

	return 0;
}
