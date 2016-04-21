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
#include <stdio.h>
#include <stdlib.h>

#include <app_manager.h>
#include <cynara-client.h>
#include <cynara-session.h>

#include "location-common-util.h"
#include "location-types.h"
#include "location-log.h"
#include "location-privacy.h"

int location_check_cynara(const char *privilege_name)
{
	cynara *cynara = NULL;
	int ret = LOCATION_ERROR_NONE;
	FILE *fp = NULL;
	char uid[16];
	char *session = NULL;
	char smack_label[100] = {0, };

	if (cynara_initialize(&cynara, NULL) != CYNARA_API_SUCCESS)
	{
		LOCATION_LOGE("cynara initialize failed");
		cynara = NULL;
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	fp = fopen("/proc/self/attr/current", "r");
	if (fp != NULL) {
		int ch = 0;
		int idx = 0;
		while (EOF != (ch = fgetc(fp))) {
			smack_label[idx] = ch;
			idx++;
		}
		fclose(fp);
	}

	pid_t pid = getpid();
	session = cynara_session_from_pid(pid);
	snprintf(uid, 16, "%d", getuid());
	ret = cynara_check(cynara, smack_label, session, uid, privilege_name);

	if (session) {
		free(session);
	}

	if (cynara) {
		cynara_finish(cynara);
	}

	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		LOCATION_LOGE("cynara_check failed [%d]", ret);
		return LOCATION_ERROR_NOT_ALLOWED;
	}

	return LOCATION_ERROR_NONE;
}
