//
// Created by ali on 12/20/23.
//

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "onm_logger.h"

// Log file
FILE *logFile = NULL;

// Function to initialize the logger
void initLogger(const char *logFileName) {
    logFile = fopen(logFileName, "a");
    if (logFile == NULL) {
        perror("Error opening log file");
        exit(1);
    }
}

void closeLogger() {
    if (logFile != NULL) {
        fclose(logFile);
    }
}

void logTimestamp() {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    fprintf(logFile, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}


void logMessage(int level, const char *format, va_list args) {
    if (LOG_LEVEL <= level) {
        logTimestamp();

        switch (level) {
            case LEVEL_INFO:
                fprintf(logFile, "[INFO] ");
                break;
            case LEVEL_DEBUG:
                fprintf(logFile, "[DEBUG] ");
                break;
            case LEVEL_WARNING:
                fprintf(logFile, "[WARNING] ");
                break;
            case LEVEL_ERROR:
                fprintf(logFile, "[ERROR] ");
                break;
        }

        vfprintf(logFile, format, args);
        fprintf(logFile, "\n");
    }
}

void LOG_INFO(const char *format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(LEVEL_INFO, format, args);
    va_end(args);
}

void LOG_DEBUG(const char *format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(LEVEL_DEBUG, format, args);
    va_end(args);
}

void LOG_WARNING(const char *format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(LEVEL_WARNING, format, args);
    va_end(args);
}

void LOG_ERROR(const char *format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(LEVEL_ERROR, format, args);
    va_end(args);
}

void onm_logger_close() {
    closeLogger();
}

int onm_logger_init() {
    // Initialize the logger
    initLogger(LOGFILE_NAME);


    return 0;
}