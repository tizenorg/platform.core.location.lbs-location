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

#ifndef __LOCATION_HYBRID_MOBILE_H__
#define __LOCATION_HYBRID_MOBILE_H__

#include <glib-object.h>

/**
 * @file location-hybrid-mobile.h
 * @brief This file contains the internal definitions and structures related to Hybrid method.
 */

G_BEGIN_DECLS

typedef struct _LocationHybrid			LocationHybrid;
typedef struct _LocationHybridClass		LocationHybridClass;

#define LOCATION_TYPE_HYBRID				(location_hybrid_get_type ())
#define LOCATION_HYBRID(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOCATION_TYPE_HYBRID, LocationHybrid))
#define LOCATION_IS_HYBRID(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOCATION_TYPE_HYBRID))
#define LOCATION_HYBRID_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), LOCATION_TYPE_HYBRID, LocationHybridClass))
#define LOCATION_IS_HYBRID_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), LOCATION_TYPE_HYBRID))
#define LOCATION_HYBRID_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), LOCATION_TYPE_HYBRID, LocationHybridClass))

struct _LocationHybrid {
	GObject parent_instance;
};

struct _LocationHybridClass {
	GObjectClass parent_class;

	void (* enabled)(guint type);
	void (* disabled)(guint type);
	void (* updated)(gint type, gpointer data, gpointer velocity, gpointer accuracy);
	void (* location_updated)(gint error, gpointer position, gpointer velocity, gpointer accuracy);
	void (* zone_in)(gpointer boundary, gpointer position, gpointer accuracy);
	void (* zone_out)(gpointer boundary, gpointer position, gpointer accuracy);
};

GType location_hybrid_get_type(void);

#define HYBRID_POSITION_EXPIRATION_TIME		9

G_END_DECLS

#endif
