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

#ifndef __LOCATION_WPS_H__
#define __LOCATION_WPS_H__

#include <glib-object.h>

/**
 * @file location-wps.h
 * @brief This file contains the internal definitions and structures related to WPS.
 */

G_BEGIN_DECLS

#define LOCATION_TYPE_WPS				(location_wps_get_type ())
#define LOCATION_WPS(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOCATION_TYPE_WPS, LocationWps))
#define LOCATION_IS_WPS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOCATION_TYPE_WPS))
#define LOCATION_WPS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), LOCATION_TYPE_WPS, LocationWpsClass))
#define LOCATION_IS_WPS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), LOCATION_TYPE_WPS))
#define LOCATION_WPS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), LOCATION_TYPE_WPS, LocationWpsClass))

typedef struct _LocationWps			LocationWps;
typedef struct _LocationWpsClass	LocationWpsClass;

struct _LocationWps {
	GObject parent_instance;
};

struct _LocationWpsClass {
	GObjectClass parent_class;

	void (* enabled)(guint type);
	void (* disabled)(guint type);
	void (* updated)(gint type, gpointer data, gpointer velocity, gpointer accuracy);
	void (* location_updated)(gint error, gpointer position, gpointer velocity, gpointer accuracy);
	void (* zone_in)(gpointer boundary, gpointer position, gpointer accuracy);
	void (* zone_out)(gpointer boundary, gpointer position, gpointer accuracy);
};

GType location_wps_get_type(void);

G_END_DECLS

#endif
