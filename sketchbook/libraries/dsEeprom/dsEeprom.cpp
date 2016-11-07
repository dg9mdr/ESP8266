//
// ************************************************************************
// dsEeprom
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
//   A library to provide simplified access to the onchip EEPROM of an
//   Arduino or ESP8266.
//   This includes a checksum over the whole content, that is 
//   automatically stored and checked on EEPROM access, a mgic byte, 
//   that signals a valid content and provides information regarding 
//   the software version with that the EEPROM was written, simplified 
//   store and restore of strings, char array and so on.
//   Please refer to the specific funtion descrition for further
//   information.
//   I'll add additional dataformats now and then. Keep an eye to my
//   github repo ...
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

#include <Arduino.h>
#include <dsEeprom.h>
#include <SimpleLog.h>



// ************************************************************************
// CRC lookup table
// ************************************************************************
//
static const PROGMEM uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

// ************************************************************************
// logger for debug/control output
// ************************************************************************
//
static SimpleLog Logger;

//
// ************************************************************************
// CRC calculation e.g. over EEPROMo for verification
// ************************************************************************
//
unsigned long dsEeprom::crc( void )
{
  unsigned long crc = ~0L;

  for (int index = EEPROM_STD_DATA_BEGIN; index < blockSize; ++index) 
  {
    crc = crc_table[(crc ^ EEPROM.read(index)) & 0x0f] ^ (crc >> 4);
    crc = crc_table[((crc ^ EEPROM.read(index)) >> 4) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}
//
// ************************************************************************
// EEPROM access
// ************************************************************************
//

dsEeprom::dsEeprom( unsigned int blockSize, unsigned char newMagic, int logLevel )
{
  Logger.Init(LOGLEVEL_DEBUG, &Serial);

  status = 0;

  if( logLevel < LOGLEVEL_QUIET || logLevel > LOGLEVEL_INFO )
  {
    logLevel = LOGLEVEL_DEFAULT;
  }

  if( blockSize <= 0 || blockSize > EEPROM_MAX_SIZE )
  {
    status |= EE_STATUS_INVALID_SIZE;
  }
  else
  {
    EEPROM.begin(blockSize);
  }

  if( magic == 0x00 )
  {
    status |= EE_STATUS_INVALID_MAGIC;
  }
  else
  {
    magic = newMagic;
  }

}

dsEeprom::~dsEeprom()
{
}

//
// ************************************************************************
// init
// ************************************************************************
//

int dsEeprom::init( unsigned int blockSize, unsigned char newMagic, int logLevel )
{

  status = EE_STATUS_OK_AND_READY;

  if( logLevel < LOGLEVEL_QUIET || logLevel > LOGLEVEL_INFO )
  {
    logLevel = LOGLEVEL_DEFAULT;
  }

  if( blockSize <= 0 || blockSize > EEPROM_MAX_SIZE )
  {
    status |= EE_STATUS_INVALID_SIZE;
  }
  else
  {
    EEPROM.begin(blockSize);
  }

  if( newMagic == 0x00 )
  {
    status |= EE_STATUS_INVALID_MAGIC;
  }
  else
  {
    magic = newMagic;
  }

  return( status );
}


//
// tell status of dsEeprom-instance
//
short dsEeprom::getStatus( void )
{
  return status;
}

//
// clear EEPROM content (set to zero )
//
void dsEeprom::wipe( void )
{
  if( blockSize > 0 )
  {
    for( int index = 0; index < blockSize; index++ )
    {
      EEPROM.write( index, '\0' );
    }

#ifdef ESP8266
    EEPROM.commit();
#endif // ESP8266

  }
}
//
//
//
// store field length to a specific position
//
int dsEeprom::storeFieldLength( char* len, int dataIndex )
{
  int retVal = 0;


  if( status & EE_STATUS_INVALID_SIZE )
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
    }

  }
  else
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "write LEN byte [%x] to pos %d\n", len[0], dataIndex);
    }
  
    EEPROM.write(dataIndex, len[0]);

    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "write LEN byte [%x] to pos %d\n", len[1], dataIndex+1);
    }
  
    EEPROM.write(dataIndex+1, len[1]);

  }

  return(retVal);
}

//
// restore field length from a specific position
//
int dsEeprom::restoreFieldLength( char* len, int dataIndex )
{
  int retVal = 0;

  if( status & EE_STATUS_INVALID_SIZE )
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
    }
  }
  else
  {
    len[0] = EEPROM.read(dataIndex);

    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "got LEN byte [%x] from pos %d\n", len[0], dataIndex);
    }

    len[1] = EEPROM.read(dataIndex+1);

    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "got LEN byte [%x] from pos %d\n", len[1], dataIndex+1);
    }
  }

  return(retVal);
}

//
// store a boolean value to a specific position
//
int dsEeprom::storeBoolean(  char* data, int dataIndex )
{
    int retVal = 0;
    short len = EEPROM_MAXLEN_BOOLEAN;

    if( status & EE_STATUS_INVALID_SIZE )
    {
        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
        }

    }
    else
    {
        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "store boolean to eeprom: Address is [%d]\n", dataIndex);
        }

        if( (retVal = storeFieldLength( (char*) &len, dataIndex )) == 0 )
        {
            if( !NOLOG )
            {
                Logger.Log(LOGLEVEL_DEBUG, "Wrote:");
            }
    
            for (int i = 0; i < len; ++i)
            {
                EEPROM.write(dataIndex + EEPROM_LEADING_LENGTH + i, data[i]);
                if( !NOLOG )
                {
                    Logger.Log(LOGLEVEL_DEBUG, " %x", data[i]); 
                }
            }
            if( !NOLOG )
            {
                Logger.Log(LOGLEVEL_DEBUG, "\n");
            }
        }
    }

    return(retVal);
}
//
// restore a boolean value from a specific position
//
int dsEeprom::restoreBoolean( char *data, int dataIndex )
{
    int retVal = 0;
    char rdValue;

    if( status & EE_STATUS_INVALID_SIZE )
    {
      if( !NOLOG )
      {
          Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
      }
    }
    else
    {
        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "restore boolean from eeprom: Address is [%d]\n", dataIndex);
        }

        rdValue = EEPROM.read(dataIndex+ EEPROM_LEADING_LENGTH);

        if( rdValue == 0 )
        {
            data[0] = false;
        }
        else
        {
            if( rdValue == 1 )
            {
                data[0] = true;
            }
            else
            {
                retVal = -1;
            }
        }
    }

    return(retVal);
}
//
// store a raw byte buffer without leading length field
// to a specific position
//
int dsEeprom::storeRaw( const char* data, short len, int dataIndex )
{
    int retVal = 0;

    if( status & EE_STATUS_INVALID_SIZE )
    {
      if( !NOLOG )
      {
          Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
      }
    }
    else
    {
        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "store raw data to eeprom: Address is [%d] - len = %d\n",
                dataIndex, len );
        }

        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "Wrote:");
        }

        for (int i = 0; i < len; ++i)
        {
            EEPROM.write(dataIndex, data[i]);

            if( !NOLOG )
            {
                Logger.Log(LOGLEVEL_DEBUG, " %x", data[i]); 
            }

        }
        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "\n");
        }
    }

    return(retVal);

}
//
// restore a byte buffer without leading length field
// from a specific position 
//
int dsEeprom::restoreRaw( String& data, int dataIndex, int len, int maxLen)
{
  int retVal = 0;
  char c;
  
  if( status & EE_STATUS_INVALID_SIZE )
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
    }
  }
  else
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "restore raw data from eeprom: Address is [%d] - maxlen = %d\n",
                dataIndex, maxLen);
    }

    if( len > 0 )
    {
      data = "";
      for( int i=0; i < len && i < maxLen; i++ )
      {
        c = EEPROM.read(dataIndex + i);
        if( !NOLOG )
        {
          Logger.Log(LOGLEVEL_DEBUG, "%c\n", c);
        }
        data += c;
      }
    }

    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, " - done!");
    }
  }

  return(retVal);
}

//
// store a byte array to a specific position
//
int dsEeprom::storeBytes( const char* data, short len, int dataIndex )
{
    int retVal = 0;

    if( status & EE_STATUS_INVALID_SIZE )
    {
      if( !NOLOG )
      {
        Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
      }
    }
    else
    {
        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "store bytes to eeprom: Address is [%d] - len = %d\n",
                dataIndex, len );
        }

        if( (retVal = storeFieldLength( (char*) &len, dataIndex )) == 0 )
        {

            if( !NOLOG )
            {
                Logger.Log(LOGLEVEL_DEBUG, "Wrote:");
            }

            for (int i = 0; i < len; ++i)
            {
                EEPROM.write(dataIndex + EEPROM_LEADING_LENGTH + i, data[i]);

                if( !NOLOG )
                {
                    Logger.Log(LOGLEVEL_DEBUG, " %x", data[i]); 
                }
            }
        }
        if( !NOLOG )
        {
            Logger.Log(LOGLEVEL_DEBUG, "\n");
        }
    }

    return(retVal);
}


//
// restore a string var from a specific position
//
int dsEeprom::restoreBytes( String& data, int dataIndex, int len, int maxLen)
{
  int retVal = 0;
  char c;
  
  if( status & EE_STATUS_INVALID_SIZE )
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
    }
  }
  else
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "restore data from eeprom: Address is [%d] - maxlen = %d\n",
                dataIndex, maxLen);
    }

    if( len > 0 )
    {
      data = "";
      for( int i=0; i < len && i < maxLen; i++ )
      {
        c = EEPROM.read(dataIndex + EEPROM_LEADING_LENGTH + i);
        if( !NOLOG )
        {
          Logger.Log(LOGLEVEL_DEBUG, "%c", c);
        }
        data += c;
      }
    }

    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, " - done!\n");
    }
  }

  return(retVal);
}

//
// store a string var to a specific position
//
int dsEeprom::storeString( String data, int maxLen, int dataIndex )
{
  int retVal = 0;
  short wrLen;

  if( status & EE_STATUS_INVALID_SIZE )
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
    }
  }
  else
  {
    data.trim();

    if( data.length() <= maxLen )
    {
      wrLen = data.length();
    }
    else
    {
      wrLen = maxLen;
    }

    retVal = storeBytes( data.c_str(), wrLen, dataIndex );
  }

  return(retVal);
}
//
// restore a string var from a specific position
//
int dsEeprom::restoreString( String& data, int dataIndex, int maxLen )
{
  int retVal = 0;
  short len = 0;
  
  if( status & EE_STATUS_INVALID_SIZE )
  {
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "eeprom has status EE_STATUS_INVALID_SIZE\n");
    }
  }
  else
  {
    if( (retVal = restoreFieldLength( (char*) &len, dataIndex )) == 0 )
    {
      retVal = restoreBytes( data, dataIndex, len, maxLen );
      if( !NOLOG )
      {
        Logger.Log(LOGLEVEL_DEBUG, " - done!\n");
      }
    }
  }

  return(retVal);
}

//
// check whether first byte in EEPROM is "magic"
//
bool dsEeprom::isValid()
{
  bool retVal = true;
  unsigned char rdMagic;

  if( magic == 0 || (rdMagic = EEPROM.read( EEPROM_POS_MAGIC )) !=  magic )
  {
    retVal = false;
    if( !NOLOG )
    {
      Logger.Log(LOGLEVEL_DEBUG, "wrong magic: %x should be %x\n", rdMagic, magic);
    }
  }
  else
  {
    magic = rdMagic;
  }

  return(retVal);
}

//
// place a "magic" to the first byte in EEPROM
//
bool dsEeprom::validate()
{
  bool retVal = false;

  if( magic != 0 )
  {
    EEPROM.write( EEPROM_POS_MAGIC, magic );
    retVal = true;

#ifdef ESP8266
    EEPROM.commit();
#endif // ESP8266
  }

  return(retVal);
}

void dsEeprom::setBlocksize( unsigned int newSize )
{
  if( newSize <= EEPROM_MAX_SIZE )
  {
    blockSize = newSize;
  }
}

//
//
//
unsigned int dsEeprom::getBlocksize( void )
{
    return( blockSize );
}

//
//
//
void dsEeprom::setMagic( short newMagic )
{
  if( newMagic != 0x00 )
  {
    magic = newMagic;
  }
}

//
//
//
unsigned char dsEeprom::getMagic( void )
{
  return( magic );
}

//
//
//
void dsEeprom::setLoglevel( short newValue )
{
  if( newValue < LOGLEVEL_QUIET || newValue > LOGLEVEL_INFO )
  {
    logLevel = LOGLEVEL_DEFAULT;
  }
  else
  {
    logLevel = newValue;
  }
}

//
//
//
short dsEeprom::getLoglevel( void )
{
  return( logLevel );
}

//
//
//

unsigned char dsEeprom::version2Magic( void )
{
  return( EEPROM_MAGIC_BYTE );
}




