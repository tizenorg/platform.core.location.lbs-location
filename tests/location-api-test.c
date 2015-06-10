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
#include <stdlib.h>
#include <glib/gprintf.h>
#include <location.h>
#include "location-api-test-util.h"

#define STR_MAX	128
LocationObject *location_obj = NULL;
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

static void GetStatus(char str[STR_MAX], LocationStatus acc_level)
{
	switch (acc_level) {
		case LOCATION_STATUS_NO_FIX:
			g_utf8_strncpy(str, "LOCATION_STATUS_NO_FIX", STR_MAX);
			break;
		case LOCATION_STATUS_2D_FIX:
			g_utf8_strncpy(str, "LOCATION_STATUS_2D_FIX", STR_MAX);
			break;
		case LOCATION_STATUS_3D_FIX:
			g_utf8_strncpy(str, "LOCATION_STATUS_3D_FIX", STR_MAX);
			break;
		default:
			g_utf8_strncpy(str, "Error: undefined status code", STR_MAX);
	}
}

static void GetMethod(char str[STR_MAX], LocationMethod method)
{
	switch (method) {
		case LOCATION_METHOD_HYBRID:
			g_utf8_strncpy(str, "LOCATION_METHOD_HYBRID", STR_MAX);
			break;
		case LOCATION_METHOD_GPS:
			g_utf8_strncpy(str, "LOCATION_METHOD_GPS", STR_MAX);
			break;
		case LOCATION_METHOD_WPS:
			g_utf8_strncpy(str, "LOCATION_METHOD_WPS", STR_MAX);
			break;
		default:
			g_utf8_strncpy(str, "Error: undefined method", STR_MAX);
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
		g_printf("fgets return NULL. \n");
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
	if (NULL == str) {
		g_printf("fgets return NULL. \n");
	}
	buf[strlen(buf) - 1] = '\0';
	ret = g_ascii_strtoll(buf, NULL, 10);
	return ret;
}

gulong g_sig_enable = 0;
gulong g_sig_disable = 0;
gulong g_sig_update = 0;
gulong g_sig_zonein = 0;
gulong g_sig_zoneout = 0;

static void PrintPolygonPosition(gpointer data, gpointer user_data)
{
	LocationPosition *position = (LocationPosition *) data;
	g_printf("[%lf %lf %lf] ", position->latitude, position->longitude, position->altitude);
}

static void PrintBoundary(LocationBoundary *boundary, void *user_data)
{
	if (boundary == NULL) return;
	if (boundary->type == LOCATION_BOUNDARY_CIRCLE) {
		g_printf("\n\tCIRCLE: center[%lf %lf %lf] radius[%lf]",
		         boundary->circle.center->latitude, boundary->circle.center->longitude, boundary->circle.center->altitude, boundary->circle.radius);
	} else if (boundary->type == LOCATION_BOUNDARY_RECT) {
		g_printf("\n\tRECT: left_top[%lf %lf %lf] right_bottom[%lf %lf %lf]",
		         boundary->rect.left_top->latitude, boundary->rect.left_top->longitude, boundary->rect.left_top->altitude,
		         boundary->rect.right_bottom->latitude, boundary->rect.right_bottom->longitude, boundary->rect.right_bottom->altitude);
	} else if (boundary->type == LOCATION_BOUNDARY_POLYGON) {
		g_printf("\n\tPOLYGON: ");

		GList *list = boundary->polygon.position_list;

		g_list_foreach(list, PrintPolygonPosition, NULL);
	}
}

static void RemoveBoundary(LocationBoundary *boundary, void *user_data)
{
	LocationObject *obj = (LocationObject *)user_data;
	if (obj == NULL || boundary == NULL) return;

	location_boundary_remove(obj, boundary);
}

static void PrintProperty(LocationObject *loc)
{
	if (!loc) return;
	LocationMethod method = LOCATION_METHOD_NONE;
	LocationPosition *pos = NULL;
	LocationAccuracy *acc = NULL;
	guint pos_interval = 0;
	guint vel_interval = 0;
	guint sat_interval = 0;
	gchar method_str[STR_MAX] = {0, };
	gchar status_str[STR_MAX] = {0, };

	g_object_get(loc, "method", &method, NULL);
	GetMethod(method_str, method);
	g_printf("method[%s] ", method_str);

	int ret = location_get_last_position(loc, &pos, &acc);
	if (ret == LOCATION_ERROR_NONE) {
		GetStatus(status_str, pos->status);
		g_printf("\nLast position [time(%d) lat(%f) long(%f) alt(%f) status(%s)]",
		         pos->timestamp, pos->latitude, pos->longitude, pos->altitude, status_str);
		location_position_free(pos);
		location_accuracy_free(acc);
	}

	if (method == LOCATION_METHOD_HYBRID || method == LOCATION_METHOD_GPS) {
		g_object_get(loc, "pos-interval", &pos_interval, NULL);
		g_object_get(loc, "vel-interval", &vel_interval, NULL);
		g_object_get(loc, "sat-interval", &sat_interval, NULL);
	} else if (method == LOCATION_METHOD_WPS) {
		g_object_get(loc, "pos-interval", &pos_interval, NULL);
		g_object_get(loc, "vel-interval", &vel_interval, NULL);
	}
	g_printf("Position interval : [%u], Velocity interval [%u], Satellite interval [%u]\n", pos_interval, vel_interval, sat_interval);

	g_printf("\nSignals: ");
	if (g_sig_enable)  g_printf("[service-enabled], ");
	if (g_sig_disable) g_printf("[service-disabled], ");
	if (g_sig_update)  g_printf("[service-updated], ");
	if (g_sig_zonein)  g_printf("[zone-in], ");
	if (g_sig_zoneout) g_printf("[zone-out]");
}

static void cb_service_enabled(GObject *self, guint status, gpointer userdata)
{
	g_printf("cb_service_enabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);

	LocationObject *loc = (LocationObject *)userdata;
	LocationAccuracy *acc = NULL;
	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationMethod method;

	g_object_get(loc, "method", &method, NULL);
	g_printf("Get property>> method:%d", method);

	if (LOCATION_ERROR_NONE == location_get_position(loc, &pos, &acc)) {
		g_printf("SYNC>> Current position> time: %d, lat: %f, long: %f, alt: %f, status: %d\n",
		         pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
		g_printf("\tAccuracy level %d (%.0f meters %.0f meters)\n",
		         acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_position_free(pos);
		location_accuracy_free(acc);
	} else g_warning("SYNC>> Current position> failed");
	if (LOCATION_ERROR_NONE == location_get_velocity(loc, &vel, &acc)) {
		g_printf("SYNC>> Current velocity> time: %d, speed: %f, direction:%f, climb:%f\n",
		         vel->timestamp, vel->speed, vel->direction, vel->climb);
		g_printf("\tAccuracy level %d (%.0f meters %.0f meters)\n",
		         acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
		location_velocity_free(vel);
		location_accuracy_free(acc);
	} else g_warning("SYNC>> Current velocity> failed\n");
}

static void
cb_service_disabled(GObject *self,
                    guint status,
                    gpointer userdata)
{
	g_printf("cb_service_disabled: status(%d) userdata(0x%x)\n", status, (unsigned int)userdata);
}

static void
cb_zone_in(GObject *self,
           gpointer boundary,
           gpointer position,
           gpointer accuracy)
{
	LocationPosition *pos = (LocationPosition *) position;
	LocationAccuracy *acc = (LocationAccuracy *) accuracy;

	g_printf("ASYNC>> ZoneIn> Current position: time: %d, lat: %f, long: %f, alt: %f\n",
	         pos->timestamp, pos->latitude, pos->longitude, pos->altitude);
	g_printf("\tAccuracy level %d (%.0f meters %.0f meters)\n",
	         acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
}

static void
cb_zone_out(GObject *self,
            gpointer boundary,
            gpointer position,
            gpointer accuracy)
{
	LocationPosition *pos = (LocationPosition *) position;
	LocationAccuracy *acc = (LocationAccuracy *) accuracy;

	g_printf("ASYNC>> ZoneOut> Current position: time: %d, lat: %f, long: %f, alt: %f\n",
	         pos->timestamp, pos->latitude, pos->longitude, pos->altitude);
	g_printf("\tAccuracy level %d (%.0f meters %.0f meters)\n",
	         acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
}

static void
cb_service_updated(GObject *self,
                   guint type,
                   gpointer data,
                   gpointer accuracy,
                   gpointer userdata)
{
	g_printf("cb_service_updated: type(%d) userdata(0x%x)", type, (unsigned int)userdata);

	LocationAccuracy *acc = (LocationAccuracy *) accuracy;
	switch (type) {
		case POSITION_UPDATED: {
				LocationPosition *pos = (LocationPosition *) data;
				g_printf("ASYNC>> Current position> time: %d, lat: %f, long: %f, alt: %f, status: %d\n",
				         pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
				g_printf("\tAccuracy level %d (%.0f meters %.0f meters)\n",
				         acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
			}
			break;
		case VELOCITY_UPDATED: {
				LocationVelocity *vel = (LocationVelocity *) data;
				g_printf("ASYNC>> Current velocity> time: %d, speed: %f, direction:%f, climb:%f\n",
				         vel->timestamp, vel->speed, vel->direction, vel->climb);
				g_printf("\tAccuracy level %d (%.0f meters %.0f meters)\n",
				         acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
			}
			break;
		case SATELLITE_UPDATED: {
				int idx = 0;
				guint prn;
				gboolean used;
				guint elevation;
				guint azimuth;
				gint snr;

				LocationSatellite *sat = (LocationSatellite *)data;
				g_printf("SYNC>> Current Satellite> time: %d, satellite in view = %d, satellite in used = %d\n", sat->timestamp, sat->num_of_sat_inview, sat->num_of_sat_used);
				g_printf("\tinview satellite information = ");
				for (idx = 0; idx < sat->num_of_sat_inview; idx++) {
					location_satellite_get_satellite_details(sat, idx, &prn, &used, &elevation, &azimuth, &snr);
					g_printf("\t\t[%02d] used: %d, prn: %d, elevation: %d, azimuth: %d, snr: %d\n", idx, used, prn, elevation, azimuth, snr);
				}
			}
			break;
		default:
			g_warning("ASYNC>> Undefined update type");
			break;
	}
}

static void print_menu()
{
	g_printf("\n================================= Location API Test =================================\n");
	g_printf("q.   Exit\n");
	g_printf("1.   location_init\n");
	g_printf("2.   location_new\n");
	g_printf("3.   location_free\n");
	g_printf("4.   location_start\n");
	g_printf("5.   location_stop\n");
	g_printf("6.   location_get_position\n");
	g_printf("6a.  location_get_last_position\n");
	g_printf("7.   location_get_velocity\n");
	g_printf("7a.  location_get_last_velocity\n");
	g_printf("8.   location_get_satellite\n");
	g_printf("8a.  location_get_last_satellite\n");
	g_printf("9.   location_get_distance\n");
	g_printf("10.  location_is_supported_method\n");
	g_printf("99.  location_send_command\n");
#ifndef _TIZEN_PUBLIC_
	g_printf("99a.  location_send_command(get_auth)\n");
	g_printf("99b.  location_send_command(add_to_list)\n");
#endif
	g_printf("a?.  signals:(1)'service-enabled',(2)'service-disabled',(3)'service-updated',(4)'zone-in',(5)'zone-out'\n");
	g_printf("b?.  disconnect signals:(1)'service-enabled',(2)'service-disabled',(3)'service-updated',(4)'zone-in',(5)'zone-out'\n");
	g_printf("c?. (1)Set boundary, (2)Get boundary, (3) Remove boundary, (4) Remove all boundaries, \n");
	g_printf("    (5)Set position interval (6) Set velocity interval (7) Set satellite interval\n");
	g_printf("==================================== Property ====================================\n");
	PrintProperty(location_obj);
	g_printf("\n==================================================================================\n");
}

int main(int argc, char **argv)
{
	char strOpt[255];
	int ret;
	char str[STR_MAX];
	GError *gerr = NULL;

	/* If application is executed by AUL, this is not needed. */
	g_setenv("PKG_NAME", "com.samsung.location-api-test", 1);

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
		g_printf("Error create gmain thread: Err domain[%d] Err code[%d] Err msg[%s]",
		         gerr->domain, gerr->code, gerr->message);
		g_error_free(gerr);
		return 0;
	}

	json_util_init(LOCATION_API_TEST_JSON_FILE);
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
			if (location_obj) {
				g_printf("Location object already existed: [0x%x]", (unsigned int)location_obj);
				continue;
			}
			g_printf("LOCATION_METHOD_HYBRID[0] LOCATION_METHOD_GPS[1] LOCATION_METHOD_WPS[2]\n");
			g_printf("Select Location Method: ");
			LocationMethod method = PromptInt();
			location_obj = location_new(method);
			if (location_obj) g_printf("Success\n");
			else             g_printf("Failed\n");
		} else if (0 == g_strcmp0("3", strOpt)) {
			ret = location_free(location_obj);
			location_obj = NULL;

			g_sig_enable = 0;
			g_sig_disable = 0;
			g_sig_update = 0;
			g_sig_zonein = 0;
			g_sig_zoneout = 0;
			GetLocationError(str, ret);
			g_printf("location_free: returned value [%s]\n", str);
		} else if (0 == g_strcmp0("4", strOpt)) {
			ret = location_start(location_obj);
			GetLocationError(str, ret);
			g_printf("location_start: returned value [%s]\n", str);
		} else if (0 == g_strcmp0("5", strOpt)) {
			ret = location_stop(location_obj);
			GetLocationError(str, ret);
			g_printf("location_stop: returned value [%s]\n", str);
		} else if (0 == g_strcmp0("6", strOpt)) {
			LocationPosition *pos = NULL;
			LocationAccuracy *acc = NULL;
			ret = location_get_position(location_obj, &pos, &acc);
			GetLocationError(str, ret);
			g_printf("location_get_position: returned value [%s]\n", str);
			if (ret == LOCATION_ERROR_NONE) {
				g_printf("time: [%d], latitude: [%f], longitude: [%f], altitude: [%f], status: [%d]\n", pos->timestamp, pos->latitude, pos->longitude, pos->altitude, pos->status);
				GetAccuracyLevel(str, acc->level);
				g_printf("level: [%s], horizontal_accuracy: [%f], vertical_accuracy: [%f]\n", str, acc->horizontal_accuracy, acc->vertical_accuracy);
			}
			if (pos) location_position_free(pos);
			if (acc) location_accuracy_free(acc);
		} else if (0 == g_strcmp0("6a", strOpt)) {
			LocationPosition *last_pos;
			LocationAccuracy *last_acc;
			int ret = 0;

			ret = location_get_last_position(location_obj, &last_pos, &last_acc);
			GetLocationError(str, ret);
			g_printf("location_get_last_position: returned value [%s]\n", str);
			if (ret == LOCATION_ERROR_NONE) {
				g_printf("SYNC>> Last position> time: %d, lat: %f, long: %f, alt: %f, status: %d",
				         last_pos->timestamp, last_pos->latitude, last_pos->longitude, last_pos->altitude, last_pos->status);
				g_printf("\tAccuracy level %d (%.0f meters %.0f meters)",
				         last_acc->level, last_acc->horizontal_accuracy, last_acc->vertical_accuracy);
				location_position_free(last_pos);
				location_accuracy_free(last_acc);
			}

		} else if (0 == g_strcmp0("7", strOpt)) {
			LocationVelocity *vel = NULL;
			LocationAccuracy *acc = NULL;
			ret = location_get_velocity(location_obj, &vel, &acc);
			GetLocationError(str, ret);
			g_printf("location_get_velocity: returned value [%s]\n", str);
			if (ret == LOCATION_ERROR_NONE) {
				g_printf("time: [%d], speed: [%f], direction: [%f], climb: [%f]\n", vel->timestamp, vel->speed, vel->direction, vel->climb);
				GetAccuracyLevel(str, acc->level);
				g_printf("level: [%s], horizontal_accuracy: [%f], vertical_accuracy: [%f]\n", str, acc->horizontal_accuracy, acc->vertical_accuracy);
			}
			if (vel) location_velocity_free(vel);
			if (acc) location_accuracy_free(acc);
		} else if (0 == g_strcmp0("7a", strOpt)) {
			LocationVelocity *last_vel = NULL;
			LocationAccuracy *last_acc = NULL;
			ret = location_get_last_velocity(location_obj, &last_vel, &last_acc);
			GetLocationError(str, ret);
			g_printf("location_get_last_velocity: returned value [%s]\n", str);
			if (ret == LOCATION_ERROR_NONE) {
				g_printf("SYNC>> Last velocity> time: %d, speed: %f, direction:%f, climb:%f",
				         last_vel->timestamp, last_vel->speed, last_vel->direction, last_vel->climb);
				g_printf("\tAccuracy level %d (%.0f meters %.0f meters)",
				         last_acc->level, last_acc->horizontal_accuracy, last_acc->vertical_accuracy);
				location_velocity_free(last_vel);
				location_accuracy_free(last_acc);
			}
		} else if (0 == g_strcmp0("8", strOpt)) {
			int ret = 0, idx = 0;
			LocationSatellite *sat = NULL;
			guint prn;
			gboolean used;
			guint elevation;
			guint azimuth;
			gint snr;

			ret = location_get_satellite(location_obj, &sat);
			GetLocationError(str, ret);
			g_printf("location_get_satellite: returned value [%s]\n", str);
			if (ret == LOCATION_ERROR_NONE) {
				g_printf("SYNC>> Current Sattelite> time = %d, satellite in view = %d, satellite in used = %d", sat->timestamp, sat->num_of_sat_inview, sat->num_of_sat_used);
				g_printf("\tinview satellite information = ");
				for (idx = 0; idx < sat->num_of_sat_inview; idx++) {
					location_satellite_get_satellite_details(sat, idx, &prn, &used, &elevation, &azimuth, &snr);
					g_printf("\t\t[%02d] used: %d, prn: %d, elevation: %d, azimuth: %d, snr: %d", idx, used, prn, elevation, azimuth, snr);
				}
				location_satellite_free(sat);
			}
		} else if (0 == g_strcmp0("8a", strOpt)) {
			int ret = 0, idx = 0;
			LocationSatellite *last_sat = NULL;
			guint prn;
			gboolean used;
			guint elevation;
			guint azimuth;
			gint snr;

			ret = location_get_last_satellite(location_obj, &last_sat);
			GetLocationError(str, ret);
			g_printf("location_get_last_satellite: returned value [%s]\n", str);
			if (ret == LOCATION_ERROR_NONE) {
				g_printf("SYNC>> Last Sattelite> time = %d, satellite in view = %d, satellite in used = %d", last_sat->timestamp, last_sat->num_of_sat_inview, last_sat->num_of_sat_used);
				g_printf("\tinview satellite information = ");
				for (idx = 0; idx < last_sat->num_of_sat_inview; idx++) {
					location_satellite_get_satellite_details(last_sat, idx, &prn, &used, &elevation, &azimuth, &snr);
					g_printf("\t\t[%02d] used: %d, prn: %d, elevation: %d, azimuth: %d, snr: %d", idx, used, prn, elevation, azimuth, snr);
				}
				location_satellite_free(last_sat);
			}
		} else if (0 == g_strcmp0("9", strOpt)) {

			gulong distance;
			int ret = 0;
			char str[STR_MAX];
			LocationPosition pos1, pos2;

			pos1.latitude = 50.0663222;
			pos1.longitude = -5.71475;

			pos2.latitude = 58.6441;
			pos2.longitude = -3.070094;

			ret = location_get_distance(&pos1, &pos2, &distance);
			GetLocationError(str, ret);
			if (ret != LOCATION_ERROR_NONE) {
				g_printf("Fail to get position. Error[%s]\n", str);
			} else {
				g_printf("The approximate distance is [%lu]\n", distance);
				g_printf("cf.) It is approximately 969954.114 meter\n");
			}
		} else if (0 == g_strcmp0("10", strOpt)) {
			int method;
			char method_str[STR_MAX] = {0, };
			char *str = NULL;
			char input[8] = {0, };
			gboolean is_supported = FALSE;

			g_printf("0.Hybrid 1.GPS 2.WPS\n");
			g_printf("Select Method :");
			str = fgets(input, 8, stdin);
			method = atoi(input);
			switch (method) {
				case LOCATION_METHOD_HYBRID:
					is_supported = location_is_supported_method(LOCATION_METHOD_HYBRID);
					break;

				case LOCATION_METHOD_GPS:
					is_supported = location_is_supported_method(LOCATION_METHOD_GPS);
					break;

				case LOCATION_METHOD_WPS:
					is_supported = location_is_supported_method(LOCATION_METHOD_WPS);
					break;
				default:
					break;

			}
			GetMethod(method_str, method);

			g_printf("Method[%s] is %s.", method_str, is_supported ? "supported" : "not supported");

		} else if (0 == g_strcmp0("99", strOpt)) {
			int ret = 0;
			const char *str = "command";
			ret = location_send_command(str);
			if (ret == 0)
				g_printf("Success to send command[%s]", str);
			else
				g_printf("Fail to send command[%s]. Error[%d]", str, ret);
#ifndef _TIZEN_PUBLIC_
		} else if (0 == g_strcmp0("99a", strOpt)) {
			int ret = 0;
			const char *str = "GET_APP_AUTHORITY";
			ret = location_send_command(str);
			if (ret == 0)
				g_printf("Success to send command[%s]", str);
			else
				g_printf("Fail to send command[%s]. Error[%d]", str, ret);
		} else if (0 == g_strcmp0("99b", strOpt)) {
			int ret = 0;
			const char *str = "ADD_APPLIST";
			ret = location_send_command(str);
			if (ret == 0)
				g_printf("Success to send command[%s]", str);
			else
				g_printf("Fail to send command[%s]. Error[%d]", str, ret);
#endif
		} else if (0 == g_strcmp0("a1", strOpt)) {
			if (location_obj && !g_sig_enable) {
				g_sig_enable = g_signal_connect(location_obj, "service-enabled", G_CALLBACK(cb_service_enabled), location_obj);
			}
		} else if (0 == g_strcmp0("a2", strOpt)) {
			if (location_obj && !g_sig_disable) {
				g_sig_disable = g_signal_connect(location_obj, "service-disabled", G_CALLBACK(cb_service_disabled), location_obj);
			}
		} else if (0 == g_strcmp0("a3", strOpt)) {
			if (location_obj && !g_sig_update) {
				g_sig_update = g_signal_connect(location_obj, "service-updated", G_CALLBACK(cb_service_updated), location_obj);
			}
		} else if (0 == g_strcmp0("a4", strOpt)) {
			if (location_obj && !g_sig_zonein) {
				g_sig_zonein = g_signal_connect(location_obj, "zone-in", G_CALLBACK(cb_zone_in), location_obj);
			}
		} else if (0 == g_strcmp0("a5", strOpt)) {
			if (location_obj && !g_sig_zoneout) {
				g_sig_zoneout = g_signal_connect(location_obj, "zone-out", G_CALLBACK(cb_zone_out), location_obj);
			}
		} else if (0 == g_strcmp0("b1", strOpt)) {
			if (location_obj && g_sig_enable) {
				g_signal_handler_disconnect(location_obj, g_sig_enable);
				g_sig_enable = 0;
			}
		} else if (0 == g_strcmp0("b2", strOpt)) {
			if (location_obj && g_sig_disable) {
				g_signal_handler_disconnect(location_obj, g_sig_disable);
				g_sig_disable = 0;
			}
		} else if (0 == g_strcmp0("b3", strOpt)) {
			if (location_obj && g_sig_update) {
				g_signal_handler_disconnect(location_obj, g_sig_update);
				g_sig_update = 0;
			}
		} else if (0 == g_strcmp0("b4", strOpt)) {
			if (location_obj && g_sig_zonein) {
				g_signal_handler_disconnect(location_obj, g_sig_zonein);
				g_sig_zonein = 0;
			}
		} else if (0 == g_strcmp0("b5", strOpt)) {
			if (location_obj && g_sig_zoneout) {
				g_signal_handler_disconnect(location_obj, g_sig_zoneout);
				g_sig_zoneout = 0;
			}
		} else if (0 == g_strcmp0("c1", strOpt)) {
			LocationBoundary *bound = NULL;
			int i = 0;
			int polygon_count = json_util_get_polygon_count();

			g_printf("[0].San jose(Rect) [1].Suwon HQ(Rect) [2].Seoul City(circle) ");
			for (i = 0; i < polygon_count; i++)
				g_printf("[%d].%s ", i + 3, json_util_get_polygon_name(i));

			g_printf("\nSelect Boundary: ");
			int opt = PromptInt();
			if (opt == 0) {
				LocationPosition *rb = location_position_new(0, 37.300, -121.86, 0, LOCATION_STATUS_2D_FIX);
				LocationPosition *lt = location_position_new(0, 37.360, -121.92, 0, LOCATION_STATUS_2D_FIX);
				bound = location_boundary_new_for_rect(lt, rb);
				location_position_free(rb);
				location_position_free(lt);

				location_boundary_add(location_obj, bound);

			} else if (opt == 1) {
				LocationPosition *rb = location_position_new(0, 37.255, 127.058, 0, LOCATION_STATUS_2D_FIX);
				LocationPosition *lt = location_position_new(0, 37.260, 127.045, 0, LOCATION_STATUS_2D_FIX);
				bound = location_boundary_new_for_rect(lt, rb);
				location_position_free(rb);
				location_position_free(lt);

				location_boundary_add(location_obj, bound);
			} else if (opt == 2) {
				LocationPosition *center = location_position_new(0, 37.566535, 126.977969, 0.0, LOCATION_STATUS_2D_FIX);
				double radius = 10.0;
				bound = location_boundary_new_for_circle(center, radius);

				location_boundary_add(location_obj, bound);
			} else if (opt > 2 && opt < 3 + polygon_count) {
				int index;
				int polygon_index = opt - 3;
				bound = json_util_get_polygon_boundary(polygon_index);
				location_boundary_add(location_obj, bound);

				LocationPosition *check_pos = NULL;
				gboolean ret = FALSE;
				int count = json_util_get_marker_position_count(polygon_index);
				for (index = 0; index < count; index++) {
					check_pos = json_util_get_marker_position(polygon_index, index);

					ret = location_boundary_if_inside(bound, check_pos);
					g_printf("[%s] is [%s] and the test result is [%s]\n", json_util_get_marker_name(polygon_index, index), json_util_result_zone_test(polygon_index, index), ret == TRUE ? "inside" : "outside");
					location_position_free(check_pos);
				}
			} else {
				g_printf("boundary is not set\n");
			}
			location_boundary_free(bound);

		} else if (0 == g_strcmp0("c2", strOpt)) {
			g_printf("Get Boundary\n");
			location_boundary_foreach(location_obj, PrintBoundary, NULL);

		} else if (0 == g_strcmp0("c3", strOpt)) {
			g_printf("Remove Boundary\n");
			LocationBoundary *bound = NULL;

			int i = 0;
			int polygon_count = json_util_get_polygon_count();

			g_printf("[0].San jose(Rect) [1].Suwon HQ(Rect) [2].Seoul City(circle) ");
			for (i = 0; i < polygon_count; i++)
				g_printf("[%d].%s ", i + 3, json_util_get_polygon_name(i));

			g_printf("\nSelect Boundary: ");
			int opt = PromptInt();
			if (opt == 0) {
				LocationPosition *rb = location_position_new(0, 37.300, -121.86, 0, LOCATION_STATUS_2D_FIX);
				LocationPosition *lt = location_position_new(0, 37.360, -121.92, 0, LOCATION_STATUS_2D_FIX);
				bound = location_boundary_new_for_rect(lt, rb);
				location_position_free(rb);
				location_position_free(lt);

			} else if (opt == 1) {
				LocationPosition *rb = location_position_new(0, 37.255, 127.058, 0, LOCATION_STATUS_2D_FIX);
				LocationPosition *lt = location_position_new(0, 37.260, 127.045, 0, LOCATION_STATUS_2D_FIX);
				bound = location_boundary_new_for_rect(lt, rb);
				location_position_free(rb);
				location_position_free(lt);

			} else if (opt == 2) {
				LocationPosition *center = location_position_new(0, 37.566535, 126.977969, 0.0, LOCATION_STATUS_2D_FIX);
				double radius = 10.0;
				bound = location_boundary_new_for_circle(center, radius);
				location_position_free(center);

			} else if (opt > 2 && opt < 3 + polygon_count) {
				int polygon_index = opt - 3;
				bound = json_util_get_polygon_boundary(polygon_index);
			} else {
				g_printf("Invalid value\n");
			}

			if (bound != NULL) location_boundary_remove(location_obj, bound);

		} else if (0 == g_strcmp0("c4", strOpt)) {
			location_boundary_foreach(location_obj, RemoveBoundary, location_obj);
		} else if (0 == g_strcmp0("c5", strOpt)) {
			guint interval = 1;
			int len = 0;
			g_printf("Input interval[1~120]:");
			len = scanf("%u", &interval);
			g_printf("changed interval to [%u]\n", interval);
			g_object_set(location_obj, "pos-interval", interval, NULL);
		} else if (0 == g_strcmp0("c6", strOpt)) {
			int len = 0;
			guint interval = 1;
			g_printf("Input interval[1~120]:");
			len = scanf("%u", &interval);
			g_printf("changed interval to [%u]\n", interval);
			g_object_set(location_obj, "vel-interval", interval, NULL);
		} else if (0 == g_strcmp0("c7", strOpt)) {
			guint interval = 1;
			int len = 0;
			g_printf("Input interval[1~120]:");
			len = scanf("%u", &interval);
			g_printf("changed interval to [%u]\n", interval);
			g_object_set(location_obj, "sat-interval", interval, NULL);
		} else if (0 == g_strcmp0("q", strOpt)) {
			g_main_loop_quit(g_mainloop);
			break;
		}
	}
	g_thread_join(g_main);
	g_printf("\n--- Exit LBS Test App ---\n");
	return 1;
}
