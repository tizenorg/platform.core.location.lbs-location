/*
 * libslp-location
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *					Genie Kim <daejins.kim@samsung.com>
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

#ifndef __LOCATION_LOG_H__
#define __LOCATION_LOG_H__

/**
 * @file location-log.h
 * @brief This file contains macro functions for logging.
 */

/* Tag defines */
#define TAG_LOCATION_FWK "LOCATION"

#include <dlog.h>
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG TAG_LOCATION_FWK
#endif

#define LOCATION_LOGD(fmt, args...)		LOGD(fmt, ##args)
#define LOCATION_LOGW(fmt, args...)		LOGW(fmt, ##args)
#define LOCATION_LOGI(fmt, args...)		LOGI(fmt, ##args)
#define LOCATION_LOGE(fmt, args...)		LOGE(fmt, ##args)
#define LOCATION_SECLOG(fmt, args...)	SECURE_LOGD(fmt, ##args)

#define _D(fmt, args...)		LOGD(fmt, ##args)
#define _W(fmt, args...)		LOGW(fmt, ##args)
#define _I(fmt, args...)		LOGI(fmt, ##args)
#define _E(fmt, args...)		LOGE(fmt, ##args)

#define LOC_IF_FAIL(ret, tag, fmt, arg...) \
	do { if (ret != 0) { tag(fmt, ##arg); return ret; } } while (0)

#define LOC_IF_FAIL_LOG(ret, tag, fmt, arg...) \
	do { if (ret != 0) tag(fmt, ##arg); } while (0)

#define LOC_COND_RET(cond, ret, tag, fmt, arg...) \
	do { if (cond) { tag(fmt, ##arg); return ret; } } while (0)

#define LOC_COND_VOID(cond, tag, fmt, arg...) \
	do { if (cond) { tag(fmt, ##arg); return ; } } while (0)

#define LOC_COND_LOG(cond, tag, fmt, arg...) \
	do { if (cond) tag(fmt, ##arg); } while (0)

#define LOC_FUNC_LOG		LOGI("%s", __func__);

#endif
