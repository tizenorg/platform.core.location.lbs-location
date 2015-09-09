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

#include <sys/types.h>
#include <unistd.h>
#include <glib.h>
#include <stdlib.h>

#include <app_manager.h>
#include <package_manager.h>
#include <pkgmgr-info.h>
#include <privacy_checker_client.h>

#include "location-common-util.h"
#include "location-types.h"
#include "location-log.h"
#include "location-privacy.h"

typedef struct _location_privilege_s {
	char *name;
	bool found;
} location_privilege_s;



void
location_privacy_initialize(void)
{
	int ret = 0;
	pid_t pid = 0;
	char *app_id = NULL;
	char *package_id = NULL;
	pkgmgrinfo_appinfo_h pkgmgrinfo_appinfo;

	pid = getpid();
	ret = app_manager_get_app_id(pid, &app_id);
	if (ret != APP_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get app_id. Err[%d]", ret);
		return;
	}

	ret = pkgmgrinfo_appinfo_get_appinfo(app_id, &pkgmgrinfo_appinfo);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get appinfo for [%s]. Err[%d]", app_id, ret);
		free(app_id);
		return;
	}

	ret = pkgmgrinfo_appinfo_get_pkgname(pkgmgrinfo_appinfo, &package_id);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get package_id for [%s]. Err[%d]", app_id, ret);
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		free(app_id);
		return;
	}

	ret = privacy_checker_initialize(package_id);
	if (ret != PRIV_MGR_ERROR_SUCCESS) {
		LOCATION_LOGE("Fail to initialize privacy checker. err[%d]", ret);
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		free(app_id);
		return;
	}

	LOCATION_LOGD("Success to initialize privacy checker");

	free(app_id);
	pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
}

void
location_privacy_finalize(void)
{
	int ret = 0;
	ret = privacy_checker_finalize();
	if (ret != PRIV_MGR_ERROR_SUCCESS) {
		LOCATION_LOGE("Fail to finalize privacy_cehecker. Err[%d]", ret);
		return;
	}

	LOCATION_LOGD("Success to finalize privacy checker");
}

int
location_get_privacy(const char *privilege_name)
{
	int ret = 0;
	pid_t pid = 0;
	char *app_id = NULL;
	char *package_id = NULL;
	int app_type = 0;
	pkgmgrinfo_appinfo_h pkgmgrinfo_appinfo;

	pid = getpid();
	ret = app_manager_get_app_id(pid, &app_id);
	if (ret != APP_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get app_id. Err[%d]", ret);
		return LOCATION_ERROR_NONE;
	}

	app_type = location_get_app_type(app_id);
	if (app_type == CPPAPP) {
		LOCATION_LOGE("CPPAPP use location");
		g_free(app_id);
		return LOCATION_ERROR_NONE;
	}

	ret = pkgmgrinfo_appinfo_get_appinfo(app_id, &pkgmgrinfo_appinfo);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get appinfo for [%s]. Err[%d]", app_id, ret);
		g_free(app_id);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	ret = pkgmgrinfo_appinfo_get_pkgname(pkgmgrinfo_appinfo, &package_id);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get package_id for [%s]. Err[%d]", app_id, ret);
		g_free(app_id);
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

#ifdef TIZEN_PROFILE_WERABLE
	if (app_type == WEBAPP) {
		LOCATION_LOGI("WEBAPP use location");
		if (location_get_webapp_privilege(package_id, privilege_name) == 0) {
			g_free(package_id);
			g_free(app_id);
			return LOCATION_ERROR_NONE;
		}
	}
#endif

	ret = privacy_checker_check_package_by_privilege(package_id, privilege_name);
	if (ret != PRIV_MGR_ERROR_SUCCESS) {
		LOCATION_LOGE("Fail to get privilege for [%s]. Err[%d]", package_id, ret);
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		g_free(app_id);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
	g_free(app_id);

	return LOCATION_ERROR_NONE;
}


int
location_check_privilege(const char *privilege_name)
{
	int ret = 0;
	pid_t pid = 0;
	char *app_id = NULL;
	char *package_id = NULL;
	int app_type = 0;
	pkgmgrinfo_appinfo_h pkgmgrinfo_appinfo;

	pid = getpid();
	ret = app_manager_get_app_id(pid, &app_id);
	if (ret != APP_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get app_id. Err[%d]", ret);
		return LOCATION_ERROR_NONE;
	}

	app_type = location_get_app_type(app_id);
	if (app_type == CPPAPP) {
		LOCATION_LOGE("CPPAPP use location");
		g_free(app_id);
		return LOCATION_ERROR_NONE;
	}

	ret = pkgmgrinfo_appinfo_get_appinfo(app_id, &pkgmgrinfo_appinfo);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get appinfo for [%s]. Err[%d]", app_id, ret);
		g_free(app_id);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	ret = pkgmgrinfo_appinfo_get_pkgname(pkgmgrinfo_appinfo, &package_id);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		LOCATION_LOGE("Fail to get package_id for [%s]. Err[%d]", app_id, ret);
		g_free(app_id);
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

#ifdef TIZEN_WERABLE
	if (app_type == WEBAPP) {
		LOCATION_LOGE("WEBAPP use location");
		if (location_get_webapp_privilege(package_id, privilege_name) == 0) {
			g_free(package_id);
			g_free(app_id);
			return LOCATION_ERROR_NONE;
		}
	}
#endif

	ret = privacy_checker_initialize(package_id);
	if (ret != PRIV_MGR_ERROR_SUCCESS) {
		LOCATION_LOGE("Fail to initialize privacy checker. err[%d]", ret);
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		g_free(app_id);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	ret = privacy_checker_check_package_by_privilege(package_id, privilege_name);
	if (ret != PRIV_MGR_ERROR_SUCCESS) {
		LOCATION_LOGE("Fail to get privilege for [%s]. Err[%d]", package_id, ret);
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		g_free(app_id);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
	g_free(app_id);

	ret = privacy_checker_finalize();
	if (ret != PRIV_MGR_ERROR_SUCCESS) {
		LOCATION_LOGE("Fail to finalize privacy_cehecker. Err[%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	return LOCATION_ERROR_NONE;
}

