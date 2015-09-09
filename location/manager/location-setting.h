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
 * See the License for the specific language governing permissions and "1
 * limitations under the License.
 */

#ifndef __LOCATION_SETTING_H__
#define __LOCATION_SETTING_H__

#include <glib.h>
#include <vconf.h>
#include <vconf-internal-location-keys.h>

/**
 * @file location-setting.h
 * @brief This file contains the definitions and functions for setting.
 */

#define LOCATION_UPDATE_INTERVAL_NONE		0
#define LOCATION_UPDATE_INTERVAL_MIN		1
#define LOCATION_UPDATE_INTERVAL_MAX		120
#define LOCATION_UPDATE_INTERVAL_DEFAULT	LOCATION_UPDATE_INTERVAL_MIN
#define LOCATION_BATCH_PERIOD_MIN			120
#define LOCATION_BATCH_PERIOD_MAX			600
#define LOCATION_BATCH_PERIOD_DEFAULT		LOCATION_BATCH_PERIOD_MIN
#define LOCATION_MIN_INTERVAL_MIN			1
#define LOCATION_MIN_INTERVAL_MAX			120
#define LOCATION_MIN_INTERVAL_DEFAULT		LOCATION_MIN_INTERVAL_MIN
#define LOCATION_MIN_DISTANCE_MIN			1.0
#define LOCATION_MIN_DISTANCE_MAX			120.0
#define LOCATION_MIN_DISTANCE_DEFAULT		LOCATION_MIN_DISTANCE_MIN

typedef void (*SettingCB)(keynode_t *key, gpointer data);


gint location_setting_send_system_event(const char *path, const int val);
gint location_setting_get_key_val(keynode_t *key);
gint location_setting_get_int(const gchar *path);
gboolean location_setting_get_bool(const gchar *path);
gchar *location_setting_get_string(const gchar *path);
gint location_setting_add_notify(const gchar *path, SettingCB setting_cb, gpointer self);
gint location_setting_ignore_notify(const gchar *path, SettingCB setting_cb);
gint location_state_add_notify(const gchar *path, SettingCB setting_cb, gpointer self);
gint location_state_ignore_notify(const gchar *path, SettingCB setting_cb);


#define setting_retval_if_fail(path) {\
		int val = location_setting_get_int(path);\
		if (val == -1){\
			return LOCATION_ERROR_UNKNOWN;\
		} else if (val == 0) {\
			return LOCATION_ERROR_SETTING_OFF;\
		}\
	}

#endif
