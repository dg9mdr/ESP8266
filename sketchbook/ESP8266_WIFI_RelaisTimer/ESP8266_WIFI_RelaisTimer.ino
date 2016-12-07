//
// ************************************************************************
// Use an ESP8266 modul as a WiFi switch
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
//   The module connects to the specified WLAN as a WiFi-client. After
//   that, it requests the current date an time via network time protocol
//   from a specific NTP-server.
//   Last step in starting up is to create a http-server listening to a
//   specified port for requests.
//   The web server handles a configuration page for up to eight channels.
//   Each channel may be switched on and off on up to two user defined 
//   times.
//   In addition for each channel there are two flags to allow switching
//   on or off using the http-API.
//   There are three modes for each channel. ON and OFF override the 
//   user defined times if their check-boxes are active.
//   AUTO activates the user-defined times for each channel and time
//   that is activated ( checked ).
//   These setting information are stored in the EEPROM and restored
//   on every reboot of the module.
//
// ************************************************************************
//
//   Current hardware is a witty esp developer board: 
//   https://blog.the-jedi.co.uk/2016/01/02/wifi-witty-esp12f-board/
//   
//   Select "NodeMCU 1.0 (ESP-12E Module)" as board in the Arduino IDE 
//
//   fyi: witty pins for integrated RGB LED
//     const int RED = 15;
//     const int GREEN = 12;
//     const int BLUE = 13;
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
// ---- suppress debug output to serial line ------------------------------
// set beQuiet = no debug output
//
#define BE_QUIET			false
bool beQuiet;
#define RELAIS_OFF               HIGH
#define RELAIS_ON                LOW
//
//
// ************************************************************************
// include necessary lib header
// ************************************************************************
//
#include <ESP8266WiFi.h>        // WiFi functionality
#include <ESP8266WebServer.h>   // HTTP server
#include <TimeLib.h>            // time keeping feature
#include <WiFiUdp.h>            // udp for network time
#include <Timezone.h>           // for adjust date/time to local timezone
                                // https://github.com/JChristensen/Timezone
#include "PCF8574.h"
#include <Wire.h>


#define __ASSERT_USE_STDERR     // 
#include <assert.h>             // for future use

#include "dsEeprom.h"           // simplified access to onchip EEPROM
#include "SimpleLog.h"          // fprintf()-like logging

//
// ************************************************************************
// defines for default settings
// ************************************************************************
// 
// set DEFAULT_WLAN_SSID and DEFAULT_WLAN_PASSPHRASE to match
// your network
//
#define DEFAULT_UDP_LISTENPORT		8888
#define DEFAULT_WLAN_SSID               ""
#define DEFAULT_WLAN_PASSPHRASE         ""
#define DEAULT_NTP_SERVERNAME		"us.pool.ntp.org"

#define DEFAULT_USE_DHCP                true
#define DEFAULT_HTTP_SERVER_IP          ""
#define DEFAULT_HTTP_SERVER_PORT        "8080"
#define DEFAULT_NODENAME                "esp8266"
#define DEFAULT_ADMIN_PASSWORD          "esp8266"

// ------ define if GPIO00 and GPIO02 are SCL/SDA to a PCF8574
//
#define ESP_HAS_PCF8574
//
// ------ undefine if GPIO00 and GPIO02 directly drive two relais
//        using a transistor amplifier
//
// #undef ESP_HAS_PCF8574


//
// ************************************************************************
// Logging
// ************************************************************************
// 
static SimpleLog Logger;
//
// ************************************************************************
// define timezone and related globals ...
// ************************************************************************
//
// Central European Time (Frankfurt, Paris)
// -- Central European Summer Time
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
// -- Central European Standard Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
Timezone CE(CEST, CET);
TimeChangeRule *tcr;
//
// ************************************************************************
// define UDP related globals ...
// ************************************************************************
//
WiFiUDP Udp;
//
// ************************************************************************
// setup NTP Server to connect to
// ************************************************************************
//
static String ntpServerName;
static String ntpServerPort;
//
// ************************************************************************
// setup local SSID
// ************************************************************************
//
static String wlanSSID;
//
// ************************************************************************
// setup passphrase for local WLAN
// ************************************************************************
//
static String wlanPassphrase;
//
// ************************************************************************
// some additional globals holding settings
// ************************************************************************
//
// get IP using DHCP
bool useDhcp;
// IP of the started HTTP-server
String wwwServerIP;
// Port on that the started HTTP-server is listening
String wwwServerPort;
// Nodename - not currently in use. Part of standard data.
String nodeName;
// Password to get access to admin page. Not currently in use.
String adminPasswd;
//
//
//
// ************************************************************************
//
// ---------- global dsEeprom instance and layout for extended EEPROM data
//
// ************************************************************************
//
dsEeprom eeprom;
//
// ------------------------------------------------------------------------
//
// ---------- layout EEPROM ext. data section
//
// ------------------------------------------------------------------------
//
#if not defined ( EEPROM_EXT_DATA_BEGIN )
#error "Missing end of standard layout. Please check dsEeprom.h"
#endif



#define EEPROM_MAXLEN_NTP_SERVER_NAME    20
#define EEPROM_MAXLEN_NTP_SERVER_PORT     5

#define EEPROM_ADD_SYS_VARS_BEGIN        EEPROM_STD_DATA_END

#define EEPROM_POS_NTP_SERVER_NAME       (EEPROM_ADD_SYS_VARS_BEGIN)
#define EEPROM_POS_NTP_SERVER_PORT       (EEPROM_POS_NTP_SERVER_NAME  + EEPROM_MAXLEN_NTP_SERVER_NAME   + EEPROM_LEADING_LENGTH)

#define EEPROM_ADD_SYS_VARS_END          (EEPROM_POS_NTP_SERVER_PORT  + EEPROM_MAXLEN_NTP_SERVER_PORT + EEPROM_LEADING_LENGTH)
#define EEPROM_EXT_DATA_BEGIN             EEPROM_ADD_SYS_VARS_END


// known: EEPROM_MAXLEN_BOOLEAN see dsEeprom.h

#define MAX_ACTION_TABLE_LINES         8

#define EEPROM_MAXLEN_TBL_ROW_NAME    15
#define EEPROM_MAXLEN_TBL_ROW_MODE     5
#define EEPROM_MAXLEN_TBL_ENABLED      EEPROM_MAXLEN_BOOLEAN
#define EEPROM_MAXLEN_TBL_HR_FROM      2
#define EEPROM_MAXLEN_TBL_MIN_FROM     2
#define EEPROM_MAXLEN_TBL_HR_TO        2
#define EEPROM_MAXLEN_TBL_MIN_TO       2
#define EEPROM_MAXLEN_TBL_EXT_ENABLED  EEPROM_MAXLEN_BOOLEAN


#define EEPROM_ACTION_TBL_ENTRY_START  EEPROM_EXT_DATA_BEGIN
#define EEPROM_POS_TBL_ROW_NAME        EEPROM_EXT_DATA_BEGIN
//
#define EEPROM_POS_TBL_ROW_MODE        (EEPROM_POS_TBL_ROW_NAME     + EEPROM_MAXLEN_TBL_ROW_NAME    + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_TBL_ENABLED1        (EEPROM_POS_TBL_ROW_MODE     + EEPROM_MAXLEN_TBL_ROW_MODE    + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_HR1_FROM        (EEPROM_POS_TBL_ENABLED1     + EEPROM_MAXLEN_TBL_ENABLED     + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_MIN1_FROM       (EEPROM_POS_TBL_HR1_FROM     + EEPROM_MAXLEN_TBL_HR_FROM     + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_HR1_TO          (EEPROM_POS_TBL_MIN1_FROM    + EEPROM_MAXLEN_TBL_MIN_FROM    + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_MIN1_TO         (EEPROM_POS_TBL_HR1_TO       + EEPROM_MAXLEN_TBL_HR_TO       + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_EXT1_ENABLED    (EEPROM_POS_TBL_MIN1_TO      + EEPROM_MAXLEN_TBL_MIN_TO      + EEPROM_LEADING_LENGTH)

#define EEPROM_POS_TBL_ENABLED2        (EEPROM_POS_TBL_EXT1_ENABLED + EEPROM_MAXLEN_TBL_EXT_ENABLED + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_HR2_FROM        (EEPROM_POS_TBL_ENABLED2     + EEPROM_MAXLEN_TBL_ENABLED     + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_MIN2_FROM       (EEPROM_POS_TBL_HR2_FROM     + EEPROM_MAXLEN_TBL_HR_FROM     + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_HR2_TO          (EEPROM_POS_TBL_MIN2_FROM    + EEPROM_MAXLEN_TBL_MIN_FROM    + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_MIN2_TO         (EEPROM_POS_TBL_HR2_TO       + EEPROM_MAXLEN_TBL_HR_TO       + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_TBL_EXT2_ENABLED    (EEPROM_POS_TBL_MIN2_TO      + EEPROM_MAXLEN_TBL_MIN_TO      + EEPROM_LEADING_LENGTH)

#define EEPROM_ACTION_TBL_ENTRY_END    (EEPROM_POS_TBL_EXT2_ENABLED + EEPROM_MAXLEN_TBL_EXT_ENABLED + EEPROM_LEADING_LENGTH)
#define EEPROM_ACTION_TBL_ENTRY_LENGTH (EEPROM_ACTION_TBL_ENTRY_END - EEPROM_ACTION_TBL_ENTRY_START)
#define EEPROM_EXT_DATA_END            (EEPROM_ACTION_TBL_ENTRY_START + (EEPROM_ACTION_TBL_ENTRY_LENGTH * MAX_ACTION_TABLE_LINES))


#if not defined (EEPROM_EXT_DATA_END)
#define EEPROM_EXT_DATA_END      EEPROM_MAX_SIZE
#endif // EEPROM_EXT_DATA_END 

#define ACTION_FLAG_ACTIVE         1
#define ACTION_FLAG_INACTIVE       2
#define ACTION_FLAG_WAITING        4
#define ACTION_FLAG_NO_ACTION      8
#define ACTION_FLAG_STOPPED       16
#define ACTION_FLAG_OVERRIDDEN    32
#define ACTION_FLAG_ALWAYS_ON     64
#define ACTION_FLAG_ALWAYS_OFF   128


struct _action_entry {
  String   name;
  String   mode;
  bool     enabled_1;
  short    actionFlag_1;
  String   hourFrom_1;
  String   minuteFrom_1;
  String   hourTo_1;
  String   minuteTo_1;
  bool     extEnable_1;
  bool     enabled_2;
  short    actionFlag_2;
  String   hourFrom_2;
  String   minuteFrom_2;
  String   hourTo_2;
  String   minuteTo_2;
  bool     extEnable_2;
};

struct _action_entry tblEntry[MAX_ACTION_TABLE_LINES];


char* _form_keywords_[MAX_ACTION_TABLE_LINES][14] = {

  { "bezeichner1", 
    "enabled1_1", 
    "hfrom1_1", 
    "mfrom1_1", 
    "hto1_1", 
    "mto1_1", 
    "enabled2_1", 
    "hfrom2_1", 
    "mfrom2_1", 
    "hto2_1", 
    "mto2_1", 
    "ext1_1", 
    "ext2_1", 
    "mode1" },

  { "bezeichner2", 
    "enabled1_2", 
    "hfrom1_2", 
    "mfrom1_2", 
    "hto1_2", 
    "mto1_2", 
    "enabled2_2", 
    "hfrom2_2", 
    "mfrom2_2", 
    "hto2_2", 
    "mto2_2", 
    "ext1_2", 
    "ext2_2", 
    "mode2" },

  { "bezeichner3", 
    "enabled1_3", 
    "hfrom1_3", 
    "mfrom1_3", 
    "hto1_3", 
    "mto1_3", 
    "enabled2_3", 
    "hfrom2_3", 
    "mfrom2_3", 
    "hto2_3", 
    "mto2_3", 
    "ext1_3", 
    "ext2_3", 
    "mode3" },

  { "bezeichner4", 
    "enabled1_4", 
    "hfrom1_4", 
    "mfrom1_4", 
    "hto1_4", 
    "mto1_4", 
    "enabled2_4", 
    "hfrom2_4", 
    "mfrom2_4", 
    "hto2_4", 
    "mto2_4", 
    "ext1_4", 
    "ext2_4", 
    "mode4" },

  { "bezeichner5", 
    "enabled1_5", 
    "hfrom1_5", 
    "mfrom1_5", 
    "hto1_5", 
    "mto1_5", 
    "enabled2_5", 
    "hfrom2_5", 
    "mfrom2_5", 
    "hto2_5", 
    "mto2_5", 
    "ext1_5", 
    "ext2_5", 
    "mode5" },

  { "bezeichner6", 
    "enabled1_6", 
    "hfrom1_6", 
    "mfrom1_6", 
    "hto1_6", 
    "mto1_6", 
    "enabled2_6", 
    "hfrom2_6", 
    "mfrom2_6", 
    "hto2_6", 
    "mto2_6", 
    "ext1_6", 
    "ext2_6", 
    "mode6" },

  { "bezeichner7", 
    "enabled1_7", 
    "hfrom1_7", 
    "mfrom1_7", 
    "hto1_7", 
    "mto1_7", 
    "enabled2_7", 
    "hfrom2_7", 
    "mfrom2_7", 
    "hto2_7", 
    "mto2_7", 
    "ext1_7", 
    "ext2_7", 
    "mode7" },

  { "bezeichner8", 
    "enabled1_8", 
    "hfrom1_8", 
    "mfrom1_8", 
    "hto1_8", 
    "mto1_8", 
    "enabled2_8", 
    "hfrom2_8", 
    "mfrom2_8", 
    "hto2_8", 
    "mto2_8", 
    "ext1_8", 
    "ext2_8", 
    "mode8" }


};


#define KW_IDX_BEZEICHNER   0
#define KW_IDX_ENABLED_1    1
#define KW_IDX_HFROM_1      2
#define KW_IDX_MFROM_1      3
#define KW_IDX_HTO_1        4
#define KW_IDX_MTO_1        5
#define KW_IDX_ENABLED_2    6
#define KW_IDX_HFROM_2      7
#define KW_IDX_MFROM_2      8
#define KW_IDX_HTO_2        9
#define KW_IDX_MTO_2       10
#define KW_IDX_EXT_1       11
#define KW_IDX_EXT_2       12
#define KW_IDX_MODE        13


#ifdef ESP_HAS_PCF8574
#define CONNECTED_RELAIS    8
#else
#define CONNECTED_RELAIS    2
#endif

// ************************************************************************
// setup local port for UDP communication
// ************************************************************************
//
unsigned int localUDPPort =	DEFAULT_UDP_LISTENPORT;
//
//
// ************************************************************************
// define clock an data pin for iic 
// ************************************************************************
//
static int default_sda_pin = 2;
static int default_scl_pin = 0;
//
// static int default_sda_pin = 4;
// static int default_scl_pin = 5;
//
//
// ************************************************************************
// create an instance of PCF8574
// ************************************************************************
//
PCF8574 PCF_38(0x38);  // add switches to lines  (used as input)
//
//
// ************************************************************************
//
// ---------- none specific code - several helper functions
//
// ************************************************************************
//
// ------------------------------------------------------------------------
//
// ---------- handle diagnostic informations given by assertion and abort execution
//
// ------------------------------------------------------------------------
//
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) 
{
    // transmit diagnostic informations through serial link. 
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}
//
// ------------------------------------------------------------------------
//
// ---------- make a string from MAC address
//
// ------------------------------------------------------------------------
//
String macToID(const uint8_t* mac)
{
    String result;
    for (int i = 0; i < 6; ++i) 
    {
        result += String(mac[i], 16);
    }
    result.toUpperCase();
    return result;
}
//
// ------------------------------------------------------------------------
//
// ---------- generate a unique nodename from MAC address
//
// ------------------------------------------------------------------------
//
void generateNodename()
{
    String id;
    uint8_t mac[6];

    WiFi.macAddress(mac);
    id=macToID(mac);
    nodeName = "ESP_" + id;
}
//
// ------------------------------------------------------------------------
//
// ---------- print several ESP8266 chip information
//
// ------------------------------------------------------------------------
//
void dumpInfo()
{
  Serial.print("WiFi.macAddress .....:");
  Serial.println(WiFi.macAddress());
  Serial.print("WiFi.localIP ........:");
  Serial.println(WiFi.localIP());
  WiFi.printDiag(Serial);

  Serial.print("ESP.getFreeHeap .....:");
  Serial.println(ESP.getFreeHeap());
  Serial.print("ESP.getChipId .......:");
  Serial.println(ESP.getChipId());
  Serial.print("ESP.getFlashChipId ..: ");
  Serial.println(ESP.getFlashChipId());
  Serial.print("ESP.getFlashChipSize : ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("ESP.getFlashChipSpeed: ");
  Serial.println(ESP.getFlashChipSpeed());
  Serial.print("ESP.getCycleCount ...: ");
  Serial.println(ESP.getCycleCount());
  Serial.print("ESP.getVcc ..........: ");
  Serial.println(ESP.getVcc());
}
//
// ------------------------------------------------------------------------
//
// ---------- RESTART the ESP ... trial an error phase to this feature
//
// ------------------------------------------------------------------------
//
void espRestart()
{
  ESP.restart();
}
//
// ************************************************************************
//
// ---------- NTP specific code 
//
// ************************************************************************
//
// ---- definitions for the NTP packet handling ----
//
// NTP time is in the first 48 bytes of message
//
const int NTP_PACKET_SIZE = 48;
//
//
// ---- functions for the NTP packet handling ----
//
// ------------------------------------------------------------------------
//
// time_t getNtpTime()
//
// ------------------------------------------------------------------------
//
time_t getNtpTime()
{
  // NTP server's ip address
  static IPAddress ntpServerIP;

  // buffer to hold incoming & outgoing packets
  static byte packetBuffer[NTP_PACKET_SIZE];

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Logger.Log(LOGLEVEL_DEBUG, "Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName.c_str(), ntpServerIP);

  Logger.Log(LOGLEVEL_DEBUG, "ntpServerName: %s\n", ntpServerName.c_str());

  sendNTPpacket(ntpServerIP, packetBuffer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Logger.Log(LOGLEVEL_DEBUG, "received NTP response\n");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  Logger.Log(LOGLEVEL_DEBUG, "NO NTP response\n");
  return 0; // return 0 if unable to get the time
}
//
// ------------------------------------------------------------------------
//
// void sendNTPpacket(IPAddress &address)
//   send an NTP request to the time server at the given address
//
// ------------------------------------------------------------------------
//
void sendNTPpacket(IPAddress &address, byte packetBuffer[])
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
//
// ************************************************************************
//
// ---------- page preparation and handling 
//
// ************************************************************************
//
// ------------------------------------------------------------------------
//
// ---------- load and handle setup page ----
//
void setupPage();
void doCreateLine( int tmRow );
void doTimeSelect( int tmNum, int tmRow, char* sRow );
void doTimeOnSelect( char *sTarget, char *sSwitch, char *sBgColor, int tmNum, int tmRow, char* sRow, char *sHour, char* sMinute );
// ------------------------------------------------------------------------
//

//
// ------------------------------------------------------------------------
//
// ---------- load and handle API page ----
//
// ------------------------------------------------------------------------
//
//
//
// ************************************************************************
//
// defines and globals related to setup()
//
// ************************************************************************
//
#define SERIAL_BAUD	115200
//
// ************************************************************************
//
// ---------- web server related globals and defines
//
// ************************************************************************
//
ESP8266WebServer server; // DEFAULT_HTTP_LISTENPORT
String pageContent;
int serverStatusCode;
//
#define SERVER_METHOD_GET       1
#define SERVER_METHOD_POST      2
#define ARGS_ADMIN_PAGE         2
//
// ************************************************************************
//
// ---------- misc. helper functions
//
// ************************************************************************
//
//
// ------------------------------------------------------------------------
//
// ---------- set properties to default values
//
// ------------------------------------------------------------------------
//
void resetAdminSettings2Default( void )
{
    ntpServerName  = DEAULT_NTP_SERVERNAME;
    wlanSSID       = DEFAULT_WLAN_SSID;
    wlanPassphrase = DEFAULT_WLAN_PASSPHRASE;
    useDhcp        = DEFAULT_USE_DHCP;
    wwwServerIP    = DEFAULT_HTTP_SERVER_IP;
    wwwServerPort  = DEFAULT_HTTP_SERVER_PORT;
    nodeName       = DEFAULT_NODENAME;
    adminPasswd    = DEFAULT_ADMIN_PASSWORD;
}

//
// ------------------------------------------------------------------------
//
// ---------- reset action flags
//
// ------------------------------------------------------------------------
//
void resetActionFlags( void )
{
    int currLine;

//    for( currLine = 0; currLine < MAX_ACTION_TABLE_LINES; currLine++ )
    for( currLine = 0; currLine < CONNECTED_RELAIS; currLine++ )
    {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "reset action flag for port %d\n", currLine );
        }

        tblEntry[currLine].actionFlag_1 = ACTION_FLAG_INACTIVE | ACTION_FLAG_NO_ACTION;
        tblEntry[currLine].actionFlag_2 = ACTION_FLAG_INACTIVE | ACTION_FLAG_NO_ACTION;
    }
	
}


//
// ------------------------------------------------------------------------
//
// ---------- store sytem and environmental settings to EEPROM
//
// ------------------------------------------------------------------------
//
int storeAdminSettings()
{
    int retVal = 0;
    unsigned long crcCalc;

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing SSID: %s to [%d]\n", wlanSSID.c_str(), EEPROM_POS_WLAN_SSID );
    }

    eeprom.storeString(  wlanSSID,      EEPROM_MAXLEN_WLAN_SSID,       EEPROM_POS_WLAN_SSID );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing Passphrase: %s to [%d]\n", wlanPassphrase.c_str(), EEPROM_POS_WLAN_PASSPHRASE );
    }

    eeprom.storeString(  wlanPassphrase,    EEPROM_MAXLEN_WLAN_PASSPHRASE, EEPROM_POS_WLAN_PASSPHRASE );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing ServerIP: %s to [%d]\n", wwwServerIP.c_str(), EEPROM_POS_SERVER_IP );
    }

    eeprom.storeString(  wwwServerIP,   EEPROM_MAXLEN_SERVER_IP,       EEPROM_POS_SERVER_IP );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing ServerPort: %s to [%d]\n", wwwServerPort.c_str(),EEPROM_POS_SERVER_PORT );
    }

    eeprom.storeString(  wwwServerPort, EEPROM_MAXLEN_SERVER_PORT,     EEPROM_POS_SERVER_PORT );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing Nodename: %s to [%d]\n", nodeName.c_str(), EEPROM_POS_NODENAME );
    }

    eeprom.storeString(  nodeName,      EEPROM_MAXLEN_NODENAME,        EEPROM_POS_NODENAME );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing adminPasswd: %s to [%d]\n", adminPasswd.c_str() , EEPROM_POS_ADMIN_PASSWORD);
    }

    eeprom.storeString(  adminPasswd,   EEPROM_MAXLEN_ADMIN_PASSWORD,  EEPROM_POS_ADMIN_PASSWORD );

    eeprom.validate();
    
    return( retVal );

}

//
// ------------------------------------------------------------------------
//
// ---------- restore sytem and environmentsl settings to EEPROM
//
// ------------------------------------------------------------------------
//
int restoreAdminSettings()
{
    int retVal = 0;
    unsigned long crcCalc;
    unsigned long crcRead;

    if( eeprom.isValid() )
    {
        crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );

        eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "restored crc: %x calc crc: %x\n", crcRead, crcCalc);
        }
        

        if( (crcCalc == crcRead) )
        {
            eeprom.restoreString(  wlanSSID,    EEPROM_POS_WLAN_SSID,       EEPROM_MAXLEN_WLAN_SSID );
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored SSID: %s\n", wlanSSID.c_str());
            }

            eeprom.restoreString(  wlanPassphrase,  EEPROM_POS_WLAN_PASSPHRASE, EEPROM_MAXLEN_WLAN_PASSPHRASE );
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored Passphrase: %s\n", wlanPassphrase.c_str());
            }

            eeprom.restoreString(  wwwServerIP, EEPROM_POS_SERVER_IP,       EEPROM_MAXLEN_SERVER_IP );
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored ServerIP: %s\n", wwwServerIP.c_str());
            }

            eeprom.restoreString(  wwwServerPort, EEPROM_POS_SERVER_PORT,     EEPROM_MAXLEN_SERVER_PORT );
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored ServerPort: %s\n", wwwServerPort.c_str());
            }

            eeprom.restoreString(  nodeName,    EEPROM_POS_NODENAME,        EEPROM_MAXLEN_NODENAME );
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored Nodename: %s\n", nodeName.c_str());
            }

            eeprom.restoreString(  adminPasswd, EEPROM_POS_ADMIN_PASSWORD,  EEPROM_MAXLEN_ADMIN_PASSWORD );
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored adminPasswd: %s\n", adminPasswd.c_str());
            }

            retVal = E_SUCCESS;
        }
        else
        {
            retVal = E_BAD_CRC;
        }
    }
    else
    {
        retVal = E_INVALID_MAGIC;
    }

    return( retVal );

}

//
// ------------------------------------------------------------------------
//
// ---------- store additional sytem and environmental settings to EEPROM
//
// ------------------------------------------------------------------------
//
int storeAddSysvars()
{
    int retVal = 0;

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing NTP-Server: %s to [%d]\n", ntpServerName.c_str(), EEPROM_POS_NTP_SERVER_NAME );
    }

    eeprom.storeString(  ntpServerName,      EEPROM_MAXLEN_NTP_SERVER_NAME,       EEPROM_POS_NTP_SERVER_NAME );
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing NTP-Server port: %s to [%d]\n", ntpServerPort.c_str(), EEPROM_POS_NTP_SERVER_PORT );
    }

    eeprom.storeString(  ntpServerPort,      EEPROM_MAXLEN_NTP_SERVER_PORT,       EEPROM_POS_NTP_SERVER_PORT );
 
    return( retVal );
}
   
//
// ------------------------------------------------------------------------
//
// ---------- restore additional sysvars to EEPROM
//
// ------------------------------------------------------------------------
//
int restoreAddSysvars()
{
    int retVal = 0;

    eeprom.restoreString(  ntpServerName,    EEPROM_POS_NTP_SERVER_NAME,       EEPROM_MAXLEN_NTP_SERVER_NAME );
    if( !beQuiet )
    {
            Logger.Log(LOGLEVEL_DEBUG, "restored NTP-Server: %s\n", ntpServerName.c_str());
    }

    eeprom.restoreString(  ntpServerPort,    EEPROM_POS_NTP_SERVER_PORT,       EEPROM_MAXLEN_NTP_SERVER_PORT );
    if( !beQuiet )
    {
            Logger.Log(LOGLEVEL_DEBUG, "restored NTP-Port: %s\n", ntpServerPort.c_str());
    }


    return( retVal );
}
//
// ------------------------------------------------------------------------
//
// ---------- store the table with action information to EEPROM
//
// ------------------------------------------------------------------------
//
int storeActionTable()
{
    int retVal = 0;
    unsigned long crcCalc;
    int currLine;


    for( currLine = 0; currLine < MAX_ACTION_TABLE_LINES; currLine++ )
    {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "name ........: %s[%d]\n", tblEntry[currLine].name.c_str(),
                        EEPROM_POS_TBL_ROW_NAME + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].name, EEPROM_MAXLEN_TBL_ROW_NAME, 
                           EEPROM_POS_TBL_ROW_NAME +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "mode ........: %s[%d]\n", tblEntry[currLine].mode.c_str(),
                        EEPROM_POS_TBL_ROW_MODE + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].mode, EEPROM_MAXLEN_TBL_ROW_MODE, 
                           EEPROM_POS_TBL_ROW_MODE+
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "enabled_1 ...: %d[%d]\n", tblEntry[currLine].enabled_1,
                        EEPROM_POS_TBL_ENABLED1 + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeBoolean((char*)&tblEntry[currLine].enabled_1, EEPROM_POS_TBL_ENABLED1 +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "hourFrom_1 ..: %s[%d]\n", tblEntry[currLine].hourFrom_1.c_str(),
                        EEPROM_POS_TBL_HR1_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].hourFrom_1, EEPROM_MAXLEN_TBL_HR_FROM, 
                           EEPROM_POS_TBL_HR1_FROM +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "minuteFrom_1 : %s[%d]\n", tblEntry[currLine].minuteFrom_1.c_str(),
                        EEPROM_POS_TBL_MIN1_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].minuteFrom_1, EEPROM_MAXLEN_TBL_MIN_FROM, 
                           EEPROM_POS_TBL_MIN1_FROM +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "hourTo_1 ....: %s[%d]\n", tblEntry[currLine].hourTo_1.c_str(),
                        EEPROM_POS_TBL_HR1_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].hourTo_1, EEPROM_MAXLEN_TBL_HR_TO, 
                           EEPROM_POS_TBL_HR1_TO +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "minuteTo_1 ..: %s[%d]\n", tblEntry[currLine].minuteTo_1.c_str(),
                        EEPROM_POS_TBL_MIN1_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].minuteTo_1, EEPROM_MAXLEN_TBL_MIN_TO, 
                           EEPROM_POS_TBL_MIN1_TO +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "extEnable_1 .: %d[%d]\n", tblEntry[currLine].extEnable_1,
                        EEPROM_POS_TBL_EXT1_ENABLED + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeBoolean((char*)&tblEntry[currLine].extEnable_1, EEPROM_POS_TBL_EXT1_ENABLED +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "enabled_2 ...: %d[%d]\n", tblEntry[currLine].enabled_2,
                        EEPROM_POS_TBL_ENABLED2 + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeBoolean((char*)&tblEntry[currLine].enabled_2, EEPROM_POS_TBL_ENABLED2 +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "hourFrom_2 ..: %s[%d]\n", tblEntry[currLine].hourFrom_2.c_str(),
                        EEPROM_POS_TBL_HR2_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].hourFrom_2, EEPROM_MAXLEN_TBL_HR_FROM, 
                           EEPROM_POS_TBL_HR2_FROM +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "minuteFrom_2 : %s[%d]\n", tblEntry[currLine].minuteFrom_2.c_str(),
                        EEPROM_POS_TBL_MIN2_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].minuteFrom_2, EEPROM_MAXLEN_TBL_MIN_FROM, 
                           EEPROM_POS_TBL_MIN2_FROM +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "hourTo_2 ....: %s[%d]\n", tblEntry[currLine].hourTo_2.c_str(),
                        EEPROM_POS_TBL_HR2_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].hourTo_2, EEPROM_MAXLEN_TBL_HR_TO, 
                           EEPROM_POS_TBL_HR2_TO +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "minuteTo_2 ..: %s[%d]\n", tblEntry[currLine].minuteTo_2.c_str(),
                        EEPROM_POS_TBL_MIN2_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        }
        eeprom.storeString(tblEntry[currLine].minuteTo_2, EEPROM_MAXLEN_TBL_MIN_TO, 
                           EEPROM_POS_TBL_MIN2_TO +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "extEnable_2 .: %d[%d]\n", tblEntry[currLine].extEnable_2,
                        EEPROM_POS_TBL_EXT2_ENABLED + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

        }
        eeprom.storeBoolean((char*)&tblEntry[currLine].extEnable_2, EEPROM_POS_TBL_EXT2_ENABLED +
                           (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
    }

    eeprom.validate();

    return( retVal );

}

//
// ------------------------------------------------------------------------
//
// ---------- restore table with action information from EEPROM
//
// ------------------------------------------------------------------------
//
int restoreActionTable()
{
    int retVal = 0;
    unsigned long crcCalc;
    unsigned long crcRead;
    int currLine;

    if( eeprom.isValid() )
    {
        crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );

        eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "restored crc: %x calc crc: %x\n", crcRead, crcCalc);
        }

        if( (crcCalc == crcRead) )
        {

            for( currLine = 0; currLine < MAX_ACTION_TABLE_LINES; currLine++ )
            {
                eeprom.restoreString(tblEntry[currLine].name, 
                                     EEPROM_POS_TBL_ROW_NAME + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_ROW_NAME );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "name ........: %s\n", tblEntry[currLine].name.c_str() );
                }
//
                eeprom.restoreString(tblEntry[currLine].mode,
                                     EEPROM_POS_TBL_ROW_MODE + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_ROW_MODE );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "mode ........: %s\n", tblEntry[currLine].mode.c_str() );
                }
//
                eeprom.restoreBoolean((char*)&tblEntry[currLine].enabled_1, EEPROM_POS_TBL_ENABLED1 +
                                   (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "enabled_1 ...: %d\n", tblEntry[currLine].enabled_1 );
                }
//
                eeprom.restoreString(tblEntry[currLine].hourFrom_1, 
                                     EEPROM_POS_TBL_HR1_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_HR_FROM );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "hourFrom_1 ..: %s\n", tblEntry[currLine].hourFrom_1.c_str() );
                }
//
                eeprom.restoreString(tblEntry[currLine].minuteFrom_1, 
                                     EEPROM_POS_TBL_MIN1_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_MIN_FROM );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "minuteFrom_1 : %s\n", tblEntry[currLine].minuteFrom_1.c_str() );
                }
//
                eeprom.restoreString(tblEntry[currLine].hourTo_1, 
                                     EEPROM_POS_TBL_HR1_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_HR_TO );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "hourTo_1 ....: %s\n", tblEntry[currLine].hourTo_1.c_str() );
                }
//
                eeprom.restoreString(tblEntry[currLine].minuteTo_1, 
                                     EEPROM_POS_TBL_MIN1_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_MIN_TO );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "minuteTo_1 ..: %s\n", tblEntry[currLine].minuteTo_1.c_str() );
                }
//
                eeprom.restoreBoolean((char*)&tblEntry[currLine].extEnable_1, EEPROM_POS_TBL_EXT1_ENABLED +
                                   (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "extEnable_1 .: %d\n", tblEntry[currLine].extEnable_1 );
                }
//
                eeprom.restoreBoolean((char*)&tblEntry[currLine].enabled_2, EEPROM_POS_TBL_ENABLED2 +
                                     (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "enabled_2 ...: %d\n", tblEntry[currLine].enabled_2 );
                }
//
                eeprom.restoreString(tblEntry[currLine].hourFrom_2, 
                                     EEPROM_POS_TBL_HR2_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_HR_FROM );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "hourFrom_2 ..: %s\n", tblEntry[currLine].hourFrom_2.c_str() );
                }
//
                eeprom.restoreString(tblEntry[currLine].minuteFrom_2, 
                                     EEPROM_POS_TBL_MIN2_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_MIN_FROM );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "minuteFrom_2 : %s\n", tblEntry[currLine].minuteFrom_2.c_str() );
                }
//
                eeprom.restoreString(tblEntry[currLine].hourTo_2, 
                                     EEPROM_POS_TBL_HR2_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_HR_TO );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "hourTo_2 ....: %s\n", tblEntry[currLine].hourTo_2.c_str() );
                }
//
                eeprom.restoreString(tblEntry[currLine].minuteTo_2, 
                                     EEPROM_POS_TBL_MIN2_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH), 
                                     EEPROM_MAXLEN_TBL_MIN_TO );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "minuteTo_2 ..: %s\n", tblEntry[currLine].minuteTo_2.c_str() );
                }
//
                eeprom.restoreBoolean((char*)&tblEntry[currLine].extEnable_2, EEPROM_POS_TBL_EXT2_ENABLED +
                                   (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, "extEnable_2 .: %d\n", tblEntry[currLine].extEnable_2 );
                }
            }
        }
        else
        {
            retVal = E_BAD_CRC;
        }
    }
    else
    {
        retVal = E_INVALID_MAGIC;
    }

    return( retVal );

}





//
// ************************************************************************
//
// ---------- toggleRelais()
//
// ************************************************************************
//
void toggleRelais(int port)
{

#ifdef ESP_HAS_PCF8574
    PCF_38.toggle(port);
#else
    static int lastState;

    digitalWrite( port, ~lastState );
    lastState = ~lastState;

#endif // ESP_HAS_PCF8574

}


//
// ************************************************************************
//
// ---------- switchRelais()
//
// ************************************************************************
//
void switchRelais(int port, int newStatus)
{

#ifdef ESP_HAS_PCF8574
    PCF_38.write(port, newStatus);
#else
    digitalWrite( port, newStatus );
#endif // ESP_HAS_PCF8574

}

//
// ************************************************************************
//
// ---------- startupActions()
//
// ************************************************************************
//
void startupActions( void )
{
  int i;
  time_t secsSinceEpoch;
  short nowMinutes;
  short chkMinutesFrom, chkMinutesTo;
  short wrapMinutes;
  time_t utc;

  utc=now();
  secsSinceEpoch = CE.toLocal(utc, &tcr);
  nowMinutes = ( hour(secsSinceEpoch) * 60 ) + minute(secsSinceEpoch);

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "nowMinutes is %d\n", nowMinutes );
  }

  for( i = 0; i < CONNECTED_RELAIS; i++ )
  {
    tblEntry[i].actionFlag_1 = (ACTION_FLAG_INACTIVE | ACTION_FLAG_NO_ACTION);
    tblEntry[i].actionFlag_2 = (ACTION_FLAG_INACTIVE | ACTION_FLAG_NO_ACTION);
 
    if( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, "setting port %d to LOW!\n", i );
    }
   
    switchRelais(i, RELAIS_OFF);

    if( tblEntry[i].enabled_1 )
    {

      if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {

        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_1.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_1.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_1.c_str()) * 60) + atoi(tblEntry[i].minuteTo_1.c_str());
 
        if( chkMinutesTo < chkMinutesFrom )
        {
          // time wrap
          wrapMinutes = (23 * 60) + 59;
          if( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, "wrap minutes time 1: %d\n", wrapMinutes);
          }

          if( nowMinutes < chkMinutesTo || nowMinutes < wrapMinutes )
          {
//
            if( nowMinutes > chkMinutesFrom )
            {
              // set this entry to active
              if( !beQuiet )
              {
                Logger.Log(LOGLEVEL_DEBUG, 
                  "Current time(%d) is after Begin(%d) but before End(%d) of time 2 ... starting action\n",
                  nowMinutes, chkMinutesFrom, chkMinutesTo );
                Logger.Log(LOGLEVEL_DEBUG, "switch port %d back to HIGH!\n", i );
              }

              tblEntry[i].actionFlag_1 |= ACTION_FLAG_ACTIVE;
              switchRelais(i, RELAIS_ON);
            }
          }
        }
        else
        {
          if( nowMinutes >= chkMinutesFrom && nowMinutes < chkMinutesTo )
          {
            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, 
                "Current time(%d) is after Begin(%d) but before End(%d) of time 1 ... starting action\n",
                nowMinutes, chkMinutesFrom, chkMinutesTo );
              Logger.Log(LOGLEVEL_DEBUG, "switch port %d back to HIGH!\n", i );
            }

            tblEntry[i].actionFlag_1 |= ACTION_FLAG_ACTIVE;
            switchRelais(i, RELAIS_ON);
          }
        }
      }
      else // if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
        if( tblEntry[i].mode.equalsIgnoreCase("on") )
        {
          alwaysOn(1, i);
        }
        else
        {
          alwaysOff(1, i);
        }
      }
    }

    if( tblEntry[i].enabled_2 )
    {

      if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {

        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_2.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_2.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_2.c_str()) * 60) + atoi(tblEntry[i].minuteTo_2.c_str());
 
        if( chkMinutesTo < chkMinutesFrom )
        {
          // time wrap
          wrapMinutes = (23 * 60) + 59;
          if( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, "wrap minutes time 2: %d\n", wrapMinutes);
          }

          if( nowMinutes < chkMinutesTo || nowMinutes < wrapMinutes )
          {
//
            if( nowMinutes > chkMinutesFrom )
            {
              // set this entry to active
              if( !beQuiet )
              {
                Logger.Log(LOGLEVEL_DEBUG, 
                  "Current time(%d) is after Begin(%d) but before End(%d) of time 2 ... starting action\n",
                  nowMinutes, chkMinutesFrom, chkMinutesTo );
                Logger.Log(LOGLEVEL_DEBUG, "switch port %d back to HIGH!\n", i );
              }

              tblEntry[i].actionFlag_2 |= ACTION_FLAG_ACTIVE;
              switchRelais(i, RELAIS_ON);
            }
          }
        }
        else
        {
          if( nowMinutes >= chkMinutesFrom && nowMinutes < chkMinutesTo )
          {
            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, 
                "Current time(%d) is after Begin(%d) but before End(%d) of time 2 ... starting action\n",
                nowMinutes, chkMinutesFrom, chkMinutesTo );
 
              Logger.Log(LOGLEVEL_DEBUG, "switch port %d back to HIGH!\n", i );
            }

            tblEntry[i].actionFlag_2 |= ACTION_FLAG_ACTIVE;
            switchRelais(i, RELAIS_ON);
          }
        }
      }
      else // if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
        if( tblEntry[i].mode.equalsIgnoreCase("on") )
        {
          alwaysOn(2, i);
        }
        else
        {
          if( tblEntry[i].mode.equalsIgnoreCase("off") )
          {
            alwaysOff(2, i);
          }
        }
      }
    }

  }
}


//
// ************************************************************************
//
// ---------- setup()
//
// ************************************************************************
//
void setup()
{

  unsigned long crcCalc;
  unsigned long crcRead;

  // witty pins for integrated RGB LED
  const int RED = 15;
  const int GREEN = 12;
  const int BLUE = 13;

  IPAddress localIP;
  beQuiet = BE_QUIET;
  
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  analogWrite(RED, 0 );
  analogWrite(GREEN, 127);
  analogWrite(BLUE, 0);
    
  Serial.begin(SERIAL_BAUD);
  delay(10);

  Logger.Init(LOGLEVEL_DEBUG, &Serial);

Logger.Log(LOGLEVEL_DEBUG,"EEPROM_ACTION_TBL_ENTRY_START = %d\n", EEPROM_ACTION_TBL_ENTRY_START );
Logger.Log(LOGLEVEL_DEBUG,"EEPROM_ACTION_TBL_ENTRY_LENGTH = %d\n", EEPROM_ACTION_TBL_ENTRY_LENGTH );
Logger.Log(LOGLEVEL_DEBUG,"EEPROM_EXT_DATA_END = %d\n", EEPROM_EXT_DATA_END );


#ifdef ESP_HAS_PCF8574
  Wire.begin(default_sda_pin,default_scl_pin);
//  Wire.begin();
//  scanIIC();
#endif // ESP_HAS_PCF8574


  resetAdminSettings2Default();
  resetActionFlags();

  if( eeprom.init( 1024, eeprom.version2Magic(), LOGLEVEL_QUIET ) < EE_STATUS_INVALID_CRC )  
//  if( eeprom.init( 1024, 0x00, LOGLEVEL_QUIET ) < EE_STATUS_INVALID_CRC )  
  {
    if( eeprom.isValid() )
    {
      analogWrite(GREEN, 0);
      analogWrite(BLUE, 127);
      
      Logger.Log(LOGLEVEL_DEBUG,"eeprom content is valid!\n");

      crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );

      eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        
      if( !beQuiet )
      {
          Logger.Log(LOGLEVEL_DEBUG, "restored crc: %x calc crc: %x\n", crcRead, crcCalc);
      }
        
      if( (crcCalc == crcRead) )
      {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "crc MATCH!\n" );
        }

        analogWrite(RED, 0 );
        analogWrite(GREEN, 127);
        analogWrite(BLUE, 0);
      
        Logger.Log(LOGLEVEL_DEBUG,"eeprom content is OK!\n");
        restoreAdminSettings();
        restoreAddSysvars();
        restoreActionTable();
      }
      else
      {
        analogWrite(RED, 127 );
        analogWrite(GREEN, 0);
        analogWrite(BLUE, 0);
      
        Logger.Log(LOGLEVEL_DEBUG,"eeprom content is INVALID!\n");
        eeprom.wipe();  
        Logger.Log(LOGLEVEL_DEBUG,"EEPROM cleared ... ");  
        resetAdminSettings2Default();
        storeAdminSettings();
        storeAddSysvars();
        storeActionTable();
        eeprom.setMagic( eeprom.version2Magic() );
        Logger.Log(LOGLEVEL_DEBUG,"magic newly calculated ... \n");

        crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );
        Logger.Log(LOGLEVEL_DEBUG,"CRC is now: %x -> write to POS %d\n", crcCalc, EEPROM_POS_CRC32 );
        eeprom.storeRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );
        eeprom.validate();

        eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "reread CRC: %x\n", crcRead );
        }
        
        eeprom.validate();   
        Logger.Log(LOGLEVEL_DEBUG,"and set to ok and ready!\n");
      }
    }
    else
    {
      analogWrite(RED, 127 );
      analogWrite(GREEN, 0);
      analogWrite(BLUE, 0);
      
      Logger.Log(LOGLEVEL_DEBUG,"eeprom content is INVALID!\n");
      eeprom.wipe();  
      Logger.Log(LOGLEVEL_DEBUG,"EEPROM cleared ... ");  
      resetAdminSettings2Default();
      storeAdminSettings();
      storeAddSysvars();
      storeActionTable();
      eeprom.setMagic( eeprom.version2Magic() );
      Logger.Log(LOGLEVEL_DEBUG,"magic newly calculated ... \n"); 
      crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );
      Logger.Log(LOGLEVEL_DEBUG,"CRC is now: %x -> write to POS %d\n", crcCalc, EEPROM_POS_CRC32 );
      eeprom.storeRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );
      eeprom.validate();

      eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);

      if( !beQuiet )
      {
          Logger.Log(LOGLEVEL_DEBUG, "reread CRC: %x\n", crcRead );
      }

      eeprom.validate();   
      Logger.Log(LOGLEVEL_DEBUG,"and set to ok and ready!\n");
    }
  }
  else
  {
    analogWrite(RED, 127 );
    analogWrite(GREEN, 0);
    analogWrite(BLUE, 0);

    Logger.Log(LOGLEVEL_DEBUG,"eeprom status is >= EE_STATUS_INVALID_CRC!\n");
    eeprom.wipe();    
    Logger.Log(LOGLEVEL_DEBUG,"EEPROM cleared ... ");  
    resetAdminSettings2Default();
    storeAdminSettings();
    storeAddSysvars();
    storeActionTable();      
    eeprom.setMagic( eeprom.version2Magic() );
    Logger.Log(LOGLEVEL_DEBUG,"magic newly calculated ... \n");       
    crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );
    Logger.Log(LOGLEVEL_DEBUG,"CRC is now: %x -> write to POS %d\n", crcCalc, EEPROM_POS_CRC32 );
    eeprom.storeRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );
    eeprom.validate();
    
    eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "reread CRC: %x\n", crcRead );
    }
            
    eeprom.validate(); 
    Logger.Log(LOGLEVEL_DEBUG,"and set to ok and ready!\n");   
  }


  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "Connecting to >%s< using password >%s<\n", wlanSSID.c_str(), wlanPassphrase.c_str());
  }


  WiFi.begin(wlanSSID.c_str(), wlanPassphrase.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, "." );
    }
  }
  
  analogWrite(RED, 0 );
  analogWrite(GREEN, 127);
  analogWrite(BLUE, 0);

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "WiFi connected\n" );
  }

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "Starting NTP over UDP\n");
  }

  Udp.begin(localUDPPort);

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "Local port: %d ... waiting for time sync", Udp.localPort());
  }

  // after syncing set initial time
  setSyncProvider(getNtpTime);
  setSyncInterval(SECS_PER_HOUR);
  setTime(now());

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "Time synced via NTP. Sync interval is set to %d seconds.\n", SECS_PER_HOUR );
  }

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "Starting HTTP server\n");
  }

  server = ESP8266WebServer( (unsigned long) wwwServerPort.toInt() );
  server.on("/", setupPage);

  server.begin();

  localIP = WiFi.localIP();
  wwwServerIP = localIP.toString();

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "Webserver started. URL is: http://%s:%d\n", wwwServerIP.c_str(), wwwServerPort.toInt());
  }
 
  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "Startup actions ...\n");
  }

  startupActions();

  dumpInfo();   
      
}


//
// ************************************************************************
//
// ---------- void scanIIC
//
// ************************************************************************
//
void scanIIC()
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

//
// ************************************************************************
//
// ---------- void alwaysOn( int timerNo, int port )
//
// ************************************************************************
//
void alwaysOn(int timerNo, int port)
{

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "... check always on for port %d and timer = %d, flag #1 is %d, flag #2 is %d\n", 
      port, timerNo, tblEntry[port].actionFlag_1, tblEntry[port].actionFlag_2);
  }


  if( timerNo == 1 )
  {
    if( (tblEntry[port].actionFlag_1 & ACTION_FLAG_ALWAYS_ON) == 0 )
    {
      tblEntry[port].actionFlag_1 |= ACTION_FLAG_ALWAYS_ON;
      tblEntry[port].actionFlag_1 |= ACTION_FLAG_ACTIVE;
      switchRelais(port, RELAIS_ON);

      if( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, "... entry for port %d time #1 is manual ALWAYS ON\n", port);
      }

    }
  }
  else
  {
    if( timerNo == 2 )
    {
      if( (tblEntry[port].actionFlag_2 & ACTION_FLAG_ALWAYS_ON) == 0 )
      {
        tblEntry[port].actionFlag_2 |= ACTION_FLAG_ALWAYS_ON;
        tblEntry[port].actionFlag_2 |= ACTION_FLAG_ACTIVE;
        switchRelais(port, RELAIS_ON);

        if( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, "... entry for port %d time #2 is manual ALWAYS ON\n", port);
        }

      }
    }
  }
}

//
// ************************************************************************
//
// ---------- void alwaysOff( int timerNo, int port )
//
// ************************************************************************
//
void alwaysOff(int timerNo, int port)
{

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "... check always off for port %d and timer = %d, flag #1 is %d, flag #2 is\n", port, timerNo, tblEntry[port].actionFlag_1, tblEntry[port].actionFlag_2);
  }

  if( timerNo == 1 )
  {
    if( (tblEntry[port].actionFlag_1 & ACTION_FLAG_ALWAYS_OFF) == 0 )
    {
      tblEntry[port].actionFlag_1 |= ACTION_FLAG_ALWAYS_OFF;
      tblEntry[port].actionFlag_1 &= ~ACTION_FLAG_ACTIVE;
      switchRelais(port, RELAIS_OFF);

      if( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, "... entry for port %d time #1 is manual ALWAYS OFF\n", port);
      }

    }
  }
  else
  {
    if( timerNo == 2 )
    {
      if( (tblEntry[port].actionFlag_2 & ACTION_FLAG_ALWAYS_OFF) == 0 )
      {
        tblEntry[port].actionFlag_2 |= ACTION_FLAG_ALWAYS_OFF;
        tblEntry[port].actionFlag_1 &= ~ACTION_FLAG_ACTIVE;
        switchRelais(port, RELAIS_OFF);

        if( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, "... entry for port %d time #2 is manual ALWAYS OFF\n", port);
        }

      }
    }
  }
}


//
// ************************************************************************
//
// ---------- int check4Action()
//
// ************************************************************************
//

int check4Action( void )
{
  int i;
  int retVal = 0;
  time_t secsSinceEpoch;
  short nowMinutes;
  short chkMinutesFrom, chkMinutesTo;
  time_t utc;

  utc=now();
  secsSinceEpoch = CE.toLocal(utc, &tcr);
  nowMinutes = ( hour(secsSinceEpoch) * 60 ) + minute(secsSinceEpoch);

  if( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, "check4Action: check for minute-value: %d\n", nowMinutes );
  }

//  for( i = 0; i < MAX_ACTION_TABLE_LINES; i++ )
  for( i = 0; i < CONNECTED_RELAIS; i++ )
  {

    if( tblEntry[i].enabled_1 )
    {

      if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {

        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_1.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_1.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_1.c_str()) * 60) + atoi(tblEntry[i].minuteTo_1.c_str());

        if( chkMinutesFrom == nowMinutes )
        {
          if( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, 
              "action is time 1 - Begin %s:%s [%d] port %d. Actionflag is %d\n", 
              tblEntry[i].hourFrom_1.c_str(), tblEntry[i].minuteFrom_1.c_str(),
              chkMinutesFrom, i, tblEntry[i].actionFlag_1);
          }

          if( (tblEntry[i].actionFlag_1 & ACTION_FLAG_ACTIVE) == 0 )
          {
            tblEntry[i].actionFlag_1 |= ACTION_FLAG_ACTIVE;

            switchRelais(i, RELAIS_ON);

            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, "set actionFlag for time 1 to active\n");
            }
          }
        }

        if(chkMinutesTo == nowMinutes )
        {
          if( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, 
              "action is time 1 - End %s:%s [%d] port %d. Actionflag is %d\n",
              tblEntry[i].hourTo_1.c_str(), tblEntry[i].minuteTo_1.c_str(),
              chkMinutesTo, i, tblEntry[i].actionFlag_1);
          }

          if( (tblEntry[i].actionFlag_1 & ACTION_FLAG_ACTIVE) != 0 )
          {
            tblEntry[i].actionFlag_1 &= ~ACTION_FLAG_ACTIVE;
            switchRelais(i, RELAIS_OFF);

            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, "set actionFlag for time 1 to inactive\n");
            }
          }
          else
          {
            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, "nothing to do ... actionFlag for time 1 is already inactive\n");
            }
          }
        }
      }
      else // if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
        if( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, "... entry for port %d time #1 is not auto\n", i);
        }

        if( tblEntry[i].mode.equalsIgnoreCase("on") )
        {
          alwaysOn(1, i);
        }
        else
        {
          if( tblEntry[i].mode.equalsIgnoreCase("off") )
          {
            alwaysOff(1, i);
          }
        }
      }
    }
    else // if( tblEntry[i].enabled_1 )
    {
      if( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, "nothing to do ... entry for port %d time #1 disabled\n", i);
      }
    }

    if( tblEntry[i].enabled_2 )
    {
      if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_2.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_2.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_2.c_str()) * 60) + atoi(tblEntry[i].minuteTo_2.c_str());

        if(chkMinutesFrom == nowMinutes )
        {
          if( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, 
              "action is time 2 - Begin %s:%s [%d] port %d. Actionflag is %d\n",
              tblEntry[i].hourFrom_2.c_str(), tblEntry[i].minuteFrom_2.c_str(),
              chkMinutesFrom, i, tblEntry[i].actionFlag_2);
          }

          if( (tblEntry[i].actionFlag_2 & ACTION_FLAG_ACTIVE) == 0 )
          {
            tblEntry[i].actionFlag_2 |= ACTION_FLAG_ACTIVE;
            switchRelais(i, RELAIS_ON);
  
            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, "set actionFlag for time 2 to active\n");
            }
          }
        }

        if(chkMinutesTo == nowMinutes )
        {
          if( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, 
              "action is time 1 - End %s:%s [%d] port %d. Actionflag is %d\n",
              tblEntry[i].hourTo_2.c_str(), tblEntry[i].minuteTo_2.c_str(),
              chkMinutesTo, i, tblEntry[i].actionFlag_2);
          }

          if( (tblEntry[i].actionFlag_2 & ACTION_FLAG_ACTIVE) != 0 )
          {
            tblEntry[i].actionFlag_2 &= ~ACTION_FLAG_ACTIVE;
            switchRelais(i, RELAIS_OFF);

            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, "set actionFlag for time 2 to inactive\n");
            }
          }
          else
          {
            if( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, "nothing to do ... actionFlag for time 2 is already inactive\n");
            }
          }
        }
      }
      else // if( tblEntry[tmRow-1].mode.equalsIgnoreCase("auto") )
      {
        if( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, "... entry for port %d time #2 is not auto\n", i);
        }

        if( tblEntry[i].mode.equalsIgnoreCase("on") )
        {
          alwaysOn(2, i);
        }
        else
        {
          alwaysOff(2, i);
        }
      }
    }
    else // if( tblEntry[i].enabled_2 )
    {
      if( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, "nothing to do ... entry for port %d time #2 disabled\n", i);
      }
    }
  }

  return( retVal );
}

//
// ************************************************************************
//
// ---------- void loop()
//
// ************************************************************************
//
void loop()
{
  time_t secsSinceEpoch;
  short nowMinutes;
  static short minutesLastCheck;
  short chkMinutesFrom, chkMinutesTo;
  time_t utc;

  utc=now();
  secsSinceEpoch = CE.toLocal(utc, &tcr);
  nowMinutes = ( hour(secsSinceEpoch) * 60 ) + minute(secsSinceEpoch);

  server.handleClient();

  if( nowMinutes > minutesLastCheck )
  {
    if( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, "loop: hour is %d, minutes is %d\n", hour(secsSinceEpoch), minute(secsSinceEpoch) );
    }

    minutesLastCheck = nowMinutes;
    check4Action();
  }
  else
  {
    delay(500);
  }

//    for (int i=0; i<8; i++)
//    {
//      switchRelais(i, 1);
//      delay(100);
//      switchRelais(i, 0);
//      delay(100);
//    }

//    for (int i=0; i<8; i++)
//    {
//      switchRelais.toggle(i);
//      delay(100);
//      switchRelais.toggle(i);
//      delay(100);
//    }
//  }
//  else
//  {
//    delay(1000);
//  }

}


/* ************** ++++++++++ ************* +++++++++++++++ */

void doTimeOnSelect( char *sTarget, char *sSwitch, char *sBgColor, int tmNum, int tmRow, char* sRow, char *sHour, char* sMinute )
{
  char sNum[10];
  sprintf(sNum, "%d", tmNum);

  pageContent += "<td align=\"center\" bgcolor=\"";
  // loght cyan #07F479  or grey #D4C9C9
  pageContent += sBgColor;
  pageContent += "\">\n";

  pageContent += "<label>";
  // einschalten/ausschalten
  pageContent += sSwitch;

  pageContent += "</label><br>\n";

// Stunde

  pageContent += "<input type=\"textdate\" name=\"h";
  pageContent += sTarget;        // = "from" or "to"
  pageContent += String(sNum);   // = 1 or 2
  pageContent += String("_");    //
  pageContent += String(sRow);   // = 1 - 8


  pageContent += "\" value=\"";
  pageContent += String(sHour);
  pageContent += "\" size=\"2\" maxlength=\"2\" >\n";

  pageContent += " Uhr ";

// Minute

  pageContent += "<input type=\"textdate\" name=\"m";
  pageContent += sTarget;        // = "from" or "to"
  pageContent += String(sNum);   // = 1 or 2
  pageContent += String("_");    //
  pageContent += String(sRow);   // = 1 - 8

  pageContent += "\" value=\"";
  pageContent += String(sMinute);
  pageContent += "\" size=\"2\" maxlength=\"2\" >\n";

  pageContent += "</td>\n";

}

void doTimeSelect( int tmNum, int tmRow, char* sRow )
{
  if( tmNum == 1 )
  {
    doTimeOnSelect( (char*)"from", (char*)"einschalten", (char*)"#07F479", tmNum, tmRow,  sRow,
                     (char*)tblEntry[tmRow-1].hourFrom_1.c_str(), (char*)tblEntry[tmRow-1].minuteFrom_1.c_str() );

    doTimeOnSelect( (char*)"to",   (char*)"ausschalten", (char*)"#D4C9C9" , tmNum, tmRow,  sRow,
                     (char*)tblEntry[tmRow-1].hourTo_1.c_str(), (char*)tblEntry[tmRow-1].minuteTo_1.c_str() );
  }
  else
  {
    doTimeOnSelect( (char*)"from", (char*)"einschalten", (char*)"#07F479", tmNum, tmRow,  sRow,
                    (char*)tblEntry[tmRow-1].hourFrom_2.c_str(), (char*)tblEntry[tmRow-1].minuteFrom_2.c_str() );

    doTimeOnSelect( (char*)"to",   (char*)"ausschalten", (char*)"#D4C9C9" , tmNum, tmRow,  sRow,
                    (char*)tblEntry[tmRow-1].hourTo_2.c_str(), (char*)tblEntry[tmRow-1].minuteTo_2.c_str() );
  }
}


void doCreateLine( int tmRow )
{
  char sRow[10];
  sprintf(sRow, "%d", tmRow);


  pageContent += "<td>\n";
  pageContent += "<input type=\"text\" name=\"bezeichner";
  pageContent += sRow;
  
  pageContent += "\" value=\"";
  pageContent += tblEntry[tmRow-1].name;  
  pageContent += "\" size=\"4\" maxlength=\"10\">\n";
  //
  pageContent += "</td>\n";

// ---- ++++++++++ ZEIT 1 enable/disable ++++++++++ -----
  
  pageContent += "<td bgcolor=\"#09CEF3\">\n";
  //
  pageContent += "<input type=\"checkbox\" name=\"enabled";
  pageContent += "1_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\" ";
  if( tblEntry[tmRow-1].enabled_1 )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\" >\n";
  //
  pageContent += "<label>Zeit 1</label>\n";
  pageContent += "</td>\n";
  doTimeSelect( 1, tmRow, sRow);

// ---- ++++++++++ ZEIT 2 enable/disable ++++++++++ -----
  
  pageContent += "<td bgcolor=\"#09CEF3\">\n";
  //  
  pageContent += " <input type=\"checkbox\" name=\"enabled";
  pageContent += "2_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\" ";
  if( tblEntry[tmRow-1].enabled_2 )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\" >\n";
  //
  pageContent += "<label>Zeit 2</label>\n";
  pageContent += "</td>\n";
  doTimeSelect( 2, tmRow, sRow);

  pageContent +="<td>\n";

// ---- ++++++++++ EXT 1 enable/disable ++++++++++ -----

  pageContent +="<input type=\"checkbox\" name=\"ext";
  pageContent += "1_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\"";
  if( tblEntry[tmRow-1].extEnable_1 )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
//
  pageContent +="<label>Ext 1</label>\n";
  pageContent +="<br>\n";

// ---- ++++++++++ EXT 2 enable/disable ++++++++++ -----
  
  pageContent +="<input type=\"checkbox\" name=\"ext";
  pageContent += "2_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\"";
  if( tblEntry[tmRow-1].extEnable_2 )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent +="<label>Ext 2</label>";

  pageContent +="</td>";
  pageContent +="<td>\n";

// ---- ++++++++++ MODE ON ++++++++++ -----

  pageContent +="<input type=\"radio\" name=\"mode";
  pageContent += sRow;
  pageContent += "\" value=\"on\"";
  if( tblEntry[tmRow-1].mode.equalsIgnoreCase("on") )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent +="<label>Ein</label>\n";
  pageContent +="<br>\n";

// ---- ++++++++++ MODE OFF ++++++++++ -----

  pageContent +="<input type=\"radio\" name=\"mode";
  pageContent += sRow;
  pageContent += "\" value=\"off\"";
  if( tblEntry[tmRow-1].mode.equalsIgnoreCase("off") )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent +="<label>Aus</label>\n";
  pageContent +="<br>\n";
  
// ---- ++++++++++ MODE AUTO ++++++++++ -----

  pageContent +="        <input type=\"radio\" name=\"mode";
  pageContent += sRow;
  pageContent += "\" value=\"auto\"";
  if( tblEntry[tmRow-1].mode.equalsIgnoreCase("auto") )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent +="<label>Auto</label>\n";
// ---------
  pageContent +="      </td>\n";

// end Zeile ...

}


void setupPage()
{

    int i;
    unsigned long crcCalc, crcRead;

    pageContent = "";

    if( !beQuiet )
    {
         Logger.Log(LOGLEVEL_DEBUG, "handleIndexPage\n");
    }

    for(i = 0; i < server.args(); i++ )
    {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
        }
    }

    if( server.method() == SERVER_METHOD_POST )
//        server.hasArg(INDEX_BUTTONNAME_ADMIN)  )
    {
        if( !beQuiet )
        {
             Logger.Log(LOGLEVEL_DEBUG, "POST REQUEST\n");
        }
    }
    else
    {
        if( !beQuiet )
        {
             Logger.Log(LOGLEVEL_DEBUG, "GET REQUEST\n");
        }
    }

    if( server.hasArg("submit") && server.arg("submit").equalsIgnoreCase("speichern") )

    {
      for( i = 0; i < 8; i++ )
      {
tblEntry[i].name         = server.arg( _form_keywords_[i][KW_IDX_BEZEICHNER] );
tblEntry[i].mode         = server.arg( _form_keywords_[i][KW_IDX_MODE] );
tblEntry[i].hourFrom_1   = server.arg( _form_keywords_[i][KW_IDX_HFROM_1] );
tblEntry[i].minuteFrom_1 = server.arg( _form_keywords_[i][KW_IDX_MFROM_1] );
tblEntry[i].hourTo_1     = server.arg( _form_keywords_[i][KW_IDX_HTO_1] );
tblEntry[i].minuteTo_1   = server.arg( _form_keywords_[i][KW_IDX_MTO_1] );
tblEntry[i].hourFrom_2   = server.arg( _form_keywords_[i][KW_IDX_HFROM_2] );
tblEntry[i].minuteFrom_2 = server.arg( _form_keywords_[i][KW_IDX_MFROM_2] );
tblEntry[i].hourTo_2     = server.arg( _form_keywords_[i][KW_IDX_HTO_2] );
tblEntry[i].minuteTo_2   = server.arg( _form_keywords_[i][KW_IDX_MTO_2] );

tblEntry[i].enabled_1   = server.arg( _form_keywords_[i][ KW_IDX_ENABLED_1] ).equalsIgnoreCase("aktiv") ? true : false ;
tblEntry[i].extEnable_1 = server.arg( _form_keywords_[i][ KW_IDX_EXT_1] ).equalsIgnoreCase("aktiv") ? true : false;
tblEntry[i].enabled_2   = server.arg( _form_keywords_[i][ KW_IDX_EXT_2] ).equalsIgnoreCase("aktiv") ? true : false;
tblEntry[i].extEnable_2 = server.arg( _form_keywords_[i][ KW_IDX_ENABLED_2] ).equalsIgnoreCase("aktiv") ? true : false;

      }

      storeActionTable();
      crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );
      Logger.Log(LOGLEVEL_DEBUG,"CRC is now: %x -> write to POS %d\n", crcCalc, EEPROM_POS_CRC32 );
      eeprom.storeRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );
      eeprom.validate();

      eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);

      if( !beQuiet )
      {
          Logger.Log(LOGLEVEL_DEBUG, "reread CRC: %x\n", crcRead );
      }

      eeprom.validate();   
      Logger.Log(LOGLEVEL_DEBUG,"and set to ok and ready!\n");
      startupActions();
// return;

  }

  pageContent += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";
  pageContent += "<html>\n";
  pageContent += "<head>\n";
  pageContent += "<meta charset=\"utf-8\">\n";
  pageContent += "<title>Home</title>\n";

  pageContent += "<style> ";
  pageContent += "input[type=textdate] { width: 1.5em; }  ";
  pageContent += "</style>\n";

  pageContent += "</head>\n";

  pageContent += "<body bgcolor=\"#D4C9C9\" text=\"#000000\" link=\"#1E90FF\" vlink=\"#0000FF\">\n";
  pageContent += "<div align=\"center\"><strong><h1>Gartenleuchten</h1></strong></div>\n";
  pageContent += "<div align=\"center\"><strong><h1>192.168.1.112</h1></strong></div>\n";
  pageContent += "<form action=\"/\" method=\"get\">\n";
  pageContent += "<hr align=\"center\"><br>\n";
  pageContent += "<table border align=\"center\">\n";

  // Zeile 1
  pageContent += "<tr>\n";
  doCreateLine( 1 );
  pageContent +="</tr>\n";

  // Zeile 2
  pageContent += "<tr>\n";
  doCreateLine( 2 );
  pageContent +="</tr>\n";

  // Zeile 3
  pageContent += "<tr>\n";
  doCreateLine( 3 );
  pageContent +="</tr>\n";

  // Zeile 4
  pageContent += "<tr>\n";
  doCreateLine( 4 );
  pageContent +="</tr>\n";

  // Zeile 5
  pageContent += "<tr>\n";
  doCreateLine( 5 );
  pageContent +="</tr>\n";

  // Zeile 6
  pageContent += "<tr>\n";
  doCreateLine( 6 );
  pageContent +="</tr>\n";

  // Zeile 7
  pageContent += "<tr>\n";
  doCreateLine( 7 );
  pageContent +="</tr>\n";

  // Zeile 8
  pageContent += "<tr>\n";
  doCreateLine( 8 );
  pageContent +="</tr>\n";

  pageContent +="</table>\n";
  pageContent +="<hr align=\"center\">\n";
  pageContent +="<div align=\"center\">\n";
  pageContent +="<input type=\"submit\" name=\"submit\" value=\"speichern\">\n";
  pageContent +="<input type=\"reset\" name=\"reset\" value=\"reset\">\n";
  pageContent +="<input type=\"submit\" name=\"ESP\" value=\"reboot\">\n";
  pageContent +="</div>\n";
  pageContent +="<hr align=\"center\"><br>\n";
  pageContent +="</form>\n";
  pageContent +="</body>\n";
  pageContent +="</html>\n";

  server.send(200, "text/html", pageContent); 

//  Serial.print(pageContent.c_str());

}

/* ************** ++++++++++ ************* +++++++++++++++ */


