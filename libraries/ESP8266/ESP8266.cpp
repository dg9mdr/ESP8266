/*
 * **************************************************
 * implementation for an ESP8266 class
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

#include "ESP8266.h"

//
// - --------- public methods - wrapper only ----------
//
uint8_t ESP8266::begin(void) 
{
  return(doBegin());
}
//
// - --------------------------------------------------
//
CHARP ESP8266::firmware(void)
{
  return(getFirmware());
}
//
// - --------------------------------------------------
//
CHARP ESP8266::quickCheck(void)
{
  return(doQuickCheck());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::APMode(void)
{
  return(setMode(CWMODE_STA));
}
//
// - --------------------------------------------------
//

uint8_t ESP8266::STAMode(void)
{
  return(setMode(CWMODE_AP));
}
//
// - --------------------------------------------------
//

uint8_t ESP8266::MIXMode(void)
{
  return(setMode(CWMODE_MIX));
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::Mode(void)
{
  return(getMode());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::restart(void)
{
  return(reset());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::joinAP(const char *ssid, const char *password) 
{
  if( ssid != NULL )
  {
    return(apJoin(ssid, password));
  }
  else
  {
    return(ESP8266_FAIL);
  }
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::listAP(void)
{
  return(apList());
}
//
// - --------------------------------------------------
//
CHARP ESP8266::whichAP()
{
  return(apCurr());
}
//
// - --------------------------------------------------
//
CHARP ESP8266::IP()
{
  return(getIP());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::startAP(const char *ssid, const char *password,
                         uint8_t channel, uint8_t encryption)
{
  if( ssid != NULL )
  {
    return(apStart(ssid, password, channel, encryption));
  }
  else
  {
    return(ESP8266_FAIL);
  }

}
//
// - --------------------------------------------------
//
uint8_t ESP8266::quitAP(void)
{
  return(apQuit());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::connectTCP(const char* ipAddress, uint16_t port, uint8_t channel)
{
  if( ipAddress != NULL )
  {
    return(doConnect(TCP_CONNECTION, ipAddress, port, channel));
  }
  else
  {
    return(ESP8266_FAIL);
  }
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::connectUDP(const char* ipAddress, uint16_t port, uint8_t channel)
{
  if( ipAddress != NULL )
  {
    return(doConnect(UDP_CONNECTION, ipAddress, port, channel));
  }
  else
  {
    return(ESP8266_FAIL);
  }
}
//
// - --------------------------------------------------
//

uint8_t ESP8266::send(int8_t connId, 
                      const char* sdata, uint8_t slen)
{
  if( sdata != NULL && slen > 0 )
  {
    return(doSend(connId, sdata, slen));
  }
  else
  {
    return(ESP8266_FAIL);
  }
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::receive(int8_t connId, 
                         const char* buffer, uint8_t buflen)
{
  if( buffer != NULL && buflen > 0 )
  {
    return(doReceive(connId, buffer, buflen));
  }
  else
  {
    return(ESP8266_FAIL);
  }
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::available(int8_t connId)
{
  return(doAvailable(connId));
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::disconnect(int8_t channel)
{
  return( doDisconnect(channel));
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::multiChannel(void)
{
  return(doMultiChannel());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::singleChannel(void)
{
  return(doSingleChannel());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::Channels(void)
{
  return(getChannel());
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::startServer(uint16_t port)
{
  return(doStartServer(port));
}
//
// - --------------------------------------------------
//

uint8_t ESP8266::stopServer(void)
{
  return(doStopServer());
}
//
// - --------------------------------------------------
//

uint8_t ESP8266::setServerTimeout(uint16_t tmout)
{
  return(doSetServerTimeout(tmout));
}
//
// - --------------------------------------------------
//
CHARP ESP8266::ServerTimeout(void)
{
  return(doGetServerTimeout());
}
//
// - --------------------------------------------------
//
void ESP8266::Response(void)
{
  readResponse();
}
//
// - --------------------------------------------------
//
void ESP8266::dbgOn( void )
{
  _do_debug = 1;
}
//
// - --------------------------------------------------
//
void ESP8266::dbgOff( void )
{
  _do_debug = 0;
}
//
// - --------------------------------------------------
//
void ESP8266::setDbg( Stream *p, uint8_t level)
{
  _p_debug = p;
  _dbgLevel = level;
}
//
// - --------------------------------------------------
//
void ESP8266::setDbgLevel( uint8_t level)
{
  _dbgLevel = level;
}
//
// - --------------------------------------------------
//
//
// ********************* internal helper ************************
//
// - --------------------------------------------------
//
uint8_t ESP8266::checkStatus(void)
{
  uint8_t retVal = ESP8266_UNDEF;

  if( parseToken( (char*) ESP_RSP_READY) )
  {
    retVal = ESP8266_READY;
    status = retVal;
Serial.println("Status -> ready");
  }
  else
  {
    if( parseToken( (char*) ESP_RSP_BUSY) )
    {
      retVal = ESP8266_BUSY;
      status = retVal;
Serial.println("Status -> busy");
    }
    else
    {
      if( parseToken( (char*) ESP_RSP_LINK) )
      {
        retVal = ESP8266_CONNECTED;
        status = retVal;
Serial.println("Status -> connected");
      }
      else
      {
        if( parseToken( (char*) ESP_RSP_UNLINK) )
        {
          retVal = ESP8266_DISCONNECTED;
          status = retVal;
Serial.println("Status -> disconnected");
        }
        else
        {
          if( parseToken( (char*) ESP_RSP_LINKIN) )
          {
            retVal = ESP8266_ACCEPTED;
            status = retVal;
Serial.println("Status -> accepted");
          }
          else
          {
            if( parseToken( (char*) ESP_RSP_DATAIN) )
            {
              retVal = ESP8266_DATA_INCOMING;
              status = retVal;
Serial.println("Status -> incoming data");
            }
          }
        }
      }
    }
  }

  return(retVal);
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::parseToken(char* token)
{
  uint8_t retVal = ESP8266_FAIL;

  if( strlen((char*) &_buffer[0]) && _pWrite > 0 )
  {
Serial.print("Search for ");
Serial.print(token);
Serial.println(" ->");
Serial.print((char*) &_buffer[0]);
Serial.println("<-");

    for( uint8_t i = 0; i < _pWrite && retVal == ESP8266_FAIL; i++ )
    {
// Serial.print("Search ");
// Serial.print(token);
// Serial.print(" from ");
// Serial.print(i);
// Serial.print(" to ");
// Serial.print(i+strlen(token)-1);
// Serial.print(" in >");
// Serial.print((char*) &_buffer[i]);
// Serial.println("<");

      if(strncmp((char*) &_buffer[i], token, strlen(token) ) == 0 )
      {
        retVal = ESP8266_SUCCESS;
      }
    }
  }
  return(retVal);
}
//
// - --------------------------------------------------
//
uint16_t ESP8266::readResponse(void)
{
  //_serial.setTimeout(500);
  _pRead = _pWrite = 0;
  memset( (char*) &_buffer[0], '\0', MAX_RINGBUFFER );
  _pWrite = _serial.readBytes( (char*) &_buffer[0], MAX_RINGBUFFER-1 );
  //_serial.setTimeout(SERIAL_DFLT_TMOUT);
if( strlen((char*) &_buffer[0]) )
{
Serial.print("BUF: ->");
Serial.print((char*) &_buffer[0]);
Serial.println("<-");
}
  checkStatus();
  return(_pWrite);
}
//
// - --------------------------------------------------
//
uint8_t ESP8266::doReset(uint8_t hwreset)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_RESTART;
  _serial.println(_command);

  while( readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_READY) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}
//
// ****************** internal functions **********************
//
uint8_t ESP8266::doBegin(void)
{
  _command=ESP8266_QUICK_CHECK;
  _serial.println(_command);
  readResponse();

  if( parseToken( (char*) ESP_RSP_OK) )
  {
    status = ESP8266_READY;
    return(ESP8266_SUCCESS);
  }
  else
  {
    return(reset());
  }
}

CHARP ESP8266::getFirmware(void)
{
  _command=ESP8266_ATPLUS;
  _command += ESP8266_FIRMWARE;
  _serial.println(_command);
  readResponse();
  return((char*) &_buffer[0]);
}

CHARP ESP8266::doQuickCheck(void)
{
  _command=ESP8266_QUICK_CHECK;
  _serial.println(_command);
  readResponse();
  return((char*) &_buffer[0]);
}

uint8_t ESP8266::setMode(uint8_t mode)
{
  _command=ESP8266_ATPLUS;
  _command += ESP8266_CWMODE;
  _command += "=";
  _command += (char) (mode+'0');
  _serial.println(_command);

  readResponse();

  if( !parseToken( (char*) ESP_RSP_ERROR) )
  {
    if(reset())
    {
Serial.println("reset -> succeeded");
      _mode = mode;
      return(ESP8266_SUCCESS);
    }
  }
  return(ESP8266_FAIL);
}

uint8_t ESP8266::getMode(void)
{
  return(_mode);
}


uint8_t ESP8266::reset(void)
{
  uint8_t retVal = ESP8266_FAIL;
  status = ESP8266_UNDEF;
//ESP8266_NO_ESP

  _serial.setTimeout(2000);

  if( (retVal = doReset(0)) != ESP8266_SUCCESS )
  {
    while(_serial.available())
    {
      _serial.read();
    }

    retVal = doReset(0);
  }

  _serial.setTimeout(SERIAL_DFLT_TMOUT);

  return(retVal);
}

uint8_t ESP8266::apJoin(const char *ssid, const char *password) 
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CWJAP;
  _command += "=\"{id}\",\"{pw}\"";
  _command.replace("{id}", ssid);
  _command.replace("{pw}", password);
  _serial.println(_command);

  while(  readResponse() )
  {
//    if( parseToken( (char*) ESP_RSP_OK) )
//    {
//      retVal = ESP8266_SUCCESS;
//    }
  }

//  if( retVal == ESP8266_SUCCESS )
//  {
    retVal = ESP8266_FAIL;
    _command=ESP8266_ATPLUS;
    _command += ESP8266_CWJAP;
    _command += "?";
    _serial.println(_command);

// _serial.setTimeout(2000);
  
    while( readResponse() )
    {
      if( parseToken((char*) ssid))
      {
        status = ESP8266_JOINED;
      }
      if( parseToken( (char*) ESP_RSP_OK) )
      {
        retVal = ESP8266_SUCCESS;
//        endfl=1;
      }
      if( parseToken( (char*) ESP_RSP_ERROR) )
      {
//        endfl=1;
      }
      if( parseToken( (char*) ESP_RSP_BUSY) )
      {
        _serial.println(_command);
      }
    }

// _serial.setTimeout(SERIAL_DFLT_TMOUT);

//  }

  return(retVal);
}

uint8_t ESP8266::apList(void)
{
  return(0);
}

CHARP ESP8266::apCurr(void)
{
  _command=ESP8266_ATPLUS;
  _command += ESP8266_CWJAP;
  _command += "?";
  _serial.println(_command);

  while( readResponse() )
  {
    Serial.println((char*) &_buffer[0]);
  }

  return((char*) &_buffer[0]);
}

CHARP ESP8266::getIP(void)
{
  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIFSR;
  _serial.println(_command);

  while( readResponse() )
  {
    Serial.println((char*) &_buffer[0]);
  }

  return((char*) &_buffer[0]);
}

uint8_t ESP8266::apStart(const char *ssid, const char *password,
                         uint8_t channel, uint8_t encryption)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CWSAP;
  _command += "=\"{id}\",\"{pw}\"";

  _command.replace("{id}", ssid);
  _command.replace("{pw}", password);

  if( channel > 0 )
  {
    _command += ",\"" + String(channel) + "\"";
  }
  
  if( encryption > 0 )
  {
    _command += ",\"" + String(encryption) + "\"";
  }

  _serial.println(_command);

  while(  readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_OK) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}

uint8_t ESP8266::apQuit(void)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CWQAP;

  readResponse();

  if( parseToken( (char*) ESP_RSP_OK) )
  {
      retVal = ESP8266_SUCCESS;
  }
  return(retVal);
}


uint8_t ESP8266::doConnect(uint8_t ctype, const char* ipAddress, uint16_t port, uint8_t channel)
{

  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPSTART;
  _command += "=";

  if(channel > 0 )
  {
    _command += (char) (channel+'0');
    _command += ",";
  }

  _command += "\"{ctype}\",\"{ip}\",{port}";
  _command.replace("{ctype}", ctype==TCP_CONNECTION?"TCP":"UDP");
  _command.replace("{ip}", ipAddress);
  _command.replace("{port}", String(port,DEC));

  _serial.println(_command);

  while( readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_OK) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}


uint8_t ESP8266::doSend(int8_t connId, const char* sdata, uint8_t slen)
{

  uint8_t retVal = ESP8266_FAIL;

  if(sdata != NULL && slen > 0 )
  {
    _command=ESP8266_ATPLUS;
    _command += ESP8266_CIPSEND;
    _command += "=";

    if( connId > 0 )
    {
      _command += String(connId) + ",";
    }

    _command += String(slen);

    _serial.println(_command);

    while( readResponse() )
    {
      if( parseToken( (char*) ESP_RSP_OK) )
      {
        retVal = ESP8266_SUCCESS;
      }
    }
  }

  return(retVal);
}

uint8_t ESP8266::doReceive(int8_t connId, const char* buffer, uint8_t buflen)
{
  return(0);
}

uint8_t ESP8266::doAvailable(int8_t connId)
{
  return(0);
}


uint8_t ESP8266::doDisconnect(int8_t channel)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPCLOSE;


  if( channel > 0 )
  {
    _command += "=" + String(channel);
  }

  _serial.println(_command);

  while( readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_OK) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}


uint8_t ESP8266::doMultiChannel(void)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPMUX;
  _command += "=1";

  _serial.println(_command);

  while( readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_OK) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}

uint8_t ESP8266::doSingleChannel(void)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPMUX;
  _command += "=0";

  _serial.println(_command);

  while( readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_OK) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}


uint8_t ESP8266::getChannel(void)
{
  return(0);
}


uint8_t ESP8266::doStartServer(uint16_t port)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPSERVER;
  _command += "=1";

  if( port > 0 )
  {
    _command += "," + String(port);
  }

  _serial.println(_command);

  while( readResponse() )
  {
    if( retVal = parseToken( (char*) ESP_RSP_OK) )
    {
//      retVal = ESP8266_SUCCESS;
      status = ESP8266_LISTENING;
Serial.print("Set status to LISTENING - retval was ->");
Serial.print(retVal);
Serial.println("<-");

    }
  }

  return(retVal);
}

uint8_t ESP8266::doStopServer(void)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPSERVER;
  _command += "=0";

  _serial.println(_command);

  while( readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_OK) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}

uint8_t ESP8266::doSetServerTimeout(uint16_t tmout)
{
  uint8_t retVal = ESP8266_FAIL;

  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPSTO;
  _command += "=";
  _command += String(tmout);

  _serial.println(_command);

  while( readResponse() )
  {
    if( parseToken( (char*) ESP_RSP_OK) )
    {
      retVal = ESP8266_SUCCESS;
    }
  }

  return(retVal);
}

CHARP ESP8266::doGetServerTimeout(void)
{
  _command=ESP8266_ATPLUS;
  _command += ESP8266_CIPSTO;
  _command += "?";

  _serial.println(_command);

  readResponse();
  return((char*) &_buffer[0]);
}

// ESP8266_CWLIF

