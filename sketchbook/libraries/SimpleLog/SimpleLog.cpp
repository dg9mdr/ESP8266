//
// ************************************************************************
// SimpleLog
// (C) 2016 Dirk Schanz aka dreamshader
// ************************************************************************
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ************************************************************************
//
//   A simple library to provide printf-like output to any stream object.
//
// ************************************************************************
//
//
//-------- History --------------------------------------------------------
//
// 2016/10/28: initial version 
// 
//
// ************************************************************************
//


#include "SimpleLog.h"

//
// ************************************************************************
// Initialize a SimpleLog instance 
//   - this is necessary for later output the log information
//
// Expected arguments:
// - int level
//   the loglevel of the SimpleLog instance. It's checked against
//   the logtype of each log message
//
// - Stream *output
//   a pointer to a valid stream object that receives the log
//   messages.
//
// Returns an integer as status code:
// - SIMPLE_LOG_SUCCESS on succes, or
// depending of the failure that occurred 
// - SIMPLE_LOG_STREAM_INVALif stream is NULL or
// - SIMPLE_LOG_LOGLEVEL_INVAL if given loglevel is invalid.
//
// ************************************************************************
//
int SimpleLog::Init(int level, Stream *output)
{
  int retVal = SIMPLE_LOG_SUCCESS;

  if( (retVal = SetLevel(level)) == SIMPLE_LOG_SUCCESS )
  {
    if( ( _strOut = output ) == NULL )
    {
      retVal = SIMPLE_LOG_STREAM_INVAL;
    }
  }

  return( retVal );
}

//
// ************************************************************************
// SimpleLog::Begin() does exactly the same as SimpleLog::Init() by
// calling SimpleLog::Init() itself and returns the status of
// the call.
// ************************************************************************
//
int SimpleLog::Begin(int level, Stream *output)
{
  return( Init(level, output) );
}

//
// ************************************************************************
// Sets the loglevel of the SimpleLog instance. 
// - This may be used for dynamically enable/disable logging and/or 
//   increase/reduce the amount of processed log information.
//
// Expected argument:
// - int newLevel
//   the new loglevel of the SimpleLog instance. It's checked against
//   the logtype of each log message
//
// Returns an integer as status code:
// - SIMPLE_LOG_SUCCESS on succes, or
// depending of the failure that occurred 
// - SIMPLE_LOG_LOGLEVEL_INVAL if given loglevel is invalid.
//
// ************************************************************************
//
int SimpleLog::SetLevel(int newLevel)
{
  int retVal = SIMPLE_LOG_SUCCESS;

  if( newLevel >= LOGLEVEL_QUIET && 
      newLevel <= LOGLEVEL_ALL_BITS )
  {
    _level = newLevel;
  }
  else
  {
    retVal = SIMPLE_LOG_LOGLEVEL_INVAL;
  }

  return( retVal );
}

//
// ************************************************************************
// public method to send a log message to the output stream
// - Check whether or not an output has to be done for the given type of
//   log message. In case of yes, send a matching prefix to the log stream.
//
// Expected argument:
// - int loglevel
//   the logtype of the log message
// - char *msg
//   the message to be send containing the format descriptors
// - a variable number of arguments, matching the format descriptors
//
// Returns an integer as status code:
// - SIMPLE_LOG_SUCCESS on succes, or
// depending of the failure that occurred 
// - SIMPLE_LOG_LOGLEVEL_INVAL if given loglevel is invalid.
//
// ************************************************************************
//
int SimpleLog::Log(int logLevel, char* msg, ...)
{
  int retVal = SIMPLE_LOG_SUCCESS;

  if( _strOut != NULL )
  {
    if( logLevel >= LOGLEVEL_QUIET && 
        logLevel <= LOGLEVEL_ALL_BITS )
    {
      switch( _level & logLevel )
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
      if( _level & logLevel )
      {
        va_list args;
        va_start(args, msg);
        retVal = logPrint(msg,args);
      }
    }
    else
    {
      retVal = SIMPLE_LOG_LOGLEVEL_INVAL;
    }
  }
  else
  {
    retVal = SIMPLE_LOG_STREAM_INVAL;
  }

  return( retVal );
}

//
// ************************************************************************
// Private function that writes the log information to the output stream
// ************************************************************************
//
int SimpleLog::logPrint(const char *format, va_list args) 
{
  int retVal = SIMPLE_LOG_SUCCESS;
  char *s;
  //
  if( _strOut != NULL )
  {
    // parse along the format string to get output format descriptors
    for (; *format != 0; ++format)
    {
      // any descriptor starts wit percent sign
      if (*format == '%') 
      {
        // point to next char
        ++format;

        // call .print() method of output string to send data in the
        // in the matching format
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
  else
  {
    retVal = SIMPLE_LOG_STREAM_INVAL;
  }

  return( retVal );
}
 
