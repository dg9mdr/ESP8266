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
//
// The idea behind this is to connect to serial devices across a WLAN. 
//
// To reach this, you may setup one ESP-8266 node as a "COM-Server". 
// To do so, browse to http://Your.ESP.IP.ADDRESS:8080 
//
// REMEMBER TO SET MATCHING VALUES FOR YOUR WLAN ACCESS!
// On the two lines about 113 and below set
// #define FACTORY_WLAN_SSID to YOUR SSID
// #define FACTORY_WLAN_PASSPHRASE to YOUR PASSPHRASE for access
//
// You can login to the admin page. Default Password is esp8266. Select
// COM-Port settings and activate COM-Server (default).
// After Restart the ESP you can connect any device with serial port
// to the ESP. Be careful, if you connect Rx/Tx! The ESP is NOT 5V tolerant
// and may be destroyed, if you don't use a levelshifter to 3V3.
// Now you may telnet to YOUR.ESP.IP.ADDRESS port 1234. All input is send
// to the connected device ia rs232. All data the serial device is sending
// is received over WLAN so you will see the output.
// This may be useful for e.g. modems, ...
// It is possible to setup another ESP as a COM-Client.
// Configuring is done alike setup in server mode.
// Now it's possible to connect two serial devices to their rs232 interfaces.
// Data transfer is done bidirectional.
// Note, that this is still a WORK IN PROGRESS pre-release ...
// Have fun
//
// ************************************************************************
//
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
// 1st version: 12/05/2016 - work in progress pre-release
//
// update ....: 12/08/2916 - replaced serial-log by SimpleLog-class
//                           replaced inline eeprom-access by dsEeprom-class
//                           check for autoconnect in setup()
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
// NOTE: This will become obselete because of replacing direct output
//       to Serial by SimpleLog-class!
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
#include <SoftwareSerial.h>
#include "ioStreams.h"

#include "dsEeprom.h"           // simplified access to onchip EEPROM
#include "SimpleLog.h"          // fprintf()-like logging

// ************************************************************************
// Logging
// ************************************************************************
// 
static SimpleLog Logger;
//
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
// ************************************************************************
//
// ---------- global dsEeprom instance and layout for extended EEPROM data
//
// ************************************************************************
//
dsEeprom eeprom;
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
        Logger.Log(LOGLEVEL_DEBUG, "storing SSID: %s\n", wlanSSID.c_str());
    }

    eeprom.storeString(  wlanSSID,      EEPROM_MAXLEN_WLAN_SSID,       EEPROM_POS_WLAN_SSID );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing Passphrase: %s\n", wlanPasswd.c_str());
    }

    eeprom.storeString(  wlanPasswd,    EEPROM_MAXLEN_WLAN_PASSPHRASE, EEPROM_POS_WLAN_PASSPHRASE );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing ServerIP: %s\n", wwwServerIP.c_str());
    }

    eeprom.storeString(  wwwServerIP,   EEPROM_MAXLEN_SERVER_IP,       EEPROM_POS_SERVER_IP );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing ServerPort: %s\n", wwwServerPort.c_str());
    }

    eeprom.storeString(  wwwServerPort, EEPROM_MAXLEN_SERVER_PORT,     EEPROM_POS_SERVER_PORT );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing Nodename: %s\n", nodeName.c_str());
    }

    eeprom.storeString(  nodeName,      EEPROM_MAXLEN_NODENAME,        EEPROM_POS_NODENAME );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing adminPasswd: %s\n", adminPasswd.c_str());
    }

    eeprom.storeString(  adminPasswd,   EEPROM_MAXLEN_ADMIN_PASSWORD,  EEPROM_POS_ADMIN_PASSWORD );
    
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing useDHCP: %d\n", useDhcp);
    }

    eeprom.storeBoolean( (char*) &useDhcp,         EEPROM_POS_USE_DHCP );
    crcCalc = eeprom.crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "calculated crc over eeprom: %x\n", crcCalc);
    }

    eeprom.storeRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );

    eeprom.validate();

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

    if( eeprom.isValid() || IGNORE_IF_CONDITION )
    {
        crcCalc = eeprom.crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );

        eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        crcRead = atol(data.c_str());

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "restored crc: %x calc crc: %x\n", crcRead, crcCalc );
        }

        if( (crcCalc == crcRead) || IGNORE_IF_CONDITION )
        {
            eeprom.restoreString(  wlanSSID,                 EEPROM_POS_WLAN_SSID,       EEPROM_MAXLEN_WLAN_SSID );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored SSID: %s\n", wlanSSID.c_str());
            }

            eeprom.restoreString(  wlanPasswd,               EEPROM_POS_WLAN_PASSPHRASE, EEPROM_MAXLEN_WLAN_PASSPHRASE );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored Passphrase: %s\n", wlanPasswd.c_str());
            }

            eeprom.restoreString(  wwwServerIP,              EEPROM_POS_SERVER_IP,       EEPROM_MAXLEN_SERVER_IP );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored ServerIP: %s\n", wwwServerIP.c_str());
            }

            eeprom.restoreString(  wwwServerPort,            EEPROM_POS_SERVER_PORT,     EEPROM_MAXLEN_SERVER_PORT );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored ServerPort: %s\n", wwwServerPort.c_str());
            }

            eeprom.restoreString(  nodeName,                 EEPROM_POS_NODENAME,        EEPROM_MAXLEN_NODENAME );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored Nodename: %s\n", nodeName.c_str());
            }

            eeprom.restoreString(  adminPasswd,              EEPROM_POS_ADMIN_PASSWORD,  EEPROM_MAXLEN_ADMIN_PASSWORD );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored adminPasswd: %s\n", adminPasswd.c_str());
            }

            eeprom.restoreBoolean( (char*) &useDhcp,         EEPROM_POS_USE_DHCP );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored useDHCP: %d\n", useDhcp);
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
        Logger.Log(LOGLEVEL_DEBUG, "storing serial over network enable: %d\n", SerialOverNetwork);
    }

    eeprom.storeBoolean( (char*) &SerialOverNetwork, EEPROM_POS_SON );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing act as COM-Server: %d\n", COMServerMode);
    }

    eeprom.storeBoolean( (char*) &COMServerMode, EEPROM_POS_COM_SERVER );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing taget IP: %s\n", useTargetIP.c_str());
    }

    eeprom.storeString( useTargetIP, EEPROM_MAXLEN_TARGET_IP, EEPROM_POS_TARGET_IP );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing use TCP protcol: %d\n", COMProtocolTCP);
    }

    eeprom.storeBoolean( (char*) &COMProtocolTCP, EEPROM_POS_COM_PROTOCOL_TCP );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing use serial hardware: %d\n", useHardSerial);
    }

    eeprom.storeBoolean( (char*) &useHardSerial, EEPROM_POS_COM_USE_HW_SERIAL );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing RxPin: %s\n", useRxPin.c_str());
    }

    eeprom.storeString( useRxPin, EEPROM_MAXLEN_COM_USE_RX_GPIO, EEPROM_POS_COM_USE_RX_GPIO );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing TxPin: %s\n", useTxPin.c_str());
    }

    eeprom.storeString( useTxPin, EEPROM_MAXLEN_COM_USE_TX_GPIO, EEPROM_POS_COM_USE_TX_GPIO );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing databits: %s\n", useDataBits.c_str());
    }

    eeprom.storeString( useDataBits, EEPROM_MAXLEN_COM_USE_DATABITS, EEPROM_POS_COM_USE_DATABITS );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing stopbits: %s\n", useStopBits.c_str());
    }

    eeprom.storeString( useStopBits, EEPROM_MAXLEN_COM_USE_STOPBITS, EEPROM_POS_COM_USE_STOPBITS );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing use flow control: %d\n", useFlowCtrl);
    }

    eeprom.storeBoolean( (char*) &useFlowCtrl, EEPROM_POS_COM_USE_FLOWCTRL );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing parity: %s\n", useParity.c_str());
    }

    eeprom.storeString( useParity, EEPROM_MAXLEN_COM_USE_PARITY, EEPROM_POS_COM_USE_PARITY );

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing use baudrate: %s\n", useBaudRate.c_str());
    }

    eeprom.storeString(  useBaudRate,   EEPROM_MAXLEN_COM_USE_BAUDRATE,  EEPROM_POS_COM_USE_BAUDRATE );

    crcCalc = eeprom.crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );
    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG, "storing CRC: %x\n", crcCalc);
    }

    eeprom.storeRaw( (char*) &crcCalc, EEPROM_MAXLEN_CRC32, EEPROM_POS_CRC32 );

    eeprom.validate();

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

    if( eeprom.isValid() || IGNORE_IF_CONDITION )
    {
        crcCalc = eeprom.crc( EEPROM_DATA_BEGIN, EEPROM_BLOCK_SIZE );

        eeprom.restoreRaw( (char*) &crcRead, EEPROM_POS_CRC32, EEPROM_MAXLEN_CRC32, EEPROM_MAXLEN_CRC32);
        crcRead = atol(data.c_str());

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "restored crc: %x calc crc: %x\n", crcRead, crcCalc);
        }

        if( (crcCalc == crcRead) || IGNORE_IF_CONDITION )
        {
            eeprom.restoreBoolean( (char*) &SerialOverNetwork, EEPROM_POS_SON );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored serial over network enable: %d\n", SerialOverNetwork);
            }

            eeprom.restoreBoolean( (char*) &COMServerMode, EEPROM_POS_COM_SERVER );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored act as COM-Server: %d\n", COMServerMode);
            }

            eeprom.restoreString( useTargetIP, EEPROM_POS_TARGET_IP, EEPROM_MAXLEN_TARGET_IP );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored taget IP: %s\n", useTargetIP.c_str());
            }

            eeprom.restoreBoolean( (char*) &COMProtocolTCP , EEPROM_POS_COM_PROTOCOL_TCP );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored use TCP protcol: %d\n", COMProtocolTCP);
            }

            eeprom.restoreBoolean( (char*) &useHardSerial, EEPROM_POS_COM_USE_HW_SERIAL );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored use serial hardware: %d\n", useHardSerial);
            }

            eeprom.restoreString( useRxPin, EEPROM_POS_COM_USE_RX_GPIO, EEPROM_MAXLEN_COM_USE_RX_GPIO );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored RxPin: %s\n", useRxPin.c_str());
            }

            eeprom.restoreString( useTxPin, EEPROM_POS_COM_USE_TX_GPIO, EEPROM_MAXLEN_COM_USE_TX_GPIO );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored TxPin: %s\n", useTxPin.c_str());
            }

            eeprom.restoreString( useDataBits, EEPROM_POS_COM_USE_DATABITS, EEPROM_MAXLEN_COM_USE_DATABITS );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored databits: %s\n", useDataBits.c_str());
            }

            eeprom.restoreString( useStopBits,  EEPROM_POS_COM_USE_STOPBITS, EEPROM_MAXLEN_COM_USE_STOPBITS );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored stopbits: %s\n", useStopBits.c_str());
            }

            eeprom.restoreBoolean( (char*) &useFlowCtrl, EEPROM_POS_COM_USE_FLOWCTRL );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored use flow control: %d\n", useFlowCtrl);
            }

            eeprom.restoreString( useParity,  EEPROM_POS_COM_USE_PARITY, EEPROM_MAXLEN_COM_USE_PARITY );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored parity: %s\n", useParity.c_str());
            }

            eeprom.restoreString(  useBaudRate,     EEPROM_POS_COM_USE_BAUDRATE, EEPROM_MAXLEN_COM_USE_BAUDRATE );

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, "restored use baudrate: %s\n", useBaudRate.c_str());
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
//        Logger.Log(LOGLEVEL_DEBUG, "%s\n", pageContent.c_str());
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
            Logger.Log(LOGLEVEL_DEBUG,"SetupSON -> start server");
        }      
//        pTelnetServer = new WiFiServer(SON_SERVER_PORT);
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "SetupSON -> server started\n");
        }      
        TelnetServer.setNoDelay(true);
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG,"SetupSON -> nodelay set\n");
        }              
        TelnetServer.begin();
    }
    else
    {
//        IPAddress targetServer = IPAddress.fromString(useTargetIP);
          if( !beQuiet )
          {
              Logger.Log(LOGLEVEL_DEBUG,"try to connect to server\n");
          }

        IPAddress targetServer;
        targetServer.fromString(useTargetIP);
        if (outgoingTelnetConnection.connect(targetServer, SON_SERVER_PORT)) 
        {
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG,"connected\n");
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

#define MAX_MSECS_FOR_AUTOCONNECT   10000 // 10 sec

void setup() 
{
    static long mSecsTriedConnect;

    LEDOff();

    adminAccessSucceeded = false;
    beQuiet = BE_QUIET;
    IPAddress localIP;

    // startup serial console ...
    Serial.begin(SERIAL_BAUD);
    Logger.Init(LOGLEVEL_DEBUG, &Serial);

//  if( eeprom.init( 1024, 0x00, LOGLEVEL_QUIET ) < EE_STATUS_INVALID_CRC )  
    if( eeprom.init( 1024, eeprom.version2Magic(), LOGLEVEL_QUIET ) < EE_STATUS_INVALID_CRC )  
    {
      if( eeprom.isValid() )
      {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG,"eeprom content is valid\n");
        }

        nodeRestoreAdminSettings();
        nodeRestoreCOMSettings();
      }
      else
      {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG,"INVALID eeprom content!\n");
            Logger.Log(LOGLEVEL_DEBUG,"reset node to factory settings.\n");
        }
        // if eeprom has no valid signature set node data to factory defaults
        //
        resetAdmin2FactorySettings();
        nodeStoreAdminSettings();

        resetCOM2FactorySettings();
        nodeStoreCOMSettings();
      }
    }
    else
    {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG,"INVALID eeprom content!\n");
            Logger.Log(LOGLEVEL_DEBUG,"reset node to factory settings.\n");
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
        Logger.Log(LOGLEVEL_DEBUG, "EEPROM in use: %d\n", EEPROM_DATA_END);
    }

    if( wlanSSID.length() > 0 &&
        wlanPasswd.length() > 0 )
    {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG, "Connecting to >%s< password from eeprom is >%s<\n", 
                wlanSSID.c_str(), wlanPasswd.c_str());
        }

        mSecsTriedConnect = millis();
        // loop until connection is established
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG,"First try auto connect\n");
        }

        while (WiFi.status() != WL_CONNECTED && (millis() - mSecsTriedConnect) < MAX_MSECS_FOR_AUTOCONNECT) 
        {
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG, ".");
            }
            delay(500);
        }

        if( WiFi.status() != WL_CONNECTED) 
        {

            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG,"auto connect FAILED\n");

            }
            // Connect to WiFi network
            WiFi.begin( wlanSSID.c_str(), wlanPasswd.c_str());
   
            // loop until connection is established
            while (WiFi.status() != WL_CONNECTED) 
            {
                delay(500);
                if( !beQuiet )
                {
                    Logger.Log(LOGLEVEL_DEBUG, ".");
                }
            }
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG,"WiFi connected\n");
            }

        }
        else
        {
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG,"AUTO CONNECTED\n");
            }
        }

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
        Logger.Log(LOGLEVEL_DEBUG, "Webserver started. URL is: http://%s:%s\n", wwwServerIP.c_str(), wwwServerPort.c_str());
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
            Logger.Log(LOGLEVEL_DEBUG, "setup -> SON active -> setup and start SON\n");
            Logger.Log(LOGLEVEL_DEBUG, "setup -> switch off logging!\n");
        }
        Logger.SetLevel (LOGLEVEL_QUIET );

        SetupSON();
        StartSON();
    }
    else
    {
//        Logger.Log(LOGLEVEL_DEBUG, "setup -> SON is off but switch off logging!\n");
//        Logger.SetLevel (LOGLEVEL_QUIET );
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
            Logger.Log(LOGLEVEL_DEBUG,"handleLoginPage -> handleAdminPage\n");
        }
        handleAdminPage();
    }
    else
    {

        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG,"handleLoginPage\n");
            for(int i = 0; i < server.args(); i++ )
            {
                Logger.Log(LOGLEVEL_DEBUG,"%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
            }
        }

        if( server.method() == SERVER_METHOD_POST && server.hasArg(LOGIN_FIELDNAME_PASSWORD) )
        {
            if( !beQuiet )
            {
                Logger.Log(LOGLEVEL_DEBUG,"POST REQUEST\n");
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
                Logger.Log(LOGLEVEL_DEBUG,"GET REQUEST\n");
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
        Logger.Log(LOGLEVEL_DEBUG,"getAdminInputValues\n");

            for(int i = 0; i < server.args(); i++ )
            {
                Logger.Log(LOGLEVEL_DEBUG,"%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
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
                Logger.Log(LOGLEVEL_DEBUG,"Admin password changed ... reset autologin flag\n");
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
       Logger.Log(LOGLEVEL_DEBUG,"handleAdminPage\n");
       for(int i = 0; i < server.args(); i++ )
       {
                Logger.Log(LOGLEVEL_DEBUG,"%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
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

// Logger.Log(LOGLEVEL_DEBUG, "%s\n", WiFi.macAddress().c_str());
// -> das hier NOCH KEINESFALLS aktivieren! WiFi.Log(LOGLEVEL_DEBUG,Diag(Serial);

// Logger.Log(LOGLEVEL_DEBUG, "%d\n",ESP.getFreeHeap());
// Logger.Log(LOGLEVEL_DEBUG, "%d\n", ESP.getChipId());
// Logger.Log(LOGLEVEL_DEBUG, "%d\n", ESP.getFlashChipId());
// Logger.Log(LOGLEVEL_DEBUG, "%d\n", ESP.getFlashChipSize());
// Logger.Log(LOGLEVEL_DEBUG, "%d\n", ESP.getFlashChipSpeed());
// Logger.Log(LOGLEVEL_DEBUG, "%d\n", ESP.getCycleCount());
// Logger.Log(LOGLEVEL_DEBUG, "%d\n", ESP.getVcc());

    if( !beQuiet )
    {
        Logger.Log(LOGLEVEL_DEBUG,"handleIndexPage");
        for(int i = 0; i < server.args(); i++ )
        {
                Logger.Log(LOGLEVEL_DEBUG,"%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
        }
    }


    if( server.method() == SERVER_METHOD_POST &&
        server.hasArg(INDEX_BUTTONNAME_ADMIN)  )
    {
        if( !beQuiet )
        {
            Logger.Log(LOGLEVEL_DEBUG,"POST REQUEST\n");
            // form contains user input and has been postet
            // to server

            for(int i = 0; i < server.args(); i++ )
            {
                Logger.Log(LOGLEVEL_DEBUG,"%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
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
            Logger.Log(LOGLEVEL_DEBUG,"GET REQUEST\n");
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
        Logger.Log(LOGLEVEL_DEBUG,"sendAuthFailedPage\n");
           for(int i = 0; i < server.args(); i++ )
           {
                Logger.Log(LOGLEVEL_DEBUG,"%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
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
            Logger.Log(LOGLEVEL_DEBUG,"POST REQUEST\n");
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
        Logger.Log(LOGLEVEL_DEBUG,"getComInputValues ...\n");
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
            Logger.Log(LOGLEVEL_DEBUG, "COMServerMode is now set to %d\n", COMServerMode);
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
        Logger.Log(LOGLEVEL_DEBUG,"handleComSettings\n");
           for(int i = 0; i < server.args(); i++ )
           {
                Logger.Log(LOGLEVEL_DEBUG,"%s = %s\n", server.argName(i).c_str(), server.arg(i).c_str() );
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
                        Logger.Log(LOGLEVEL_DEBUG,"Client has dropped Connection\n");              
                    }

                    incomingTelnetConnection.stop();
                    SONConnected = false;

                    if( !beQuiet )
                    {
                        Logger.Log(LOGLEVEL_DEBUG,"free'd IP slot for new client to connect\n");                
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
                    Logger.Log(LOGLEVEL_DEBUG,"Connection request\n");          
                }

                if( !SONConnected )
                {
                    if( !beQuiet )
                    {
                        Logger.Log(LOGLEVEL_DEBUG,"SON is NOT connected\n");              
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
                    Logger.Log(LOGLEVEL_DEBUG,"try to connect to server\n");
                }

                IPAddress targetServer;
                targetServer.fromString(useTargetIP);
                if (outgoingTelnetConnection.connect(targetServer, SON_SERVER_PORT)) 
                {
                    if( !beQuiet )
                    {
                        Logger.Log(LOGLEVEL_DEBUG,"connected\n");
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
                        Logger.Log(LOGLEVEL_DEBUG,"Server has dropped Connection\n");              
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
