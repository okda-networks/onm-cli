//
// Created by ali on 12/20/23.
//

#ifndef ONMCLI_ONM_LOGGER_H
#define ONMCLI_ONM_LOGGER_H

#define LOGFILE_NAME "onmcli.log"

#define LEVEL_INFO    1
#define LEVEL_ERROR   2
#define LEVEL_WARNING 3
#define LEVEL_DEBUG   4



// Current log level (adjust as needed)
#define LOG_LEVEL   LEVEL_DEBUG

int onm_logger_init();

void LOG_INFO(const char *format, ...);

void LOG_DEBUG(const char *format, ...);

void LOG_WARNING(const char *format, ...);

void LOG_ERROR(const char *format, ...);

#endif //ONMCLI_ONM_LOGGER_H
