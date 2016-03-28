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

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "ESP8266_EEPROM_Layout.h"
#include "ESP8266_EEPROM_Access.h"


//
// --------------------- CRC lookup table --------------------------------
//
static const PROGMEM uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

//
// ---------------------- CRC calculation function -----------------------
//
static unsigned long crc_update(unsigned long crc, byte data)
{
    byte tbl_idx;
    tbl_idx = crc ^ (data >> (0 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    tbl_idx = crc ^ (data >> (1 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    return crc;
}
//
// --------------------- calc crc over a char array ----------------------
//
static unsigned long crc_string(char *s)
{
  unsigned long crc = ~0L;
  while (*s)
    crc = crc_update(crc, *s++);
  crc = ~crc;
  return crc;
}
//
// ------------ calculate crc of a specific range of eeprom --------------
//
static unsigned long crc_eeprom(int startPos, int length)
{
  unsigned long crc = ~0L;
  int endPos;
  byte currByte;

  if( startPos > 0 && 
      startPos < EEPROM_SIZE && 
      length > 0 && 
      ( endPos = (startPos + length) ) < EEPROM_SIZE )
  {

    for (int index = startPos; index < endPos; ++index) 
    {
      currByte = EEPROM.read(index);
      crc = crc_table[(crc ^ currByte) & 0x0f] ^ (crc >> 4);
      crc = crc_table[(crc ^ (currByte >> 4)) & 0x0f] ^ (crc >> 4);
      crc = ~crc;
    }
  }

  return crc;
}
//
// ************************************************************************
// EEPROM access
// ************************************************************************

unsigned long eeChrArrayCRC(char *s)
{
  return( crc_string(s) );
}


unsigned long eeStrCRC(String s)
{
  return( crc_string( (char*) s.c_str()) );
}

unsigned long eeCRC(int startPos, int length)
{
  return( crc_eeprom(startPos, length) );
}
//
//
// clear EEPROM content (set to zero )
//
void eeWipe( void )
{
  for( int index = 0; index < EEPROM_SIZE; index++ )
  {
    EEPROM.write( index, '\0' );
  }
  EEPROM.commit();
}
//
// store field length to a specific position
//
int eeStoreFieldLength( char* len, int dataIndex )
{
  int retVal = 0;

  EEPROM.write(dataIndex, len[0]);

  EEPROM.write(dataIndex+1, len[1]);

  return(retVal);
}
//
// restore field length from a specific position
//
int eeRestoreFieldLength( char* len, int dataIndex )
{
  int retVal = 0;

  len[0] = EEPROM.read(dataIndex);

  len[1] = EEPROM.read(dataIndex+1);

  return(retVal);
}
//
// store a byte array to a specific position
//
int eeStoreBytes( const char* data, short len, int dataIndex )
{
  int retVal = 0;

  eeStoreFieldLength( (char*) &len, dataIndex );

  for (int i = 0; i < len; ++i)
  {
    EEPROM.write(dataIndex + LEN_TRAILING_LENGTH + i, data[i]);
  }

  return(retVal);
}
//
// store a string var to a specific position
//
int eeStoreString( String data, int dataIndex )
{
  int retVal = 0;

  retVal = eeStoreBytes( data.c_str(), (short) data.length(), dataIndex );

  return(retVal);
}
//
// restore a string var from a specific position
//
int eeRestoreString( String& data, int dataIndex, int maxLen )
{
  int retVal = 0;
  short len = 0;
  char c;
  
  eeRestoreFieldLength( (char*) &len, dataIndex );

  if( len > 0 )
  {
    data = "";
    for( int i=0; i < len && i < maxLen; i++ )
    {
      c = EEPROM.read(dataIndex + LEN_TRAILING_LENGTH + i);
      data += c;
    }
  }

  return(retVal);
}
//
// check whether first byte in EEPROM is "magic"
//
bool eeIsValid()
{
  bool retVal = true;
  char magicByte;

  if( (magicByte = EEPROM.read( DATA_POS_MAGIC )) !=  EEPROM_MAGIC_BYTE )
  {
    retVal = false;
  }

  return(retVal);
}
//
// place a "magic" to the first byte in EEPROM
//
bool eeValidate()
{
  bool retVal = true;
  EEPROM.write( DATA_POS_MAGIC, EEPROM_MAGIC_BYTE );
  EEPROM.commit();

  return(retVal);
}


