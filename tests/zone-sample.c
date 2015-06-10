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
#include <location.h>

static GMainLoop *loop = NULL;

static gboolean
exit_program(gpointer data)
{
	g_main_loop_quit(loop);
	g_debug("Quit g_main_loop");
	return FALSE;
}

static void
cb_service_enabled(GObject *self,
                   guint status,
                   gpointer userdata)
{
	g_debug("cb_service_enabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);
}

static void
cb_service_disabled(GObject *self,
                    guint status,
                    gpointer userdata)
{
	g_debug("cb_service_disabled: status(%d) userdata(0x%x)", status, (unsigned int)userdata);
}

static void
cb_zone_in(GObject *self,
           gpointer boundary,
           gpointer position,
           gpointer accuracy)
{
	LocationPosition *pos = (LocationPosition *) position;
	LocationAccuracy *acc = (LocationAccuracy *) accuracy;

	g_debug("ASYNC>> ZoneIn> Current position: time: %d, lat: %f, long: %f, alt: %f",
	        pos->timestamp, pos->latitude, pos->longitude, pos->altitude);
	g_debug("\tAccuracy level %d (%.0f meters %.0f meters)",
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

	g_debug("ASYNC>> ZoneOut> Current position: time: %d, lat: %f, long: %f, alt: %f",
	        pos->timestamp, pos->latitude, pos->longitude, pos->altitude);
	g_debug("\tAccuracy level %d (%.0f meters %.0f meters)",
	        acc->level, acc->horizontal_accuracy, acc->vertical_accuracy);
}

int
main(int argc, char *argv[])
{
	LocationObject *loc = NULL;

	location_init();

	loop = g_main_loop_new(NULL, TRUE);

	loc  = location_new(LOCATION_METHOD_GPS);
	if (!loc) {
		g_debug("location_new failed");
		return -1;
	}

	LocationPosition *rb = location_position_new(0, 37.258, 127.056, 0, LOCATION_STATUS_2D_FIX);
	LocationPosition *lt = location_position_new(0, 37.260, 127.054, 0, LOCATION_STATUS_2D_FIX);
	LocationBoundary *bound = location_boundary_new_for_rect(lt, rb);
	location_position_free(rb);
	location_position_free(lt);
	if (bound) {
		g_object_set(loc, "boundary", bound, NULL);
		location_boundary_free(bound);
	} else g_warning("failed to location_boundary_new_for_rect()");
	g_object_get(loc, "boundary", &bound, NULL);
	if (bound) {
		g_debug("Set property>> boundary> type: %d, (%f,%f),(%f,%f)",
		        bound->type,
		        bound->rect.right_bottom->latitude, bound->rect.right_bottom->longitude,
		        bound->rect.left_top->latitude, bound->rect.left_top->longitude);
		location_boundary_free(bound);
	} else 	g_warning("failed to set property> boundary");

	g_signal_connect(loc, "service-enabled", G_CALLBACK(cb_service_enabled), loc);
	g_signal_connect(loc, "service-disabled", G_CALLBACK(cb_service_disabled), loc);
	g_signal_connect(loc, "zone-in", G_CALLBACK(cb_zone_in), loc);
	g_signal_connect(loc, "zone-out", G_CALLBACK(cb_zone_out), loc);

	if (LOCATION_ERROR_NONE != location_start(loc)) {
		g_debug("location_start failed");
		return -1;
	}

	g_timeout_add_seconds(60, exit_program, NULL);
	g_main_loop_run(loop);

	location_stop(loc);
	location_free(loc);

	return 0;
}
