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

#include <glib.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "module-internal.h"
#include "location-log.h"

#define MAX_MODULE_INDEX 4
const char *MODULE_PATH_PREFIX = "/usr/lib/location/module";

static GMod *gmod_new(const char *module_name, gboolean is_resident)
{
	if (!module_name)
		return NULL;

	GMod *gmod = g_new0(GMod, 1);
	gmod->name = g_strdup(module_name);
	if (!gmod->name) {
		g_free(gmod);
		return NULL;
	}
	gmod->path = g_module_build_path(MODULE_PATH_PREFIX, gmod->name);
	if (!gmod->path) {
		g_free(gmod->name);
		g_free(gmod);
		return NULL;
	}
	gmod->module = g_module_open(gmod->path, G_MODULE_BIND_LAZY);
	if (!gmod->module) {
		g_free(gmod->name);
		g_free(gmod->path);
		g_free(gmod);
		return NULL;
	}
	if (is_resident)
		g_module_make_resident(gmod->module);

	return gmod;
}

static void gmod_free(GMod *gmod)
{
	if (gmod->name)
		g_free(gmod->name);
	if (gmod->path)
		g_free(gmod->path);
	if (gmod->module)
		g_module_close(gmod->module);
	g_free(gmod);
}

static gboolean
gmod_find_sym(GMod *gmod, gpointer *init_func, gpointer *shutdown_func)
{
	char sym[256];
	g_stpcpy(sym, "init");
	if (!g_module_symbol(gmod->module, sym, init_func)) {
		LOCATION_LOGW("symbol not found: %s", sym);
		return FALSE;
	}
	g_stpcpy(sym, "shutdown");
	if (!g_module_symbol(gmod->module, sym, shutdown_func)) {
		LOCATION_LOGW("symbol not found: %s", sym);
		return FALSE;
	}
	return TRUE;
}

static gpointer mod_new(const char *module_name)
{
	gpointer ret_mod = NULL;
	if (!module_name)
		return NULL;

	GMod *gmod = NULL;
	gpointer init = NULL;
	gpointer shutdown = NULL;
	gmod = gmod_new(module_name, TRUE);
	if (!gmod) {
		LOCATION_LOGW("module(%s) new failed", module_name);
		return NULL;
	}
	if (!gmod_find_sym(gmod, &init, &shutdown)) {
		LOCATION_LOGW("symbol (init, shutdown) finding failed");
		gmod_free(gmod);
		return NULL;
	}
	if (!init || !shutdown) {
		LOCATION_LOGW("init, shutdown symbol is NULL");
		gmod_free(gmod);
		return NULL;
	}
	if (g_str_has_prefix(module_name, "gps")) {
		LocationGpsMod *_mod = g_new0(LocationGpsMod, 1);
		_mod->gmod = gmod;
		_mod->init = init;
		_mod->shutdown = shutdown;
		_mod->handler = _mod->init(&(_mod->ops));
		if (!_mod->handler) {
			LOCATION_LOGW("module init failed");
			gmod_free(_mod->gmod);
			ret_mod = NULL;
		} else
			ret_mod = (gpointer) _mod;
	} else if (g_str_has_prefix(module_name, "wps")) {
		LocationWpsMod *_mod = g_new0(LocationWpsMod, 1);
		_mod->gmod = gmod;
		_mod->init = init;
		_mod->shutdown = shutdown;
		_mod->handler = _mod->init(&(_mod->ops));
		if (!_mod->handler) {
			LOCATION_LOGW("module init failed");
			gmod_free(_mod->gmod);
			ret_mod = NULL;
		} else
			ret_mod = (gpointer) _mod;
	} else {
		LOCATION_LOGW("module name (%s) is wrong", module_name);
		ret_mod = NULL;
	}
	return ret_mod;
}

static void mod_free(gpointer mod, const char *module_name)
{
	if (!mod || !module_name)
		return;

	if (0 == g_strcmp0(module_name, "gps")) {
		LocationGpsMod *_mod = (LocationGpsMod *) mod;
		if (_mod->shutdown && _mod->handler) {
			_mod->shutdown(_mod->handler);
		}
		_mod->handler = NULL;
		_mod->init = NULL;
		_mod->shutdown = NULL;
		gmod_free(_mod->gmod);
		_mod->gmod = NULL;
	} else if (0 == g_strcmp0(module_name, "wps")) {
		LocationWpsMod *_mod = (LocationWpsMod *) mod;
		if (_mod->shutdown && _mod->handler) {
			_mod->shutdown(_mod->handler);
		}
		_mod->handler = NULL;
		_mod->init = NULL;
		_mod->shutdown = NULL;
		gmod_free(_mod->gmod);
		_mod->gmod = NULL;
	} else
		LOCATION_LOGW("module name (%s) is wrong", module_name);

	g_free(mod);
}

static gboolean mod_is_supported(const char *module_name)
{
	GMod *gmod = NULL;
	gmod = gmod_new(module_name, FALSE);
	if (!gmod) {
		return FALSE;
	}
	gmod_free(gmod);

	return TRUE;
}

gboolean module_init(void)
{
	if (!g_module_supported()) {
		LOCATION_LOGW("module is not supported");
		return FALSE;
	}
	return TRUE;
}

void module_free(gpointer mod, const char *module_name)
{
	if (!mod || !module_name)
		return;
	mod_free(mod, module_name);
}

gpointer module_new(const char *module_name)
{
	if (!module_name)
		return NULL;
	int index = 0;
	char name[256];

	gpointer mod = NULL;
	for (index = -1; index < MAX_MODULE_INDEX; index++) {
		if (index >= 0) {
			if (0 >= g_snprintf(name, 256, "%s%d", module_name, index)) {
				LOCATION_LOGW("module name(%s) is wrong", name);
				continue;
			}
		} else {
			if (0 >= g_snprintf(name, 256, "%s", module_name)) {
				LOCATION_LOGW("module name(%s) is wrong", name);
				continue;
			}
		}
		mod = mod_new(name);
		if (mod) {
			LOCATION_LOGW("module (%s) open success", name);
			break;
		}
		LOCATION_LOGW("module (%s) open failed", name);
	}
	return mod;
}

gboolean module_is_supported(const char *module_name)
{
	if (!module_name)
		return FALSE;

	int index = 0;
	gboolean ret = FALSE;
	gboolean found = FALSE;

	char name[256] = { 0, };

	for (index = -1; index < MAX_MODULE_INDEX; index++) {
		if (index >= 0) {
			g_snprintf(name, 256, "%s%d", module_name, index);
		} else {
			g_snprintf(name, 256, "%s", module_name);
		}

		ret = mod_is_supported(name);
		if (ret == TRUE) {
			found = TRUE;
			LOCATION_LOGW("module name(%s) is found", name);
			break;
		}
	}

	return found;
}

gchar *mod_get_realpath(const gchar *module_name)
{
	gchar origin_path[PATH_MAX] = { 0, };
	gchar link_path[PATH_MAX] = { 0, };
	gchar *path = NULL;

	snprintf(link_path, PATH_MAX, "%s/lib%s.so", MODULE_PATH_PREFIX, module_name);

	if (realpath(link_path, origin_path) == NULL || strlen(origin_path) == 0) {
		LOCATION_LOGE("Fail to get real path of [%s]", module_name);
		return NULL;
	}

	path = strrchr(origin_path, '/');
	if (!path)
		return NULL;

	return g_strdup(path);
}
