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

#ifndef __LOCATION_FUSED_H__
#define __LOCATION_FUSED_H__

#include <glib-object.h>

/**
 * @file location-passive.h
 * @brief This file contains the internal definitions and structures related to FUSED.
 */

G_BEGIN_DECLS

#define LOCATION_TYPE_FUSED				(location_fused_get_type ())
#define LOCATION_FUSED(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOCATION_TYPE_FUSED, LocationFused))
#define LOCATION_IS_FUSED(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOCATION_TYPE_FUSED))
#define LOCATION_FUSED_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), LOCATION_TYPE_FUSED, LocationFusedClass))
#define LOCATION_IS_FUSED_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), LOCATION_TYPE_FUSED))
#define LOCATION_FUSED_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), LOCATION_TYPE_FUSED, LocationFusedClass))


#define SQUARE(x) ((x)*(x))
#define NOISE_LEVEL				0.5
#define MOTION_LEVEL			2.0
#define GRAVITY					-(9.81)
#define SAMPLING_FREQUENCY		10.0	// default sensor sampling interval is 100ms
#define IMMOBILITY_INTERVAL		30.0
#define INTEGRATION_INTERVAL	10.0
#define CALIBRATION_INTERVAL	60

#define IMMOBILITY_THRESHOLD (SQUARE(NOISE_LEVEL) / 18.0 * (NOISE_LEVEL * M_PI + 4.0 * SQUARE(GRAVITY)))
#define IMMOBILITY_LEVEL (SQUARE(NOISE_LEVEL * 2) / 18.0 * (NOISE_LEVEL * 2 * M_PI + 4 * SQUARE(GRAVITY)))
#define MOVEMENT_THRESHOLD (SQUARE(MOTION_LEVEL	) / 18.0 * (MOTION_LEVEL * M_PI + 4 * SQUARE(GRAVITY)))
#define CALIBRATION_SAMPLES (SAMPLING_FREQUENCY * CALIBRATION_INTERVAL)
#define RMA_REPLACEMENT_RATE (1.0 / (SAMPLING_FREQUENCY * INTEGRATION_INTERVAL))
#define RMA_DECAY_RATE		 (1 - RMA_REPLACEMENT_RATE)



typedef struct _LocationFused		LocationFused;
typedef struct _LocationFusedClass	LocationFusedClass;

struct _LocationFused {
	GObject parent_instance;
};

/*
struct _LocationFusedValues {
	gdouble _G;
	gdouble _B1;
	gdouble _B2;
	gdouble _u[TIME_SHIFT];
	gdouble _v[TIME_SHIFT];
};*/

struct _LocationFusedClass {
	GObjectClass parent_class;

	void (* enabled)(guint type);
	void (* disabled)(guint type);
	void (* service_updated)(gint type, gpointer data, gpointer velocity, gpointer accuracy);
	void (* location_updated)(gint error, gpointer position, gpointer velocity, gpointer accuracy);
	void (* zone_in)(gpointer boundary, gpointer position, gpointer accuracy);
	void (* zone_out)(gpointer boundary, gpointer position, gpointer accuracy);
	void (* status_changed)(guint type);
};

GType location_fused_get_type(void);

G_END_DECLS

#endif
