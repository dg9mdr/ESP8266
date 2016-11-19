#ifndef LOGGING_H
#define LOGGING_H
#include <inttypes.h>
#include <stdarg.h>

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

extern "C" {
}

#define LOGLEVEL_QUIET        0
#define LOGLEVEL_CRITICAL     1
#define LOGLEVEL_ERROR        2
#define LOGLEVEL_WARNING      4
#define LOGLEVEL_DEBUG        8
#define LOGLEVEL_INFO        16

#define LOGLEVEL_ALL_BITS    ( LOGLEVEL_QUIET | \
                               LOGLEVEL_CRITICAL | \
                               LOGLEVEL_ERROR | \
                               LOGLEVEL_WARNING | \
                               LOGLEVEL_DEBUG | \
                               LOGLEVEL_INFO )


#define LOGLEVEL_DEFAULT     LOGLEVEL_DEBUG

// return values for the several functions. After changing all function types from void to int this allows a better error handling for the log-functions themselves.

#define SIMPLE_LOG_SUCCESS         0
#define SIMPLE_LOG_STREAM_INVAL   -1
#define SIMPLE_LOG_LOGLEVEL_INVAL -2
#define SIMPLE_LOG_ARG_MISMATCH   -3



class SimpleLog {
private:
    int _level;
    Stream *_strOut;
public:
    SimpleLog() {};
    int Init(int level = LOGLEVEL_DEBUG, Stream *output = NULL);
    int Begin(int level = LOGLEVEL_DEBUG, Stream *output = NULL);
    int SetLevel(int level = LOGLEVEL_DEBUG);
    int Log(int level, char *format, ...);
private:
    int logPrint(const char *format, va_list args);
};

#endif




