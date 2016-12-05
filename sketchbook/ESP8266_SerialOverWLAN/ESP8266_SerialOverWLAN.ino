//
// ************************************************************************
// Connect serial devices (rs232) over WLAN
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
// For witty breakout board:
//   select "NodeMCU 1.0 (ESP-12E Module)" as board in the Arduino IDE 
//
// witty pins for integrated RGB LED:
//     const int RED = 15;
//     const int GREEN = 12;
//     const int BLUE = 13;
//
//
//-------- History -------------------------------------------------
//
// 1st version: 
// update:
//
// ************************************************************************
// program flow
// ************************************************************************
//  
// ---- ignore if condition ------------------------------------------------
// ignore EEPROM checks - process data at all even it is not necessary
#define IGNORE_IF_CONDITION              1
//  
// ---- suppress debug output to serial line -------------------------------
// be quiet = no output
#define BE_QUIET              false
//
//
// ************************************************************************
// error codes
// ************************************************************************
//  
#define E_SUCCESS                         0
#define E_BAD_CRC                        -1
#define E_INVALID_MAGIC                  -2

//
// ************************************************************************
// include necessary lib header
// ************************************************************************
//
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "ioStreams.h"
//
// ************************************************************************
// defines for factory settings
// ************************************************************************
//
// node setting defaults
// 
#define FACTORY_WWW_SERVER_IP            ""
#define FACTORY_WWW_LISTENPORT           "8080"
#define FACTORY_WLAN_SSID                ""
#define FACTORY_WLAN_PASSPHRASE          ""
#define FACTORY_NODENAME                 "espnode"
#define FACTORY_ADMIN_PASSWORD           "esp8266"
#define FACTORY_USE_DHCPD_4IP            true
//
// COM setting defaults
// 
#define FACTORY_COM_SON_ENABLED          true
#define FACTORY_COM_SERVER_MODE          true
#define FACTORY_COM_PROTOCOL_TCP         true

#define FACTORY_COM_TARGET_IP            ""

#define FACTORY_USE_HW_SERIAL            true
#define FACTORY_USE_RX_GPIO              "00"
#define FACTORY_USE_TX_GPIO              "00"
#define FACTORY_USE_DATABITS             "8"
#define FACTORY_USE_STOPBITS             "1"
#define FACTORY_USE_FLOWCTRL             false
#define FACTORY_USE_PARITY               "N"
#define FACTORY_USE_BAUDRATE             "19200"
//
// ************************************************************************
// define field names of the various forms
// ************************************************************************
//
// /admin page -----------------------------------------------------------
//
#define ADMIN_FIELDNAME_SSID             "ssid"
#define ADMIN_FIELDNAME_PASSPHRASE       "pass"
#define ADMIN_CHKNAME_USEDHCPD           "usedhcp"
#define ADMIN_FIELDNAME_SRVPORT          "srvport"
#define ADMIN_FIELDNAME_NODENAME         "node"
#define ADMIN_FIELDNAME_ADMINPW          "adminpw"
#define ADMIN_FIELDNAME_SERVER_IP        "serverip"

#define ADMIN_OPTGRP_LABEL_SSID          "SSID:"
#define ADMIN_OPTGRP_LABEL_PASSPHRASE    "Passphrase:"
#define ADMIN_OPTGRP_LABEL_USE_DHCP      "Receive IP address via dhcp"
#define ADMIN_OPTGRP_LABEL_SERVER_IP     "Server IP:"
#define ADMIN_OPTGRP_LABEL_SERVER_PORT   "WWW server port:"
#define ADMIN_OPTGRP_LABEL_NODENAME      "Node name:"
#define ADMIN_OPTGRP_LABEL_ADMIN_PASSWD  "Admin password:"
//
#define ADMIN_ACTION_COMMIT              "commit settings"
#define ADMIN_ACTION_RESET               "reset form"
#define ADMIN_ACTION_FACTORY             "factory settings"
#define ADMIN_ACTION_CLOSE               "close"
#define ADMIN_ACTION_COMSETTINGS         "COM port settings"
#define ADMIN_ACTION_RESTART             "restart node"
//
#define ADMIN_BUTTONNAME_COMMIT          "ok"
#define ADMIN_BUTTONNAME_FORMRESET       "rst"
#define ADMIN_BUTTONNAME_FACTORY         "factory"
#define ADMIN_BUTTONNAME_CLOSE           "close"
#define ADMIN_BUTTONNAME_COMSETTINGS     "comsetup"
#define ADMIN_BUTTONNAME_RESTART         "restart"

//
// /comsettings page -----------------------------------------------------------
//
#define COM_OPTGRP_LABEL_SON             "Serial over network"
#define COM_LABEL_ENABLE_SON             "enable"
#define COM_LABEL_DISABLE_SON            "disable"
#define COM_RADIONAME_SON                "SONStatus"
#define COM_RADIO_SON_ENABLED            "ON"
#define COM_RADIO_SON_DISABLED           "OFF"

#define COM_OPTGRP_LABEL_OPERATIONMODE   "Operation mode:"
#define COM_OPTGRP_LABEL_PROTOCOL        "Connection protocol:"

#define COM_RADIO_OPERATIONMODE_CLIENT   "C"
#define COM_RADIO_OPERATIONMODE_SERVER   "S"
#define COM_LABEL_OPERATIONMODE_CLIENT   "COM-Client"
#define COM_LABEL_OPERATIONMODE_SERVER   "COM-Server"

#define COM_RADIO_PROTOCOL_TCP           'T'
#define COM_RADIO_PROTOCOL_UDP           'U'
#define COM_LABEL_PROTOCOL_TCP           "TCP"
#define COM_LABEL_PROTOCOL_UDP           "UDP"

#define COM_TARGET_IP                    "targetIP"
#define COM_LABEL_TARGET_IP              "Target IP:"

// type of serial communication
#define COM_RADIONAME_OPERATIONMODE      "opmode"
#define COM_RADIONAME_PROTOCOL           "protocol"
#define COM_OPTGRP_LABEL_SERIAL_TYPE     "Serial type*"
#define COM_RADIONAME_COM_TYPE           "comtype"
#define COM_RADIO_COM_TYPE_HW            "hardware"
#define COM_LABEL_COM_TYPE_HW            "Hardware"
#define COM_RADIO_COM_TYPE_SW            "software"
#define COM_LABEL_COM_TYPE_SW            "Software"

// Pin to use as Rx
#define COM_OPTGRP_LABEL_GPIO_RX         "GPIO to use as Rx"
#define COM_RADIONAME_GPIO_RX            "rxGPIO"
#define COM_RADIO_GPIO_RX_00             "00"
#define COM_LABEL_GPIO_RX_00             "GPIO 00"
#define COM_RADIO_GPIO_RX_02             "02"
#define COM_LABEL_GPIO_RX_02             "GPIO 02"
#define COM_RADIO_GPIO_RX_04             "04"
#define COM_LABEL_GPIO_RX_04             "GPIO 04"
#define COM_RADIO_GPIO_RX_05             "05"
#define COM_LABEL_GPIO_RX_05             "GPIO 05"
#define COM_RADIO_GPIO_RX_09             "09"
#define COM_LABEL_GPIO_RX_09             "GPIO 09"
#define COM_RADIO_GPIO_RX_10             "10"
#define COM_LABEL_GPIO_RX_10             "GPIO 10"
#define COM_RADIO_GPIO_RX_12             "12"
#define COM_LABEL_GPIO_RX_12             "GPIO 12"
#define COM_RADIO_GPIO_RX_13             "13"
#define COM_LABEL_GPIO_RX_13             "GPIO 13"
#define COM_RADIO_GPIO_RX_14             "14"
#define COM_LABEL_GPIO_RX_14             "GPIO 14"
#define COM_RADIO_GPIO_RX_15             "15"
#define COM_LABEL_GPIO_RX_15             "GPIO 15"
#define COM_RADIO_GPIO_RX_16             "16"
#define COM_LABEL_GPIO_RX_16             "GPIO 16"

// Pin to use as Tx
#define COM_OPTGRP_LABEL_GPIO_TX         "GPIO to use as Tx"
#define COM_RADIONAME_GPIO_TX            "txGPIO"
#define COM_RADIO_GPIO_TX_00             "00"
#define COM_LABEL_GPIO_TX_00             "GPIO 00"
#define COM_RADIO_GPIO_TX_02             "02"
#define COM_LABEL_GPIO_TX_02             "GPIO 02"
#define COM_RADIO_GPIO_TX_04             "04"
#define COM_LABEL_GPIO_TX_04             "GPIO 04"
#define COM_RADIO_GPIO_TX_05             "05"
#define COM_LABEL_GPIO_TX_05             "GPIO 05"
#define COM_RADIO_GPIO_TX_09             "09"
#define COM_LABEL_GPIO_TX_09             "GPIO 09"
#define COM_RADIO_GPIO_TX_10             "10"
#define COM_LABEL_GPIO_TX_10             "GPIO 10"
#define COM_RADIO_GPIO_TX_12             "12"
#define COM_LABEL_GPIO_TX_12             "GPIO 12"
#define COM_RADIO_GPIO_TX_13             "13"
#define COM_LABEL_GPIO_TX_13             "GPIO 13"
#define COM_RADIO_GPIO_TX_14             "14"
#define COM_LABEL_GPIO_TX_14             "GPIO 14"
#define COM_RADIO_GPIO_TX_15             "15"
#define COM_LABEL_GPIO_TX_15             "GPIO 15"
#define COM_RADIO_GPIO_TX_16             "16"
#define COM_LABEL_GPIO_TX_16             "GPIO 16"

// Charcter width
#define COM_OPTGRP_LABEL_DATALEN         "Data bits"
#define COM_RADIONAME_DATALEN            "data"
#define COM_RADIO_DATALEN_SIX            "6"
#define COM_RADIO_DATALEN_SEVEN          "7"
#define COM_RADIO_DATALEN_EIGHT          "8"

#define COM_RADIO_LBL_DATALEN_6BIT       "6 bit"
#define COM_RADIO_LBL_DATALEN_7BIT       "7 bit"
#define COM_RADIO_LBL_DATALEN_8BIT       "8 bit"

// Stop bits
#define COM_OPTGRP_LABEL_STOPBITS        "Stopbits"
#define COM_RADIONAME_STOPBIT            "stop"
#define COM_RADIO_STOPBIT_ONE            "1"
#define COM_RADIO_STOPBIT_TWO            "2"

#define COM_RADIO_LBL_STOPBIT_ONE        "1 stopbit"
#define COM_RADIO_LBL_STOPBIT_TWO        "2 stopbit"

// Handshake
#define COM_OPTGRP_LABEL_FLOWCTRL        "Handshake"
#define COM_RADIONAME_FLOWCTRL           "flow"
#define COM_RADIO_LBL_HANDSHAKE_SW       "Software"
#define COM_RADIO_LBL_HANDSHAKE_NO       "disabled"

// Parity
#define COM_OPTGRP_LABEL_PARITY          "Parity"
#define COM_RADIONAME_PARITY             "par"
#define COM_RADIO_PARITY_NO              "N"
#define COM_RADIO_PARITY_EVEN            "E"
#define COM_RADIO_PARITY_ODD             "O"

#define COM_RADIO_LBL_PARITY_NO          "No"
#define COM_RADIO_LBL_PARITY_EVEN        "Even"
#define COM_RADIO_LBL_PARITY_ODD         "Odd"


// Transfer speed
#define COM_OPTGRP_LABEL_SPEED           "Transfer speed:"
#define COM_SELECTION_NAME_BAUD         "baud"
#define COM_OPTION_LBL_B9600             "  9600 Baud"
#define COM_OPTION_VALUE_B9600           "9600"
#define COM_OPTION_LBL_B19200            " 19200 Baud"
#define COM_OPTION_VALUE_B19200          "19200"
#define COM_OPTION_LBL_B38400            " 38400 Baud"
#define COM_OPTION_VALUE_B38400          "38400"
#define COM_OPTION_LBL_B57600            " 57600 Baud"
#define COM_OPTION_VALUE_B57600          "57600"
#define COM_OPTION_LBL_B74880            " 74880 Baud"
#define COM_OPTION_VALUE_B74880          "74880"
#define COM_OPTION_LBL_B115200           "1115200 Baud"
#define COM_OPTION_VALUE_B115200         "115200"
#define COM_OPTION_LBL_B230400           "230400 Baud"
#define COM_OPTION_VALUE_B230400         "230400"
#define COM_OPTION_LBL_B460800           "460800 Baud"
#define COM_OPTION_VALUE_B460800         "460800"
#define COM_OPTION_LBL_B921600           "921600 Baud"
#define COM_OPTION_VALUE_B921600         "921600"

// Buttons on this page
#define COM_BUTTONNAME_FORMRESET         "rst"
#define COM_ACTION_RESET                 "reset form"
#define COM_BUTTONNAME_COMMIT            "ok"
#define COM_ACTION_COMMIT                "commit settings"
#define COM_BUTTONNAME_CLOSE             "close"
#define COM_ACTION_CLOSE                 "close"
#define COM_BUTTONNAME_FACTORY           "factory"
#define COM_ACTION_FACTORY               "factory settings"

//
// /index page ------------------------------------------------------------
//
//
#define INDEX_BUTTONNAME_ADMIN           "adminpage"
#define INDEX_ACTION_ADMIN_COMMIT        "node administration"
//
// /login page -----------------------------------------------------------
//
#define LOGIN_FIELDNAME_PASSWORD         "adminpw"
//
#define LOGIN_ACTION_COMMIT              "login"
//
#define LOGIN_BUTTONNAME_COMMIT          "ok"
//
// /Login failed page --------------------------------------------------------
//
#define AUTHFAIL_ACTION_AGAIN            "try again"
#define AUTHFAIL_ACTION_CANCEL           "cancel"
//
#define AUTHFAIL_BUTTONNAME_AGAIN        "again"
#define AUTHFAIL_BUTTONNAME_CANCEL       "cancel"
//
// ************************************************************************
// define some default values
// ************************************************************************
//
#define SERIAL_BAUD                  115200
#define SON_SERVER_PORT                1234
//
// layout of EEPROM data
//
#define EEPROM_BLOCK_SIZE              1024
#define EEPROM_MAGIC_BYTE              0x6f
//
#define EEPROM_LEADING_LENGTH             2  // means two byte representing 
                                            // the real length of the data field
#define EEPROM_MAXLEN_MAGIC               1
#define EEPROM_MAXLEN_CRC32               4
//
#define EEPROM_MAXLEN_BOOLEAN             1
//
#define EEPROM_MAXLEN_WLAN_SSID          32  // max. length a SSID may have
#define EEPROM_MAXLEN_WLAN_PASSPHRASE    64  // max. length of a password
#define EEPROM_MAXLEN_SERVER_IP          16
#define EEPROM_MAXLEN_SERVER_PORT         4
#define EEPROM_MAXLEN_NODENAME           32
#define EEPROM_MAXLEN_ADMIN_PASSWORD     32
#define EEPROM_MAXLEN_USE_DHCP            EEPROM_MAXLEN_BOOLEAN
#define EEPROM_MAXLEN_SON                 EEPROM_MAXLEN_BOOLEAN


//
#define EEPROM_MAXLEN_COM_SERVER          EEPROM_MAXLEN_BOOLEAN
#define EEPROM_MAXLEN_TARGET_IP           16 // 192.123.123.122
#define EEPROM_MAXLEN_COM_PROTOCOL_TCP    EEPROM_MAXLEN_BOOLEAN
#define EEPROM_MAXLEN_COM_USE_HW_SERIAL   EEPROM_MAXLEN_BOOLEAN
#define EEPROM_MAXLEN_COM_USE_RX_GPIO     3
#define EEPROM_MAXLEN_COM_USE_TX_GPIO     3
#define EEPROM_MAXLEN_COM_USE_DATABITS    2
#define EEPROM_MAXLEN_COM_USE_STOPBITS    2
#define EEPROM_MAXLEN_COM_USE_FLOWCTRL    EEPROM_MAXLEN_BOOLEAN
#define EEPROM_MAXLEN_COM_USE_PARITY      1
#define EEPROM_MAXLEN_COM_USE_BAUDRATE    7
// layout of the eeprom:
//
#define EEPROM_HEADER_BEGIN              0
//
#define EEPROM_POS_MAGIC                 0
#define EEPROM_POS_CRC32                 (EEPROM_POS_MAGIC + EEPROM_MAXLEN_MAGIC)
//
#define EEPROM_HEADER_END                (EEPROM_POS_CRC32 + EEPROM_MAXLEN_CRC32)
//
// data area begins here
//
#define EEPROM_DATA_BEGIN                EEPROM_HEADER_END       
//
#define EEPROM_POS_WLAN_SSID             EEPROM_DATA_BEGIN
//
#define EEPROM_POS_WLAN_PASSPHRASE       (EEPROM_POS_WLAN_SSID + EEPROM_MAXLEN_WLAN_SSID + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_SERVER_IP             (EEPROM_POS_WLAN_PASSPHRASE + EEPROM_MAXLEN_WLAN_PASSPHRASE + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_SERVER_PORT           (EEPROM_POS_SERVER_IP + EEPROM_MAXLEN_SERVER_IP + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_NODENAME              (EEPROM_POS_SERVER_PORT + EEPROM_MAXLEN_SERVER_PORT + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_ADMIN_PASSWORD        (EEPROM_POS_NODENAME + EEPROM_MAXLEN_NODENAME + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_USE_DHCP              (EEPROM_POS_ADMIN_PASSWORD + EEPROM_MAXLEN_ADMIN_PASSWORD + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_SON                   (EEPROM_POS_USE_DHCP + EEPROM_MAXLEN_USE_DHCP + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_SERVER            (EEPROM_POS_SON + EEPROM_MAXLEN_SON + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_TARGET_IP             (EEPROM_POS_COM_SERVER + EEPROM_MAXLEN_COM_SERVER + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_PROTOCOL_TCP      (EEPROM_POS_TARGET_IP + EEPROM_MAXLEN_TARGET_IP + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_HW_SERIAL     (EEPROM_POS_COM_PROTOCOL_TCP + EEPROM_MAXLEN_COM_PROTOCOL_TCP + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_RX_GPIO       (EEPROM_POS_COM_USE_HW_SERIAL + EEPROM_MAXLEN_COM_USE_HW_SERIAL + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_TX_GPIO       (EEPROM_POS_COM_USE_RX_GPIO + EEPROM_MAXLEN_COM_USE_RX_GPIO + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_DATABITS      (EEPROM_POS_COM_USE_TX_GPIO + EEPROM_MAXLEN_COM_USE_TX_GPIO + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_STOPBITS      (EEPROM_POS_COM_USE_DATABITS + EEPROM_MAXLEN_COM_USE_DATABITS + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_FLOWCTRL      (EEPROM_POS_COM_USE_STOPBITS + EEPROM_MAXLEN_COM_USE_STOPBITS + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_PARITY        (EEPROM_POS_COM_USE_FLOWCTRL + EEPROM_MAXLEN_COM_USE_FLOWCTRL + EEPROM_LEADING_LENGTH)
//
#define EEPROM_POS_COM_USE_BAUDRATE      (EEPROM_POS_COM_USE_PARITY + EEPROM_MAXLEN_COM_USE_PARITY + EEPROM_LEADING_LENGTH)
//
#define EEPROM_DATA_END                  (EEPROM_POS_COM_USE_BAUDRATE + EEPROM_MAXLEN_COM_USE_BAUDRATE + EEPROM_LEADING_LENGTH)
//
//
// ... further stuff here like above scheme
//
// ************************************************************************
// some globals holding settings
// ************************************************************************
//
// change to your SSID in /admin webpage
String wlanSSID;
// change to your passphrase in /admin webpage
String wlanPasswd;
// change in /admin webpage
bool useDhcp;
// change to IP you prefer in /admin webpage
String wwwServerIP;
// change to port you prefer in /admin webpage
String wwwServerPort;
// change nodename in /admin webpage
String nodeName;
// change password in /admin webpage
String adminPasswd;
// remember previous successful login
bool adminAccessSucceeded;
// remember admin login password
String loginPasswd = "";
// enable serial over network
bool SerialOverNetwork;
// serial over network status
bool SONRunning;
// serial over network status
bool SONConnected;
// act as a COM-Server
bool COMServerMode;
// COM share protocol either TCP or UDP
bool COMProtocolTCP;
// use hardware serial on fixed pins
bool useHardSerial;
// which pin as Rx if software serial
String useRxPin;
// which pin as Tx if software serial
String useTxPin;
// amount of bits per unit
String useDataBits;
// amount of stopbits during transfer
String useStopBits;
// wich flow control ( only software or none )
bool useFlowCtrl;
// what parity to use
String useParity;
// transfer speed
String useBaudRate;
// target IP if COM-Client
String useTargetIP;
// serial I/O
SoftwareSerial *pSoftSerial;
// suppress debug output over serial port
bool beQuiet;
//
// ************************************************************************
// web server related globals and defines
// ************************************************************************
//
ESP8266WebServer server; // (WWW_LISTENPORT);
String pageContent;
int serverStatusCode;

ioStreams localStreams;

WiFiServer TelnetServer(SON_SERVER_PORT);
WiFiClient incomingTelnetConnection;
WiFiClient outgoingTelnetConnection;
//
#define SERVER_METHOD_GET       1
#define SERVER_METHOD_POST      2
#define ARGS_ADMIN_PAGE         2
//
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
//
// ************************************************************************
// forward declarations
// ************************************************************************
//
// page handling functions
//
void handleLoginPage(void);
void handleAdminPage(void);
void handleIndexPage(void);
void sendAuthFailedPage(void);
void handleComSettings(void);
//
// store/restore settings done in /admin page
//
int nodeStoreAdminSettings(void);
int nodeRestoreAdminSettings(void);
//
// store/restore settings done in /comsettings page
//
int nodeStoreCOMSettings(void);
int nodeRestoreCOMSettings(void);
//
// ************************************************************************
// CRC calculation e.g. over EEPROMo for verification
// ************************************************************************
//
unsigned long eeprom_crc(int startPos, int length)
{
  unsigned long crc = ~0L;

  for (int index = startPos; index < (startPos + length); ++index) 
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
//
// clear EEPROM content (set to zero )
//
void eeWipe( void )
{
  for( int index = 0; index < EEPROM_BLOCK_SIZE; index++ )
  {
    EEPROM.write( index, '\0' );
  }
  EEPROM.commit();
}
//
//
//
// store field length to a specific position
//
int eeStoreFieldLength( char* len, int dataIndex )
{
  int retVal = 0;

  if( !beQuiet )
  {
    Serial.print("write LEN byte [");
    Serial.print(len[0],HEX);
    Serial.print("] to pos ");
    Serial.println(dataIndex);
  }
  
  EEPROM.write(dataIndex, len[0]);

  if( !beQuiet )
  {
    Serial.print("write LEN byte [");
    Serial.print(len[1],HEX);
    Serial.print("] to pos ");
    Serial.println(dataIndex+1);
  }
  
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

  if( !beQuiet )
  {
    Serial.print("got LEN byte [");
    Serial.print(len[0],HEX);
    Serial.print("] from pos ");
    Serial.println(dataIndex);
  }

  len[1] = EEPROM.read(dataIndex+1);

  if( !beQuiet )
  {
    Serial.print("got LEN byte [");
    Serial.print(len[1],HEX);
    Serial.print("] from pos ");
    Serial.println(dataIndex+1);  
  }

  return(retVal);
}

//
// store a boolean value to a specific position
//
int eeStoreBoolean(  char* data, int dataIndex )
{
    int retVal = 0;
    short len = EEPROM_MAXLEN_BOOLEAN;

    if( !beQuiet )
    {
        Serial.print("store boolean to eeprom: Address is [");
        Serial.print(dataIndex);
        Serial.println("]");
    }

    if( (retVal = eeStoreFieldLength( (char*) &len, dataIndex )) == 0 )
    {
        for (int i = 0; i < len; ++i)
        {
            EEPROM.write(dataIndex + EEPROM_LEADING_LENGTH + i, data[i]);
            if( !beQuiet )
            {
                Serial.print("Wrote: ");
                Serial.println(data[i]); 
            }
        }
    }

    return(retVal);
}
//
// restore a boolean value from a specific position
//
int eeRestoreBoolean( char *data, int dataIndex )
{
    int retVal = 0;
    char rdValue;

    if( !beQuiet )
    {
        Serial.print("restore boolean from eeprom: Address is [");
        Serial.print(dataIndex);
        Serial.println("]");
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

    return(retVal);
}
//
// store a raw byte buffer without leading length field
// to a specific position
//
int eeStoreRaw( const char* data, short len, int dataIndex )
{
    int retVal = 0;

    if( !beQuiet )
    {
        Serial.print("store raw data to eeprom: Address is [");
        Serial.print(dataIndex);
        Serial.print("] - len = ");
        Serial.println(len);
    }

    for (int i = 0; i < len; ++i)
    {
        EEPROM.write(dataIndex, data[i]);
        Serial.print("Wrote: ");
        Serial.println(data[i], HEX); 
    }

  return(retVal);
}
//
// restore a byte buffer without leading length field
// from a specific position 
//
int eeRestoreRaw( String& data, int dataIndex, int len, int maxLen)
{
  int retVal = 0;
  char c;
  
  if( !beQuiet )
  {
    Serial.print("restore raw data from eeprom: Address is [");
    Serial.print(dataIndex);
    Serial.print("] - maxlen = ");
    Serial.println(maxLen);
  }

  if( len > 0 )
  {
    data = "";
    for( int i=0; i < len && i < maxLen; i++ )
    {
      c = EEPROM.read(dataIndex + i);
      if( !beQuiet )
      {
        Serial.print(c, HEX);
      }
      data += c;
    }
  }

  if( !beQuiet )
  {
    Serial.println(" - done!");
  }

  return(retVal);
}

//
// store a byte array to a specific position
//
int eeStoreBytes( const char* data, short len, int dataIndex )
{
    int retVal = 0;

    if( !beQuiet )
    {
        Serial.print("store byte to eeprom: Address is [");
        Serial.print(dataIndex);
        Serial.print("] - len = ");
        Serial.println(len);
    }

    if( (retVal = eeStoreFieldLength( (char*) &len, dataIndex )) == 0 )
    {
        for (int i = 0; i < len; ++i)
        {
            EEPROM.write(dataIndex + EEPROM_LEADING_LENGTH + i, data[i]);
            if( !beQuiet )
            {
                Serial.print("Wrote: ");
                Serial.println(data[i]); 
            }
        }
    }

  return(retVal);
}


//
// restore a string var from a specific position
//
int eeRestoreBytes( String& data, int dataIndex, int len, int maxLen)
{
  int retVal = 0;
  char c;
  
  if( !beQuiet )
  {
    Serial.print("restore data from eeprom: Address is [");
    Serial.print(dataIndex);
    Serial.print("] - maxlen = ");
    Serial.println(maxLen);
  }

  if( len > 0 )
  {
    data = "";
    for( int i=0; i < len && i < maxLen; i++ )
    {
      c = EEPROM.read(dataIndex + EEPROM_LEADING_LENGTH + i);
      if( !beQuiet )
      {
        Serial.print(c);
      }
      data += c;
    }
  }

  if( !beQuiet )
  {
    Serial.println(" - done!");
  }

  return(retVal);
}

//
// store a string var to a specific position
//
int eeStoreString( String data, int maxLen, int dataIndex )
{
  int retVal = 0;
  short wrLen;

  if( data.length() <= maxLen )
  {
    wrLen = data.length();
  }
  else
  {
    wrLen = maxLen;
  }

  retVal = eeStoreBytes( data.c_str(), wrLen, dataIndex );

  return(retVal);
}
//
// restore a string var from a specific position
//
int eeRestoreString( String& data, int dataIndex, int maxLen )
{
  int retVal = 0;
  short len = 0;
  
  if( (retVal = eeRestoreFieldLength( (char*) &len, dataIndex )) == 0 )
  {
    retVal = eeRestoreBytes( data, dataIndex, len, maxLen );
    if( !beQuiet )
    {
      Serial.println(" - done!");
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

  if( (magicByte = EEPROM.read( EEPROM_POS_MAGIC )) !=  EEPROM_MAGIC_BYTE )
  {
    retVal = false;
    if( !beQuiet )
    {
      Serial.print("wrong magic: ");
      Serial.println( magicByte );
    }
  }

  return(retVal);
}

//
// place a "magic" to the first byte in EEPROM
//
bool eeValidate()
{
  bool retVal = true;
  EEPROM.write( EEPROM_POS_MAGIC, EEPROM_MAGIC_BYTE );
  EEPROM.commit();

  return(retVal);
}
//
// ************************************************************************
// store all control values (e.g. SSID) to EEPROM
// returns true if ok - otherwise false
// ************************************************************************
//
int nodeStoreAdminSettings()
{
    int retVal = 0;
    unsigned long crcCalc;

    if( !beQuiet )
    {
        Serial.print("storing SSID: ");
        Serial.println(wlanSSID);
    }

    eeStoreString(  wlanSSID,      EEPROM_MAXLEN_WLAN_SSID,       EEPROM_POS_WLAN_SSID );
    
    if( !beQuiet )
    {
        Serial.print("storing Passphrase: ");
        Serial.println(wlanPasswd);
    }

    eeStoreString(  wlanPasswd,    EEPROM_MAXLEN_WLAN_PASSPHRASE, EEPROM_POS_WLAN_PASSPHRASE );
    
    if( !beQuiet )
    {
        Serial.print("storing ServerIP: ");
        Serial.println(wwwServerIP);
    }

    eeStoreString(  wwwServerIP,   EEPROM_MAXLEN_SERVER_IP,       EEPROM_POS_SERVER_IP );
    
    if( !beQuiet )
    {
        Serial.print("storing ServerPort: ");
        Serial.println(wwwServerPort);
    }

    eeStoreString(  wwwServerPort, EEPROM_MAXLEN_SERVER_PORT,     EEPROM_POS_SERVER_PORT );
    
    if( !beQuiet )
    {
        Serial.print("storing Nodename: ");
        Serial.println(nodeName);
    }

    eeStoreString(  nodeName,      EEPROM_MAXLEN_NODENAME,        EEPROM_POS_NODENAME );
    
    if( !beQuiet )
    {
        Serial.print("storing adminPasswd: ");
        Serial.println(adminPasswd);
    }

    eeStoreString(  adminPasswd,   EEPROM_MAXLEN_ADMIN_PASSWORD,  EEPROM_POS_ADMIN_PASSWORD );
    
    if( !beQuiet )
    {
        Serial.print("storing useDHCP: ");
        Serial.println(useDhcp);
    }

    eeStoreBoolean( (char*) &useDhcp,         EEPROM_POS_USE_DHCP );
    crcCalc = eeprom_crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );

    if( !beQuiet )
    {
        Serial.print("calculated crc over eeprom: ");
        Serial.println(crcCalc, HEX);
    }

    eeStoreRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );

    eeValidate();

    return( retVal );
}
//
// ************************************************************************
// restore all control values (e.g. SSID) from EEPROM
// returns true if ok - otherwise false
// ************************************************************************
//
int nodeRestoreAdminSettings()
{
    int retVal = 0;
    unsigned long crcCalc;
    unsigned long crcRead;
    String data = "";

    if( eeIsValid() || IGNORE_IF_CONDITION )
    {
        crcCalc = eeprom_crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );

        eeRestoreRaw( data, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        crcRead = atol(data.c_str());

        if( !beQuiet )
        {
            Serial.print("restored crc: ");
            Serial.print(crcRead, HEX);
            Serial.print(" calc crc: ");
            Serial.println(crcCalc, HEX);
        }

        if( (crcCalc == crcRead) || IGNORE_IF_CONDITION )
        {
            eeRestoreString(  wlanSSID,                 EEPROM_POS_WLAN_SSID,       EEPROM_MAXLEN_WLAN_SSID );

            if( !beQuiet )
            {
                Serial.print("restored SSID: ");
                Serial.println(wlanSSID);
            }

            eeRestoreString(  wlanPasswd,               EEPROM_POS_WLAN_PASSPHRASE, EEPROM_MAXLEN_WLAN_PASSPHRASE );

            if( !beQuiet )
            {
                Serial.print("restored Passphrase: ");
                Serial.println(wlanPasswd);
            }

            eeRestoreString(  wwwServerIP,              EEPROM_POS_SERVER_IP,       EEPROM_MAXLEN_SERVER_IP );

            if( !beQuiet )
            {
                Serial.print("restored ServerIP: ");
                Serial.println(wwwServerIP);
            }

            eeRestoreString(  wwwServerPort,            EEPROM_POS_SERVER_PORT,     EEPROM_MAXLEN_SERVER_PORT );

            if( !beQuiet )
            {
                Serial.print("restored ServerPort: ");
                Serial.println(wwwServerPort);
            }

            eeRestoreString(  nodeName,                 EEPROM_POS_NODENAME,        EEPROM_MAXLEN_NODENAME );

            if( !beQuiet )
            {
                Serial.print("restored Nodename: ");
                Serial.println(nodeName);
            }

            eeRestoreString(  adminPasswd,              EEPROM_POS_ADMIN_PASSWORD,  EEPROM_MAXLEN_ADMIN_PASSWORD );

            if( !beQuiet )
            {
                Serial.print("restored adminPasswd: ");
                Serial.println(adminPasswd);
            }

            eeRestoreBoolean( (char*) &useDhcp,         EEPROM_POS_USE_DHCP );

            if( !beQuiet )
            {
                Serial.print("restored useDHCP: ");
                Serial.println(useDhcp);
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
// ************************************************************************
// make a string from MAC address
// ************************************************************************
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
// ************************************************************************
// generate a unique nodename from MAC address
// ************************************************************************
//
void generateNodename()
{
    String id, hostname;
    uint8_t mac[6];

    WiFi.macAddress(mac);
    id=macToID(mac);
    nodeName = "ESP_" + id;
}
//
// ************************************************************************
// reset all values of /admin page to factory settings
// ************************************************************************
//
void resetAdmin2FactorySettings()
{

    wlanSSID = FACTORY_WLAN_SSID;
    wlanPasswd = FACTORY_WLAN_PASSPHRASE;
    useDhcp = FACTORY_USE_DHCPD_4IP;

    wwwServerIP = FACTORY_WWW_SERVER_IP;
    wwwServerPort = FACTORY_WWW_LISTENPORT;
    // nodeName
    generateNodename();
    adminPasswd = FACTORY_ADMIN_PASSWORD;

    return;
}
//
// ************************************************************************
// store all control values (e.g. baudrate) of /comsettings page to EEPROM
// returns true if ok - otherwise false
// ************************************************************************
//
int nodeStoreCOMSettings()
{
    int retVal = 0;
    unsigned long crcCalc;

    if( !beQuiet )
    {
        Serial.print("storing serial over network enable: ");
        Serial.println(SerialOverNetwork);
    }

    eeStoreBoolean( (char*) &SerialOverNetwork, EEPROM_POS_SON );

    if( !beQuiet )
    {
        Serial.print("storing act as COM-Server: ");
        Serial.println(COMServerMode);
    }

    eeStoreBoolean( (char*) &COMServerMode, EEPROM_POS_COM_SERVER );

    if( !beQuiet )
    {
        Serial.print("storing taget IP: ");
        Serial.println(useTargetIP);
    }

    eeStoreString( useTargetIP, EEPROM_MAXLEN_TARGET_IP, EEPROM_POS_TARGET_IP );

    if( !beQuiet )
    {
        Serial.print("storing use TCP protcol: ");
        Serial.println(COMProtocolTCP);
    }

    eeStoreBoolean( (char*) &COMProtocolTCP, EEPROM_POS_COM_PROTOCOL_TCP );

    if( !beQuiet )
    {
        Serial.print("storing use serial hardware: ");
        Serial.println(useHardSerial);
    }

    eeStoreBoolean( (char*) &useHardSerial, EEPROM_POS_COM_USE_HW_SERIAL );

    if( !beQuiet )
    {
        Serial.print("storing RxPin: ");
        Serial.println(useRxPin);
    }

    eeStoreString( useRxPin, EEPROM_MAXLEN_COM_USE_RX_GPIO, EEPROM_POS_COM_USE_RX_GPIO );

    if( !beQuiet )
    {
        Serial.print("storing TxPin: ");
        Serial.println(useTxPin);
    }

    eeStoreString( useTxPin, EEPROM_MAXLEN_COM_USE_TX_GPIO, EEPROM_POS_COM_USE_TX_GPIO );

    if( !beQuiet )
    {
        Serial.print("storing databits: ");
        Serial.println(useDataBits);
    }

    eeStoreString( useDataBits, EEPROM_MAXLEN_COM_USE_DATABITS, EEPROM_POS_COM_USE_DATABITS );

    if( !beQuiet )
    {
        Serial.print("storing stopbits: ");
        Serial.println(useStopBits);
    }

    eeStoreString( useStopBits, EEPROM_MAXLEN_COM_USE_STOPBITS, EEPROM_POS_COM_USE_STOPBITS );

    if( !beQuiet )
    {
        Serial.print("storing use flow control: ");
        Serial.println(useFlowCtrl);
    }

    eeStoreBoolean( (char*) &useFlowCtrl, EEPROM_POS_COM_USE_FLOWCTRL );

    if( !beQuiet )
    {
        Serial.print("storing parity: ");
        Serial.println(useParity);
    }

    eeStoreString( useParity, EEPROM_MAXLEN_COM_USE_PARITY, EEPROM_POS_COM_USE_PARITY );

    if( !beQuiet )
    {
        Serial.print("storing use baudrate: ");
        Serial.println(useBaudRate);
    }

    eeStoreString(  useBaudRate,   EEPROM_MAXLEN_COM_USE_BAUDRATE,  EEPROM_POS_COM_USE_BAUDRATE );

    crcCalc = eeprom_crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );
    if( !beQuiet )
    {
        Serial.print("storing CRC: ");
        Serial.println(crcCalc, HEX);
    }

    eeStoreRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );

    eeValidate();

    return( retVal );
}
//
// ************************************************************************
// restore all control values (e.g. baudrate) of /comsettings page from EEPROM
// returns true if ok - otherwise false
// ************************************************************************
//
int nodeRestoreCOMSettings()
{
    int retVal = 0;
    unsigned long crcCalc;
    unsigned long crcRead;
    String data = "";

    if( eeIsValid() || IGNORE_IF_CONDITION )
    {
        crcCalc = eeprom_crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );

        eeRestoreRaw( data, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        crcRead = atol(data.c_str());

        if( !beQuiet )
        {
            Serial.print("restored crc: ");
            Serial.print(crcRead, HEX);
            Serial.print(" calc crc: ");
            Serial.println(crcCalc, HEX);
        }

        if( (crcCalc == crcRead) || IGNORE_IF_CONDITION )
        {
            eeRestoreBoolean( (char*) &SerialOverNetwork, EEPROM_POS_SON );

            if( !beQuiet )
            {
                Serial.print("restored serial over network enable: ");
                Serial.println(SerialOverNetwork);
            }

            eeRestoreBoolean( (char*) &COMServerMode, EEPROM_POS_COM_SERVER );

            if( !beQuiet )
            {
                Serial.print("restored act as COM-Server: ");
                Serial.println(COMServerMode);
            }

            eeRestoreString( useTargetIP, EEPROM_POS_TARGET_IP, EEPROM_MAXLEN_TARGET_IP );

            if( !beQuiet )
            {
                Serial.print("restored taget IP: ");
                Serial.println(useTargetIP);
            }

            eeRestoreBoolean( (char*) &COMProtocolTCP , EEPROM_POS_COM_PROTOCOL_TCP );

            if( !beQuiet )
            {
                Serial.print("restored use TCP protcol: ");
                Serial.println(COMProtocolTCP);
            }

            eeRestoreBoolean( (char*) &useHardSerial, EEPROM_POS_COM_USE_HW_SERIAL );

            if( !beQuiet )
            {
                Serial.print("restored use serial hardware: ");
                Serial.println(useHardSerial);
            }

            eeRestoreString( useRxPin, EEPROM_POS_COM_USE_RX_GPIO, EEPROM_MAXLEN_COM_USE_RX_GPIO );

            if( !beQuiet )
            {
                Serial.print("restored RxPin: ");
                Serial.println(useRxPin);
            }

            eeRestoreString( useTxPin, EEPROM_POS_COM_USE_TX_GPIO, EEPROM_MAXLEN_COM_USE_TX_GPIO );

            if( !beQuiet )
            {
                Serial.print("restored TxPin: ");
                Serial.println(useTxPin);
            }

            eeRestoreString( useDataBits, EEPROM_POS_COM_USE_DATABITS, EEPROM_MAXLEN_COM_USE_DATABITS );

            if( !beQuiet )
            {
                Serial.print("restored databits: ");
                Serial.println(useDataBits);
            }

            eeRestoreString( useStopBits,  EEPROM_POS_COM_USE_STOPBITS, EEPROM_MAXLEN_COM_USE_STOPBITS );

            if( !beQuiet )
            {
                Serial.print("restored stopbits: ");
                Serial.println(useStopBits);
            }

            eeRestoreBoolean( (char*) &useFlowCtrl, EEPROM_POS_COM_USE_FLOWCTRL );

            if( !beQuiet )
            {
                Serial.print("restored use flow control: ");
                Serial.println(useFlowCtrl);
            }

            eeRestoreString( useParity,  EEPROM_POS_COM_USE_PARITY, EEPROM_MAXLEN_COM_USE_PARITY );

            if( !beQuiet )
            {
                Serial.print("restored parity: ");
                Serial.println(useParity);
            }

            eeRestoreString(  useBaudRate,     EEPROM_POS_COM_USE_BAUDRATE, EEPROM_MAXLEN_COM_USE_BAUDRATE );

            if( !beQuiet )
            {
                Serial.print("restored use baudrate: ");
                Serial.println(useBaudRate);
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
// ************************************************************************
// reset all values of /admin page to factory settings
// ************************************************************************
//
void resetCOM2FactorySettings()
{
     // enable COM srvice
     SerialOverNetwork = FACTORY_COM_SON_ENABLED;

    // COM sharing
    COMServerMode = FACTORY_COM_SERVER_MODE;

    // COM protocol
    COMProtocolTCP = FACTORY_COM_PROTOCOL_TCP;
   
    // use hardware serial
    useHardSerial = FACTORY_USE_HW_SERIAL;

    // no taget ip as default
    useTargetIP = FACTORY_COM_TARGET_IP;

    // Rx pin for software serial
    useRxPin = FACTORY_USE_RX_GPIO;

    // Tx pin for software serial
    useTxPin = FACTORY_USE_TX_GPIO;

    // amount of data bits
    useDataBits = FACTORY_USE_DATABITS;

    // amount of stop bits
    useStopBits = FACTORY_USE_STOPBITS;

    // use software flow control
    useFlowCtrl = FACTORY_USE_FLOWCTRL;

    // use parity
    useParity = FACTORY_USE_PARITY;

    // baudrate to use
    useBaudRate = FACTORY_USE_BAUDRATE;

    return;
}
//
// ************************************************************************
// dump info ...
// ************************************************************************
//
void dumpInfo()
{

    pageContent += "This node (id=" + nodeName + ") is connected to WLAN-Network " + wlanSSID + ".<br>\n";
    pageContent += "DHCP is ";
    if( !useDhcp )
    {
        pageContent += "not ";
    }
    pageContent += "activated, a Web-Server is listening on " + wwwServerIP + ":" + wwwServerPort + "<br>\n";
    pageContent += "Serial Over Network is ";
    if( !SerialOverNetwork )
    {
        pageContent += "not ";
    }
    pageContent += "activated<br>\n";
    if( SerialOverNetwork )
    {
        pageContent += "The node acts as ";
    
        if( COMServerMode )
        {
            pageContent += "COM-Server and is listening on " + wwwServerIP + ":" + SON_SERVER_PORT + "<br>\n";
        }
        else
        {
            pageContent += "COM-Client for";
            pageContent += "target address " + useTargetIP + ":" + SON_SERVER_PORT + " <br>\n";
        
        }
    
        pageContent += "The node uses ";
        
        if( COMProtocolTCP )
        {
            pageContent += "TCP";
        }
        else
        {
            pageContent += "UDP";
        }
    
        pageContent += " as protocol for COM services.<br>\n";

        pageContent += "The serial transfer is done by ";
    
        if(useHardSerial)
        {
            pageContent += "hardware.<br>\n";
        }
        else
        {
            pageContent += "software, ";
            pageContent += "using Pins ";
            pageContent += useRxPin + "as Rx- and ";
            pageContent += useTxPin + "as Tx-pin.<br>\n";
        
        }
    
        pageContent += "Transfer parameters are set to ";
        pageContent += useDataBits + " databits, ";
        pageContent += useStopBits + " stopbits ";
        pageContent += "using ";
    
        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_NO)) )
        {
            pageContent += "no ";
        }
        
        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_EVEN)) )
        {
            pageContent += "even ";
        }
        
        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_ODD)) )
        {
            pageContent += "odd ";
        }
    
        pageContent += "parity ";
        pageContent += "at a transfer rate of ";
        pageContent += useBaudRate;
        pageContent += " Baud with ";
        
        if( useFlowCtrl )
        {
            pageContent += "software ";
        }
        else
        {
                pageContent += "no ";
        }

        pageContent += "flow control enabled.<br>\n";
    }
    
//    if( !beQuiet )
//    {
//        Serial.println(pageContent);
//    }
}
//
// ************************************************************************
// read all char off the UART
// ************************************************************************
//
void flushCOMPort()
{
    while(Serial.available() > 0)
    {
        Serial.read();
    }
}
//
// ************************************************************************
// process SON (Serial Over Network) 
// ************************************************************************
//
void processSON()
{
    if( SONRunning && SONConnected )
    {

        if( COMServerMode )
        {
            if (incomingTelnetConnection && incomingTelnetConnection.connected())
            {
                if(incomingTelnetConnection.available())
                {
                  //get data from the telnet connection and write to serial port
                  while(incomingTelnetConnection.available()) 
                  {
                      Serial.write(incomingTelnetConnection.read());
                  }
                }
            }

            //check UART for data
            if(Serial.available())
            {
                size_t len = Serial.available();
                uint8_t sbuf[len];
                Serial.readBytes(sbuf, len);
        
                if(incomingTelnetConnection && incomingTelnetConnection.connected())
                {
                    incomingTelnetConnection.write(sbuf, len);
                    delay(1);
                }
            }
        }
        else
        {
            if (outgoingTelnetConnection && outgoingTelnetConnection.connected())
            {
                if(outgoingTelnetConnection.available())
                {
                  //get data from the telnet connection and write to serial port
                  while(outgoingTelnetConnection.available()) 
                  {
                      Serial.write(outgoingTelnetConnection.read());
                  }
                }
            }

            //check UART for data
            if(Serial.available())
            {
                size_t len = Serial.available();
                uint8_t sbuf[len];
                Serial.readBytes(sbuf, len);
        
                if(outgoingTelnetConnection && outgoingTelnetConnection.connected())
                {
                    outgoingTelnetConnection.write(sbuf, len);
                    delay(1);
                }
            }
        }

    }
}
//
// ************************************************************************
// setup SON (Serial Over Network) 
// ************************************************************************
//
void SetupSON()
{
    uint8_t serialConfig;
    unsigned long lBaudRate;

    if( useDataBits.equalsIgnoreCase(COM_RADIO_DATALEN_SIX) )
    {
        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_NO)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_6N1;
            }
            else
            {
                serialConfig = SERIAL_6N2;
            }
        }

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_EVEN)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_6E1;
            }
            else
            {
                serialConfig = SERIAL_6E2;
            }
        }

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_ODD)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_6O1;
            }
            else
            {
                serialConfig = SERIAL_6O2;
            }
        }
    }

    if( useDataBits.equalsIgnoreCase(COM_RADIO_DATALEN_SEVEN) )
    {
        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_NO)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_7N1;
            }
            else
            {
                serialConfig = SERIAL_7N2;
            }
        }

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_EVEN)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_7E1;
            }
            else
            {
                serialConfig = SERIAL_7E2;
            }
        }

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_ODD)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_7O1;
            }
            else
            {
                serialConfig = SERIAL_7O2;
            }
        }
    }

    if( useDataBits.equalsIgnoreCase(COM_RADIO_DATALEN_EIGHT) )
    {
        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_NO)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_8N1;
            }
            else
            {
                serialConfig = SERIAL_8N2;
            }
        }

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_EVEN)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_8E1;
            }
            else
            {
                serialConfig = SERIAL_8E2;
            }
        }

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_ODD)) )
        {
            if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
            {
                serialConfig = SERIAL_8O1;
            }
            else
            {
                serialConfig = SERIAL_8O2;
            }
        }
    }

    if( !useHardSerial )
    {
        lBaudRate = (unsigned long) useBaudRate.toInt();
      
        pSoftSerial = new SoftwareSerial( useRxPin.toInt(), useTxPin.toInt() );
        pSoftSerial->begin(lBaudRate);
    }
    else
    {
        lBaudRate = (unsigned long) useBaudRate.toInt();
//        Serial.begin(lBaudRate, serialConfig);
    }

    if( COMServerMode )
    {
        if( !beQuiet )
        {
            Serial.println("SetupSON -> start server");
        }      
//        pTelnetServer = new WiFiServer(SON_SERVER_PORT);
        if( !beQuiet )
        {
            Serial.println("SetupSON -> server started");
        }      
        TelnetServer.setNoDelay(true);
        if( !beQuiet )
        {
            Serial.println("SetupSON -> nodelay set");
        }              
        TelnetServer.begin();
    }
    else
    {
//        IPAddress targetServer = IPAddress.fromString(useTargetIP);
          if( !beQuiet )
          {
              Serial.println("try to connect to server");
          }

        IPAddress targetServer;
        targetServer.fromString(useTargetIP);
        if (outgoingTelnetConnection.connect(targetServer, SON_SERVER_PORT)) 
        {
            if( !beQuiet )
            {
                Serial.println("connected");
            }
            SONConnected = true;
        }
    }
}
//
// ************************************************************************
// start SON (Serial Over Network) 
// ************************************************************************
//
void StartSON()
{
SONRunning = true;
}
//
// ************************************************************************
// stop SON (Serial Over Network) 
// ************************************************************************
//
void StopSON()
{
SONRunning = false;
SONConnected = false;
}
//
// ************************************************************************
// setup module ...
// ************************************************************************
//
void LEDOff(void);

void setup() 
{
    LEDOff();

    adminAccessSucceeded = false;
    beQuiet = BE_QUIET;
    IPAddress localIP;

    // startup serial console ...
    Serial.begin(SERIAL_BAUD);
    delay(1);

    EEPROM.begin(EEPROM_BLOCK_SIZE);

    if( eeIsValid() )
    {
        if( !beQuiet )
        {
            Serial.println("eeprom content is valid");
            nodeRestoreAdminSettings();
            nodeRestoreCOMSettings();
        }
    }
    else
    {
        if( !beQuiet )
        {
            Serial.println("INVALID eeprom content!");
            Serial.println("reset node to factory settings.");
        }
        // if eeprom has no valid signature set node data to factory defaults
        //
        resetAdmin2FactorySettings();
        nodeStoreAdminSettings();

        resetCOM2FactorySettings();
        nodeStoreCOMSettings();
    }

    if( !beQuiet )
    {
        Serial.print("EEPROM in use: ");
        Serial.println(EEPROM_DATA_END);
    }

    if( wlanSSID.length() > 0 &&
        wlanPasswd.length() > 0 )
    {
        if( !beQuiet )
        {
            Serial.println();
            Serial.print("Connecting to >");
            Serial.print(wlanSSID);
            Serial.print("< password from eeprom is >");
            Serial.print(wlanPasswd);
            Serial.println("<\n");
        }

        // Connect to WiFi network
        WiFi.begin( wlanSSID.c_str(), wlanPasswd.c_str());
   
        // loop until connection is established
        while (WiFi.status() != WL_CONNECTED) 
        {
            delay(500);
            if( !beQuiet )
            {
                Serial.print(".");
            }
        }
        Serial.println("");
        Serial.println("WiFi connected");
    }

    server = ESP8266WebServer( wwwServerPort.toInt() );

    server.on("/", handleIndexPage);
    server.on("/admin", handleAdminPage);
    server.on("/login", handleLoginPage);
    server.on("/comsettings", handleComSettings);

    server.begin();

    localIP = WiFi.localIP();
    wwwServerIP = localIP.toString();

    if( !beQuiet )
    {
        Serial.print("Webserver started. URL is: ");
        Serial.print("http://");
        Serial.print(localIP);
        Serial.print(":");
        Serial.println(wwwServerPort);
    }
 
    pageContent = "";
    dumpInfo();   
    pageContent = "";

    SONRunning = false;
    SONConnected = false;
    flushCOMPort();
    if( SerialOverNetwork )
    {
        if( !beQuiet )
        {
            Serial.println("setup -> SON active -> setup and start SON");
        }
        SetupSON();
        StartSON();
    }
}
//
// ************************************************************************
// page preparation and handling 
// ************************************************************************
//
// ---- load and handle /login page ----
//

void handleLoginPage()
{

    if( adminAccessSucceeded )
    {
        if( !beQuiet )
        {
            Serial.println("handleLoginPage -> handleAdminPage");
        }
        handleAdminPage();
    }
    else
    {

        if( !beQuiet )
        {
            Serial.println("handleLoginPage");
            for(int i = 0; i < server.args(); i++ )
            {
                Serial.print( server.argName(i) );
                Serial.print( " = " );
                Serial.println( server.arg(i) );
            }
        }

        if( server.method() == SERVER_METHOD_POST && server.hasArg(LOGIN_FIELDNAME_PASSWORD) )
        {
            if( !beQuiet )
            {
                Serial.println("POST REQUEST");
            }
            // form contains user input and has been postet
            // to server

            loginPasswd = server.arg(LOGIN_FIELDNAME_PASSWORD);

            if( adminPasswd.equals(loginPasswd) )
            {
                adminAccessSucceeded = true;
                handleAdminPage();
            }
            else
            {
                adminAccessSucceeded = false;
                sendAuthFailedPage();
            }
        }
        else
        {
            if( !beQuiet )
            {
                Serial.println("GET REQUEST");
            }

            pageContent  = "<!DOCTYPE html>\r";
            pageContent += "<html>\r";
            pageContent += "<head>\r";
            pageContent += "<meta charset=\"utf-8\" />\r";
            pageContent += "<title>Administrator</title>\r";
            pageContent += "</head>\r";
            pageContent += "<body bgcolor=\"#00C9FF\" text=\"#FDFF00\" link=\"#FF0200\" vlink=\"#A900FF\" alink=\"#20FF00\">\r";
            pageContent += "<div align=\"center\"><strong><h1>Administrator account</h1></strong></div>\r";
            pageContent += "<form action=\"/login\" method=\"post\">\r";
            pageContent += "<table align=\"center\">\r";
            pageContent += "<tr>\r";
            pageContent += "  <td><div align=\"center\">Password:</div></td>\r";
            pageContent += "  <td><div align=\"center\"><input type=\"text\" name=\"" +
                                String(LOGIN_FIELDNAME_PASSWORD) + "\" value=\"" + loginPasswd + "\"> ";

            pageContent += "</tr>\r";
            pageContent += "</table>\r";
            pageContent += "<hr align=\"center\"><br>\r";
            pageContent += "<div align=\"center\">\r";
            pageContent += "<input type=\"submit\" name=\"" +
                           String(LOGIN_BUTTONNAME_COMMIT) + "\" value=\"" + String(LOGIN_ACTION_COMMIT) + "\"> ";

            pageContent += "</div><br>\r";
            pageContent += "<hr align=\"center\"><br>\r";
            pageContent += "</form>\r";
            pageContent += "</body>\r";
            pageContent += "</html>\r";

            server.send(200, "text/html", pageContent);  

        }
    }
}
//
//
// ---- get user input data from admin page ----
//
void getAdminInputValues()
{
    if( !beQuiet )
    {
        Serial.println("getAdminInputValues");

            for(int i = 0; i < server.args(); i++ )
            {
                Serial.print( server.argName(i) );
                Serial.print( " = " );
                Serial.println( server.arg(i) );
            }
    }
    // Store user values an do other funny things ...

    if( server.hasArg(ADMIN_FIELDNAME_SSID) )
    {
        wlanSSID = server.arg(ADMIN_FIELDNAME_SSID);
    }

    if( server.hasArg(ADMIN_FIELDNAME_PASSPHRASE) )
    {
        wlanPasswd = server.arg(ADMIN_FIELDNAME_PASSPHRASE);
    }

    if( server.hasArg(ADMIN_CHKNAME_USEDHCPD) )
    {
        useDhcp = server.arg(ADMIN_CHKNAME_USEDHCPD);
    }

    if( server.hasArg(ADMIN_FIELDNAME_SERVER_IP) )
    {
        wwwServerIP = server.arg(ADMIN_FIELDNAME_SERVER_IP);
    }

    if( server.hasArg(ADMIN_FIELDNAME_SRVPORT) )
    {
        wwwServerPort = server.arg(ADMIN_FIELDNAME_SRVPORT);
    }

    if( server.hasArg(ADMIN_FIELDNAME_NODENAME) )
    {
        nodeName = server.arg(ADMIN_FIELDNAME_NODENAME);
    }

    if( server.hasArg(ADMIN_FIELDNAME_ADMINPW) )
    {

        adminPasswd = server.arg(ADMIN_FIELDNAME_ADMINPW);
        if( !adminPasswd.equals(loginPasswd) )
        {
            if( !beQuiet )
            {
                Serial.println("Admin password changed ... reset autologin flag");
            }
            adminAccessSucceeded = false;
        }
    }

    // All user values retrieved ...
    nodeStoreAdminSettings();

    return;

}
//
// ---- load and handle /admin page ----
//
void handleAdminPage()
{
    String btnValue = "";


   if( !beQuiet )
   {
       Serial.println("handleAdminPage");
       for(int i = 0; i < server.args(); i++ )
       {
           Serial.print( server.argName(i) );
           Serial.print( " = " );
           Serial.println( server.arg(i) );
       }
   }

   if( !adminAccessSucceeded )
   {
       handleLoginPage();
   }

   if( server.method() == SERVER_METHOD_POST &&
        (server.hasArg(ADMIN_BUTTONNAME_COMMIT) ||
         server.hasArg(ADMIN_BUTTONNAME_FORMRESET) ||
         server.hasArg(ADMIN_BUTTONNAME_FACTORY) ||
         server.hasArg(ADMIN_BUTTONNAME_CLOSE) ||
         server.hasArg(ADMIN_BUTTONNAME_COMSETTINGS) ||
         server.hasArg(ADMIN_BUTTONNAME_RESTART) ))
    {

        if( server.hasArg(ADMIN_BUTTONNAME_COMMIT) )
        {
            btnValue = server.arg(ADMIN_BUTTONNAME_COMMIT);
            if( btnValue.equals(ADMIN_ACTION_COMMIT) )
            {
                getAdminInputValues();
            }
        }

        if( server.hasArg(ADMIN_BUTTONNAME_FACTORY) )
        {
            btnValue = server.arg(ADMIN_BUTTONNAME_FACTORY);
            if( btnValue.equals(ADMIN_ACTION_FACTORY) )
            {
                resetAdmin2FactorySettings();
            }
        }

        if( server.hasArg(ADMIN_BUTTONNAME_FORMRESET) )
        {
            btnValue = server.arg(ADMIN_BUTTONNAME_FORMRESET);
            if( btnValue.equals(ADMIN_ACTION_RESET) )
            {
                nodeRestoreAdminSettings();
            }
        }

         if( server.hasArg(ADMIN_BUTTONNAME_COMSETTINGS) )
         {
             btnValue = server.arg(ADMIN_BUTTONNAME_COMSETTINGS);
             if( btnValue.equals(ADMIN_ACTION_COMSETTINGS) )
             {
                 handleComSettings();
             }
         }

         if( server.hasArg(ADMIN_BUTTONNAME_RESTART) )
         {
             btnValue = server.arg(ADMIN_BUTTONNAME_RESTART);
             if( btnValue.equals(ADMIN_ACTION_RESTART) )
             {
                 server.close();
                 ESP.restart();
                 return;
             }
         }

        handleIndexPage();
    }
    else
    {
        pageContent  = "<!DOCTYPE html>\n";
        pageContent += "<html>\n";
        pageContent += "<head>\n";
        pageContent += "<meta charset=\"utf-8\" />\n";
        pageContent += "<title>ESP8266 Settings</title>\n";
        pageContent += "</head>\n";
        pageContent += "<body bgcolor=\"#00C9FF\" text=\"#FDFF00\" link=\"#FF0200\" vlink=\"#A900FF\" alink=\"#20FF00\">\n";
        pageContent += "<div align=\"center\"><strong><h1>Change settings for this node</h1></strong></div>\n";
        pageContent += "<form action=\"/admin\" method=\"post\">\n";
        pageContent += "<table align=\"center\">\n";
        pageContent += "<tr>\n";
        pageContent += "<td> <div align=\"center\">\n";
        pageContent += "<optgroup label=\"" + String(ADMIN_OPTGRP_LABEL_SSID) + "\"\n";
        pageContent += "</td>\n";

        pageContent += "<td><div align=\"center\"><input type=\"text\" name=\"" +
                       String(ADMIN_FIELDNAME_SSID) + "\" value=\"" +
                       wlanSSID + "\" maxlength="+ String(EEPROM_MAXLEN_WLAN_SSID) + " ></div></td>\n";

        pageContent += "<td><div align=\"center\"></div></td>\n";
        pageContent += "</optgroup>\n";

        pageContent += "</tr>\n";
        pageContent += "<tr>\n";

        pageContent += "<td><div align=\"center\">\n";
        pageContent += "<optgroup label=\"" + String(ADMIN_OPTGRP_LABEL_PASSPHRASE) + "\"\n";
        pageContent += "</td>\n";

        pageContent += "  <td><div align=\"center\"><input type=\"text\" name=\"" +
                       String(ADMIN_FIELDNAME_PASSPHRASE) + "\" value=\"" + 
                       wlanPasswd + "\" maxlength="+ String(EEPROM_MAXLEN_WLAN_PASSPHRASE) + " ></div></td>\n";

        pageContent += "<td><div align=\"center\"></div></td>\n";
        pageContent += "</optgroup>\n";

        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";

        pageContent += "<td><div align=\"center\">\n";
        pageContent += "<optgroup label=\"" + String(ADMIN_OPTGRP_LABEL_USE_DHCP) + "\"\n";
        pageContent += "</td>\n";

        pageContent += "  <td><div align=\"center\"><input type=\"checkbox\" name=\"" +
                       String(ADMIN_CHKNAME_USEDHCPD) + "\" value=\"" + 
                       useDhcp + "\"></div></td>\n";

        pageContent += "<td><div align=\"center\"></div></td>\n";
        pageContent += "</optgroup>\n";

        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "</tr>\n";

        pageContent += "<tr>\r";
        pageContent += "  <td> <div align=\"center\">\n";
        pageContent += "    <optgroup label=\"" + String(ADMIN_OPTGRP_LABEL_SERVER_IP) + "\" </td>\n";
        pageContent += "  <td><div align=\"center\"><input type=\"text\" name=\"";
        pageContent += String(ADMIN_FIELDNAME_SERVER_IP) + "\" value=\"" + wwwServerIP + "\" maxlength=\"";
        pageContent += String(EEPROM_MAXLEN_SERVER_IP) + "\"> ";
        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\"></div></td>\n";
        pageContent += "  </optgroup>\n";
        pageContent += "</tr>\n";


        pageContent += "<tr>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";

        pageContent += "<td> <div align=\"center\">\n";
        pageContent += "<optgroup label=\"" + String(ADMIN_OPTGRP_LABEL_SERVER_PORT) + "\"\n";
        pageContent += "</td>\n";
	

        pageContent += "  <td><div align=\"center\"><input type=\"text\" name=\"" +
                       String(ADMIN_FIELDNAME_SRVPORT) + "\" value=\"" + 
                       wwwServerPort + "\" size=\"4\" maxlength="+ String(EEPROM_MAXLEN_SERVER_PORT) + " ></div></td>\n";
//                       wwwServerPort + "\" size=\"4\" maxlength=\"4\"></div></td>\n";

        pageContent += "<td><div align=\"center\"></div></td>\n";
        pageContent += "</optgroup>\n";

        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "  <td><hr align=\"center\"></td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";

        pageContent += "<td> <div align=\"center\">\n";
        pageContent += "<optgroup label=\"" + String(ADMIN_OPTGRP_LABEL_NODENAME) + "\"\n";
        pageContent += "</td>\n";

        pageContent += "  <td><div align=\"center\"><input type=\"text\" name=\"" +
                       String(ADMIN_FIELDNAME_NODENAME) + "\" value=\"" + 
                       nodeName + "\" maxlength="+ String(EEPROM_MAXLEN_NODENAME) + " ></div></td>\n";

        pageContent += "<td><div align=\"center\"></div></td>\n";
        pageContent += "</optgroup>\n";

        pageContent += "</tr>\n";
        pageContent += "<tr>\n";

        pageContent += "<td> <div align=\"center\">\n";
        pageContent += "<optgroup label=\"" + String(ADMIN_OPTGRP_LABEL_ADMIN_PASSWD) + "\"\n";
        pageContent += "</td>\n";

        pageContent += "  <td><div align=\"center\"><input type=\"text\" name=\"" +
                       String(ADMIN_FIELDNAME_ADMINPW) + "\" value=\"" + 
                       adminPasswd +"\" maxlength="+ String(EEPROM_MAXLEN_ADMIN_PASSWORD) + " ></div></td>\n";

        pageContent += "<td><div align=\"center\"></div></td>\n";
        pageContent += "</optgroup>\n";

        pageContent += "</tr>\n";

        pageContent += "<tr>\n";
        pageContent += "<td> <hr align=\"center\"></td>\n";
        pageContent += "<td><hr align=\"center\"></td>\n";
        pageContent += "<td><hr align=\"center\"></td>\n";
        pageContent += "</tr>\n";

        pageContent += "</table>\n";
        pageContent += "<div align=\"center\">\n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(ADMIN_BUTTONNAME_FORMRESET) + "\" value=\"" + 
                       String(ADMIN_ACTION_RESET) + "\"> \n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(ADMIN_BUTTONNAME_COMMIT) + "\" value=\"" + 
                       String(ADMIN_ACTION_COMMIT) + "\"> \n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(ADMIN_BUTTONNAME_FACTORY) + "\" value=\"" + 
                       String(ADMIN_ACTION_FACTORY) + "\"> \n";

        pageContent += "<br><br>\n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(ADMIN_BUTTONNAME_CLOSE) + "\" value=\"" + 
                       String(ADMIN_ACTION_CLOSE) + "\"> \n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(ADMIN_BUTTONNAME_COMSETTINGS) + "\" value=\"" + 
                       String(ADMIN_ACTION_COMSETTINGS) + "\"> \n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(ADMIN_BUTTONNAME_RESTART) + "\" value=\"" + 
                       String(ADMIN_ACTION_RESTART) + "\"> \n";

        pageContent += "</div><br>\n";
        pageContent += "</form>\n";
        pageContent += "</body>\n";
        pageContent += "</html>\n";
        server.send(200, "text/html", pageContent);  
    }

    return;

}
//
// ---- load and handle / page ----
//
void handleIndexPage(void)
{
    String indexAction = "";

// Serial.println(WiFi.macAddress());
// Serial.println(WiFi.localIP());
// WiFi.printDiag(Serial);

// Serial.println(ESP.getFreeHeap());
// Serial.println(ESP.getChipId());
// Serial.println(ESP.getFlashChipId());
// Serial.println(ESP.getFlashChipSize());
// Serial.println(ESP.getFlashChipSpeed());
// Serial.println(ESP.getCycleCount());
// Serial.println(ESP.getVcc());

    if( !beQuiet )
    {
        Serial.println("handleIndexPage");
        for(int i = 0; i < server.args(); i++ )
        {
            Serial.print( server.argName(i) );
            Serial.print( " = " );
            Serial.println( server.arg(i) );
        }
    }


    if( server.method() == SERVER_METHOD_POST &&
        server.hasArg(INDEX_BUTTONNAME_ADMIN)  )
    {
        if( !beQuiet )
        {
            Serial.println("POST REQUEST");
            // form contains user input and has been postet
            // to server

            for(int i = 0; i < server.args(); i++ )
            {
                Serial.print( server.argName(i) );
                Serial.print( " = " );
                Serial.println( server.arg(i) );
            }
        }

        if( server.hasArg(INDEX_BUTTONNAME_ADMIN) )
        {
            indexAction = server.arg(INDEX_BUTTONNAME_ADMIN);
            if( adminAccessSucceeded )
            {
                handleAdminPage();
            }
            else
            {
                handleLoginPage();
            }
        }
        else
        {
            // unknown action
            pageContent = "<!DOCTYPE HTML>\r\n";
            pageContent += "<html></p>";
            pageContent += "<br>empty strings not allowed!<br>";
            pageContent += "</html>";
            server.send(200, "text/html", pageContent);  
        }
    }
    else
    {
        if( !beQuiet )
        {
            Serial.println("GET REQUEST");
        }
        pageContent = "<!DOCTYPE html>";
        pageContent += "<html>";
        pageContent += "<head>";
        pageContent += "<meta charset=\"utf-8\" />";
        pageContent += "<title>Home</title>";
        pageContent += "</head>";
        pageContent += "<body bgcolor=\"#00C9FF\" text=\"#FDFF00\" link=\"#FF0200\" vlink=\"#A900FF\" alink=\"#20FF00\">";
        pageContent += "<div align=\"center\"><strong><h1>node home</h1></strong></div>";

        pageContent += "<hr align=\"center\"><br>";
        pageContent += "<div align=\"center\">";
    dumpInfo();
        pageContent += "</div><br>";

        // display all information of current node that don't require admin access
        pageContent += "<form action=\"/\" method=\"post\">";
        pageContent += "<hr align=\"center\"><br>";
        pageContent += "<div align=\"center\">";

        pageContent += "<input type=\"submit\" name=\"" + String(INDEX_BUTTONNAME_ADMIN) + 
                       "\" value=\"" + String(INDEX_ACTION_ADMIN_COMMIT) + "\"> ";

        pageContent += "</div><br>";
        pageContent += "<hr align=\"center\"><br>";
        pageContent += "</form>";
        pageContent += "</body>";
        pageContent += "</html>";
        server.send(200, "text/html", pageContent);  
    }

    return;
}
//
// ---- load and send out an error page if login failed ----
//
void sendAuthFailedPage()
{

    if( !beQuiet )
    {
        Serial.println("sendAuthFailedPage");
           for(int i = 0; i < server.args(); i++ )
           {
                Serial.print( server.argName(i) );
                Serial.print( " = " );
                Serial.println( server.arg(i) );
           }
    }


    if( server.method() == SERVER_METHOD_POST &&
        (server.hasArg(AUTHFAIL_BUTTONNAME_AGAIN) ||
         server.hasArg(AUTHFAIL_BUTTONNAME_CANCEL) ) )
    {
        if( server.hasArg(AUTHFAIL_BUTTONNAME_AGAIN) )
        {
            handleLoginPage();
        }

        if( server.hasArg(AUTHFAIL_BUTTONNAME_CANCEL) )
        {
            handleIndexPage();
        }
    }
    else
    {
        if( !beQuiet )
        {
            Serial.println("POST REQUEST");
        }
        pageContent  = "<!DOCTYPE html>";
        pageContent += "<html>";
        pageContent += "<head>";
        pageContent += "<meta charset=\"utf-8\" />";
        pageContent += "<title>Administrator</title>";
        pageContent += "</head>";
        pageContent += "<body bgcolor=\"#00C9FF\" text=\"#FDFF00\" link=\"#FF0200\" vlink=\"#A900FF\" alink=\"#20FF00\">";
        pageContent += "<div align=\"center\"><strong><h1 style=\"color: #FF0000; \">Authorization failed!</h1></strong></div>";
        pageContent += "<form action=\"/login\" method=\"post\">";
        pageContent += "<table align=\"center\">";
        pageContent += "</table>";
        pageContent += "<hr align=\"center\"><br>";
        pageContent += "<div align=\"center\">";
        pageContent += "    <input type=\"submit\" name=\"" +
                            String(AUTHFAIL_BUTTONNAME_AGAIN) + 
                            "\" value=\"" + String(AUTHFAIL_ACTION_AGAIN) + "\"> ";
// name = "again" value="try again"> ";
        pageContent += "    <input type=\"submit\" name=\"" +
                            String(AUTHFAIL_BUTTONNAME_CANCEL) + 
                            "\" value=\"" + String(AUTHFAIL_ACTION_CANCEL) + "\"> ";
// name = "cancel" value="cancel">";
        pageContent += "</div><br>";
        pageContent += "<hr align=\"center\"><br>";
        pageContent += "</form>";
        pageContent += "</body>";
        pageContent += "</html>";
        server.send(200, "text/html", pageContent);  
    }

    return;
}

// ******************************************** COMSETTINGS *********************************
//
// ---- get user input data from comsettings page ----
//
void getComInputValues()
{
    String btnValue = "";
    String opMode = "";
    bool nodeRestart = false;

    if( !beQuiet )
    {
        Serial.println("getComInputValues ...");
    }

    btnValue = server.arg(COM_RADIONAME_SON);

    if( btnValue.equals(COM_RADIO_SON_ENABLED) )
    {
        SerialOverNetwork = true;
    }
    else
    {
        SerialOverNetwork = false;
    }

    if( server.hasArg(COM_RADIONAME_OPERATIONMODE) )
    {
        nodeRestart = false;
    	opMode = server.arg(COM_RADIONAME_OPERATIONMODE);
        if( opMode.equalsIgnoreCase( COM_RADIO_OPERATIONMODE_SERVER ) )
        {
            if( ! COMServerMode )
            {
                nodeRestart = true;
            }
        }
        else
        {
            if( COMServerMode )
            {
                nodeRestart = true;
            }
        }

        COMServerMode = opMode.equalsIgnoreCase( COM_RADIO_OPERATIONMODE_SERVER );

        if( !beQuiet )
        {
            Serial.println();
            Serial.print("COMServerMode is now set to");
            Serial.println(COMServerMode);
        }

    }

    if( server.hasArg(COM_RADIONAME_PROTOCOL) )
    {
        COMProtocolTCP = server.arg(COM_RADIONAME_PROTOCOL);
    }

    if( server.hasArg(COM_RADIONAME_COM_TYPE) )
    {
        btnValue = server.arg(COM_RADIONAME_COM_TYPE);
        if( btnValue.equalsIgnoreCase(COM_RADIO_COM_TYPE_HW) )
        {
            useHardSerial = true;
        }
        else
        {
            useHardSerial = false;
        }
    }

    if( server.hasArg(COM_TARGET_IP) )
    {
        useTargetIP = server.arg(COM_TARGET_IP);
    }

    if( server.hasArg(COM_RADIONAME_GPIO_RX) )
    {
        useRxPin = server.arg(COM_RADIONAME_GPIO_RX);
    }

    if( server.hasArg(COM_RADIONAME_GPIO_TX) )
    {
        useTxPin = server.arg(COM_RADIONAME_GPIO_TX);
    }

    if( server.hasArg(COM_RADIONAME_DATALEN) )
    {
        useDataBits = server.arg(COM_RADIONAME_DATALEN);

    }

    if( server.hasArg(COM_RADIONAME_STOPBIT) )
    {
        useStopBits = server.arg(COM_RADIONAME_STOPBIT);
    }

    if( server.hasArg(COM_RADIONAME_FLOWCTRL) )
    {
        btnValue = server.arg(COM_RADIONAME_FLOWCTRL);
        if( btnValue.equalsIgnoreCase(COM_RADIO_LBL_HANDSHAKE_SW) )
        {
            useFlowCtrl = true;
        }
        else
        {
            useFlowCtrl = false;
        }
    }

    if( server.hasArg(COM_RADIONAME_PARITY) )
    {
        useParity = server.arg(COM_RADIONAME_PARITY);

    }

    if( server.hasArg(COM_SELECTION_NAME_BAUD) )
    {
        useBaudRate = server.arg(COM_SELECTION_NAME_BAUD);

    }

    nodeStoreCOMSettings();

    if( nodeRestart )
    {
        server.close();
        ESP.restart();
    }

    if( SerialOverNetwork )
    {
        if( !SONRunning )
        {
            StartSON();
        }
    }
    else
    {
        if( SONRunning )
        {
            StopSON();
        }
    }

}
//
// ---- load and handle /comsettings page ----
//
void handleComSettings()
{
    String btnValue = "";


    if( !beQuiet )
    {
        Serial.println("handleComSettings");
           for(int i = 0; i < server.args(); i++ )
           {
                Serial.print( server.argName(i) );
                Serial.print( " = " );
                Serial.println( server.arg(i) );
           }
    }

   if( !adminAccessSucceeded )
   {
       handleLoginPage();
   }

   if( server.method() == SERVER_METHOD_POST &&
        ( server.hasArg(COM_BUTTONNAME_FORMRESET) ||
         server.hasArg(COM_BUTTONNAME_COMMIT) ||
         server.hasArg(COM_BUTTONNAME_FACTORY) ||
         server.hasArg(COM_BUTTONNAME_CLOSE) ))
    {
        if( server.hasArg(COM_BUTTONNAME_FORMRESET) )
        {
            btnValue = server.arg(COM_BUTTONNAME_FORMRESET);
            if( btnValue.equals(COM_ACTION_RESET) )
            {
                nodeRestoreCOMSettings();
            }
        }

        if( server.hasArg(COM_BUTTONNAME_COMMIT) )
        {
            btnValue = server.arg(COM_BUTTONNAME_COMMIT);
            if( btnValue.equals(COM_ACTION_COMMIT) )
            {
                getComInputValues();
            }
        }

        if( server.hasArg(COM_BUTTONNAME_FACTORY) )
        {
            btnValue = server.arg(COM_BUTTONNAME_FACTORY);
            if( btnValue.equals(COM_ACTION_FACTORY) )
            {
                resetCOM2FactorySettings();
            }
        }

        handleIndexPage();
    }
    else
    {

        pageContent  = "<!DOCTYPE html>\n";
        pageContent += "<html>\n";
        pageContent += "<head>\n";
        pageContent += "<meta charset=\"utf-8\" />\n";
        pageContent += "<title>COM Setup</title>\n";
        pageContent += "</head>\n";
        pageContent += "<body bgcolor=\"#00C9FF\" text=\"#FDFF00\"";
        pageContent += "link=\"#FF0200\" vlink=\"#A900FF\" alink=\"#20FF00\">\n";
        pageContent += "<div align=\"center\"><strong>";
        pageContent += "<h1>Change local COM port settings</h1></strong></div>\n";
//        pageContent += "<form action=\"/comsettings\" method=\"get\">\n";
        pageContent += "<form action=\"/comsettings\" method=\"post\">\n";
        pageContent += "<table align=\"center\">\n";

        pageContent += "<!-- Delimiter -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "</tr>\n";
//
// ************************ ENABLE SERIAL OVER NETWORK ************************
//
        pageContent += "<tr>\n";
        pageContent += "  <td> <div align=\"center\">\n";
	pageContent += "    <optgroup label=\"" + String(COM_OPTGRP_LABEL_SON) + "\"\n";
        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_ENABLE_SON) + "</label>\n";
        pageContent += "    <input type=\"radio\" name=\"";
        pageContent +=      String(COM_RADIONAME_SON) + "\" value=\"";
        pageContent +=      String(COM_RADIO_SON_ENABLED) + "\"";

        if( SerialOverNetwork )
        {
            pageContent += String(" checked>\n");
        }
        else
        {
            pageContent += String(" unchecked>\n");
        }

        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_DISABLE_SON) + "</label>\n";
        pageContent += "    <input type=\"radio\" name=\"";
        pageContent +=      String(COM_RADIONAME_SON) + "\" value=\"";
        pageContent +=      String(COM_RADIO_SON_DISABLED) + "\"";

        if( SerialOverNetwork )
        {
            pageContent += String(" unchecked>\n");
        }
        else
        {
            pageContent += String(" checked>\n");
        }

        pageContent += "  </td>\n";
        pageContent += "<div align=\"center\">\n";
        pageContent += "  </optgroup>\n";
        pageContent += "</tr>\n";
//
// ********************************* PROTOCOL *********************************
//
        pageContent += "<tr>\n";
        pageContent += "  <td> <div align=\"center\">\n";
	pageContent += "    <optgroup label=\"" + String(COM_OPTGRP_LABEL_PROTOCOL) + "\"\n";
        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\">\n";
        pageContent += "    <label> " + String(COM_LABEL_PROTOCOL_TCP) + " \n";
        pageContent += "    <input type=\"radio\" name=\"" + String(COM_RADIONAME_PROTOCOL) + "\" value=\"" + 
                            String(COM_RADIO_PROTOCOL_TCP) + "\"";

        if( COMProtocolTCP )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\">\n";
        pageContent += "    <label> " + String(COM_LABEL_PROTOCOL_UDP) + " \n";
        pageContent += "    <input type=\"radio\" name=\"" + String(COM_RADIONAME_PROTOCOL) + "\" value=\"" + 
                            String(COM_RADIO_PROTOCOL_UDP) + "\"";

        if( COMProtocolTCP )
        {
            pageContent += " unchecked>\n";
        }
        else
        {
            pageContent += " checked>\n";
        }

        pageContent += "  </td>\n";
        pageContent += "  </optgroup>\n";
        pageContent += "</tr>\n";

//
// ******************************** SERIAL TYPE *******************************
//
        pageContent += "<!-- Serial type (sw/hw) -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String( COM_OPTGRP_LABEL_SERIAL_TYPE ) + "\">\n";

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_COM_TYPE_HW) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_COM_TYPE);
        if( useHardSerial )
        {
            pageContent += "\" value=\"" + String(COM_RADIO_COM_TYPE_HW) + "\" checked>\n";
        }
        else
        {
            pageContent += "\" value=\"" + String(COM_RADIO_COM_TYPE_HW) + "\" unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_COM_TYPE_SW) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_COM_TYPE);
        if( useHardSerial )
        {
            pageContent += "\" value=\"" + String(COM_RADIO_COM_TYPE_SW) + "\" unchecked>\n";
        }
        else
        {
            pageContent += "\" value=\"" + String(COM_RADIO_COM_TYPE_SW) + "\" checked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";

        pageContent += "<!-- Delimiter -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "</tr>\n";
//
// ****************************** OPERATION MODE ******************************
//

        pageContent += "<tr>\n";
        pageContent += "  <td> <div align=\"center\">\n";
	pageContent += "    <optgroup label=\"" + String(COM_OPTGRP_LABEL_OPERATIONMODE) + "\"\n";
        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_OPERATIONMODE_SERVER) + "</label>\n";
        pageContent += "    <input type=\"radio\" name=\"";
        pageContent +=      String(COM_RADIONAME_OPERATIONMODE) + "\" value=\"";
        pageContent +=      String(COM_RADIO_OPERATIONMODE_SERVER) + "\"";

        if( COMServerMode )
        {
            pageContent += String(" checked>\n");
        }
        else
        {
            pageContent += String(" unchecked>\n");
        }

        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_OPERATIONMODE_CLIENT) + "</label>\n";
        pageContent += "    <input type=\"radio\" name=\"";
        pageContent +=      String(COM_RADIONAME_OPERATIONMODE) + "\" value=\"";
        pageContent +=      String(COM_RADIO_OPERATIONMODE_CLIENT) + "\"";

        if( COMServerMode )
        {
            pageContent += " unchecked>\n";
        }
        else
        {
            pageContent += " checked>\n";
        }

        pageContent += "  </td>\n";
        pageContent += "<div align=\"center\">\n";
        pageContent += "  </optgroup>\n";
        pageContent += "</tr>\n";

//
// ******************************** TARGET IP *********************************
//


        pageContent += "<tr>\r";
        pageContent += "  <td> <div align=\"center\">\n";
	pageContent += "    <optgroup label=\"" + String(COM_LABEL_TARGET_IP) + "\" </td>\n";
        pageContent += "  <td> <div align=\"center\"></div></td>\n";
        pageContent += "  <td><div align=\"center\"><input type=\"text\" name=\"";
        pageContent += String(COM_TARGET_IP) + "\" value=\"" + useTargetIP + "\" maxlength=\"";
        pageContent += String(EEPROM_MAXLEN_TARGET_IP) + "\"> ";
        pageContent += "  </td>\n";
        pageContent += "  <td> <div align=\"center\"></div></td>\n";
        pageContent += "  </optgroup>\n";
        pageContent += "</tr>\n";

//
// ****************************************************************************
//
        pageContent += "<!-- Delimiter -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "</tr>\n";

//
// ********************************** RX PIN **********************************
//
        pageContent += "<!-- Rx pin -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String(COM_OPTGRP_LABEL_GPIO_RX) + "\">\n";

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_RX_00) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_00) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_00) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_RX_02) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_02) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_02) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_RX_04) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_04) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_04) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_RX_05) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_05) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_05) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_RX_09) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_09) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_09) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_RX_10) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_10) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_10) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_RX_12) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_12) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_12) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_RX_13) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_13) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_13) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_RX_14) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_14) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_14) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_RX_15) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_15) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_15) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_RX_16) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_RX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_RX_16) + "\"";

        if( useRxPin.equalsIgnoreCase(COM_RADIO_GPIO_RX_16) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";
//
// ********************************** TX PIN **********************************
//
        pageContent += "<!-- Tx pin -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String(COM_OPTGRP_LABEL_GPIO_TX) + "\">\n";

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_TX_00) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_00) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_00) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_TX_02) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_02) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_02) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_TX_04) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_04) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_04) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_TX_05) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_05) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_05) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_TX_09) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_09) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_09) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_TX_10) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_10) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_10) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_TX_12) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_12) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_12) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_TX_13) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_13) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_13) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_TX_14) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_14) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_14) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "</tr>\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "    <label>" + String(COM_LABEL_GPIO_TX_15) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_15) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_15) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_LABEL_GPIO_TX_16) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_GPIO_TX) +
                                 "\" value=\"" + String(COM_RADIO_GPIO_TX_16) + "\"";

        if( useTxPin.equalsIgnoreCase(COM_RADIO_GPIO_TX_16) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";
//
// ********************************** DATABITS **********************************
//

        pageContent += "<!-- Delimiter -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "</tr>\n";
        pageContent += "<!-- Data bits -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String(COM_OPTGRP_LABEL_DATALEN) + "\">\n";
        pageContent += "    </td>\n";

        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_DATALEN_6BIT) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_DATALEN) +
                                "\" value=\"" + String(COM_RADIO_DATALEN_SIX) + "\"";

        if( useDataBits.equalsIgnoreCase(COM_RADIO_DATALEN_SIX) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";

        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_DATALEN_7BIT) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_DATALEN) +
                                "\" value=\"" + String(COM_RADIO_DATALEN_SEVEN) + "\"";

        if( useDataBits.equalsIgnoreCase(COM_RADIO_DATALEN_SEVEN) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }
        pageContent += "    </td>\n";

        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_DATALEN_8BIT) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_DATALEN) +
                                "\" value=\"" + String(COM_RADIO_DATALEN_EIGHT) + "\"";

        if( useDataBits.equalsIgnoreCase(COM_RADIO_DATALEN_EIGHT) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }
        pageContent += "    </td>\n";

        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";
//
// ********************************** STOPBITS **********************************
//
        pageContent += "<!-- Stop bits -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String(COM_OPTGRP_LABEL_STOPBITS) + "\">\n";

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_STOPBIT_ONE) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_STOPBIT) +
                                "\" value=\"" + String(COM_RADIO_STOPBIT_ONE) + "\"";

        if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_ONE) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_STOPBIT_TWO) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_STOPBIT) +
                                "\" value=\"" + String(COM_RADIO_STOPBIT_TWO) + "\"";

        if( useStopBits.equalsIgnoreCase(COM_RADIO_STOPBIT_TWO) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";
//
// ********************************** PARITY **********************************
//


        pageContent += "<!-- Parity -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String(COM_OPTGRP_LABEL_PARITY) + "\">\n";

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_PARITY_NO) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_PARITY) +
                                "\" value=\"" + String(COM_RADIO_PARITY_NO) + "\"";

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_NO)) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_PARITY_EVEN) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_PARITY) +
                                "\" value=\"" + String(COM_RADIO_PARITY_EVEN) + "\"";

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_EVEN)) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_PARITY_ODD) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_PARITY) +
                                "\" value=\"" + String(COM_RADIO_PARITY_ODD) + "\"";

        if( useParity.equalsIgnoreCase(String(COM_RADIO_PARITY_ODD)) )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }
        pageContent += "    </td>\n";

        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";
//
// ********************************** HANDSHAKE **********************************
//


        pageContent += "<!-- Handshake -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String(COM_OPTGRP_LABEL_FLOWCTRL) + "\">\n";

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_HANDSHAKE_SW) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_FLOWCTRL) +
                                "\" value=\"" + String(COM_RADIO_LBL_HANDSHAKE_SW) + "\"";

        if( useFlowCtrl )
        {
            pageContent += " checked>\n";
        }
        else
        {
            pageContent += " unchecked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <label>" + String(COM_RADIO_LBL_HANDSHAKE_NO) + "</label>\n";
        pageContent += "        <input type=\"radio\" name=\"" + String(COM_RADIONAME_FLOWCTRL) +
                                "\" value=\"" + String(COM_RADIO_LBL_HANDSHAKE_NO) + "\"";

        if( useFlowCtrl )
        {
            pageContent += " unchecked>\n";
        }
        else
        {
            pageContent += " checked>\n";
        }

        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    <td> <div align=\"center\"> </td>\n";
        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";

//
// ****************************** TRANSFER SPEED ******************************
//

        pageContent += "<!-- Delimiter -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    </div></td>\n";
        pageContent += "</tr>\n";
        pageContent += "<!-- Baud rate -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <optgroup label=\"" + String(COM_OPTGRP_LABEL_SPEED) + "\">\n";
        pageContent += "    </td>\n";
        pageContent += "    <td> <div align=\"center\"></div></td>\n";
        pageContent += "    <td> <div align=\"center\">\n";
        pageContent += "        <select name=\"" + String(COM_SELECTION_NAME_BAUD) + "\" size=\"1\">\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B9600)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B9600) + "\">" +
                                    String(COM_OPTION_LBL_B9600) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B19200)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B19200) + "\">" +
                                    String(COM_OPTION_LBL_B19200) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B38400)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B38400) + "\">" +
                                    String(COM_OPTION_LBL_B38400) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B57600)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B57600) + "\">" +
                                    String(COM_OPTION_LBL_B57600) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B74880)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B74880) + "\">" +

                                    String(COM_OPTION_LBL_B74880) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B115200)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B115200) + "\">" +
                                    String(COM_OPTION_LBL_B115200) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B230400)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B230400) + "\">" +
                                    String(COM_OPTION_LBL_B230400) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B460800)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B460800) + "\">" +
                                    String(COM_OPTION_LBL_B460800) + "</option>\n";

        pageContent += "            <option ";
        if( useBaudRate.equalsIgnoreCase(String(COM_OPTION_VALUE_B921600)) )
        {
            pageContent += "        selected ";
        }
        pageContent += "            value=\"" + String(COM_OPTION_VALUE_B921600) + "\">" +
                                    String(COM_OPTION_LBL_B921600) + "</option>\n";

        pageContent += "        </select>\n";
        pageContent += "    </td>\n";
        pageContent += "    </optgroup>\n";
        pageContent += "</tr>\n";

        pageContent += "<!-- Delimiter -->\n";
        pageContent += "<tr>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "    <td> <div align=\"center\"> <hr>\n";
        pageContent += "    </div></td>\n";
        pageContent += "</tr>\n";
        pageContent += "</table>\n";
        pageContent += "<div align=\"center\">\n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(COM_BUTTONNAME_FORMRESET) + "\" value=\"" + 
                       String(COM_ACTION_RESET) + "\"> \n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(COM_BUTTONNAME_COMMIT) + "\" value=\"" + 
                       String(COM_ACTION_COMMIT) + "\"> \n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(COM_BUTTONNAME_FACTORY) + "\" value=\"" + 
                       String(COM_ACTION_FACTORY) + "\"> \n";

        pageContent += "<input type=\"submit\" name=\"" +
                       String(COM_BUTTONNAME_CLOSE) + "\" value=\"" + 
                       String(COM_ACTION_CLOSE) + "\"> \n";

        pageContent += "</div>\n";
        pageContent += "</form>\n";
        pageContent += "<br>";
        pageContent += "<div align=\"center\">\n";
        pageContent += "*If you select Software-Serial, you also have to specify\n";
        pageContent += " a GPIO to use as Rx and one to use as Tx\n";
        pageContent += "</div>\n";

        pageContent += "</body>\n";
        pageContent += "</html>\n";

        server.send(200, "text/html", pageContent);  
    }

    return;

}

// ************************************************ LOOP ************************************

#define  HAS_STATUS_LED
const int RED = 15;
const int GREEN = 12;
const int BLUE = 13;
#define LED_INTVAL   2000
void LEDOff( void )
{
#ifdef HAS_STATUS_LED

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);

    analogWrite(RED, 0 );
    analogWrite(GREEN, 0);
    analogWrite(BLUE, 0);

#endif // HAS_STATUS_LED
}

void signalLED( void )
{
#ifdef HAS_STATUS_LED

    static int currLED, lastLED;
    static long lastLEDChange;
    static int lastChangeMode;

    if( millis() - lastLEDChange >= LED_INTVAL )
    {
        if( lastChangeMode )
        {
            if( SONConnected )
            {
                currLED = GREEN;
            }
            else
            {
                currLED = RED;
            }

            lastChangeMode = 0;
        }
        else
        {
            if( COMServerMode )
            {
                currLED = BLUE;
            }
            else
            {
                currLED = GREEN;
            }
            
            lastChangeMode = 1;
        }

        analogWrite(lastLED, 0);
        analogWrite(currLED, 127);
        lastLED = currLED;
        lastLEDChange = millis();

    }

#endif // HAS_STATUS_LED
}

//
// ************************************************************************
// main loop is quite simple ...
// ************************************************************************
//
void loop() 
{
    signalLED();

    if( SerialOverNetwork )
    {
        if( SONConnected )
        {
            if( COMServerMode )
            {
                if( !incomingTelnetConnection.connected() )
                {
                    if( !beQuiet )
                    {
                        Serial.println("Client has dropped Connection");              
                    }

                    incomingTelnetConnection.stop();
                    SONConnected = false;

                    if( !beQuiet )
                    {
                        Serial.println("free'd IP slot for new client to connect");                
                    }
                } 
            }
        }

      
        //check if there are any new clients
        if( COMServerMode )
        {
            if (TelnetServer.hasClient())
            {
                if( !beQuiet )
                {
                    Serial.println("Connection request");          
                }

                if( !SONConnected )
                {
                    if( !beQuiet )
                    {
                        Serial.println("SON is NOT connected");              
                    }
                    incomingTelnetConnection = TelnetServer.available();
                    SONConnected = true;
                }
                else
                {
                    if( !incomingTelnetConnection.connected() )
                    {
                        incomingTelnetConnection.stop();
                        incomingTelnetConnection = TelnetServer.available();
                        SONConnected = true;
                    }
                    else
                    {
                        WiFiClient Client = TelnetServer.available();
                        Client.stop();
                        SONConnected = false;
                    }
                }
            }
        }
        else
        {

            if( !SONConnected )
            {
                if( !beQuiet )
                {
                    Serial.println("try to connect to server");
                }

                IPAddress targetServer;
                targetServer.fromString(useTargetIP);
                if (outgoingTelnetConnection.connect(targetServer, SON_SERVER_PORT)) 
                {
                    if( !beQuiet )
                    {
                        Serial.println("connected");
                    }
                    SONConnected = true;
                }
            }
            else
            {
                if (!outgoingTelnetConnection.connected()) 
                {
                    if( !beQuiet )
                    {
                        Serial.println("Server has dropped Connection");              
                    }

                    outgoingTelnetConnection.stop();
                    SONConnected = false;
                }
            }

        }

        processSON();
    }
    
    delay(1);

    server.handleClient();
    
    delay(1);

}

//
// <meta http-equiv="refresh" content="3; URL=http://www.example.com/">
//

// ************************************************ END *************************************

#ifdef NEVERDEF

#include "ioStreams.h"


ioStreams::ioStreams()
{
 _Serial = NULL;
}

void ioStreams::begin(HardwareSerial *serIn)
{
 _Serial = serIn;
 _Serial->begin(38400);
 _Serial->println("Ready to Rip!");
}

// ************************************************ END *************************************
#ifndef _IO_STREAMS_
#define _IO_STREAMS_

#include <inttypes.h>
#include <stdarg.h>
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

extern "C" {
}

class ioStreams
{
 public:
   ioStreams();
   void begin(HardwareSerial *serIn);    
 private:
   HardwareSerial *_Serial;
};

#endif // _IO_STREAMS_


// ************************************************ END *************************************

#endif
