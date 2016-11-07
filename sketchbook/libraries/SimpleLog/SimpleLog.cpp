#include "SimpleLog.h"


void SimpleLog::Init(int level, Stream *output)
{
  SetLevel(level);
  _strOut = output;
}

void SimpleLog::Begin(int level, Stream *output)
{
  SetLevel(level);
  _strOut = output;
}

void SimpleLog::SetLevel(int level)
{
  if( level >= 0 && 
      level <= (LOGLEVEL_CRITICAL |
                LOGLEVEL_ERROR |
                LOGLEVEL_WARNING  |
                LOGLEVEL_DEBUG |
                LOGLEVEL_INFO) )
  {
    _level = level;
  }
}

void SimpleLog::Log(int Level, char* msg, ...)
{
  if( _strOut != NULL )
  {
    if( (_level & Level) == LOGLEVEL_CRITICAL ||

        (_level & Level) == LOGLEVEL_ERROR    ||

        (_level & Level) == LOGLEVEL_WARNING  ||

        (_level & Level) == LOGLEVEL_DEBUG    ||

        (_level & Level) == LOGLEVEL_INFO )
    {
      switch( Level )
      {
        case LOGLEVEL_CRITICAL:
          _strOut->print ("CRITICAL: ");
          break;
        case LOGLEVEL_ERROR:
          _strOut->print ("ERROR: ");
          break;
        case LOGLEVEL_WARNING:
          _strOut->print ("WARNING: ");
          break;
        case LOGLEVEL_DEBUG:
          _strOut->print ("DEBUG: ");
          break;
        case LOGLEVEL_INFO:
          _strOut->print ("INFO: ");
          break;
      }
      va_list args;
      va_start(args, msg);
      logPrint(msg,args);
    }
  }
}

void SimpleLog::logPrint(const char *format, va_list args) 
{
  char *s;
  //
  if( _strOut != NULL )
  {
    for (; *format != 0; ++format)
    {
      if (*format == '%') 
      {
        ++format;

        switch(*format)
        {
          case 's':
            s = (char *)va_arg( args, int );
            _strOut->print(s);
            continue;
          case 'd':
            _strOut->print(va_arg( args, int ),DEC);
            continue;
          case 'x':
          case 'X':
            _strOut->print(va_arg( args, int ),HEX);
            continue;
          case 'b':
          case 'B':
            _strOut->print(va_arg( args, int ),BIN);
            continue;
          case 'f':
            _strOut->print(va_arg( args, double ),2);
            continue;
          case 'l':
            _strOut->print(va_arg( args, long ),DEC);
            continue;
          case 'c':
            _strOut->print(va_arg( args, int ));
            continue;
          default:
            if( *format)
            {
              _strOut->print(*format);
            }
            break;
        }
      }
      else
      {
        if( *format)
        {
          _strOut->print(*format);
        }
      }
    }
  }
}
 
