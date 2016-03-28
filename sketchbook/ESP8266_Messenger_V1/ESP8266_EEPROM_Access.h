//
// ************************************************************************
// Use an ESP8266 as a remote sensor node.
// (C) 2016 Dirk Schanz aka dreamshader
//
// functional description:
// Goal is to add more functionality, sensors and communication
// protocols step by step.
//
// ************************************************************************
//
// At this point a "thank you very much" to all the authors sharing
// their work, knowledge and all the very useful stuff.
// You did a great job!
//
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
//-------- History -------------------------------------------------------
//
// 1st version: 02/08/16
//         Join an existing WLAN, start a webserver, read temperature
//         from a DS18B20 and show it on a "local webpage" ...
// update:
//
// -----------------------------------------------------------------------
//

#include "ESP8266_Config.h"

#include <EEPROM.h>

unsigned long eeChrArrayCRC(char *s);
unsigned long eeStrCRC(String s);
extern unsigned long eeCRC(int startPos, int length);
extern void eeWipe( void );
extern int eeStoreFieldLength( char* len, int dataIndex );
extern int eeRestoreFieldLength( char* len, int dataIndex );
extern int eeStoreBytes( const char* data, short len, int dataIndex );
extern int eeStoreString( String data, int dataIndex );
extern int eeRestoreString( String& data, int dataIndex, int maxLen );
extern bool eeIsValid( void );
extern bool eeValidate( void );


