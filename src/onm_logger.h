/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef ONMCLI_ONM_LOGGER_H
#define ONMCLI_ONM_LOGGER_H

#define LOGFILE_NAME "onmcli.log"

#include "config.h"

#define LOG_LEVEL   ONM_LOG_LEVEL

int onm_logger_init();

void LOG_INFO(const char *format, ...);

void LOG_DEBUG(const char *format, ...);

void LOG_WARNING(const char *format, ...);

void LOG_ERROR(const char *format, ...);

#endif //ONMCLI_ONM_LOGGER_H
