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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "location.h"
#include "location-common-util.h"
#include "location-setting.h"
#include "location-log.h"
#include <app_manager.h>


int location_application_get_authority(void)
{
	return LOCATION_APP_ON;
}

int location_application_set_authority(int auth)
{
	return LOCATION_ERROR_NONE;
}

static gint compare_position(gconstpointer a, gconstpointer b)
{
	g_return_val_if_fail(a, 1);
	g_return_val_if_fail(b, -1);

	if (location_position_equal((LocationPosition *) a, (LocationPosition *)b) == TRUE) {
		return 0;
	}

	return -1;
}

static int
boundary_compare(gconstpointer comp1, gconstpointer comp2)
{
	g_return_val_if_fail(comp1, 1);
	g_return_val_if_fail(comp2, -1);

	int ret = -1;

	LocationBoundaryPrivate *priv1 = (LocationBoundaryPrivate *)comp1;
	LocationBoundaryPrivate *priv2 = (LocationBoundaryPrivate *)comp2;

	if (priv1->boundary->type == priv2->boundary->type) {
		switch (priv1->boundary->type) {
			case LOCATION_BOUNDARY_CIRCLE: {
					if (location_position_equal(priv1->boundary->circle.center, priv2->boundary->circle.center)
					    && priv1->boundary->circle.radius == priv2->boundary->circle.radius) {
						ret = 0;
					}
					break;
				}
			case LOCATION_BOUNDARY_RECT: {
					if (location_position_equal(priv1->boundary->rect.left_top, priv2->boundary->rect.left_top)
					    && location_position_equal(priv1->boundary->rect.right_bottom, priv2->boundary->rect.right_bottom)) {
						ret = 0;
					}
					break;
				}
			case LOCATION_BOUNDARY_POLYGON: {
					GList *boundary1_next = NULL;
					GList *boundary2_start = NULL, *boundary2_prev = NULL, *boundary2_next = NULL;
					if (g_list_length(priv1->boundary->polygon.position_list) != g_list_length(priv2->boundary->polygon.position_list)) {
						return -1;
					}

					/* Find a matching index of Boundary2 with Boundary1's 1st postion. */
					boundary2_start = g_list_find_custom(priv2->boundary->polygon.position_list, g_list_nth_data(priv1->boundary->polygon.position_list, 0), (GCompareFunc) compare_position);
					if (boundary2_start == NULL) return -1;

					boundary2_prev = g_list_previous(boundary2_start);
					boundary2_next = g_list_next(boundary2_start);
					if (boundary2_prev == NULL) boundary2_prev = g_list_last(priv2->boundary->polygon.position_list);
					if (boundary2_next == NULL) boundary2_next = g_list_first(priv2->boundary->polygon.position_list);

					boundary1_next = g_list_next(priv1->boundary->polygon.position_list);
					if (boundary1_next && location_position_equal((LocationPosition *)boundary1_next->data, (LocationPosition *)boundary2_prev->data) == TRUE) {
						boundary1_next = g_list_next(boundary1_next);
						while (boundary1_next) {
							boundary2_prev = g_list_previous(boundary2_prev);
							if (boundary2_prev == NULL) boundary2_prev = g_list_last(priv2->boundary->polygon.position_list);
							if (location_position_equal((LocationPosition *)boundary1_next->data, (LocationPosition *) boundary2_prev->data) == FALSE) {
								return -1;
							}
							boundary1_next = g_list_next(boundary1_next);
						}
						ret = 0;
					} else if (boundary1_next && location_position_equal((LocationPosition *)boundary1_next->data, (LocationPosition *)boundary2_next->data) == TRUE) {
						boundary1_next = g_list_next(boundary1_next);
						while (boundary1_next) {
							boundary2_next = g_list_next(boundary2_next);
							if (boundary2_next == NULL) boundary2_next = g_list_first(priv2->boundary->polygon.position_list);
							if (location_position_equal((LocationPosition *)boundary1_next->data, (LocationPosition *) boundary2_next->data) == FALSE) {
								return -1;
							}
							boundary1_next = g_list_next(boundary1_next);
						}
						ret = 0;
					} else {
						return -1;
					}
					break;
				}
			default: {
					ret = -1;
					break;
				}

		}
	}

	return ret;
}

int set_prop_boundary(GList **prev_boundary_priv_list, GList *new_boundary_priv_list)
{
	LOCATION_LOGD("ENTER >>>");
	g_return_val_if_fail(new_boundary_priv_list, LOCATION_ERROR_PARAMETER);

	int index = 0;
	GList *check_list = NULL;

	LocationBoundaryPrivate *new_priv = NULL;

	while ((new_priv = (LocationBoundaryPrivate *) g_list_nth_data(new_boundary_priv_list, index)) != NULL) {
		*prev_boundary_priv_list = g_list_first(*prev_boundary_priv_list);
		check_list = g_list_find_custom(*prev_boundary_priv_list, new_priv, (GCompareFunc)boundary_compare);
		if (check_list == NULL) {
			LocationBoundaryPrivate *copy_priv = g_slice_new0(LocationBoundaryPrivate);
			if (!copy_priv) break;
			copy_priv->boundary = location_boundary_copy(new_priv->boundary);
			if (!copy_priv->boundary) break;
			copy_priv->zone_status = new_priv->zone_status;
			*prev_boundary_priv_list = g_list_append(*prev_boundary_priv_list, copy_priv);

			LOCATION_LOGD("copy_priv: %p, copy_priv->boundary: %p, copy_priv->boundary->type: %d",
			              copy_priv, copy_priv->boundary, copy_priv->boundary->type);
		}
		location_boundary_free(new_priv->boundary);
		index++;
	}

	LOCATION_LOGD("EXIT <<<");
	return LOCATION_ERROR_NONE;
}

int set_prop_removal_boundary(GList **prev_boundary_list, LocationBoundary *boundary)
{
	LOCATION_LOGD("ENTER >>>");
	g_return_val_if_fail(*prev_boundary_list, LOCATION_ERROR_PARAMETER);

	GList *check_list = NULL;
	LocationBoundaryPrivate *remove_priv = g_slice_new0(LocationBoundaryPrivate);
	g_return_val_if_fail(remove_priv, LOCATION_ERROR_PARAMETER);

	remove_priv->boundary = location_boundary_copy(boundary);
	g_return_val_if_fail(remove_priv->boundary, LOCATION_ERROR_PARAMETER);

	check_list = g_list_find_custom(*prev_boundary_list, remove_priv, (GCompareFunc) boundary_compare);
	if (check_list) {
		LOCATION_LOGD("Found");
		*prev_boundary_list = g_list_delete_link(*prev_boundary_list, check_list);
	}

	if (g_list_length(*prev_boundary_list) == 0) {
		LOCATION_LOGD("Boundary List is empty");
		g_list_free(*prev_boundary_list);
		*prev_boundary_list = NULL;
	}

	location_boundary_free(remove_priv->boundary);
	g_slice_free(LocationBoundaryPrivate, remove_priv);

	LOCATION_LOGD("EXIT <<<");
	return LOCATION_ERROR_NONE;
}

void free_boundary_list(gpointer data)
{
	LocationBoundaryPrivate *priv = (LocationBoundaryPrivate *)data;

	location_boundary_free(priv->boundary);
	g_slice_free(LocationBoundaryPrivate, priv);
}

int location_get_app_type(char *target_app_id)
{
        int ret = 0;
        pid_t pid = 0;
        char *app_id = NULL;
        app_info_h app_info;
        char *type = NULL;

        if (target_app_id == NULL) {
                pid = getpid();
                ret = app_manager_get_app_id(pid, &app_id);
                if (ret != APP_MANAGER_ERROR_NONE) {
                        LOCATION_LOGE("Fail to get app_id. Err[%d]", ret);
                        return LOCATION_ERROR_NONE;
                }
        } else {
                app_id = g_strdup(target_app_id);
        }

        ret = app_info_create(app_id, &app_info);
        if (ret != APP_MANAGER_ERROR_NONE) {
                LOCATION_LOGE("Fail to get app_info. Err[%d]", ret);
                g_free(app_id);
                return LOCATION_ERROR_NONE;
        }

        ret = app_info_get_type(app_info, &type);
        if (ret != APP_MANAGER_ERROR_NONE) {
                LOCATION_LOGE("Fail to get type. Err[%d]", ret);
                g_free(app_id);
                app_info_destroy(app_info);
                return LOCATION_ERROR_NONE;
        }

        if (strcmp(type, "c++app") == 0) {
                ret = CPPAPP;
        } else if (strcmp(type, "webapp") == 0) {
                ret = WEBAPP;
        } else {
                ret = CAPP;
        }

        g_free(type);
        g_free(app_id);
        app_info_destroy(app_info);

        return ret;
}
