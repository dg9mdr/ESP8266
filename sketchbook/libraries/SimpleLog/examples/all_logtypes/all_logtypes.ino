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
// An example how logging is called showing the meaning of
//    logLevel and loglevel types in messages
// ************************************************************************
//

static SimpleLog Logger;

void setup() 
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    // initialize SimpleLog instance
    Logger.Init(LOGLEVEL_DEBUG, &Serial);

    // show a message when setup is done
    Logger.Log(LOGLEVEL_DEBUG,"setup done, logLevel set to LOGLEVEL_DEBUG\n");

}

void sendAll()
{
    Serial.println( "Now send a message of type LOGLEVEL_QUIET" );
    Logger.Log( LOGLEVEL_QUIET, "This message has the type LOGLEVEL_QUIET\n" );

    Serial.println( "Now send a message of type LOGLEVEL_CRITICAL" );
    Logger.Log( LOGLEVEL_CRITICAL, "This message has the type LOGLEVEL_CRITICAL\n" );

    Serial.println( "Now send a message of type LOGLEVEL_ERROR" );
    Logger.Log( LOGLEVEL_ERROR, "This message has the type LOGLEVEL_ERROR\n" );

    Serial.println( "Now send a message of type LOGLEVEL_WARNING" );
    Logger.Log( LOGLEVEL_WARNING, "This message has the type LOGLEVEL_WARNING\n" );

    Serial.println( "Now send a message of type LOGLEVEL_DEBUG" );
    Logger.Log( LOGLEVEL_DEBUG, "This message has the type LOGLEVEL_DEBUG\n" );

    Serial.println( "Now send a message of type LOGLEVEL_INFO" );
    Logger.Log( LOGLEVEL_INFO, "This message has the type LOGLEVEL_INFO\n" );

    // now combine type some messages

    Serial.println( "Now send a message of type LOGLEVEL_ERROR | LOGLEVEL_DEBUG" );
    Logger.Log( LOGLEVEL_ERROR | LOGLEVEL_DEBUG, "This message has the type LOGLEVEL_ERROR | LOGLEVEL_DEBUG\n" );

    Serial.println( "Now send a message of type LOGLEVEL_INFO | LOGLEVEL_CRITICAL" );
    Logger.Log( LOGLEVEL_INFO | LOGLEVEL_CRITICAL, "This message has the type LOGLEVEL_INFO | LOGLEVEL_CRITICAL\n" );


}

void loop() 
{
    Serial.println( "---------- LOOP BEGIN -------------------------------" );
    Logger.SetLevel (LOGLEVEL_QUIET );
    Serial.println( "logLevel is set to LOGLEVEL_QUIET" );
    sendAll();
    Serial.println( "-----------------------------------------------------" );
delay(2000);

    Logger.SetLevel( LOGLEVEL_CRITICAL );
    Serial.println( "logLevel is set to LOGLEVEL_CRITICAL" );
    sendAll();
    Serial.println( "-----------------------------------------------------" );
delay(2000);

    Logger.SetLevel( LOGLEVEL_ERROR );
    Serial.println( "logLevel is set to LOGLEVEL_ERROR" );
    sendAll();
    Serial.println( "-----------------------------------------------------" );
delay(2000);

    Logger.SetLevel( LOGLEVEL_WARNING );
    Serial.println( "logLevel is set to LOGLEVEL_WARNING" );
    sendAll();
    Serial.println( "-----------------------------------------------------" );
delay(2000);

    Logger.SetLevel( LOGLEVEL_DEBUG );
    Serial.println( "logLevel is set to LOGLEVEL_DEBUG" );
    sendAll();
    Serial.println( "-----------------------------------------------------" );
delay(2000);

    Logger.SetLevel( LOGLEVEL_INFO );
    Serial.println( "logLevel is set to LOGLEVEL_INFO" );
    sendAll();
    Serial.println( "---------- LOOP END ---------------------------------" );
    Serial.println();
    Serial.println();
delay(2000);
}

// LOGLEVEL_QUIET
// LOGLEVEL_CRITICAL
// LOGLEVEL_ERROR
// LOGLEVEL_WARNING
// LOGLEVEL_DEBUG
// LOGLEVEL_INFO

