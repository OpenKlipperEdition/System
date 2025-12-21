/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: t -*-
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*#include "config.h"*/
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dlog.h>
#include <syslog.h>

#define LOG_BUF_SIZE	1024
/*
  LOG_EMERG：紧急情况，需要立即通知技术人员。
  LOG_ALERT：应该被立即改正的问题，如系统数据库被破坏，ISP连接丢失。
  LOG_CRIT：重要情况，如硬盘错误，备用连接丢失。
  LOG_ERR：错误，不是非常紧急，在一定时间内修复即可。
  LOG_WARNING：警告信息，不是错误，比如系统磁盘使用了85%等。
  LOG_NOTICE：不是错误情况，也不需要立即处理。
  LOG_INFO：情报信息，正常的系统消息，比如骚扰报告，带宽数据等，不需要处理。
  LOG_DEBUG：包含详细的开发情报的信息，通常只在调试一个程序时使用。
*/
static int priority2syslog(int proi)
{
		int table_proi[9] = {
				LOG_CRIT,LOG_INFO,LOG_INFO,LOG_DEBUG,LOG_INFO,LOG_WARNING,LOG_ERR,LOG_EMERG,LOG_ALERT,
		};
		if(proi >= 9)
				return LOG_ERR;
		return table_proi[proi];
}


void __dlog_fatal_assert(int prio)
{
#ifdef FATAL_ON
	assert(!(prio == DLOG_FATAL));
#endif
}

static int dlog_should_log(log_id_t log_id, const char* tag, int prio)
{
	if (log_id < 0 || LOG_ID_MAX <= log_id)
		return DLOG_ERROR_INVALID_PARAMETER;

	return DLOG_ERROR_NONE;
}

int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
	int ret;
	ret = dlog_should_log(log_id, tag, prio);
	if (ret < 0)
		return ret;

	vsyslog(LOG_USER | priority2syslog(prio),fmt,ap);
#ifdef FATAL_ON
	__dlog_fatal_assert(prio);
#endif
	return 0;
}

int __dlog_vprint_assert(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
	int ret;
	ret = dlog_should_log(log_id, tag, prio);

	if (ret < 0)
		return ret;

	vsyslog(LOG_USER | priority2syslog(prio),fmt,ap);

	__dlog_fatal_assert(prio);
	return 0;
}

int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
	int ret;
	va_list ap;
	ret = dlog_should_log(log_id, tag, prio);

	if (ret < 0)
		return ret;

	va_start(ap, fmt);
	vsyslog(LOG_USER | priority2syslog(prio),fmt,ap);
	va_end(ap);

#ifdef FATAL_ON
	__dlog_fatal_assert(prio);
#endif
	return 0;
}

int __dlog_print_assert(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
	int ret;
	va_list ap;

	ret = dlog_should_log(log_id, tag, prio);

	if (ret < 0)
		return ret;
	va_start(ap, fmt);
	vprintf(fmt,ap);
	vsyslog(LOG_USER | priority2syslog(prio),fmt,ap);
	va_end(ap);

	__dlog_fatal_assert(prio);
	return 0;
}

int dlog_vprint(log_priority prio, const char *tag, const char *fmt, va_list ap)
{
	vsyslog(LOG_USER | priority2syslog(prio),fmt,ap);
	return 0;
}

int dlog_print(log_priority prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	//vprintf(fmt,ap);
	vsyslog(LOG_USER | priority2syslog(prio),fmt,ap);
	va_end(ap);

	return 0;
}
