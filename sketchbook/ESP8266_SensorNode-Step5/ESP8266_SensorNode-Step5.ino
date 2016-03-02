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
// ************************************************************************
// set these defines to match your ESP-module 
// ************************************************************************
// 
#define USE_ESP01
// #undef USE_ESP01
//
// #define USE_ESP07
#undef USE_ESP07
//
// #define USE_ESP12
#undef USE_ESP12
//
// #define USE_ESP12E
#undef USE_ESP12E
//
// which pin is used for factory settings
#define GPIO_AS_RESET           0  // GPIO00 is always possible
// #undef GPIO_AS_RESET              // no reset
//
// ************************************************************************
// set these defines to match software features
// ************************************************************************
// use logging ( otherwise no logging code will be generated )
#define USE_LOGGING
// #undef USE_LOGGING
//
//
// ************************************************************************
// set these defines to match your sensors
// ************************************************************************
//
// --------------------- sensor type DS18B20 -----------------------------
#define USE_DS18B20
// #undef USE_DS18B20
// ------------------ DS18B20 dependend macros ---------------------------
#ifdef USE_DS18B20
#define DS18B20_READINTVAL   5000  // read interval in ms
#define ONE_WIRE_BUS            2  // pin DS18B20 is connected to
                                   // note: be careful if using GPIO0 for that
                                   //       cause of ESP boots to UPLOAD mode
                                   //       if GPIO0 is LOW on (re)start
#endif // USE_DS18B20
// ------------------- sensor type DHT11/DHT22 ---------------------------
// #define USE_DHT
#undef USE_DHT
// ----------------- DHT11/DHT22 dependend macros ------------------------
#ifdef USE_DHT
#define DHTTYPE             DHT22
#define DHTPIN                  2
#define DHT_READINTVAL       3000  // read interval in ms
#define DHT_READ_FAHRENHEIT  true
#define DHT_READ_CELSIUS    false
//
#define DHT_DEGREE_UNIT      DHT_READ_CELSIUS
#endif // USE_DHT
//
// ************************************************************************
// set these defines to match the WEB-API you want to serve
// ************************************************************************
//
// ------------- use SHC to store and visualize data ---------------------
// #define USE_WEBAPI_SHC
#undef USE_WEBAPI_SHC
// --------------------- SHC dependend macros ----------------------------
#ifdef USE_WEBAPI_SHC
//
#endif // USE_WEBAPI_SHC
//
// ----------- use EMONCMS to store and visualize data -------------------
#define USE_WEBAPI_EMONCMS
// #undef USE_WEBAPI_EMONCMS
// ------------------- EMONCMS dependend macros --------------------------
#ifdef USE_WEBAPI_EMONCMS
//
#define EMONCMS_FEEDERID_CUMM          26

#endif // USE_WEBAPI_EMONCMS

//
// ************************************************************************
// set these defines to match your favored ESPs behavior
// ************************************************************************
//  create a wifi client
#define USE_WIFICLIENT
//  create a webserver
#define USE_WWWSERVER
//  port for the webserver
#define WWW_LISTENPORT         80
//  support for SD cards
#define SD_SUPPORT
//
// ************************************************************************
// set pin that is used as CS for sd ard reader
// ************************************************************************
//
// ESP-01 has no SPI bus
#if defined( USE_ESP01 ) 
#undef SD_SUPPORT
#else
#ifdef SD_SUPPORT
##define SERVER_ROOT          "/WWWROOT"
#define CHECKSUM_FILE       "CRCDEF.TXT"
#define SPI_CHIPSEL                    2
#endif // SD_SUPPORT
#endif // USE_ESP01
#define EEPROM_SIZE                 4096
//
// ************************************************************************
// include necessary lib header
// ************************************************************************
//
#ifdef USE_LOGGING
#include "SimpleLog.h"
#endif //
//
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#ifdef USE_WIFICLIENT
// #include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#endif // USE_WIFICLIENT

#include <ArduinoJson.h>
#include <EEPROM.h>

#ifdef SD_SUPPORT
#include <SPI.h>
#include <SD.h>
#endif // SD_SUPPORT

#ifdef USE_DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#endif // USE_DS18B20

#ifdef USE_DHT
#include <DHT.h>
#endif // USE_DHT
//
// ************************************************************************
// define some default values
// ************************************************************************
//
// ------------------------ misc. values ---------------------------------
#define SERIAL_BAUD               115200
//
// -------------------- EEPROM related stuff -----------------------------
#define EEPROM_MAGIC_BYTE           0xEA
#define LEN_TRAILING_LENGTH            2  // means two byte representing 
                                          // the real length of the data field
//
// ------- data to join WLAN --------
//
#define LEN_SSID_MAX                  32  // max. length a SSID may have
#define LEN_PASSWORD_MAX              64  // max. length of a password
//
// ----- data to access EMONCMS -----
//
#define LEN_EMONCMS_READAPI_KEY_MAX   32  // max. length of a emoncms key
#define LEN_EMONCMS_WRITEAPI_KEY_MAX  32  // max. length of a emoncms key
#define LEN_EMONCMS_HOST_MAX          80  // max. length for the emoncms host
#define LEN_EMONCMS_PORT_MAX           2  // max. length for the emoncms port
#define LEN_EMONCMS_URL_MAX           80  // max. length for the emoncms url
#define LEN_EMONCMS_FEED_FMT_MAX      80  // max. length for the formatter
#define LEN_EMONCMS_FEED_URLBUF_MAX  160  // max. length for the feeder url

//
// ----- data to access SHC -----
//
#define LEN_SHC_READAPI_KEY_MAX       32  // max. length of a SHC key
#define LEN_SHC_WRITEAPI_KEY_MAX      32  // max. length of a SHC key
#define LEN_SHC_HOST_MAX              80  // max. length for the SHC host
#define LEN_SHC_PORT_MAX               2  // max. length for the SHC port
#define LEN_SHC_URL_MAX               80  // max. length for the SHC url
#define LEN_SHC_FEED_FMT_MAX          80  // max. length for the formatter
#define LEN_SHC_FEED_URLBUF_MAX      160  // max. length for the feeder url

//
// ----- data for DS18B20 table -----
//
#define LEN_DS18B20_ID_MAX            16
#define LEN_DS18B20_FEEDER_ID_MAX      2
#define LEN_DS18B20_NAME_MAX          30
#define LEN_DS18B20_NUM_ENTRIES        2
#define LEN_DS18B20_VAL_FLAG           1

//
// ----------------------- EEPROM layout ---------------------------------
#define DATA_POS_MAGIC                 0
#define DATA_LEN_MAGIC                 1
#define DATA_POS_CRC32                 1
#define DATA_LEN_CRC32                 4
#define DATA_HDR_END                   5
// data area begins here
//
// ------- data to join WLAN --------
//
#define DATA_POS_SSID                  DATA_HDR_END
#define DATA_LEN_SSID                  (LEN_TRAILING_LENGTH + LEN_SSID_MAX)
#define DATA_POS_PASSWORD              (DATA_POS_SSID + DATA_LEN_SSID)
#define DATA_LEN_PASSWORD              (LEN_TRAILING_LENGTH + LEN_PASSWORD_MAX)
//
// ----- data to access EMONCMS -----
//
#define DATA_POS_EMONCMS_READAPI_KEY   (DATA_POS_PASSWORD + DATA_LEN_PASSWORD)
#define DATA_LEN_EMONCMS_READAPI_KEY   (LEN_TRAILING_LENGTH + LEN_EMONCMS_READAPI_KEY_MAX)
#define DATA_POS_EMONCMS_WRITEAPI_KEY  (DATA_POS_EMONCMS_READAPI_KEY + DATA_LEN_EMONCMS_READAPI_KEY)
#define DATA_LEN_EMONCMS_WRITEAPI_KEY  (LEN_TRAILING_LENGTH + LEN_EMONCMS_WRITEAPI_KEY_MAX)
#define DATA_POS_EMONCMS_HOST          (DATA_POS_EMONCMS_WRITEAPI_KEY + DATA_LEN_EMONCMS_WRITEAPI_KEY)
#define DATA_LEN_EMONCMS_HOST          (LEN_TRAILING_LENGTH + LEN_EMONCMS_HOST_MAX)
#define DATA_POS_EMONCMS_PORT          (DATA_POS_EMONCMS_HOST + DATA_LEN_EMONCMS_HOST)
#define DATA_LEN_EMONCMS_PORT          (LEN_TRAILING_LENGTH + LEN_EMONCMS_PORT_MAX)
#define DATA_POS_EMONCMS_URL           (DATA_POS_EMONCMS_PORT + DATA_LEN_EMONCMS_PORT)
#define DATA_LEN_EMONCMS_URL           (LEN_TRAILING_LENGTH + LEN_EMONCMS_URL_MAX)
#define DATA_POS_EMONCMS_FEED_FMT      (DATA_POS_EMONCMS_URL + DATA_LEN_EMONCMS_URL)
#define DATA_LEN_EMONCMS_FEED_FMT      (LEN_TRAILING_LENGTH + LEN_EMONCMS_FEED_FMT_MAX)
//
// ----- data to access SHC -----
//
#define DATA_POS_SHC_READAPI_KEY   (DATA_POS_PASSWORD + DATA_LEN_PASSWORD)
#define DATA_LEN_SHC_READAPI_KEY   (LEN_TRAILING_LENGTH + LEN_SHC_READAPI_KEY_MAX)
#define DATA_POS_SHC_WRITEAPI_KEY  (DATA_POS_SHC_READAPI_KEY + DATA_LEN_SHC_READAPI_KEY)
#define DATA_LEN_SHC_WRITEAPI_KEY  (LEN_TRAILING_LENGTH + LEN_SHC_WRITEAPI_KEY_MAX)
#define DATA_POS_SHC_HOST          (DATA_POS_SHC_WRITEAPI_KEY + DATA_LEN_SHC_WRITEAPI_KEY)
#define DATA_LEN_SHC_HOST          (LEN_TRAILING_LENGTH + LEN_SHC_HOST_MAX)
#define DATA_POS_SHC_PORT          (DATA_POS_SHC_HOST + DATA_LEN_SHC_HOST)
#define DATA_LEN_SHC_PORT          (LEN_TRAILING_LENGTH + LEN_SHC_PORT_MAX)
#define DATA_POS_SHC_URL           (DATA_POS_SHC_PORT + DATA_LEN_SHC_PORT)
#define DATA_LEN_SHC_URL           (LEN_TRAILING_LENGTH + LEN_SHC_URL_MAX)
#define DATA_POS_SHC_FEED_FMT      (DATA_POS_SHC_URL + DATA_LEN_SHC_URL)
#define DATA_LEN_SHC_FEED_FMT      (LEN_TRAILING_LENGTH + LEN_SHC_FEED_FMT_MAX)
//
// ----- table of DS18B20 sensors -----
//
#define MAX_DS19B20_ENTRIES           10
//
#define DATA_POS_DS18B20_TBL_BEGIN (DATA_POS_SHC_FEED_FMT + DATA_LEN_SHC_FEED_FMT)
//

#define DATA_LEN_NUM_TBL_ENTRIES   LEN_DS18B20_NUM_ENTRIES
#define DATA_POS_DS18B20_FIRST_REC (DATA_POS_DS18B20_TBL_BEGIN + DATA_LEN_NUM_TBL_ENTRIES)
//
#define DATA_LEN_VALIDATION_FLAG   LEN_DS18B20_VAL_FLAG
#define DATA_LEN_SENSOR_ID         (LEN_TRAILING_LENGTH + LEN_DS18B20_ID_MAX)
#define DATA_LEN_SENSOR_FEEDER_ID  (LEN_TRAILING_LENGTH + LEN_DS18B20_FEEDER_ID_MAX)
#define DATA_LEN_SENSOR_NAME       (LEN_TRAILING_LENGTH + LEN_DS18B20_NAME_MAX)
//
#define LEN_DS18B20_SENSOR_RECORD  (DATA_LEN_VALIDATION_FLAG + DATA_LEN_SENSOR_ID + DATA_LEN_SENSOR_FEEDER_ID + DATA_LEN_SENSOR_NAME)
//
#define LEN_DS18B20_TABLE_SIZE     ((LEN_DS18B20_SENSOR_RECORD * MAX_DS19B20_ENTRIES) + DATA_LEN_NUM_TBL_ENTRIES)
//
#define DATA_POS_DS18B20_TBL_END   (DATA_POS_DS18B20_TBL_BEGIN + LEN_DS18B20_TABLE_SIZE)
//
// ---- special marker to make life easier
#define DATA_ENDPOS_EEPROM             DATA_POS_DS18B20_TBL_END

//
//
// ... further EEPROM stuff here like above scheme
//
// ************************************************************************
// set these values to match your network configuration
// ************************************************************************
// change to your SSID 
String ssid = "SSID";
// change to your passphrase
String password = "YOUR-PASSWD";
//
// ************************************************************************
// further global stuff
// ************************************************************************
//
// -------------------------- logging ------------------------------------
//
#ifdef USE_LOGGING
//
SimpleLog Logger;
//
#endif // USE_LOGGING
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
// ---------------------- sd card support --------------------------------
//
#ifdef SD_SUPPORT
bool hasSD;
#endif // SD_SUPPORT
//
// ------------------------- WWWSERVER -----------------------------------
//
#ifdef USE_WWWSERVER
  // create a local web server on port WWW_LISTENPORT
// WiFiServer server(WWW_LISTENPORT);
ESP8266WebServer server(WWW_LISTENPORT);

String pageContent;
int serverStatusCode;

#define SERVER_METHOD_GET               1
#define SERVER_METHOD_POST              2

// amount of input fields in the form
#define ARGS_ADMIN_PAGE                 6

// names of the input fields for emoncms
#define FORM_DATAFIELD_NAME_SSID        "ssid"
#define FORM_DATAFIELD_NAME_PASSWORD    "pass"
#define FORM_DATAFIELD_NAME_EMON_RDKEY  "emonrdkey"
#define FORM_DATAFIELD_NAME_EMON_WRKEY  "emonwrkey"
#define FORM_DATAFIELD_NAME_EMON_HOST   "emonhost"
#define FORM_DATAFIELD_NAME_EMON_PORT   "emonport"
#define FORM_DATAFIELD_NAME_EMON_URL    "emonurl"
#define FORM_DATAFIELD_NAME_EMON_FORMAT "emonfmt"

// names of the input fields for shc
#define FORM_DATAFIELD_NAME_SSID        "ssid"
#define FORM_DATAFIELD_NAME_PASSWORD    "pass"
#define FORM_DATAFIELD_NAME_SHC_RDKEY  "shcrdkey"
#define FORM_DATAFIELD_NAME_SHC_WRKEY  "shcwrkey"
#define FORM_DATAFIELD_NAME_SHC_HOST   "shchost"
#define FORM_DATAFIELD_NAME_SHC_PORT   "shcport"
#define FORM_DATAFIELD_NAME_SHC_URL    "shcurl"
#define FORM_DATAFIELD_NAME_SHC_FORMAT "shcfmt"

#endif // USE_WWWSERVER
//
// -------------------------- DS18B20 ------------------------------------
//
#ifdef USE_DS18B20
//
struct _ser_map {
    int feedId;
    char serial[LEN_DS18B20_FEEDER_ID_MAX];
    char name[LEN_DS18B20_NAME_MAX];
};

//
struct _ser_map serMappingTable[MAX_DS19B20_ENTRIES];
//

int serToFeederId( char* feeder )
{
    int i, found;

    for( i = found = 0; found == 0 && serMappingTable[i].serial != NULL; i++ )
    {
        if( feeder != NULL )
        {
            if(strcmp(feeder, serMappingTable[i].serial) == 0 )
            {
                found = serMappingTable[i].feedId;
            }
        }
    }
    return(found);
}

  // Initialize 1wire bus
OneWire oneWire(ONE_WIRE_BUS);
  // create DS18B20 object on the bus
DallasTemperature DS18B20(&oneWire);

  // temperature - phase 1, one DS18B20 only
  // will go to a structure/array in one of the next steps
float ds18b20Temp;

#endif // USE_DS18B20
//
// ------------------------ DHT11/DHT22 ----------------------------------
//
#ifdef USE_DHT
float dhtHumidity, dhtTemp;    // Values read from sensor

// Initialize DHT sensor 
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01 
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266
 
#endif // USE_DHT
//
// -------------------------- EMONCMS ------------------------------------
//
#ifdef USE_WEBAPI_EMONCMS
//
String emoncmsReadApiKey = "76a82bbb2879d0da909b4082ea3b953b";
String emoncmsWriteApiKey = "2a580c6225c0c3366dfbb17490554bb6";
String emoncmsHost = "192.168.1.121";
String emoncmsPort = "80";
String emoncmsUrl = "/emoncms/feed/";
String emoncmsFeedFmt = "insert.json?apikey=%s&id=%d&value=%s";

#else // not defined USE_WEBAPI_EMONCMS
  // set variables to empty string to avoid errors when storing values to EEPROM
String emoncmsReadApiKey = "";
String emoncmsWriteApiKey = "";
String emoncmsHost = "";
String emoncmsPort = "";
String emoncmsUrl = "";
String emoncmsFeedFmt = "";
//
#endif // USE_WEBAPI_EMONCMS
//
// ---------------------------- SHC --------------------------------------
//
#ifdef USE_WEBAPI_SHC
//
String shcReadApiKey = "76a82bbb2879d0da909b4082ea3b953b";
String shcWriteApiKey = "2a580c6225c0c3366dfbb17490554bb6";
String shcHost = "192.168.1.121";
String shcPort = "80";
String shcUrl = "/emoncms/feed/";
String shcFeedFmt = "insert.json?apikey=%s&id=%d&value=%s";

#else // not defined USE_WEBAPI_SHC
  // set variables to empty string to avoid errors when storing values to EEPROM
String shcReadApiKey = "";
String shcWriteApiKey = "";
String shcHost = "";
String shcPort = "";
String shcUrl = "";
String shcFeedFmt = "";
//
#endif // USE_WEBAPI_SHC
//
// ------------------------ WIFICLIENT -----------------------------------
//
#ifdef USE_WIFICLIENT
// wifi client related stuff goes here
#endif // USE_WIFICLIENT
//
// ---------------------- CRC calculation function -----------------------
//
unsigned long crc_update(unsigned long crc, byte data)
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
unsigned long crc_string(char *s)
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
unsigned long crc_eeprom(int startPos, int length)
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

  Logger.Log(LOGLEVEL_DEBUG,"write LEN byte [%x] to pos %d", len[0], dataIndex);
  
  EEPROM.write(dataIndex, len[0]);

  Logger.Log(LOGLEVEL_DEBUG,"write LEN byte [%x] to pos %d", len[1], dataIndex+1);
  
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

  Logger.Log(LOGLEVEL_DEBUG,"got LEN byte [%x] from pos %d", len[0], dataIndex);

  len[1] = EEPROM.read(dataIndex+1);

  Logger.Log(LOGLEVEL_DEBUG,"got LEN byte [%x] from pos %d", len[1], dataIndex+1);

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
    Logger.Log(LOGLEVEL_DEBUG,"Wrote: %x", data[i]); 
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
  
  Logger.Log(LOGLEVEL_DEBUG,"restore data from eeprom: Address is [%d] - maxlen = %d", dataIndex, maxLen);

  eeRestoreFieldLength( (char*) &len, dataIndex );

  Logger.Log(LOGLEVEL_DEBUG,"len = %d", len);

  if( len > 0 )
  {
    data = "";
    for( int i=0; i < len && i < maxLen; i++ )
    {
      c = EEPROM.read(dataIndex + LEN_TRAILING_LENGTH + i);
      Logger.Log(LOGLEVEL_DEBUG,"%c", c);
      data += c;
    }
  }
  Logger.Log(LOGLEVEL_DEBUG," - done!");
    

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
    Logger.Log(LOGLEVEL_DEBUG,"wrong magic: %x", magicByte);
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
//
// place a "magic" to the first byte in EEPROM
//
bool eeStoreSettings( void )
{
  eeStoreString( ssid, DATA_POS_SSID );
  eeStoreString( password, DATA_POS_PASSWORD );
//
#ifdef USE_WEBAPI_EMONCMS
//
  eeStoreString( emoncmsReadApiKey, DATA_POS_EMONCMS_READAPI_KEY );
  eeStoreString( emoncmsWriteApiKey, DATA_POS_EMONCMS_WRITEAPI_KEY );
  eeStoreString( emoncmsHost, DATA_POS_EMONCMS_HOST );
  eeStoreString( emoncmsPort, DATA_POS_EMONCMS_PORT );
  eeStoreString( emoncmsUrl, DATA_POS_EMONCMS_URL );
  eeStoreString( emoncmsFeedFmt, DATA_POS_EMONCMS_FEED_FMT );
//
#endif // USE_WEBAPI_EMONCMS
//
//
#ifdef USE_WEBAPI_SHC
//
  eeStoreString( shcReadApiKey, DATA_POS_EMONCMS_READAPI_KEY );
  eeStoreString( shcWriteApiKey, DATA_POS_EMONCMS_WRITEAPI_KEY );
  eeStoreString( shcHost, DATA_POS_EMONCMS_HOST );
  eeStoreString( shcPort, DATA_POS_EMONCMS_PORT );
  eeStoreString( shcUrl, DATA_POS_EMONCMS_URL );
  eeStoreString( shcFeedFmt, DATA_POS_EMONCMS_FEED_FMT );
//
#endif // USE_WEBAPI_SHC
//

  return( eeValidate() );
}
//
// calculate position in EEPROM of record with given number
//
int eeRecordOffset( int recno )
{
  int retVal = -1;

  if( recno > 0 && recno < MAX_DS19B20_ENTRIES )
  {
    retVal = DATA_POS_DS18B20_TBL_BEGIN;

    retVal += (LEN_DS18B20_SENSOR_RECORD * recno );
  }
  return( retVal );
}
//
// calculate position in EEPROM of validation flag of a record
//
int eeValidationFlagOffset( int recno )
{
  int retVal = -1;

// DATA_LEN_VALIDATION_FLAG

  return( retVal );
}
//
// calculate position in EEPROM of sensor id of a record
//
int eeSensorIdOffset( int recno )
{
  int retVal = -1;

// DATA_LEN_SENSOR_ID

  return( retVal );
}
//
// calculate position in EEPROM of feeder id of a record
//
int eeFeederIdOffset( int recno )
{
  int retVal = -1;

// DATA_LEN_SENSOR_FEEDER_ID

  return( retVal );
}
//
// calculate position in EEPROM of name of a sensor
//
int eeSensorNameOffset( int recno )
{
  int retVal = -1;

// DATA_LEN_SENSOR_NAME

  return( retVal );
}

//
// ************************************************************************
// RESTART the ESP ... trial an error phase to this feature
// ************************************************************************
//
void espRestart()
{
  ESP.restart();
}
//
// ************************************************************************
// functions for SD card support
// ************************************************************************
//
#ifdef SD_SUPPORT
//
// check for valid sd card
//
bool validateCard( void )
{
  bool retVal = false;
  String pathStr = SERVER_ROOT;

  if( SD.exists(pathStr.c_str()) )
  {
    pathStr += "/";
    pathStr += CHECKSUM_FILE;
    if( SD.exists(pathStr.c_str()) )
    {
      retVal = true;
    }
  }

  return( retVal );
}
#endif // SD_SUPPORT
//
// ************************************************************************
// functions related to EMONCMS ( e.g. get feed url )
// ************************************************************************
//
//
// --- feed2EMONCMS ----------------------------------
//
// Perform an HTTP GET request to EMONCMS
void feed2EMONCMS() 
{

#ifdef USE_WIFICLIENT
  HTTPClient http;
  char urlBuffer[LEN_EMONCMS_FEED_URLBUF_MAX+1];
  char str_temp[6];


// emoncmsHost
// emoncmsPort
// emoncmsUrl
// emoncmsFeedFmt

  Logger.Log(LOGLEVEL_DEBUG,"try to connect to [");
  Logger.Log(LOGLEVEL_DEBUG,"%s", emoncmsHost.c_str());
  Logger.Log(LOGLEVEL_DEBUG,"], port is ");
  Logger.Log(LOGLEVEL_DEBUG,"%d", emoncmsPort.toInt());

#ifdef USE_DHT
  /* 4 is mininum width, 2 is precision; float value is copied onto str_temp*/
  dtostrf(dhtHumidity, 4, 2, str_temp);  
#endif // USE_DHT

  memset(urlBuffer, '\0', sizeof(urlBuffer));
  sprintf(urlBuffer, "%s", emoncmsUrl.c_str());
  sprintf(&urlBuffer[strlen(urlBuffer)], emoncmsFeedFmt.c_str(), emoncmsWriteApiKey.c_str(), 987, str_temp);

  Logger.Log(LOGLEVEL_DEBUG,"feed url is:");
//  Logger.Log(LOGLEVEL_DEBUG,"%s", String(urlBuffer));

  // configure traged server and url
  http.begin(emoncmsHost.c_str(), emoncmsPort.toInt(), String(urlBuffer));

  // start connection and send HTTP header
  int httpCode = http.GET();
  if(httpCode) 
  {
    // HTTP header has been send and Server response header has been handled
//    Logger.Logf("GET... code: %d\n", httpCode);

    // file found at server
    if(httpCode == 200) 
    {
      String payload = http.getString();
//      Logger.Log(LOGLEVEL_DEBUG,payload);
      server.send(200, "text/html", payload); 

      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(payload.c_str());

      bool success       = root["success"];
      const char* infoMsg = root["message"];

      Logger.Log(LOGLEVEL_DEBUG,"result is successfully [");
//      Logger.Log(LOGLEVEL_DEBUG,success);
      Logger.Log(LOGLEVEL_DEBUG,"] ... message from server: ");
      Logger.Log(LOGLEVEL_DEBUG, "%s", infoMsg);

     } 
     else 
     {
       Logger.Log(LOGLEVEL_DEBUG,"GET... failed\n");
       String payload = "<!DOCTYPE HTML>\r\n";
       payload += "<html></p>";
       payload += "<br>call to feed API failed!<br>";
       payload += "</html>";
       server.send(200, "text/html", payload);  
     }
   }
   else
   {
     Logger.Log(LOGLEVEL_DEBUG,"send header... failed\n");
     String payload = "<!DOCTYPE HTML>\r\n";
     payload += "<html></p>";
     payload += "<br>send GET header failed!<br>";
     payload += "</html>";
     server.send(200, "text/html", payload);  
   }

#endif // USE_WIFICLIENT
}


//
// ************************************************************************
// setup module ...
// ************************************************************************
//
void setup() 
{
  String n_ssid = "";
  String n_password = "";

//  logLevel = LOGLEVEL_ERROR;

  // startup serial console ...
  Serial.begin(SERIAL_BAUD);
  delay(10);

  Logger.Init(LOGLEVEL_DEBUG, &Serial);

#ifdef SD_SUPPORT
  Logger.Log(LOGLEVEL_DEBUG, "\nInitializing SD card...");
  if (!SD.begin(SPI_CHIPSEL)) 
  {
    Logger.Log(LOGLEVEL_DEBUG, "initialization failed!\n");
    hasSD = false;
  }
  else
  {
    Logger.Log(LOGLEVEL_DEBUG,LOGLEVEL_DEBUG, "... initialization done.\n");
    Logger.Log(LOGLEVEL_DEBUG, "Validating SD card ...");
    if( (hasSD = validateCard()) )
    {
      Logger.Log(LOGLEVEL_DEBUG,"... SD card is ok.");
      hasSD = true;
    }
    else
    {
      Logger.Log(LOGLEVEL_DEBUG,"... invalid SD card.");
      hasSD = false;
    }
  }
#endif // SD_SUPPORT

  // prepare access to EEPROM 
  EEPROM.begin(EEPROM_SIZE);

  // note: If you want to read ssid and password from EEPROM, just change
  //       n_ssid to ssid and n_password to password.
  //       I didn't do that because I will play around with EEPROM
  //       contents in the next future
  if( eeIsValid() )
  {
    Logger.Log(LOGLEVEL_DEBUG,"eeprom content is valid");
    eeRestoreString( n_ssid, DATA_POS_SSID, LEN_SSID_MAX );
    eeRestoreString( n_password, DATA_POS_PASSWORD, LEN_PASSWORD_MAX );
  }
  else
  {
    Logger.Log(LOGLEVEL_DEBUG,"INVALID eeprom content!");
    eeWipe();
    Logger.Log(LOGLEVEL_DEBUG,"EEPROM cleared!");
  }

  Logger.Log(LOGLEVEL_DEBUG, "ssid from eeprom is %s", n_ssid.c_str() );
  Logger.Log(LOGLEVEL_DEBUG, "password from eeprom is %s", n_password.c_str() );

#ifdef USE_DHT
  dht.begin();           // initialize temperature sensor
#endif // USE_DHT

#ifdef USE_DS18B20  
  DS18B20.begin();
#endif // USE_DS18B20

#ifdef USE_WIFICLIENT
  Logger.Log(LOGLEVEL_DEBUG,"Connecting to %s\n", ssid.c_str() );
   
  // Set mode to station
  WiFi.mode(WIFI_STA);

  // Connect to WiFi network
  WiFi.begin( ssid.c_str(), password.c_str());
   
  // loop until connection is established
  // note that the program stays here if 
  // joining the network fails for any
  // reason
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
//    Logger.Log(LOGLEVEL_DEBUG,".");
  }

  Logger.Log(LOGLEVEL_DEBUG,"");
  Logger.Log(LOGLEVEL_DEBUG,"WiFi connected");
#endif // USE_WIFICLIENT
   
  // that's a quite funny feature of the ESP8266WebServer-class
  // it is possible to install a handler for each "virtual page" you
  // want to serve. Each access to a "location" not handled inline or
  // by a handler will return a 404 (page not found) to the client.
  // You may add an inline handler using the following scheme:
  //  server.on("/", []() {
  //      your code goes here ...
  //      ...
  //  });
  // above handler would called every time / is requested.

  server.on("/", handleIndexPage);
  server.on("/admin", handleAdminPage);
  server.on("/sensors", handleSensorPage);
#ifdef USE_WEBAPI_EMONCMS
  server.on("/emoncms", feed2EMONCMS);
#endif // USE_WEBAPI_EMONCMS
//
#ifdef USE_WEBAPI_SHC
  server.on("/shc", feed2EMONCMS);
#endif // USE_WEBAPI_SHC

#ifdef USE_WWWSERVER
  // start www-server
  server.begin();
  Logger.Log(LOGLEVEL_DEBUG,"Webserver started");
 
// #if defined(USE_WIFICLIENT) || defined(USE_WWWSERVER)

  // Print the IP address
  Logger.Log(LOGLEVEL_DEBUG,"Webserver URL is: ");
  Logger.Log(LOGLEVEL_DEBUG,"http://");
//  Logger.Log(LOGLEVEL_DEBUG,WiFi.localIP());
  Logger.Log(LOGLEVEL_DEBUG,"/");
#endif // USE_WWWSERVER
    
}

//
// ************************************************************************
// page handling ...
// ************************************************************************

void handleSensorPage()
{
  String n_sensorId = "Sensor xx";
  String n_feederId = "2";
  String n_sensorName = "Ich habe einen Namen";

  static int pageArgs = 0;

  if( server.method() == SERVER_METHOD_GET )
  {
    // form was requested initially
    // set input fields to empty strings
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent +=   "<form method='post' action='sensors'>";

    for( int i = 0; i < MAX_DS19B20_ENTRIES; i++ )
    {
      n_sensorId = "Sensor-"+String(i);
//
      pageContent +=     "<label>Id: </label>";
      pageContent +=      "<input name='id_" + n_sensorId + 
                           "' value=" + n_sensorId + " length=" +
                           String(LEN_DS18B20_ID_MAX) + "> (max. " +
                           String(LEN_DS18B20_ID_MAX) + " chars)";
      pageContent +=      "<br>";
//
      pageContent +=     "<label>Feeder-Id: </label>";
      pageContent +=      "<input name='feederId_" + n_sensorId +
                           "' value=" + n_feederId + " length=" +
                           String(LEN_DS18B20_FEEDER_ID_MAX) + "> (max. " +
                           String(LEN_DS18B20_FEEDER_ID_MAX);
      pageContent +=      "<br>";
//
      pageContent +=     "<label>Sensor-Bezeichnung: </label>";
      pageContent +=      "<input name='name_" + n_sensorId +
                           "' value=" + n_sensorName + " length=" +
                           String(LEN_DS18B20_NAME_MAX) + "> (max. " +
                           String(LEN_DS18B20_NAME_MAX) ;
      pageContent +=      "<br>";
//

      pageArgs += 3;

    } // end for i < MAX_DS19B20_ENTRIES

    pageContent +=      "<input type='submit'>";
    pageContent +=   "</form>";
    pageContent += "</html>";
    server.send(200, "text/html", pageContent);  
  }
  else
  {
    if( server.method() == SERVER_METHOD_POST )
    {
      // form contains user input and was postet
      // to server

      // reset page content
      pageContent = "";

      if( server.args() == pageArgs )
      {

          pageContent = "<!DOCTYPE HTML>\r\n";
          pageContent += "<html></p>";
          pageContent += "<br>Settings stored<br>";
          pageContent += "</html>";
          server.send(200, "text/html", pageContent); 

           if(0)
            { 
            }
            else
            {
              pageContent = "<!DOCTYPE HTML>\r\n";
              pageContent += "<html></p>";
              pageContent += "<br>empty strings for ssid/password are not allowed!<br>";
              pageContent += "</html>";
              server.send(200, "text/html", pageContent);  
            }
      }
      else
      {
        pageContent = "<!DOCTYPE HTML>\r\n";
        pageContent += "<html></p>";
        pageContent += "<br>too few arguments<br>";
        pageContent += "</html>";
        server.send(200, "text/html", pageContent);  
      }
    }
  }
}








//
// handle /admin page containing a form with a submit button
//
void handleAdminPage()
{
  
  String n_ssid = "";
  String n_password = "";
  String n_ReadApiKey = "";
  String n_WriteApiKey = "";
  String n_Host = "";
  String n_Port = "";
  String n_Url = "";
  String n_FeedFmt = "";

  Logger.Log(LOGLEVEL_DEBUG,"AdminIndex page");

  Logger.Log(LOGLEVEL_DEBUG, "%s", server.method() );
  Logger.Log(LOGLEVEL_DEBUG, "%s", server.args() );

  if( server.method() == SERVER_METHOD_GET )
  {
    // form was requested initially
    // set input fields to empty strings
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent +=   "<form method='post' action='admin'>";
//
    pageContent +=     "<label>SSID: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_SSID) +
                         "' value=" + ssid + " length=" +
                         String(LEN_SSID_MAX) + "> (max. " +
                         String(LEN_SSID_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>PASSWD: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_PASSWORD) +
                         "' value=" + password + " length=" +
                         String(LEN_PASSWORD_MAX) + "> (max. " +
                         String(LEN_PASSWORD_MAX) + " chars)";
    pageContent +=      "<br>";
//
#ifdef USE_WEBAPI_EMONCMS
//
    pageContent +=     "<label>EMONCMS read API key: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_EMON_RDKEY) +
                         "' value=" + emoncmsReadApiKey + " length=" +
                         String(LEN_EMONCMS_READAPI_KEY_MAX) + "> (max. " +
                         String(LEN_EMONCMS_READAPI_KEY_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>EMONCMS write API key: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_EMON_WRKEY) +
                         "' value=" + emoncmsWriteApiKey + " length=" +
                         String(LEN_EMONCMS_WRITEAPI_KEY_MAX) + "> (max. " +
                         String(LEN_EMONCMS_WRITEAPI_KEY_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>EMONCMS host: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_EMON_HOST) +
                         "' value=" + emoncmsHost + " length=" +
                         String(LEN_EMONCMS_HOST_MAX) + "> (max. " +
                         String(LEN_EMONCMS_HOST_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>EMONCMS port: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_EMON_PORT) +
                         "' value=" + emoncmsPort + " length=" +
                         String(LEN_EMONCMS_PORT_MAX) + "> (max. " +
                         String(LEN_EMONCMS_PORT_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>EMONCMS feeder URL: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_EMON_URL) +
                         "' value=" + emoncmsUrl + " length=" +
                         String(LEN_EMONCMS_URL_MAX) + "> (max. " +
                         String(LEN_EMONCMS_URL_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>EMONCMS feeder format: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_EMON_FORMAT) +
                        "' value=" + emoncmsFeedFmt + " length=" +
                         String(LEN_EMONCMS_FEED_FMT_MAX) + "> (max. " +
                         String(LEN_EMONCMS_FEED_FMT_MAX) + " chars)";
    pageContent +=      "<br>";
//
#endif // USE_WEBAPI_EMONCMS
//
//
#ifdef USE_WEBAPI_SHC
//
    pageContent +=     "<label>SHC read API key: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_SHC_RDKEY) +
                         "' value=" + shcReadApiKey + " length=" +
                         String(LEN_SHC_READAPI_KEY_MAX) + "> (max. " +
                         String(LEN_SHC_READAPI_KEY_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>SHC write API key: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_SHC_WRKEY) +
                         "' value=" + shcWriteApiKey + " length=" +
                         String(LEN_SHC_WRITEAPI_KEY_MAX) + "> (max. " +
                         String(LEN_SHC_WRITEAPI_KEY_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>SHC host: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_SHC_HOST) +
                         "' value=" + shcHost + " length=" +
                         String(LEN_SHC_HOST_MAX) + "> (max. " +
                         String(LEN_SHC_HOST_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>SHC port: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_SHC_PORT) +
                         "' value=" + shcPort + " length=" +
                         String(LEN_SHC_PORT_MAX) + "> (max. " +
                         String(LEN_SHC_PORT_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>SHC feeder URL: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_SHC_URL) +
                         "' value=" + shcUrl + " length=" +
                         String(LEN_SHC_URL_MAX) + "> (max. " +
                         String(LEN_SHC_URL_MAX) + " chars)";
    pageContent +=      "<br>";
//
    pageContent +=     "<label>SHC feeder format: </label>";
    pageContent +=      "<input name='" + String(FORM_DATAFIELD_NAME_SHC_FORMAT) +
                        "' value=" + shcFeedFmt + " length=" +
                         String(LEN_SHC_FEED_FMT_MAX) + "> (max. " +
                         String(LEN_SHC_FEED_FMT_MAX) + " chars)";
    pageContent +=      "<br>";
//
#endif // USE_WEBAPI_SHC
//

    pageContent +=      "<input type='submit'>";
    pageContent +=   "</form>";
    pageContent += "</html>";
    server.send(200, "text/html", pageContent);  
  }
  else
  {
    if( server.method() == SERVER_METHOD_POST )
    {
      // form contains user input and was postet
      // to server

      // reset page content
      pageContent = "";

      if( server.args() == ARGS_ADMIN_PAGE )
      {
        for(int i = 0; i < server.args(); i++ )
        {
//          Logger.Log(LOGLEVEL_DEBUG, "%s", server.argName(i) );
          Logger.Log(LOGLEVEL_DEBUG, " = " );
//          Logger.Log(LOGLEVEL_DEBUG, "%s",server.arg(i) );
        }

        n_ssid = server.arg(FORM_DATAFIELD_NAME_SSID);
        n_password = server.arg(FORM_DATAFIELD_NAME_PASSWORD);
//
#ifdef USE_WEBAPI_EMONCMS
//
        n_ReadApiKey = server.arg(FORM_DATAFIELD_NAME_EMON_RDKEY);
        n_WriteApiKey = server.arg(FORM_DATAFIELD_NAME_EMON_WRKEY);
        n_Host = server.arg(FORM_DATAFIELD_NAME_EMON_HOST);
        n_Port = server.arg(FORM_DATAFIELD_NAME_EMON_PORT);
        n_Url = server.arg(FORM_DATAFIELD_NAME_EMON_URL);
        n_FeedFmt = server.arg(FORM_DATAFIELD_NAME_EMON_FORMAT);
//
#endif // USE_WEBAPI_EMONCMS
//

//
#ifdef USE_WEBAPI_SHC
//
        n_ReadApiKey = server.arg(FORM_DATAFIELD_NAME_SHC_RDKEY);
        n_WriteApiKey = server.arg(FORM_DATAFIELD_NAME_SHC_WRKEY);
        n_Host = server.arg(FORM_DATAFIELD_NAME_SHC_HOST);
        n_Port = server.arg(FORM_DATAFIELD_NAME_SHC_PORT);
        n_Url = server.arg(FORM_DATAFIELD_NAME_SHC_URL);
        n_FeedFmt = server.arg(FORM_DATAFIELD_NAME_SHC_FORMAT);
//
#endif // USE_WEBAPI_SHC
//

        if (n_ssid.length() > 0 && 
            n_password.length() > 0 &&
            n_ReadApiKey.length() > 0 && 
            n_WriteApiKey.length() > 0 &&
            n_Host.length() > 0 &&
            n_Port.length() > 0 &&
            n_Url.length() > 0 &&
            n_FeedFmt.length() > 0 )
        {

          //
          // here we go to store SSID and PASSWD to EEPROM
          //
          eeStoreString( n_ssid, DATA_POS_SSID );
          eeStoreString( n_password, DATA_POS_PASSWORD );
//
#ifdef USE_WEBAPI_EMONCMS
//
        eeStoreString( n_ReadApiKey, DATA_POS_EMONCMS_READAPI_KEY );
        eeStoreString( n_WriteApiKey, DATA_POS_EMONCMS_WRITEAPI_KEY );
        eeStoreString( n_Host, DATA_POS_EMONCMS_HOST );
        eeStoreString( n_Port, DATA_POS_EMONCMS_PORT );
        eeStoreString( n_Url, DATA_POS_EMONCMS_URL );
        eeStoreString( n_FeedFmt, DATA_POS_EMONCMS_FEED_FMT );
//
#endif // USE_WEBAPI_EMONCMS
//

//
#ifdef USE_WEBAPI_SHC
//
        eeStoreString( n_ReadApiKey, DATA_POS_SHC_READAPI_KEY );
        eeStoreString( n_WriteApiKey, DATA_POS_SHC_WRITEAPI_KEY );
        eeStoreString( n_Host, DATA_POS_SHC_HOST );
        eeStoreString( n_Port, DATA_POS_SHC_PORT );
        eeStoreString( n_Url, DATA_POS_SHC_URL );
        eeStoreString( n_FeedFmt, DATA_POS_SHC_FEED_FMT );
//
#endif // USE_WEBAPI_SHC
//

          eeValidate();
       
          pageContent = "<!DOCTYPE HTML>\r\n";
          pageContent += "<html></p>";
          pageContent += "<br>Settings stored<br>";
          pageContent += "</html>";
          server.send(200, "text/html", pageContent);  
        }
        else
        {
          pageContent = "<!DOCTYPE HTML>\r\n";
          pageContent += "<html></p>";
          pageContent += "<br>empty strings for ssid/password are not allowed!<br>";
          pageContent += "</html>";
          server.send(200, "text/html", pageContent);  
        }
      }
      else
      {
        pageContent = "<!DOCTYPE HTML>\r\n";
        pageContent += "<html></p>";
        pageContent += "<br>too few arguments<br>";
        pageContent += "</html>";
        server.send(200, "text/html", pageContent);  
      }
    }
  }
}

//
// handle / page containing the actual temperature
//
void handleIndexPage()
{
  Logger.Log(LOGLEVEL_DEBUG,"Index page");

  // send response to client
  pageContent  = "<!DOCTYPE HTML>\r\n<html>";

#ifdef USE_DS18B20  
  pageContent += "Temperature is now: ";
  pageContent += String(ds18b20Temp);  
  pageContent += " &deg;C";
#endif // USE_DS18B20

#ifdef USE_DHT
  pageContent += "Temperature is now: ";
  pageContent += String(dhtTemp);  
  // what kind of unit? 
  if( DHT_DEGREE_UNIT == DHT_READ_CELSIUS )
  {
    pageContent += " &deg;C<br>";
  }
  else
  {
    pageContent += " &deg;F<br>";
  }
  pageContent += "Humidity is now ..: ";
  pageContent += String(dhtHumidity);  
  pageContent += " %";
#endif // USE_DHT

  pageContent += "<br><br>";
  pageContent += "</html>";

  server.send(200, "text/html", pageContent);
 
}

//
// ************************************************************************
// main loop is quite simple ...
// ************************************************************************
//
void loop() 
{
  static long lastMillis;

#ifdef USE_DS18B20
  // check for next read interval has reached
  if( (millis() - lastMillis) >= DS18B20_READINTVAL )
  {
//    lastMillis = millis();
    DS18B20.requestTemperatures();

    for( int i = 0; i < DS18B20.getDeviceCount(); i++ )
    {
      ds18b20Temp = DS18B20.getTempCByIndex(i);
      // info output to serial console ...
      Logger.Log(LOGLEVEL_DEBUG,"Temperature: %f", ds18b20Temp);
    }
    lastMillis = millis();
  }
#endif // USE_DS18B20

#ifdef USE_DHT
  // check for next read interval has reached
  if( (millis() - lastMillis) >= DHT_READINTVAL )
  {
    lastMillis = millis();
    dhtHumidity = dht.readHumidity();          // Read humidity (percent)
    dhtTemp = dht.readTemperature(DHT_DEGREE_UNIT);     // Read temperature as Fahrenheit
    // info output to serial console ...
    Logger.Log(LOGLEVEL_DEBUG,"Temperature: %f - Humidity: %f", dhtTemp, dhtHumidity);
  }

  // Check if any reads failed and exit early (to try again).
  if (isnan(dhtHumidity) || isnan(dhtTemp)) 
  {
    Logger.Log(LOGLEVEL_DEBUG,"Failed to read from DHT sensor!");
  }
#endif // USE_DHT

 
  server.handleClient();

}

// -------------------------------- nothing behind this line ------------------------------------------

