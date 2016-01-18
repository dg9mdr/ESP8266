/*
 * **************************************************
 * definitions for an ESP8266 class
 * (C) 2014 Dirk Schanz aka dreamshader
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------
 *
 * special thanks for inspiration to
 * http://www.electrodragon.com/w/Wi07c
 * https://nurdspace.nl/ESP8266
 *
 * **************************************************
*/

#ifndef _ESP8266_H_
#define _ESP8266_H_

#include <Arduino.h>


#define CHARP   char*



//
// * ****************** API related *****************
//
#define ESP8266_SUCCESS    1
#define ESP8266_FAIL       0

#define CWMODE_UNKNOWN     0
#define CWMODE_STA         1
#define CWMODE_AP          2
#define CWMODE_MIX         3

#define TCP_CONNECTION     1
#define UDP_CONNECTION     2

#define ESP8266_UNDEF         0
#define ESP8266_READY         1
#define ESP8266_JOINED 	      2
#define ESP8266_LISTENING     3
#define ESP8266_ACCEPTED      4
#define ESP8266_CONNECTED     5
#define ESP8266_DATA_INCOMING 6
#define ESP8266_DISCONNECTED  7
#define ESP8266_BUSY          8
#define ESP8266_ONLINE        9
#define ESP8266_OFFLINE      10
#define ESP8266_NO_ESP       11	
#define ESP8266_ERROR        12	
#define ESP8266_FATAL        13	

#define ESP8266_ERR_NOERR  0

//
// * ************* for internal use only ************
//

#define CR   '\r'
#define LF   '\n'
#define LFCR '\n\r'

#define MAX_RINGBUFFER      128
#define SERIAL_DFLT_TMOUT  1000

// #include <avr/pgmspace.h>
// 
// use PROGMEM to optimize usage of SRAM
//
// PROGMEM  dataType  variableName[] = {}; // use this form
// prog_char      - a signed char (1 byte) -127 to 128
// prog_uchar     - an unsigned char (1 byte) 0 to 255
// prog_int16_t   - a signed int (2 bytes) -32,767 to 32,768
// prog_uint16_t  - an unsigned int (2 bytes) 0 to 65,535
// prog_int32_t   - a signed long (4 bytes) -2,147,483,648 to * 2,147,483,647.
// prog_uint32_t  - an unsigned long (4 bytes) 0 to 4,294,967,295

#define ESP_RSP_READY       "ready"
#define ESP_RSP_OK          "OK"
#define ESP_RSP_ERROR       "ERROR"
#define ESP_RSP_NO_CHANGE   "no change"
#define ESP_RSP_UNKNOWN_CMD "no this fun"
#define ESP_RSP_BUSY        "busy"
#define ESP_RSP_LINK        "Linked"
#define ESP_RSP_UNLINK      "Unlink"
#define ESP_RSP_DATAIN      "+IPD"
#define ESP_RSP_LINKIN      "Link"

#define ESP8266_ATPLUS      "AT+"
#define ESP8266_QUICK_CHECK "AT"
#define ESP8266_RESTART     "RST"
#define ESP8266_FIRMWARE    "GMR"
#define ESP8266_CWMODE      "CWMODE"
#define ESP8266_CWJAP       "CWJAP"
#define ESP8266_CWLAP       "CWLAP"
#define ESP8266_CWQAP       "CWQAP"
#define ESP8266_CWSAP       "CWSAP"
#define ESP8266_CWLIF       "CWLIF"
#define ESP8266_CIPSTATUS   "CIPSTATUS"
#define ESP8266_CIPSTART    "CIPSTART"
#define ESP8266_CIPMODE     "CIPMODE"
#define ESP8266_CIPSEND     "CIPSEND"
#define ESP8266_CIPCLOSE    "CIPCLOSE"
#define ESP8266_CIFSR       "CIFSR"
#define ESP8266_CIPMUX      "CIPMUX"
#define ESP8266_CIPSERVER   "CIPSERVER"
#define ESP8266_CIPSTO      "CIPSTO"

class ESP8266
{
private:
  Stream& _serial;    // rs232 where the module is connected to
  uint8_t _reset_pin; // pin where RST pin of the module is connected to
  Stream *_p_debug;   // point to where debug info should go to
  uint8_t _dbgLevel;  // debug info level
  uint8_t _do_debug;  // switch debug on/off
  uint8_t _mode;      // setMode
  uint8_t _espError;  // internel error code

  uint16_t _pWrite;   // write index of ringbuffer
  uint16_t _pRead;    // read index of ringbuffer
                      // byte buffer to read module responses
  byte _buffer[MAX_RINGBUFFER];
  String _command;    // the one and only string item for contruct AT-commands

public:
  uint8_t status;     // status of module
  uint8_t errno;      // current error code

protected:
  uint16_t readResponse(void);
  uint8_t parseToken(char* token);
  uint8_t checkStatus(void);

  uint8_t doBegin();
  uint8_t setMode(uint8_t mode);
  uint8_t getMode(void);
  uint8_t doReset(uint8_t hwflag);
  uint8_t reset(void);
  CHARP getFirmware(void);
  CHARP doQuickCheck(void);
  uint8_t apJoin(const char *ssid, const char *password);
  uint8_t apList(void);
  CHARP apCurr(void);
  CHARP getIP(void);
  uint8_t apStart(const char *ssid, const char *password,
                  uint8_t channel, uint8_t encryption);
  uint8_t apQuit(void);
  uint8_t doConnect(uint8_t ctype, const char* ipAddress, uint16_t port, uint8_t channel);
  uint8_t doSend(int8_t connId, const char* sdata, uint8_t slen);
  uint8_t doReceive(int8_t connId, const char* buffer, uint8_t buflen);
  uint8_t doAvailable(int8_t connId);
  uint8_t doDisconnect(int8_t channel);
  uint8_t doMultiChannel(void);
  uint8_t doSingleChannel(void);
  uint8_t getChannel(void);
  uint8_t doStartServer(uint16_t port);
  uint8_t doStopServer(void);
  uint8_t doSetServerTimeout(uint16_t tmout);
  CHARP doGetServerTimeout(void);


public:
// misc internal stuff
  void setDbg( Stream *p, uint8_t level );
  void setDbgLevel( uint8_t level );
  void dbgOn( void );
  void dbgOff( void );
// ESP8266 related
  ESP8266(Stream& s = Serial, uint8_t hwreset = 0): \
          _serial(s), _reset_pin(hwreset), _p_debug(NULL), \
          _dbgLevel(0), _do_debug(0), _mode(CWMODE_UNKNOWN), \
          _espError(ESP8266_ERR_NOERR), \
          _pWrite(0), _pRead(0), \
          status(ESP8266_UNDEF), errno(ESP8266_ERR_NOERR) {}
  uint8_t begin(void);
// utility functions
  uint8_t restart();
  CHARP firmware(void);
  CHARP quickCheck(void);
// modes
  uint8_t APMode(void);
  uint8_t STAMode(void);
  uint8_t MIXMode(void);
  uint8_t Mode(void);
// use single/multi channel mode
  uint8_t singleChannel(void); // default
  uint8_t multiChannel(void);
  uint8_t Channels(void);
// STA mode related
  uint8_t joinAP(const char *ssid, const char *password);
  CHARP whichAP(void);
  uint8_t listAP(void);
  CHARP IP(void);
// AP mode related
  uint8_t startAP(const char *ssid, const char *password,
                         uint8_t channel = 0, uint8_t encryption = 0);
  uint8_t quitAP(void);
// client side
  uint8_t connectTCP(const char* ipAddress, uint16_t port = 0, uint8_t channel = 0);
  uint8_t connectUDP(const char* ipAddress, uint16_t port = 0, uint8_t channel = 0);
  uint8_t send(int8_t connId = -1, 
                      const char* sdata = NULL, uint8_t slen = 0);
  uint8_t receive(int8_t connId = -1, 
                         const char* buffer = NULL, uint8_t buflen = 0);
  uint8_t available(int8_t connId = -1);
  uint8_t disconnect(int8_t channel = -1);
// server 
  uint8_t startServer(uint16_t port = 0);
  uint8_t stopServer(void);
  uint8_t setServerTimeout(uint16_t tmout = 0);
  CHARP ServerTimeout(void);
  void Response(void);


};

#endif // _ESP8266_H_

