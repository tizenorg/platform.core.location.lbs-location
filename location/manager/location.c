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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <stdio.h>
#include <pthread.h>
#include <vconf.h>

#include "location.h"
#include "location-log.h"
#include "location-setting.h"
#include "location-ielement.h"
#include "location-hybrid.h"
#include "location-gps.h"
#include "location-wps.h"
#include "location-position.h"
#include "module-internal.h"
#include "location-common-util.h"
#ifndef TIZEN_PROFILE_TV
#include "location-privacy.h"
#endif

#define LOCATION_PRIVILEGE	"http://tizen.org/privilege/location"
#define LOCATION_ENABLE_PRIVILEGE	"http://tizen.org/privilege/location.enable"

typedef struct _LocationSetting {
	LocationSettingCb callback;
	void *user_data;
} LocationSetting;

static LocationSetting g_location_setting;

static char *__convert_setting_key(LocationMethod method)
{
	char *key = NULL;
	switch (method) {
		case LOCATION_METHOD_HYBRID:
			key = g_strdup(VCONFKEY_LOCATION_USE_MY_LOCATION);
			break;
		case LOCATION_METHOD_GPS:
			key = g_strdup(VCONFKEY_LOCATION_ENABLED);
			break;
		case LOCATION_METHOD_WPS:
			key = g_strdup(VCONFKEY_LOCATION_NETWORK_ENABLED);
			break;
		default:
			break;
	}

	return key;
}

static LocationMethod __convert_method_from_key(const char *key)
{
	LocationMethod _method = LOCATION_METHOD_NONE;
	if (g_strcmp0(key, VCONFKEY_LOCATION_USE_MY_LOCATION) == 0) {
		_method = LOCATION_METHOD_HYBRID;
	} else if (g_strcmp0(key, VCONFKEY_LOCATION_ENABLED) == 0) {
		_method = LOCATION_METHOD_GPS;
	} else if (g_strcmp0(key, VCONFKEY_LOCATION_NETWORK_ENABLED) == 0) {
		_method = LOCATION_METHOD_WPS;
	}

	return _method;
}

static void __location_setting_cb(keynode_t *key, gpointer data)
{
	if (key == NULL || data == NULL) {
		return;
	}
	LocationSetting *_setting = (LocationSetting *)data;
	LocationMethod _method = LOCATION_METHOD_NONE;

	if (_setting->callback) {
		_method = __convert_method_from_key(vconf_keynode_get_name(key));
		_setting->callback(_method, location_setting_get_key_val(key), _setting->user_data);
	}
}

EXPORT_API
int location_init(void)
{
#if !GLIB_CHECK_VERSION (2, 35, 0)
	g_type_init();
#endif

#if !GLIB_CHECK_VERSION (2, 31, 0)
	if (!g_thread_supported()) g_thread_init(NULL);
#endif
	if (FALSE == module_init())
		return LOCATION_ERROR_NOT_AVAILABLE;

#ifndef TIZEN_PROFILE_TV
	location_privacy_initialize();
#endif

	return LOCATION_ERROR_NONE;
}

EXPORT_API LocationObject *
location_new(LocationMethod method)
{
	LocationObject *self = NULL;
	LOCATION_LOGD("method: %d", method);

	switch (method) {
		case LOCATION_METHOD_HYBRID:
			self = g_object_new(LOCATION_TYPE_HYBRID, NULL);
			break;
		case LOCATION_METHOD_GPS:
			self = g_object_new(LOCATION_TYPE_GPS, NULL);
			break;
		case LOCATION_METHOD_WPS:
			self = g_object_new(LOCATION_TYPE_WPS, NULL);
			break;
		default:
			break;
	}

	if (!self) LOCATION_LOGE("Fail to create location object. Method [%d]", method);
	return self;
}

EXPORT_API int
location_free(LocationObject *obj)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);

#ifndef TIZEN_PROFILE_TV
	location_privacy_finalize();
#endif
	g_object_unref(obj);
	return LOCATION_ERROR_NONE;
}

EXPORT_API int
location_request_single_location(LocationObject *obj, int timeout)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_request_single_location(LOCATION_IELEMENT(obj), timeout);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to request single location. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_start(LocationObject *obj)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_start(LOCATION_IELEMENT(obj));
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to start. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_stop(LocationObject *obj)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_stop(LOCATION_IELEMENT(obj));
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to stop. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_start_batch(LocationObject *obj)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_start_batch(LOCATION_IELEMENT(obj));
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to Batch start. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_stop_batch(LocationObject *obj)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_stop_batch(LOCATION_IELEMENT(obj));
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to Batch stop. Error [%d]", ret);

	return ret;
}

EXPORT_API gboolean
location_is_supported_method(LocationMethod method)
{
	gboolean is_supported = FALSE;

	switch (method) {
		case LOCATION_METHOD_HYBRID:
			if (module_is_supported("gps") || module_is_supported("wps"))
				is_supported = TRUE;
			break;
		case LOCATION_METHOD_GPS:
			is_supported = module_is_supported("gps");
			break;
		case LOCATION_METHOD_WPS:
			is_supported = module_is_supported("wps");
			break;
		default:
			break;
	}

	return is_supported;
}

EXPORT_API int
location_is_enabled_method(LocationMethod method, int *is_enabled)
{
	g_return_val_if_fail(is_enabled, LOCATION_ERROR_PARAMETER);
	int vconf_val = 0;
	int vconf_ret = VCONF_ERROR;

	char *_key = __convert_setting_key(method);
	if (!_key) {
		LOCATION_LOGE("Invalid method[%d]", method);
		return LOCATION_ERROR_NOT_SUPPORTED;
	}

	vconf_ret = vconf_get_int(_key, &vconf_val);
	if (vconf_ret != VCONF_OK) {
		LOCATION_SECLOG("failed [%s], error [%d]", _key, vconf_ret);
		g_free(_key);
		return LOCATION_ERROR_NOT_AVAILABLE;
	} else {
		LOCATION_SECLOG("[%s]:[%d]", _key, vconf_val);
	}

	*is_enabled = vconf_val;
	g_free(_key);

	return LOCATION_ERROR_NONE;
}

EXPORT_API int
location_enable_method(const LocationMethod method, const int enable)
{
	int ret = 0;
	char *_key = NULL;

#ifndef TIZEN_PROFILE_TV
	ret = location_check_privilege(LOCATION_ENABLE_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	/* for itself */
	_key = __convert_setting_key(method);
	if (!_key) {
		LOCATION_LOGE("Invalid method[%d]", method);
		return LOCATION_ERROR_NOT_SUPPORTED;
	}
	ret = vconf_set_int(_key, enable);
	if (ret != VCONF_OK) {
		LOCATION_SECLOG("vconf_set_int failed [%s], ret=[%d]", _key, ret);
		g_free(_key);
		return LOCATION_ERROR_NOT_ALLOWED;
	} else {
		ret = location_setting_send_system_event(_key, enable);
		if (ret != LOCATION_ERROR_NONE) {
			LOCATION_SECLOG("Fail to send system event [%s], ret=[%d]", _key, ret);
			g_free(_key);
			return LOCATION_ERROR_NOT_ALLOWED;
		}
	}
	g_free(_key);

	/* for hybrid */
	_key = __convert_setting_key(LOCATION_METHOD_HYBRID);
	if (!_key) {
		LOCATION_LOGE("Invalid method[%d]", LOCATION_METHOD_HYBRID);
		return LOCATION_ERROR_NOT_SUPPORTED;
	}

	if (enable) {
		ret = vconf_set_int(_key, enable);
		if (ret != VCONF_OK) {
			LOCATION_SECLOG("vconf_set_int failed [%s], ret=[%d]", _key, ret);
			g_free(_key);
			return LOCATION_ERROR_NOT_ALLOWED;
		} else {
		 	ret = location_setting_send_system_event(_key, enable);
			if (ret != LOCATION_ERROR_NONE) {
				LOCATION_SECLOG("Fail to send system event [%s], ret=[%d]", _key, ret);
				g_free(_key);
				return LOCATION_ERROR_NOT_ALLOWED;
			}
		}
	}
	else {
		int i = 0;
		int enabled_state = 0;

		for (i = LOCATION_METHOD_GPS; i < LOCATION_METHOD_MAX; i++) {
			_key = __convert_setting_key(i);
			enabled_state |= location_setting_get_int(_key);
			g_free(_key);
		}
		if (!enabled_state) {
			_key = __convert_setting_key(LOCATION_METHOD_HYBRID);
			ret = vconf_set_int(_key, enable);
			if (ret != VCONF_OK) {
				LOCATION_SECLOG("vconf_set_int failed [%s], error [%d]", _key, ret);
				g_free(_key);
				return LOCATION_ERROR_NOT_ALLOWED;
			} else {
				LOCATION_SECLOG("[%s]:[%d]", _key, ret);
			 	ret = location_setting_send_system_event(_key, enable);
				if (ret != LOCATION_ERROR_NONE) {
					g_free(_key);
					LOCATION_SECLOG("Fail to send system event [%s], error [%d]", _key, ret);
					return LOCATION_ERROR_NOT_ALLOWED;
				}
			}
			g_free(_key);
		}
	}

	return ret;
}

EXPORT_API int
location_add_setting_notify(LocationMethod method, LocationSettingCb callback, void *user_data)
{
	int ret = LOCATION_ERROR_NONE;
	char *_key = __convert_setting_key(method);
	if (!_key) {
		LOCATION_LOGE("Invalid method[%d]", method);
		return LOCATION_ERROR_PARAMETER;
	}

	g_location_setting.callback = callback;
	g_location_setting.user_data = user_data;

	ret = location_setting_add_notify(_key, __location_setting_cb, &g_location_setting);
	g_free(_key);

	return ret;
}

EXPORT_API int
location_ignore_setting_notify(LocationMethod method, LocationSettingCb callback)
{
	int ret = LOCATION_ERROR_NONE;
	char *_key = __convert_setting_key(method);
	if (!_key) {
		LOCATION_LOGE("Invalid method[%d]", method);
		return LOCATION_ERROR_PARAMETER;
	}

	g_location_setting.callback = NULL;
	g_location_setting.user_data = NULL;

	ret = location_setting_ignore_notify(_key, __location_setting_cb);
	g_free(_key);

	return ret;
}


EXPORT_API int
location_get_position(LocationObject *obj,
                      LocationPosition **position,
                      LocationAccuracy **accuracy)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_position(LOCATION_IELEMENT(obj), position, accuracy);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_position. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_position_ext(LocationObject *obj,
                          LocationPosition **position,
                          LocationVelocity **velocity,
                          LocationAccuracy **accuracy)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_position_ext(LOCATION_IELEMENT(obj), position, velocity, accuracy);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_position_ext. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_last_position(LocationObject *obj,
                           LocationPosition **position,
                           LocationAccuracy **accuracy)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_last_position(LOCATION_IELEMENT(obj), position, accuracy);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_last_position. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_last_position_ext(LocationObject *obj,
                               LocationPosition **position,
                               LocationVelocity **velocity,
                               LocationAccuracy **accuracy)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_last_position_ext(LOCATION_IELEMENT(obj), position, velocity, accuracy);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_last_position_ext. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_nmea(LocationObject *obj, char **nmea)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(nmea, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_nmea(LOCATION_IELEMENT(obj), nmea);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_nmea. Error [%d]", ret);

	return ret;
}


EXPORT_API int
location_get_satellite(LocationObject *obj, LocationSatellite **satellite)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(satellite, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_satellite(LOCATION_IELEMENT(obj), satellite);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_satellite. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_batch(LocationObject *obj, LocationBatch **batch)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(batch, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_batch(LOCATION_IELEMENT(obj), batch);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_batch. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_last_satellite(LocationObject *obj, LocationSatellite **satellite)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(satellite, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_last_satellite(LOCATION_IELEMENT(obj), satellite);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_last_satellite. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_velocity(LocationObject *obj,
                      LocationVelocity **velocity,
                      LocationAccuracy **accuracy)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_velocity(LOCATION_IELEMENT(obj), velocity, accuracy);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_velocity. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_last_velocity(LocationObject *obj,
                           LocationVelocity **velocity,
                           LocationAccuracy **accuracy)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_get_last_velocity(LOCATION_IELEMENT(obj), velocity, accuracy);
	if (ret != LOCATION_ERROR_NONE) LOCATION_LOGE("Fail to get_last_velocity. Error [%d]", ret);

	return ret;
}

EXPORT_API int
location_get_accessibility_state(LocationAccessState *state)
{
	int auth = location_application_get_authority();
	switch (auth) {
		case LOCATION_APP_OFF:
			*state = LOCATION_ACCESS_DENIED;
			break;
		case LOCATION_APP_ON:
			*state = LOCATION_ACCESS_ALLOWED;
			break;
		case LOCATION_APP_NOT_FOUND:
			*state = LOCATION_ACCESS_NONE;
			break;
		default:
			return LOCATION_ERROR_UNKNOWN;
	}

	LOCATION_LOGD("get_accessibility_state [%d]", auth);
	return LOCATION_ERROR_NONE;
}

EXPORT_API int
location_set_accessibility_state(LocationAccessState state)
{
	int auth = LOCATION_APP_NOT_FOUND;
	int ret = LOCATION_ERROR_NONE;

	switch (state) {
		case LOCATION_ACCESS_DENIED:
			auth = LOCATION_APP_OFF;
			break;
		case LOCATION_ACCESS_ALLOWED:
			auth = LOCATION_APP_ON;
			break;
		case LOCATION_ACCESS_NONE:
		default:
			return LOCATION_ERROR_PARAMETER;
	}

	ret = location_application_set_authority(auth);
	LOCATION_LOGD("set_accessibility_state [%d], Error[%d]", auth, ret);
	return ret;
}

EXPORT_API int
location_send_command(const char *cmd)
{
	g_return_val_if_fail(cmd, LOCATION_ERROR_PARAMETER);
	return LOCATION_ERROR_NOT_AVAILABLE;
}

EXPORT_API int
location_set_option(LocationObject *obj, const char *option)
{
	g_return_val_if_fail(obj, LOCATION_ERROR_PARAMETER);
	int ret = LOCATION_ERROR_NONE;

#ifndef TIZEN_PROFILE_TV
	ret = location_get_privacy(LOCATION_PRIVILEGE);
	if (ret != LOCATION_ERROR_NONE) {
		LOCATION_LOGE("Cannot use location service for privacy[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}
#endif

	ret = location_ielement_set_option(LOCATION_IELEMENT(obj), option);
	if (ret != LOCATION_ERROR_NONE)
		LOCATION_LOGE("Fail to get_velocity. Error [%d]", ret);
	return ret;
}
