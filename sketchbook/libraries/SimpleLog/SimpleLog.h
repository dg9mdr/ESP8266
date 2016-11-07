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

#define LOGLEVEL_DEFAULT     LOGLEVEL_DEBUG

class SimpleLog {
private:
    int _level;
    Stream *_strOut;
public:
    SimpleLog() {};
    void Init(int level = LOGLEVEL_DEBUG, Stream *output = NULL);
    void Begin(int level = LOGLEVEL_DEBUG, Stream *output = NULL);
    void SetLevel(int level = LOGLEVEL_DEBUG);
    void Log(int level, char *format, ...);
private:
    void logPrint(const char *format, va_list args);
};

#endif




