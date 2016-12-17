//
// ************************************************************************
// Use an ESP8266 modul as a WiFi switch
// (C) 2016 Dirk Schanz aka dreamshader
// ************************************************************************
// 41148
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
//   REMEMBER TO SET MATCHING VALUES FOR YOUR WLAN ACCESS!
//   On the two lines around 104 and below set
//   #define FACTORY_WLAN_SSID to YOUR SSID
//   #define FACTORY_WLAN_PASSPHRASE to YOUR PASSPHRASE for access
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
// 2016/12/08: added flag handling
// 2016/12/12: added functions for web-update
//
//
// ************************************************************************
//
// ---- suppress debug output to serial line ------------------------------
// set beQuiet = no debug output
//
#define BE_QUIET			false
bool beQuiet;
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

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// #define __ASSERT_USE_STDERR     //
// #include <assert.h>             // for future use

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
//
#ifdef ESP_HAS_PCF8574
#define RELAIS_OFF               HIGH
#define RELAIS_ON                LOW
#else
#define RELAIS_OFF               HIGH
#define RELAIS_ON                LOW
#endif // ESP_HAS_PCF8574
//
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
String ntpServerName;
String ntpServerPort;
//
// ************************************************************************
// setup local SSID
// ************************************************************************
//
String wlanSSID;
//
// ************************************************************************
// setup for  online-updates
// ************************************************************************
//
String updateUrl;
short lastRelease;
short lastVersion;
short lastPatchlevel;
//
short lastNumber;
//
short nextUpdateMonth;
short nextUpdateDay;
short nextUpdateWeekDay;
short nextUpdateHour;
short nextUpdateMinute;
short updateInterval;
short updateMode;
//
#define EEPROM_MAXLEN_UPDATE_URL         45
#define EEPROM_MAXLEN_LAST_RELEASE        2
#define EEPROM_MAXLEN_LAST_VERSION        2
#define EEPROM_MAXLEN_LAST_PATCHLEVEL     2
//
#define EEPROM_MAXLEN_LAST_NUMBER         2
//
#define EEPROM_MAXLEN_NEXT_UPDATE_MONTH   2
#define EEPROM_MAXLEN_NEXT_UPDATE_DAY     2
#define EEPROM_MAXLEN_NEXT_UPDATE_WEEKDAY 2
#define EEPROM_MAXLEN_NEXT_UPDATE_HOUR    2
#define EEPROM_MAXLEN_NEXT_UPDATE_MINUTE  2
//
#define EEPROM_MAXLEN_UPDATE_INTERVAL     2
#define EEPROM_MAXLEN_UPDATE_MODE         2
//
//
#define DEFAULT_UPDATE_URL              "http://192.168.1.122/ESP8266/"
#define DEFAULT_UPDATE_LAST_RELEASE     0
#define DEFAULT_UPDATE_LAST_VERSION     0
#define DEFAULT_UPDATE_LAST_PATCHLEVEL  0
#define DEFAULT_UPDATE_LAST_NUMBER      0
#define DEFAULT_UPDATE_NEXT_MONTH       0
#define DEFAULT_UPDATE_NEXT_DAY         0
#define DEFAULT_UPDATE_NEXT_WEEKDAY     0
#define DEFAULT_UPDATE_NEXT_HOUR        0
#define DEFAULT_UPDATE_NEXT_MINUTE      0
#define DEFAULT_UPDATE_INTERVAL         0
#define DEFAULT_UPDATE_MODE             0
//
// ************************************************************************
// setup passphrase for local WLAN
// ************************************************************************
//
String wlanPassphrase;
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



#define EEPROM_MAXLEN_NTP_SERVER_NAME    20
#define EEPROM_MAXLEN_NTP_SERVER_PORT     5

#define EEPROM_ADD_SYS_VARS_BEGIN        EEPROM_STD_DATA_END

#define EEPROM_POS_NTP_SERVER_NAME       (EEPROM_ADD_SYS_VARS_BEGIN)
#define EEPROM_POS_NTP_SERVER_PORT       (EEPROM_POS_NTP_SERVER_NAME  + EEPROM_MAXLEN_NTP_SERVER_NAME   + EEPROM_LEADING_LENGTH)

#define EEPROM_ADD_SYS_VARS_END          (EEPROM_POS_NTP_SERVER_PORT  + EEPROM_MAXLEN_NTP_SERVER_PORT + EEPROM_LEADING_LENGTH)

#ifdef EEPROM_EXT_DATA_BEGIN
#undef EEPROM_EXT_DATA_BEGIN
#define EEPROM_EXT_DATA_BEGIN             EEPROM_ADD_SYS_VARS_END
#else
#define EEPROM_EXT_DATA_BEGIN             EEPROM_ADD_SYS_VARS_END
#endif

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
// #define EEPROM_EXT_DATA_END            (EEPROM_ACTION_TBL_ENTRY_START + (EEPROM_ACTION_TBL_ENTRY_LENGTH * MAX_ACTION_TABLE_LINES))
//
//
//
#define EEPROM_POS_UPDATE_URL            (EEPROM_ACTION_TBL_ENTRY_START + (EEPROM_ACTION_TBL_ENTRY_LENGTH * MAX_ACTION_TABLE_LINES))
#define EEPROM_POS_LAST_RELEASE          (EEPROM_POS_UPDATE_URL          + EEPROM_MAXLEN_UPDATE_URL          + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_LAST_VERSION          (EEPROM_POS_LAST_RELEASE        + EEPROM_MAXLEN_LAST_RELEASE        + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_LAST_PATCHLEVEL       (EEPROM_POS_LAST_VERSION        + EEPROM_MAXLEN_LAST_VERSION        + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_LAST_NUMBER           (EEPROM_POS_LAST_PATCHLEVEL     + EEPROM_MAXLEN_LAST_PATCHLEVEL     + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_NEXT_UPDATE_MONTH     (EEPROM_POS_LAST_NUMBER         + EEPROM_MAXLEN_LAST_NUMBER         + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_NEXT_UPDATE_DAY       (EEPROM_POS_NEXT_UPDATE_MONTH   + EEPROM_MAXLEN_NEXT_UPDATE_MONTH   + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_NEXT_UPDATE_WEEKDAY   (EEPROM_POS_NEXT_UPDATE_DAY     + EEPROM_MAXLEN_NEXT_UPDATE_DAY     + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_NEXT_UPDATE_HOUR      (EEPROM_POS_NEXT_UPDATE_WEEKDAY + EEPROM_MAXLEN_NEXT_UPDATE_WEEKDAY + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_NEXT_UPDATE_MINUTE    (EEPROM_POS_NEXT_UPDATE_HOUR    + EEPROM_MAXLEN_NEXT_UPDATE_HOUR    + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_UPDATE_INTERVAL       (EEPROM_POS_NEXT_UPDATE_MINUTE  + EEPROM_MAXLEN_NEXT_UPDATE_MINUTE  + EEPROM_LEADING_LENGTH)
#define EEPROM_POS_UPDATE_MODE           (EEPROM_POS_UPDATE_INTERVAL     + EEPROM_MAXLEN_UPDATE_INTERVAL     + EEPROM_LEADING_LENGTH)

#define EEPROM_EXT_DATA_END              (EEPROM_POS_UPDATE_MODE         + EEPROM_MAXLEN_UPDATE_MODE         + EEPROM_LEADING_LENGTH)

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

#define NUM_SETUP_FORM_FIELDS     14

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

String formFieldName[NUM_SETUP_FORM_FIELDS];


#define KW_IDX_BEZEICHNER   0
#define KW_IDX_MODE         1
#define KW_IDX_ENABLED_1    2
#define KW_IDX_HFROM_1      3
#define KW_IDX_MFROM_1      4
#define KW_IDX_HTO_1        5
#define KW_IDX_MTO_1        6
#define KW_IDX_EXT_1        7
#define KW_IDX_ENABLED_2    8
#define KW_IDX_HFROM_2      9
#define KW_IDX_MFROM_2     10
#define KW_IDX_HTO_2       11
#define KW_IDX_MTO_2       12
#define KW_IDX_EXT_2       13

#define DEFAULT_FORMVAR_BEZEICHNER  ""
#define DEFAULT_FORMVAR_MODE        ""
#define DEFAULT_FORMVAR_ENABLED_1   false
#define DEFAULT_FORMVAR_HFROM_1     ""
#define DEFAULT_FORMVAR_MFROM_1     ""
#define DEFAULT_FORMVAR_HTO_1       ""
#define DEFAULT_FORMVAR_MTO_1       ""
#define DEFAULT_FORMVAR_EXT_1       false
#define DEFAULT_FORMVAR_ENABLED_2   false
#define DEFAULT_FORMVAR_HFROM_2     ""
#define DEFAULT_FORMVAR_MFROM_2     ""
#define DEFAULT_FORMVAR_HTO_2       ""
#define DEFAULT_FORMVAR_MTO_2       ""
#define DEFAULT_FORMVAR_EXT_2       false
    


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
// void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp)
// {
//    // transmit diagnostic informations through serial link.
//    Serial.println(__func);
//    Serial.println(__file);
//    Serial.println(__lineno, DEC);
//    Serial.println(__sexp);
//    Serial.flush();
//    // abort program execution.
//    abort();
// }
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
  id = macToID(mac);
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

#ifdef DO_LOG

  if ( !beQuiet )
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

#endif // DO_LOG

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
#ifdef DO_LOG
  Logger.Log(LOGLEVEL_DEBUG, (const char*) "Transmit NTP Request");
#endif // DO_LOG
  // get a random server from the pool
  WiFi.hostByName(ntpServerName.c_str(), ntpServerIP);

#ifdef DO_LOG
  Logger.Log(LOGLEVEL_DEBUG, (const char*) "ntpServerName: %s\n", ntpServerName.c_str());
#endif // DO_LOG

  sendNTPpacket(ntpServerIP, packetBuffer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
#ifdef DO_LOG
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "received NTP response\n");
#endif // DO_LOG
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
#ifdef DO_LOG
  Logger.Log(LOGLEVEL_DEBUG, (const char*) "NO NTP response\n");
#endif // DO_LOG
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
void apiPage();
void setupPage();
void doCreateLine( int tmRow );
void doTimeSelect( int tmNum, int tmRow, char* sRow );
void doTimeOnSelect( char *sTarget, char *sSwitch, char *sBgColor, int tmNum, char* sRow, const char *sHour, const char* sMinute );
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
  for ( currLine = 0; currLine < CONNECTED_RELAIS; currLine++ )
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "reset action flag for port %d\n", currLine );
    }
#endif // DO_LOG

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

  eeprom.storeString(  wlanSSID,      EEPROM_MAXLEN_WLAN_SSID,       EEPROM_POS_WLAN_SSID );

  eeprom.storeString(  wlanPassphrase,    EEPROM_MAXLEN_WLAN_PASSPHRASE, EEPROM_POS_WLAN_PASSPHRASE );

  eeprom.storeString(  wwwServerIP,   EEPROM_MAXLEN_SERVER_IP,       EEPROM_POS_SERVER_IP );

  eeprom.storeString(  wwwServerPort, EEPROM_MAXLEN_SERVER_PORT,     EEPROM_POS_SERVER_PORT );

  eeprom.storeString(  nodeName,      EEPROM_MAXLEN_NODENAME,        EEPROM_POS_NODENAME );

  eeprom.storeString(  adminPasswd,   EEPROM_MAXLEN_ADMIN_PASSWORD,  EEPROM_POS_ADMIN_PASSWORD );

  eeprom.validate();

  return ( retVal );

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

  if ( eeprom.isValid() )
  {
    crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );

    eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);


    if ( (crcCalc == crcRead) )
    {
      eeprom.restoreString(  wlanSSID,    EEPROM_POS_WLAN_SSID,       EEPROM_MAXLEN_WLAN_SSID );
      eeprom.restoreString(  wlanPassphrase,  EEPROM_POS_WLAN_PASSPHRASE, EEPROM_MAXLEN_WLAN_PASSPHRASE );
      eeprom.restoreString(  wwwServerIP, EEPROM_POS_SERVER_IP,       EEPROM_MAXLEN_SERVER_IP );
      eeprom.restoreString(  wwwServerPort, EEPROM_POS_SERVER_PORT,     EEPROM_MAXLEN_SERVER_PORT );
      eeprom.restoreString(  nodeName,    EEPROM_POS_NODENAME,        EEPROM_MAXLEN_NODENAME );
      eeprom.restoreString(  adminPasswd, EEPROM_POS_ADMIN_PASSWORD,  EEPROM_MAXLEN_ADMIN_PASSWORD );
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

  return ( retVal );

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

  eeprom.storeString(  ntpServerName,      EEPROM_MAXLEN_NTP_SERVER_NAME,       EEPROM_POS_NTP_SERVER_NAME );
  eeprom.storeString(  ntpServerPort,      EEPROM_MAXLEN_NTP_SERVER_PORT,       EEPROM_POS_NTP_SERVER_PORT );


  return ( retVal );
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
  eeprom.restoreString(  ntpServerPort,    EEPROM_POS_NTP_SERVER_PORT,       EEPROM_MAXLEN_NTP_SERVER_PORT );

  eeprom.validate();

  return ( retVal );
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
  int currLine;


  // for( currLine = 0; currLine < MAX_ACTION_TABLE_LINES; currLine++ )
  for ( currLine = 0; currLine < CONNECTED_RELAIS; currLine++ )
  {
    eeprom.storeString(tblEntry[currLine].name, EEPROM_MAXLEN_TBL_ROW_NAME,
                       EEPROM_POS_TBL_ROW_NAME +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
    eeprom.storeString(tblEntry[currLine].mode, EEPROM_MAXLEN_TBL_ROW_MODE,
                       EEPROM_POS_TBL_ROW_MODE +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

    eeprom.storeBoolean((char*)&tblEntry[currLine].enabled_1, EEPROM_POS_TBL_ENABLED1 +
                        (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
    eeprom.storeString(tblEntry[currLine].hourFrom_1, EEPROM_MAXLEN_TBL_HR_FROM,
                       EEPROM_POS_TBL_HR1_FROM +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
    eeprom.storeString(tblEntry[currLine].minuteFrom_1, EEPROM_MAXLEN_TBL_MIN_FROM,
                       EEPROM_POS_TBL_MIN1_FROM +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
    eeprom.storeString(tblEntry[currLine].hourTo_1, EEPROM_MAXLEN_TBL_HR_TO,
                       EEPROM_POS_TBL_HR1_TO +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
    eeprom.storeString(tblEntry[currLine].minuteTo_1, EEPROM_MAXLEN_TBL_MIN_TO,
                       EEPROM_POS_TBL_MIN1_TO +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
    eeprom.storeBoolean((char*)&tblEntry[currLine].extEnable_1, EEPROM_POS_TBL_EXT1_ENABLED +
                        (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

    eeprom.storeBoolean((char*)&tblEntry[currLine].enabled_2, EEPROM_POS_TBL_ENABLED2 +
                        (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

    eeprom.storeString(tblEntry[currLine].hourFrom_2, EEPROM_MAXLEN_TBL_HR_FROM,
                       EEPROM_POS_TBL_HR2_FROM +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

    eeprom.storeString(tblEntry[currLine].minuteFrom_2, EEPROM_MAXLEN_TBL_MIN_FROM,
                       EEPROM_POS_TBL_MIN2_FROM +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

    eeprom.storeString(tblEntry[currLine].hourTo_2, EEPROM_MAXLEN_TBL_HR_TO,
                       EEPROM_POS_TBL_HR2_TO +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

    eeprom.storeString(tblEntry[currLine].minuteTo_2, EEPROM_MAXLEN_TBL_MIN_TO,
                       EEPROM_POS_TBL_MIN2_TO +
                       (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );

    eeprom.storeBoolean((char*)&tblEntry[currLine].extEnable_2, EEPROM_POS_TBL_EXT2_ENABLED +
                        (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
  }

  eeprom.validate();

  return ( retVal );

}

//
// ------------------------------------------------------------------------
//
// ---------- initialize the table with empty information
//
// ------------------------------------------------------------------------
//
void resetActionTable2Default()
{
  int currLine;
  
  for ( currLine = 0; currLine < CONNECTED_RELAIS; currLine++ )
  {
    tblEntry[currLine].name         = DEFAULT_FORMVAR_BEZEICHNER;
    tblEntry[currLine].mode         = DEFAULT_FORMVAR_MODE;
    tblEntry[currLine].enabled_1    = DEFAULT_FORMVAR_ENABLED_1;
    tblEntry[currLine].hourFrom_1   = DEFAULT_FORMVAR_HFROM_1;
    tblEntry[currLine].minuteFrom_1 = DEFAULT_FORMVAR_MFROM_1;
    tblEntry[currLine].hourTo_1     = DEFAULT_FORMVAR_HTO_1;
    tblEntry[currLine].minuteTo_1   = DEFAULT_FORMVAR_MTO_1;
    
    tblEntry[currLine].extEnable_1  = DEFAULT_FORMVAR_EXT_1;
    tblEntry[currLine].enabled_2    = DEFAULT_FORMVAR_ENABLED_2;
    tblEntry[currLine].hourFrom_2   = DEFAULT_FORMVAR_HFROM_2;
    tblEntry[currLine].minuteFrom_2 = DEFAULT_FORMVAR_MFROM_2;
    tblEntry[currLine].hourTo_2     = DEFAULT_FORMVAR_HTO_2;
    tblEntry[currLine].minuteTo_2   = DEFAULT_FORMVAR_MTO_2;
    tblEntry[currLine].extEnable_2  = DEFAULT_FORMVAR_EXT_2;
  }

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

  if ( eeprom.isValid() )
  {
    crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );

    eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);

    if ( crcCalc == crcRead )
    {

      // for( currLine = 0; currLine < MAX_ACTION_TABLE_LINES; currLine++ )
      for ( currLine = 0; currLine < CONNECTED_RELAIS; currLine++ )
      {
        eeprom.restoreString(tblEntry[currLine].name,
                             EEPROM_POS_TBL_ROW_NAME + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_ROW_NAME );
        eeprom.restoreString(tblEntry[currLine].mode,
                             EEPROM_POS_TBL_ROW_MODE + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_ROW_MODE );
        eeprom.restoreBoolean((char*)&tblEntry[currLine].enabled_1, EEPROM_POS_TBL_ENABLED1 +
                              (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        eeprom.restoreString(tblEntry[currLine].hourFrom_1,
                             EEPROM_POS_TBL_HR1_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_HR_FROM );
        eeprom.restoreString(tblEntry[currLine].minuteFrom_1,
                             EEPROM_POS_TBL_MIN1_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_MIN_FROM );
        eeprom.restoreString(tblEntry[currLine].hourTo_1,
                             EEPROM_POS_TBL_HR1_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_HR_TO );
        eeprom.restoreString(tblEntry[currLine].minuteTo_1,
                             EEPROM_POS_TBL_MIN1_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_MIN_TO );
        eeprom.restoreBoolean((char*)&tblEntry[currLine].extEnable_1, EEPROM_POS_TBL_EXT1_ENABLED +
                              (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        eeprom.restoreBoolean((char*)&tblEntry[currLine].enabled_2, EEPROM_POS_TBL_ENABLED2 +
                              (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
        eeprom.restoreString(tblEntry[currLine].hourFrom_2,
                             EEPROM_POS_TBL_HR2_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_HR_FROM );
        eeprom.restoreString(tblEntry[currLine].minuteFrom_2,
                             EEPROM_POS_TBL_MIN2_FROM + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_MIN_FROM );
        eeprom.restoreString(tblEntry[currLine].hourTo_2,
                             EEPROM_POS_TBL_HR2_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_HR_TO );
        eeprom.restoreString(tblEntry[currLine].minuteTo_2,
                             EEPROM_POS_TBL_MIN2_TO + (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH),
                             EEPROM_MAXLEN_TBL_MIN_TO );
        eeprom.restoreBoolean((char*)&tblEntry[currLine].extEnable_2, EEPROM_POS_TBL_EXT2_ENABLED +
                              (currLine * EEPROM_ACTION_TBL_ENTRY_LENGTH) );
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

  return ( retVal );

}

//
// ------------------------------------------------------------------------
//
// ---------- store update info to EEPROM
//
// ------------------------------------------------------------------------
//
int storeUpdateInfo()
{
  int retVal = 0;

  eeprom.storeString( updateUrl,               EEPROM_MAXLEN_UPDATE_URL,           EEPROM_POS_UPDATE_URL);
  eeprom.storeRaw( (char*) &lastRelease,       EEPROM_MAXLEN_LAST_RELEASE,         EEPROM_POS_LAST_RELEASE);
  eeprom.storeRaw( (char*) &lastVersion,       EEPROM_MAXLEN_LAST_VERSION,         EEPROM_POS_LAST_VERSION);
  eeprom.storeRaw( (char*) &lastPatchlevel,    EEPROM_MAXLEN_LAST_PATCHLEVEL,      EEPROM_POS_LAST_PATCHLEVEL);
  eeprom.storeRaw( (char*) &lastNumber,        EEPROM_MAXLEN_LAST_NUMBER,          EEPROM_POS_LAST_NUMBER);
  eeprom.storeRaw( (char*) &nextUpdateMonth,   EEPROM_MAXLEN_NEXT_UPDATE_MONTH,    EEPROM_POS_NEXT_UPDATE_MONTH);
  eeprom.storeRaw( (char*) &nextUpdateDay,     EEPROM_MAXLEN_NEXT_UPDATE_DAY,      EEPROM_POS_NEXT_UPDATE_DAY);
  eeprom.storeRaw( (char*) &nextUpdateWeekDay, EEPROM_MAXLEN_NEXT_UPDATE_WEEKDAY,  EEPROM_POS_NEXT_UPDATE_WEEKDAY);
  eeprom.storeRaw( (char*) &nextUpdateHour,    EEPROM_MAXLEN_NEXT_UPDATE_HOUR,     EEPROM_POS_NEXT_UPDATE_HOUR);
  eeprom.storeRaw( (char*) &nextUpdateMinute,  EEPROM_MAXLEN_NEXT_UPDATE_MINUTE,   EEPROM_POS_NEXT_UPDATE_MINUTE);
  eeprom.storeRaw( (char*) &updateInterval,    EEPROM_MAXLEN_UPDATE_INTERVAL,      EEPROM_POS_UPDATE_INTERVAL);
  eeprom.storeRaw( (char*) &updateMode,        EEPROM_MAXLEN_UPDATE_MODE,          EEPROM_POS_UPDATE_MODE);

  eeprom.validate();

  return ( retVal );

}

//
// ------------------------------------------------------------------------
//
// ---------- restore update info from EEPROM
//
// ------------------------------------------------------------------------
//
int restoreUpdateInfo()
{
  int retVal = 0;

  eeprom.restoreString( updateUrl,               EEPROM_POS_UPDATE_URL,           EEPROM_MAXLEN_UPDATE_URL);
  eeprom.restoreRaw( (char*) &lastRelease,       EEPROM_POS_LAST_RELEASE,         EEPROM_MAXLEN_LAST_RELEASE,        EEPROM_MAXLEN_LAST_RELEASE);
  eeprom.restoreRaw( (char*) &lastVersion,       EEPROM_POS_LAST_VERSION,         EEPROM_MAXLEN_LAST_VERSION,        EEPROM_MAXLEN_LAST_VERSION);
  eeprom.restoreRaw( (char*) &lastPatchlevel,    EEPROM_POS_LAST_PATCHLEVEL,      EEPROM_MAXLEN_LAST_PATCHLEVEL,     EEPROM_MAXLEN_LAST_PATCHLEVEL);
  eeprom.restoreRaw( (char*) &lastNumber,        EEPROM_POS_LAST_NUMBER,          EEPROM_MAXLEN_LAST_NUMBER,         EEPROM_MAXLEN_LAST_NUMBER);
  eeprom.restoreRaw( (char*) &nextUpdateMonth,   EEPROM_POS_NEXT_UPDATE_MONTH,    EEPROM_MAXLEN_NEXT_UPDATE_MONTH,   EEPROM_MAXLEN_NEXT_UPDATE_MONTH);
  eeprom.restoreRaw( (char*) &nextUpdateDay,     EEPROM_POS_NEXT_UPDATE_DAY,      EEPROM_MAXLEN_NEXT_UPDATE_DAY,     EEPROM_MAXLEN_NEXT_UPDATE_DAY);
  eeprom.restoreRaw( (char*) &nextUpdateWeekDay, EEPROM_POS_NEXT_UPDATE_WEEKDAY,  EEPROM_MAXLEN_NEXT_UPDATE_WEEKDAY, EEPROM_MAXLEN_NEXT_UPDATE_WEEKDAY);
  eeprom.restoreRaw( (char*) &nextUpdateHour,    EEPROM_POS_NEXT_UPDATE_HOUR,     EEPROM_MAXLEN_NEXT_UPDATE_HOUR,    EEPROM_MAXLEN_NEXT_UPDATE_HOUR);
  eeprom.restoreRaw( (char*) &nextUpdateMinute,  EEPROM_POS_NEXT_UPDATE_MINUTE,   EEPROM_MAXLEN_NEXT_UPDATE_MINUTE,  EEPROM_MAXLEN_NEXT_UPDATE_MINUTE);
  eeprom.restoreRaw( (char*) &updateInterval,    EEPROM_POS_UPDATE_INTERVAL,      EEPROM_MAXLEN_UPDATE_INTERVAL,     EEPROM_MAXLEN_UPDATE_INTERVAL);
  eeprom.restoreRaw( (char*) &updateMode,        EEPROM_POS_UPDATE_MODE,          EEPROM_MAXLEN_UPDATE_MODE,         EEPROM_MAXLEN_UPDATE_MODE);

  return ( retVal );
}

//
// ------------------------------------------------------------------------
//
// ---------- reset uopdate information to default values
//
// ------------------------------------------------------------------------
//
void resetUpdateInfo2Default()
{
  updateUrl         = DEFAULT_UPDATE_URL;
  lastRelease       = DEFAULT_UPDATE_LAST_RELEASE;
  lastVersion       = DEFAULT_UPDATE_LAST_VERSION;
  lastPatchlevel    = DEFAULT_UPDATE_LAST_PATCHLEVEL;
  lastNumber        = DEFAULT_UPDATE_LAST_NUMBER;
  nextUpdateMonth   = DEFAULT_UPDATE_NEXT_MONTH;
  nextUpdateDay     = DEFAULT_UPDATE_NEXT_DAY;
  nextUpdateWeekDay = DEFAULT_UPDATE_NEXT_WEEKDAY;
  nextUpdateHour    = DEFAULT_UPDATE_NEXT_HOUR;
  nextUpdateMinute  = DEFAULT_UPDATE_NEXT_MINUTE;
  updateInterval    = DEFAULT_UPDATE_INTERVAL;
  updateMode        = DEFAULT_UPDATE_MODE;
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

// #ifdef DO_LOG
// if ( !beQuiet )
// {
//    Logger.Log(LOGLEVEL_DEBUG, "switch relais[%d] to %d\n", port, newStatus);
// }
// #endif // DO_LOG

#ifdef ESP_HAS_PCF8574
  PCF_38.write(port, newStatus);
#else
  digitalWrite( port, newStatus );
#endif // ESP_HAS_PCF8574

  if( newStatus == RELAIS_OFF)
  {
    tblEntry[port].actionFlag_1 &= ~ACTION_FLAG_ACTIVE;
  }
  else
  {
    tblEntry[port].actionFlag_1 |= ACTION_FLAG_ACTIVE;
  }

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

  utc = now();
  secsSinceEpoch = CE.toLocal(utc, &tcr);
  nowMinutes = ( hour(secsSinceEpoch) * 60 ) + minute(secsSinceEpoch);

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "nowMinutes is %d\n", nowMinutes );
  }
#endif // DO_LOG

  for ( i = 0; i < CONNECTED_RELAIS; i++ )
  {
    tblEntry[i].actionFlag_1 = (ACTION_FLAG_INACTIVE | ACTION_FLAG_NO_ACTION);
    tblEntry[i].actionFlag_2 = (ACTION_FLAG_INACTIVE | ACTION_FLAG_NO_ACTION);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "setting port %d to LOW!\n", i );
    }
#endif // DO_LOG

    switchRelais(i, RELAIS_OFF);

    if ( tblEntry[i].enabled_1 )
    {

      if ( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {

        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_1.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_1.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_1.c_str()) * 60) + atoi(tblEntry[i].minuteTo_1.c_str());

        if ( chkMinutesTo < chkMinutesFrom )
        {
          // time wrap
          wrapMinutes = (23 * 60) + 59;
#ifdef DO_LOG
          if ( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, (const char*) "wrap minutes time 1: %d\n", wrapMinutes);
          }
#endif // DO_LOG

          if ( nowMinutes < chkMinutesTo || nowMinutes < wrapMinutes )
          {
            //
            if ( nowMinutes > chkMinutesFrom )
            {
              // set this entry to active
#ifdef DO_LOG
              if ( !beQuiet )
              {
                Logger.Log(LOGLEVEL_DEBUG,
                           (const char*) "Current time(%d) is after Begin(%d) but before End(%d) of time 2 ... starting action\n",
                           nowMinutes, chkMinutesFrom, chkMinutesTo );
                Logger.Log(LOGLEVEL_DEBUG, (const char*) "switch port %d back to HIGH!\n", i );
              }
#endif // DO_LOG

              switchRelais(i, RELAIS_ON);
            }
          }
        }
        else
        {
          if ( nowMinutes >= chkMinutesFrom && nowMinutes < chkMinutesTo )
          {
#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG,
                         (const char*) "Current time(%d) is after Begin(%d) but before End(%d) of time 1 ... starting action\n",
                         nowMinutes, chkMinutesFrom, chkMinutesTo );
              Logger.Log(LOGLEVEL_DEBUG, (const char*) "switch port %d back to HIGH!\n", i );
            }
#endif // DO_LOG

            switchRelais(i, RELAIS_ON);
          }
        }
      }
      else // if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
        if ( tblEntry[i].mode.equalsIgnoreCase("on") )
        {
          alwaysOn(1, i);
        }
        else
        {
          alwaysOff(1, i);
        }
      }
    }

    if ( tblEntry[i].enabled_2 )
    {

      if ( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {

        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_2.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_2.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_2.c_str()) * 60) + atoi(tblEntry[i].minuteTo_2.c_str());

        if ( chkMinutesTo < chkMinutesFrom )
        {
          // time wrap
          wrapMinutes = (23 * 60) + 59;
#ifdef DO_LOG
          if ( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG, (const char*) "wrap minutes time 2: %d\n", wrapMinutes);
          }
#endif // DO_LOG

          if ( nowMinutes < chkMinutesTo || nowMinutes < wrapMinutes )
          {
            //
            if ( nowMinutes > chkMinutesFrom )
            {
              // set this entry to active
#ifdef DO_LOG
              if ( !beQuiet )
              {
                Logger.Log(LOGLEVEL_DEBUG,
                           (const char*) "Current time(%d) is after Begin(%d) but before End(%d) of time 2 ... starting action\n",
                           nowMinutes, chkMinutesFrom, chkMinutesTo );
                Logger.Log(LOGLEVEL_DEBUG, (const char*) "switch port %d back to HIGH!\n", i );
              }
#endif // DO_LOG

              switchRelais(i, RELAIS_ON);
            }
          }
        }
        else
        {
          if ( nowMinutes >= chkMinutesFrom && nowMinutes < chkMinutesTo )
          {
#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG,
                         (const char*) "Current time(%d) is after Begin(%d) but before End(%d) of time 2 ... starting action\n",
                         nowMinutes, chkMinutesFrom, chkMinutesTo );

              Logger.Log(LOGLEVEL_DEBUG, (const char*) "switch port %d back to HIGH!\n", i );
            }
#endif // DO_LOG

            switchRelais(i, RELAIS_ON);
          }
        }
      }
      else // if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
        if ( tblEntry[i].mode.equalsIgnoreCase("on") )
        {
          alwaysOn(2, i);
        }
        else
        {
          if ( tblEntry[i].mode.equalsIgnoreCase("off") )
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
#define FIRMWARE_CHECK   "0.0.1"
#define AUTO_RECONNECT_SECONDS   120
void setup()
{

  unsigned long crcCalc;
  unsigned long crcRead;
  unsigned long lastMillis;

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

#ifdef DO_LOG
  Logger.Log(LOGLEVEL_DEBUG, (const char*) "EEPROM_ACTION_TBL_ENTRY_START = %d\n", EEPROM_ACTION_TBL_ENTRY_START );
  Logger.Log(LOGLEVEL_DEBUG, (const char*) "EEPROM_ACTION_TBL_ENTRY_LENGTH = %d\n", EEPROM_ACTION_TBL_ENTRY_LENGTH );
  Logger.Log(LOGLEVEL_DEBUG, (const char*) "EEPROM_EXT_DATA_END = %d\n", EEPROM_EXT_DATA_END );
  Logger.Log(LOGLEVEL_DEBUG, (const char*) "Current firmware is = %s\n", FIRMWARE_CHECK );
#endif // DO_LOG

#ifdef ESP_HAS_PCF8574
  Wire.begin(default_sda_pin, default_scl_pin);
  //  Wire.begin();
  //  scanIIC();
#endif // ESP_HAS_PCF8574


  resetAdminSettings2Default();
  resetUpdateInfo2Default();
  resetActionTable2Default();

  resetActionFlags();

  if ( eeprom.init( EEPROM_EXT_DATA_END, eeprom.version2Magic(), LOGLEVEL_QUIET ) < EE_STATUS_INVALID_CRC )
//  if( eeprom.init( EEPROM_EXT_DATA_END, 0x00, LOGLEVEL_QUIET ) < EE_STATUS_INVALID_CRC )
  {
    if ( eeprom.isValid() )
    {
      analogWrite(GREEN, 0);
      analogWrite(BLUE, 127);

#ifdef DO_LOG
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "eeprom content is valid!\n");
#endif // DO_LOG

      crcCalc = eeprom.crc( EEPROM_STD_DATA_BEGIN, EEPROM_EXT_DATA_END );

      eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);

#ifdef DO_LOG
      if ( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, (const char*) "restored crc: %x calc crc: %x\n", crcRead, crcCalc);
      }
#endif // DO_LOG

      if ( (crcCalc == crcRead) )
      {
#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "crc MATCH!\n" );
        }
#endif // DO_LOG

        analogWrite(RED, 0 );
        analogWrite(GREEN, 127);
        analogWrite(BLUE, 0);

        restoreAdminSettings();
        restoreAddSysvars();
        restoreActionTable();
        restoreUpdateInfo();
      }
      else
      {
        analogWrite(RED, 127 );
        analogWrite(GREEN, 0);
        analogWrite(BLUE, 0);

        eeprom.wipe();
        resetAdminSettings2Default();
        resetUpdateInfo2Default();
        resetActionTable2Default();
        storeAdminSettings();
        storeAddSysvars();
        storeActionTable();
        storeUpdateInfo();
        eeprom.setMagic( eeprom.version2Magic() );
        eeprom.validate();
      }
    }
    else
    {
      analogWrite(RED, 127 );
      analogWrite(GREEN, 0);
      analogWrite(BLUE, 0);

      eeprom.wipe();
      resetAdminSettings2Default();
      resetUpdateInfo2Default();
      resetActionTable2Default();
      storeAdminSettings();
      storeAddSysvars();
      storeActionTable();
      storeUpdateInfo();
      eeprom.setMagic( eeprom.version2Magic() );
      eeprom.validate();
    }
  }
  else
  {
    analogWrite(RED, 127 );
    analogWrite(GREEN, 0);
    analogWrite(BLUE, 0);

    eeprom.wipe();
    resetAdminSettings2Default();
    resetUpdateInfo2Default();
    resetActionTable2Default();
    storeAdminSettings();
    storeAddSysvars();
    storeActionTable();
    storeUpdateInfo();
    eeprom.setMagic( eeprom.version2Magic() );
    eeprom.validate();
  }



#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "Connecting to >%s< using password >%s<\n", wlanSSID.c_str(), wlanPassphrase.c_str());
  }
#endif // DO_LOG

  WiFi.mode(WIFI_STA);

  lastMillis = millis();
  while ( millis() - lastMillis < AUTO_RECONNECT_SECONDS && WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  if ( WiFi.status() != WL_CONNECTED )
  {
    WiFi.begin(wlanSSID.c_str(), wlanPassphrase.c_str());
    while ( WiFi.status() != WL_CONNECTED )
    {
      delay(500);
    }
  }

  analogWrite(RED, 0 );
  analogWrite(GREEN, 127);
  analogWrite(BLUE, 0);

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "WiFi connected\n" );
  }
#endif // DO_LOG

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "Starting NTP over UDP\n");
  }
#endif // DO_LOG

  Udp.begin(localUDPPort);

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "Local port: %d ... waiting for time sync", Udp.localPort());
  }
#endif // DO_LOG

  // after syncing set initial time
  setSyncProvider(getNtpTime);
  setSyncInterval(SECS_PER_HOUR);
  setTime(now());

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "Time synced via NTP. Sync interval is set to %d seconds.\n", SECS_PER_HOUR );
  }
#endif // DO_LOG

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "Starting HTTP server\n");
  }
#endif // DO_LOG

  server = ESP8266WebServer( (unsigned long) wwwServerPort.toInt() );
  server.on("/", setupPage);
  server.on("/api", apiPage);

  server.begin();

  localIP = WiFi.localIP();
  wwwServerIP = localIP.toString();

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "Webserver started. URL is: http://%s:%d\n", wwwServerIP.c_str(), wwwServerPort.toInt());
  }
#endif // DO_LOG

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "Startup actions ...\n");
  }
#endif // DO_LOG

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
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
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

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "... check always on for port %d and timer = %d, flag #1 is %d, flag #2 is %d\n",
               port, timerNo, tblEntry[port].actionFlag_1, tblEntry[port].actionFlag_2);
  }
#endif // DO_LOG


  if ( timerNo == 1 )
  {
    if ( (tblEntry[port].actionFlag_1 & ACTION_FLAG_ALWAYS_ON) == 0 )
    {
      tblEntry[port].actionFlag_1 |= ACTION_FLAG_ALWAYS_ON;
      switchRelais(port, RELAIS_ON);

#ifdef DO_LOG
      if ( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, (const char*) "... entry for port %d time #1 is manual ALWAYS ON\n", port);
      }
#endif // DO_LOG

    }
  }
  else
  {
    if ( timerNo == 2 )
    {
      if ( (tblEntry[port].actionFlag_2 & ACTION_FLAG_ALWAYS_ON) == 0 )
      {
        tblEntry[port].actionFlag_2 |= ACTION_FLAG_ALWAYS_ON;
        switchRelais(port, RELAIS_ON);

#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "... entry for port %d time #2 is manual ALWAYS ON\n", port);
        }
#endif // DO_LOG

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

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "... check always off for port %d and timer = %d, flag #1 is %d, flag #2 is\n", port, timerNo, tblEntry[port].actionFlag_1, tblEntry[port].actionFlag_2);
  }
#endif // DO_LOG

  if ( timerNo == 1 )
  {
    if ( (tblEntry[port].actionFlag_1 & ACTION_FLAG_ALWAYS_OFF) == 0 )
    {
      tblEntry[port].actionFlag_1 |= ACTION_FLAG_ALWAYS_OFF;
      switchRelais(port, RELAIS_OFF);

#ifdef DO_LOG
      if ( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, (const char*) "... entry for port %d time #1 is manual ALWAYS OFF\n", port);
      }
#endif // DO_LOG

    }
  }
  else
  {
    if ( timerNo == 2 )
    {
      if ( (tblEntry[port].actionFlag_2 & ACTION_FLAG_ALWAYS_OFF) == 0 )
      {
        tblEntry[port].actionFlag_2 |= ACTION_FLAG_ALWAYS_OFF;
        switchRelais(port, RELAIS_OFF);

#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "... entry for port %d time #2 is manual ALWAYS OFF\n", port);
        }
#endif // DO_LOG

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

  utc = now();
  secsSinceEpoch = CE.toLocal(utc, &tcr);
  nowMinutes = ( hour(secsSinceEpoch) * 60 ) + minute(secsSinceEpoch);

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "check4Action: check for minute-value: %d\n", nowMinutes );
  }
#endif // DO_LOG

  //  for( i = 0; i < MAX_ACTION_TABLE_LINES; i++ )
  for ( i = 0; i < CONNECTED_RELAIS; i++ )
  {


#ifdef DO_LOG
  if ( !beQuiet )
  {
Logger.Log(LOGLEVEL_DEBUG," [%d] Enable: %d, mode: %s, hr1from: %s min1from: %s hr1to: %s min1to: %s hr2from: %s min2from: %s hr2to: %s min2to: %s \n",
	i, tblEntry[i].enabled_1, tblEntry[i].mode.c_str(), tblEntry[i].hourFrom_1.c_str(), tblEntry[i].minuteFrom_1.c_str(),
tblEntry[i].hourTo_1.c_str(), tblEntry[i].minuteTo_1.c_str(), tblEntry[i].hourFrom_2.c_str(), tblEntry[i].minuteFrom_2.c_str(),
tblEntry[i].hourTo_2.c_str(), tblEntry[i].minuteTo_2.c_str() ); 
  }
#endif // DO_LOG

    if ( tblEntry[i].enabled_1 )
    {

      if ( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {

        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_1.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_1.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_1.c_str()) * 60) + atoi(tblEntry[i].minuteTo_1.c_str());

        if ( chkMinutesFrom == nowMinutes )
        {
#ifdef DO_LOG
          if ( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG,
                       (const char*) "action is time 1 - Begin %s:%s [%d] port %d. Actionflag is %d\n",
                       tblEntry[i].hourFrom_1.c_str(), tblEntry[i].minuteFrom_1.c_str(),
                       chkMinutesFrom, i, tblEntry[i].actionFlag_1);
          }
#endif // DO_LOG

          if ( (tblEntry[i].actionFlag_1 & ACTION_FLAG_ACTIVE) == 0 )
          {
            tblEntry[i].actionFlag_1 |= ACTION_FLAG_ACTIVE;

            switchRelais(i, RELAIS_ON);

#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, (const char*) "set actionFlag for time 1 to active\n");
            }
#endif // DO_LOG
          }
        }

        if (chkMinutesTo == nowMinutes )
        {
#ifdef DO_LOG
          if ( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG,
                       (const char*) "action is time 1 - End %s:%s [%d] port %d. Actionflag is %d\n",
                       tblEntry[i].hourTo_1.c_str(), tblEntry[i].minuteTo_1.c_str(),
                       chkMinutesTo, i, tblEntry[i].actionFlag_1);
          }
#endif // DO_LOG

          if ( (tblEntry[i].actionFlag_1 & ACTION_FLAG_ACTIVE) != 0 )
          {
            switchRelais(i, RELAIS_OFF);

#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, (const char*) "set actionFlag for time 1 to inactive\n");
            }
#endif // DO_LOG
          }
          else
          {
#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, (const char*) "nothing to do ... actionFlag for time 1 is already inactive\n");
            }
#endif // DO_LOG
          }
        }
      }
      else // if( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "... entry for port %d time #1 is not auto\n", i);
        }
#endif // DO_LOG

        if ( tblEntry[i].mode.equalsIgnoreCase("on") )
        {
          alwaysOn(1, i);
        }
        else
        {
          if ( tblEntry[i].mode.equalsIgnoreCase("off") )
          {
            alwaysOff(1, i);
          }
        }
      }
    }
    else // if( tblEntry[i].enabled_1 )
    {
#ifdef DO_LOG
      if ( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, (const char*) "nothing to do ... entry for port %d time #1 disabled\n", i);
      }
#endif // DO_LOG
    }

    if ( tblEntry[i].enabled_2 )
    {
      if ( tblEntry[i].mode.equalsIgnoreCase("auto") )
      {
        chkMinutesFrom = (atoi(tblEntry[i].hourFrom_2.c_str()) * 60) + atoi(tblEntry[i].minuteFrom_2.c_str());
        chkMinutesTo = (atoi(tblEntry[i].hourTo_2.c_str()) * 60) + atoi(tblEntry[i].minuteTo_2.c_str());

        if (chkMinutesFrom == nowMinutes )
        {
#ifdef DO_LOG
          if ( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG,
                       (const char*) "action is time 2 - Begin %s:%s [%d] port %d. Actionflag is %d\n",
                       tblEntry[i].hourFrom_2.c_str(), tblEntry[i].minuteFrom_2.c_str(),
                       chkMinutesFrom, i, tblEntry[i].actionFlag_2);
          }
#endif // DO_LOG

          if ( (tblEntry[i].actionFlag_2 & ACTION_FLAG_ACTIVE) == 0 )
          {
            switchRelais(i, RELAIS_ON);

#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, (const char*) "set actionFlag for time 2 to active\n");
            }
#endif // DO_LOG
          }
        }

        if (chkMinutesTo == nowMinutes )
        {
#ifdef DO_LOG
          if ( !beQuiet )
          {
            Logger.Log(LOGLEVEL_DEBUG,
                       (const char*) "action is time 1 - End %s:%s [%d] port %d. Actionflag is %d\n",
                       tblEntry[i].hourTo_2.c_str(), tblEntry[i].minuteTo_2.c_str(),
                       chkMinutesTo, i, tblEntry[i].actionFlag_2);
          }
#endif // DO_LOG

          if ( (tblEntry[i].actionFlag_2 & ACTION_FLAG_ACTIVE) != 0 )
          {
            switchRelais(i, RELAIS_OFF);

#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, (const char*) "set actionFlag for time 2 to inactive\n");
            }
#endif // DO_LOG
          }
          else
          {
#ifdef DO_LOG
            if ( !beQuiet )
            {
              Logger.Log(LOGLEVEL_DEBUG, (const char*) "nothing to do ... actionFlag for time 2 is already inactive\n");
            }
#endif // DO_LOG
          }
        }
      }
      else // if( tblEntry[tmRow-1].mode.equalsIgnoreCase("auto") )
      {
#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "... entry for port %d time #2 is not auto\n", i);
        }
#endif // DO_LOG

        if ( tblEntry[i].mode.equalsIgnoreCase("on") )
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
#ifdef DO_LOG
      if ( !beQuiet )
      {
        Logger.Log(LOGLEVEL_DEBUG, (const char*) "nothing to do ... entry for port %d time #2 disabled\n", i);
      }
#endif // DO_LOG
    }
  }

  return ( retVal );
}

//
// ************************************************************************
//
// ---------- int handleUpdate()
//
// ************************************************************************
//
int handleUpdate( void )
{
  static t_httpUpdate_return retVal;
  String newFirmwareFile, markerFile;
  HTTPClient http;
  int httpCode;

  //    ESPhttpUpdate.rebootOnUpdate( true );

  newFirmwareFile = updateUrl + String("firmware.bin") + String(lastNumber + 1);
  markerFile = updateUrl + String("firmware.") + String(lastNumber + 1);

  http.begin(markerFile.c_str());
  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "marker file %s exists\n", markerFile.c_str());
    }
#endif // DO_LOG

    http.end();
    lastNumber++;
    storeUpdateInfo();

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "Handle update, next firmware is: %s\n", newFirmwareFile.c_str());
    }
#endif // DO_LOG
    retVal = ESPhttpUpdate.update(newFirmwareFile.c_str());

    //        retVal = ESPhttpUpdate.update(newFirmwareFile.c_str());

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "Update done retval was %x\n", retVal);
    }
#endif // DO_LOG


    //        switch( (retVal = ESPhttpUpdate.update(newFirmwareFile.c_str())) )
    switch ( retVal )
    {
      case HTTP_UPDATE_FAILED:
#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "HTTP_UPDATE_FAILED! Error (%d): %s\n",
                     ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        }
#endif // DO_LOG
        break;
      case HTTP_UPDATE_NO_UPDATES:
        //                lastNumber++;
        //                storeUpdateInfo();
#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "HTTP_UPDATE_NO_UPDATES\n");
        }
#endif // DO_LOG
        break;
      case HTTP_UPDATE_OK:
        //                lastNumber++;
        //                storeUpdateInfo();
#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "HTTP_UPDATE_OK - lastNumber = %d\n", lastNumber);
        }
#endif // DO_LOG
        break;
      default:
#ifdef DO_LOG
        if ( !beQuiet )
        {
          Logger.Log(LOGLEVEL_DEBUG, (const char*) "Unknown retval %d\n", (int) retVal);
        }
#endif // DO_LOG
        break;
    }
  }
  else
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "NO marker file %s\n", markerFile.c_str());
    }
#endif // DO_LOG

    http.end();
  }

  return ((int) retVal);
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
  static short lastUpdateCheck;
  time_t secsSinceEpoch;
  short nowMinutes;
  static short minutesLastCheck;
  time_t utc;

  utc = now();
  secsSinceEpoch = CE.toLocal(utc, &tcr);
  nowMinutes = ( hour(secsSinceEpoch) * 60 ) + minute(secsSinceEpoch);

  // the weekday now (Sunday is day 1)
  if ( weekday(secsSinceEpoch) != lastUpdateCheck )
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "loop: day has changed from %d to %d\n",
                 lastUpdateCheck, weekday(secsSinceEpoch));
    }
#endif // DO_LOG

    //    if( handleUpdate() == HTTP_UPDATE_OK )
    //    {
    //        HTTP_UPDATE_FAILED
    //        HTTP_UPDATE_NO_UPDATES

    //      lastUpdateCheck = weekday(secsSinceEpoch);
    //    }
  }

  server.handleClient();

  if ( nowMinutes > minutesLastCheck )
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "loop: hour is %d, minutes is %d\n", hour(secsSinceEpoch), minute(secsSinceEpoch) );
    }
#endif // DO_LOG

    minutesLastCheck = nowMinutes;
    check4Action();
    delay(2);

//    handleUpdate(); // <- RAUS!!
  }
  else
  {
    delay(5);
  }


}


/* ************** ++++++++++ ************* +++++++++++++++ */

void doTimeOnSelect( char *sTarget, char *sSwitch, char *sBgColor, int tmNum, char* sRow, const char *sHour, const char* sMinute )
{
  char sNum[10];
  sprintf(sNum, "%d", tmNum);

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log( LOGLEVEL_DEBUG, (const char*) "sMinute is %s\n", sMinute );
  }
#endif // DO_LOG

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
  if ( tmNum == 1 )
  {
    doTimeOnSelect( "from", "einschalten", "#07F479", tmNum, sRow,
                    tblEntry[tmRow - 1].hourFrom_1.c_str(), tblEntry[tmRow - 1].minuteFrom_1.c_str() );

    doTimeOnSelect( "to", "ausschalten", "#D4C9C9" , tmNum, sRow,
                    tblEntry[tmRow - 1].hourTo_1.c_str(), tblEntry[tmRow - 1].minuteTo_1.c_str() );
  }
  else
  {
    if ( tmNum == 2 )
    {
      doTimeOnSelect( "from", "einschalten", "#07F479", tmNum, sRow,
                    tblEntry[tmRow - 1].hourFrom_2.c_str(), tblEntry[tmRow - 1].minuteFrom_2.c_str() );

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "doTimeSelect -> tblEntry[%d].minuteFrom_2.c_str(): %s\n",
               (tmRow - 1), tblEntry[tmRow - 1].minuteFrom_2.c_str() );
  }
#endif // DO_LOG

      doTimeOnSelect( "to", "ausschalten", "#D4C9C9" , tmNum, sRow,
                    tblEntry[tmRow - 1].hourTo_2.c_str(), tblEntry[tmRow - 1].minuteTo_2.c_str() );
#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "doTimeSelect -> tblEntry[%d].minuteTo_2.c_str(): %s\n",
               (tmRow - 1), tblEntry[tmRow - 1].minuteTo_2.c_str() );
  }
#endif // DO_LOG

    }
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
  pageContent += tblEntry[tmRow - 1].name;
  pageContent += "\" size=\"4\" maxlength=\"10\">\n";
  //
  pageContent += "</td>\n";

  // ---- ++++++++++ ZEIT 1 enable/disable ++++++++++ -----

  pageContent += "<td bgcolor=\"#09CEF3\">\n";
  //
  pageContent += "<input type=\"checkbox\" name=\"enabled1_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\" ";
  if ( tblEntry[tmRow - 1].enabled_1 )
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
  pageContent += " <input type=\"checkbox\" name=\"enabled2_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\" ";
  if ( tblEntry[tmRow - 1].enabled_2 )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\" >\n";
  //
  pageContent += "<label>Zeit 2</label>\n";
  pageContent += "</td>\n";
  doTimeSelect( 2, tmRow, sRow);

  pageContent += "<td>\n";

  // ---- ++++++++++ EXT 1 enable/disable ++++++++++ -----

  pageContent += "<input type=\"checkbox\" name=\"ext1_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\" ";
  if ( tblEntry[tmRow - 1].extEnable_1 )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  //
  pageContent += "<label>Ext 1</label>\n";
  pageContent += "<br>\n";

  // ---- ++++++++++ EXT 2 enable/disable ++++++++++ -----

  pageContent += "<input type=\"checkbox\" name=\"ext2_";
  pageContent += sRow;
  pageContent += "\" value=\"aktiv\" ";
  if ( tblEntry[tmRow - 1].extEnable_2 )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent += "<label>Ext 2</label>";

  pageContent += "</td>";
  pageContent += "<td>\n";

  // ---- ++++++++++ MODE ON ++++++++++ -----

  pageContent += "<input type=\"radio\" name=\"mode";
  pageContent += sRow;
  pageContent += "\" value=\"on\" ";
  if ( tblEntry[tmRow - 1].mode.equalsIgnoreCase("on") )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent += "<label>Ein</label>\n";
  pageContent += "<br>\n";

  // ---- ++++++++++ MODE OFF ++++++++++ -----

  pageContent += "<input type=\"radio\" name=\"mode";
  pageContent += sRow;
  pageContent += "\" value=\"off\" ";
  if ( tblEntry[tmRow - 1].mode.equalsIgnoreCase("off") )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent += "<label>Aus</label>\n";
  pageContent += "<br>\n";

  // ---- ++++++++++ MODE AUTO ++++++++++ -----

  pageContent += "<input type=\"radio\" name=\"mode";
  pageContent += sRow;
  pageContent += "\" value=\"auto\" ";
  if ( tblEntry[tmRow - 1].mode.equalsIgnoreCase("auto") )
  {
    pageContent += " checked ";
  }
  pageContent += "size=\"1\">\n";
  pageContent += "<label>Auto</label>\n";
  // ---------
  pageContent += "</td>\n";

  // end Zeile ...

}


void setupPage()
{

  int i;
  IPAddress localIP;

  pageContent = "";
  localIP = WiFi.localIP();

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "handleIndexPage\n");
  }
#endif // DO_LOG

  for (i = 0; i < server.args(); i++ )
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
    }
#endif // DO_LOG
  }

  if ( server.method() == SERVER_METHOD_POST )
    //        server.hasArg(INDEX_BUTTONNAME_ADMIN)  )
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "POST REQUEST\n");
    }
#endif // DO_LOG
  }
  else
  {
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "GET REQUEST\n");
    }
#endif // DO_LOG
  }

  if ( server.hasArg("submit") && server.arg("submit").equalsIgnoreCase("speichern") )

  {
    // for( i = 0; i < MAX_ACTION_TABLE_LINES; i++ )
    for ( i = 0; i < CONNECTED_RELAIS; i++ )
    {

      formFieldName[KW_IDX_BEZEICHNER] = String("bezeichner") + String(i+1);
      formFieldName[KW_IDX_ENABLED_1]  = String("enabled1_")  + String(i+1);
      formFieldName[KW_IDX_HFROM_1]    = String("hfrom1_")    + String(i+1);
      formFieldName[KW_IDX_MFROM_1]    = String("mfrom1_")    + String(i+1);
      formFieldName[KW_IDX_HTO_1]      = String("hto1_")      + String(i+1);
      formFieldName[KW_IDX_MTO_1]      = String("mto1_")      + String(i+1);
      formFieldName[KW_IDX_ENABLED_2]  = String("enabled2_")  + String(i+1);
      formFieldName[KW_IDX_HFROM_2]    = String("hfrom2_")    + String(i+1);
      formFieldName[KW_IDX_MFROM_2]    = String("mfrom2_")    + String(i+1);
      formFieldName[KW_IDX_HTO_2]      = String("hto2_")      + String(i+1);
      formFieldName[KW_IDX_MTO_2]      = String("mto2_")      + String(i+1);
      formFieldName[KW_IDX_EXT_1]      = String("ext1_")      + String(i+1);
      formFieldName[KW_IDX_EXT_2]      = String("ext2_")      + String(i+1);
      formFieldName[KW_IDX_MODE]       = String("mode")       + String(i+1);


      tblEntry[i].name         = server.arg(formFieldName[KW_IDX_BEZEICHNER]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].name         = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_BEZEICHNER].c_str(), server.arg(formFieldName[KW_IDX_BEZEICHNER]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].mode         = server.arg(formFieldName[KW_IDX_MODE]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].mode         = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_MODE].c_str(), server.arg(formFieldName[KW_IDX_MODE]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].hourFrom_1   = server.arg(formFieldName[KW_IDX_HFROM_1]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*)"tblEntry[%d].hourFrom_1    = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_HFROM_1].c_str(), server.arg(formFieldName[KW_IDX_HFROM_1]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].minuteFrom_1 = server.arg(formFieldName[KW_IDX_MFROM_1]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].minuteFrom_1 = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_MFROM_1].c_str(), server.arg(formFieldName[KW_IDX_MFROM_1]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].hourTo_1     = server.arg(formFieldName[KW_IDX_HTO_1]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*)"tblEntry[%d].hourTo_1      = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_HTO_1].c_str(), server.arg(formFieldName[KW_IDX_HTO_1]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].minuteTo_1   = server.arg(formFieldName[KW_IDX_MTO_1]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].minuteTo_1   = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_MTO_1].c_str(), server.arg(formFieldName[KW_IDX_MTO_1]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].hourFrom_2   = server.arg(formFieldName[KW_IDX_HFROM_2]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].hourFrom_2   = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_HFROM_2].c_str(), server.arg(formFieldName[KW_IDX_HFROM_2]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].minuteFrom_2 = server.arg(formFieldName[KW_IDX_MFROM_2]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].minuteFrom_2 = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_MFROM_2].c_str(), server.arg(formFieldName[KW_IDX_MFROM_2]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].hourTo_2     = server.arg(formFieldName[KW_IDX_HTO_2]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].hourTo_2     = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_HTO_2].c_str(), server.arg(formFieldName[KW_IDX_HTO_2]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].minuteTo_2   = server.arg(formFieldName[KW_IDX_MTO_2]);

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].minuteTo_2   = server.arg(\"%s\") ->%s\n", 
                  i, formFieldName[KW_IDX_MTO_2].c_str(), server.arg(formFieldName[KW_IDX_MTO_2]).c_str());
    }
#endif // DO_LOG

      tblEntry[i].enabled_1   = server.arg(formFieldName[KW_IDX_ENABLED_1]).equalsIgnoreCase("aktiv") ? true : false;

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].enabled_1   = server.arg(\"%s\") -> %d\n", 
                  i, formFieldName[KW_IDX_ENABLED_1].c_str(), tblEntry[i].enabled_1);
    }
#endif // DO_LOG
      
      tblEntry[i].extEnable_1 = server.arg(formFieldName[KW_IDX_EXT_1]).equalsIgnoreCase("aktiv") ? true : false;

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].extEnable_1 = server.arg(\"%s\") -> %d\n", 
                  i, formFieldName[KW_IDX_EXT_1].c_str(), tblEntry[i].extEnable_1);
    }
#endif // DO_LOG
            
      tblEntry[i].enabled_2   = server.arg(formFieldName[KW_IDX_ENABLED_2]).equalsIgnoreCase("aktiv") ? true : false;

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].enabled_2   = server.arg(\"%s\") -> %d\n", 
                  i, formFieldName[KW_IDX_ENABLED_2].c_str(), tblEntry[i].enabled_2);
    }
#endif // DO_LOG
      
      tblEntry[i].extEnable_2 = server.arg(formFieldName[KW_IDX_EXT_2]).equalsIgnoreCase("aktiv") ? true : false;

#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log( LOGLEVEL_DEBUG, (const char*) "tblEntry[%d].extEnable_2 = server.arg(\"%s\") -> %d\n", 
                  i, formFieldName[KW_IDX_EXT_2].c_str(), tblEntry[i].extEnable_2);
    }
#endif // DO_LOG
      

    }

    storeActionTable();
    eeprom.validate();

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


  pageContent += "<div align=\"center\"><strong><h1>" + String(localIP.toString()) + "</h1></strong></div>\n";

  pageContent += "<form action=\"/\" method=\"get\">\n";
  pageContent += "<hr align=\"center\"><br>\n";
  pageContent += "<table border align=\"center\">\n";

  // Zeile 1
  pageContent += "<tr>\n";
  doCreateLine( 1 );
  pageContent += "</tr>\n";

  // Zeile 2
  pageContent += "<tr>\n";
  doCreateLine( 2 );
  pageContent += "</tr>\n";

  // Zeile 3
  pageContent += "<tr>\n";
  doCreateLine( 3 );
  pageContent += "</tr>\n";

  // Zeile 4
  pageContent += "<tr>\n";
  doCreateLine( 4 );
  pageContent += "</tr>\n";

  // Zeile 5
  pageContent += "<tr>\n";
  doCreateLine( 5 );
  pageContent += "</tr>\n";

  // Zeile 6
  pageContent += "<tr>\n";
  doCreateLine( 6 );
  pageContent += "</tr>\n";

  // Zeile 7
  pageContent += "<tr>\n";
  doCreateLine( 7 );
  pageContent += "</tr>\n";

  // Zeile 8
  pageContent += "<tr>\n";
  doCreateLine( 8 );
  pageContent += "</tr>\n";

  pageContent += "</table>\n";
  pageContent += "<hr align=\"center\">\n";
  pageContent += "<div align=\"center\">\n";
  pageContent += "<input type=\"submit\" name=\"submit\" value=\"speichern\">\n";
  pageContent += "<input type=\"reset\" name=\"reset\" value=\"reset\">\n";
  pageContent += "<input type=\"submit\" name=\"ESP\" value=\"reboot\">\n";
  pageContent += "</div>\n";
  pageContent += "<hr align=\"center\"><br>\n";
  pageContent += "</form>\n";
  pageContent += "</body>\n";
  pageContent += "</html>\n";

  server.send(200, "text/html", pageContent);

  //  Serial.print(pageContent.c_str());

}

//
// ************************************************************************
//
// ---------- void apiPage()
//
// ************************************************************************
//

void apiPage()
{

  int i;
  pageContent = "";


  pageContent += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";
  pageContent += "<html>\n";
  pageContent += "<head>\n";
  pageContent += "<meta charset=\"utf-8\">\n";
  pageContent += "<title>Web-API</title>\n";
  pageContent += "</head>\n";
  pageContent += "<body bgcolor=\"#D4C9C9\" text=\"#000000\" link=\"#1E90FF\" vlink=\"#0000FF\">\n";

#ifdef DO_LOG
  if ( !beQuiet )
  {
    Logger.Log(LOGLEVEL_DEBUG, (const char*) "apiPage<br>\n");
  }
#endif // DO_LOG

  pageContent += String("apiPage<br>\n");

  for (i = 0; i < server.args(); i++ )
  {
    pageContent += server.argName(i) + String("=") + server.arg(i) + String("<br>\n");
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "%s = %s<br>\n", server.argName(i).c_str(), server.arg(i).c_str() );
    }
#endif // DO_LOG
  }

  if ( server.method() == SERVER_METHOD_POST )
    //        server.hasArg(INDEX_BUTTONNAME_ADMIN)  )
  {
    pageContent += String("POST REQUEST<br>\n");
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "POST REQUEST<br>\n");
    }
#endif // DO_LOG
  }
  else
  {
    pageContent += String("GET REQUEST<br>\n");
#ifdef DO_LOG
    if ( !beQuiet )
    {
      Logger.Log(LOGLEVEL_DEBUG, (const char*) "GET REQUEST<br>\n");
    }
#endif // DO_LOG
  }

  pageContent += "</body>\n";
  pageContent += "</html>\n";

  server.send(200, "text/html", pageContent);
}

/* ************** ++++++++++ ************* +++++++++++++++ */

#ifdef HTTP_CLIENT
HTTPClient http;
http.begin("http://192.168.0.104:8080/webappfordemo/Version");
int httpCode = http.GET();
if (httpCode == HTTP_CODE_OK)
{
  Serial.print("HTTP response code ");
  Serial.println(httpCode);
  String response = http.getString();
  Serial.println(response);
}

// We now create a URI for the request
String url = "/stan";

Serial.print("Requesting URL: ");
Serial.println(url);

// This will send the request to the server
client.print(String("GET ") + url + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "Connection: close\r\n\r\n");
delay(10);

// Read all the lines of the reply from server and print them to Serial
Serial.println("Respond:");
while (client.available()) {
  String line = client.readStringUntil('\r');
  Serial.print(line);
}
#endif // HTTP_CLIENT

