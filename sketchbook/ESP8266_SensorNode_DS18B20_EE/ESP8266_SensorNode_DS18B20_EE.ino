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
//-------- History -------------------------------------------------
//
// 1st version: 02/08/16
//         Join an existing WLAN, start a webserver, read temperature
//         from a DS18B20 and show it on a "local webpage" ...
// update:
//
// -----------------------------------------------------------------
//
// ************************************************************************
// set these defines to match your ESP-module 
// ************************************************************************
//
// #undef USE_ESP01
#define USE_ESP01
#undef USE_ESP12
// #define USE_ESP12
#undef USE_ESP12E
// #define USE_ESP12E
//
// ************************************************************************
// set these defines to match your sensors
// ************************************************************************
//
#define USE_DS18B20
#define DS18B20_READINTVAL 5000  // read interval in ms
#define ONE_WIRE_BUS          2  // pin DS18B20 is connected to
                                 // note: be careful if using GPIO0 for that
                                 //       cause of ESP boots to UPLOAD mode
                                 //       if GPIO0 is LOW on (re)start
//
// ************************************************************************
// set these defines to match your favored ESPs behavior
// ************************************************************************
//
#define USE_WIFICLIENT
#define USE_WWWSERVER
#define WWW_LISTENPORT           80
//
// ************************************************************************
// include necessary lib header
// ************************************************************************
//
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <EEPROM.h>
#include <Wire.h>
#ifdef USE_DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#endif // USE_DS18B20
//
// ************************************************************************
// define some default values
// ************************************************************************
//
#define SERIAL_BAUD        115200
#define EEPROM_BLOCK_SIZE    1024
#define EEPROM_MAGIC_BYTE    0xEA
#define LEN_TRAILING_LENGTH     2  // means two byte representing 
                                   // the real length of the data field
#define LEN_SSID_MAX           32  // max. length a SSID may have
#define LEN_PASSWORD_MAX       64  // max. length of a password
// layout of the eeprom:
//
#define DATA_POS_MAGIC          0
#define DATA_LEN_MAGIC          1
// data area begins here
#define DATA_POS_SSID          (DATA_POS_MAGIC + DATA_LEN_MAGIC)
#define DATA_LEN_SSID          (LEN_TRAILING_LENGTH + 32)
#define DATA_POS_PASSWORD      (DATA_POS_SSID + DATA_LEN_SSID)
#define DATA_LEN_PASSWORD      (LEN_TRAILING_LENGTH + 64)
//
// ... further stuff here like above scheme
//
// ************************************************************************
// set these values to match your network configuration
// ************************************************************************
// change to your SSID 
String ssid = "TP-LINK";
// change to your passphrase
String password = "Das_ist_eine_1a_sichere_Passphrase";
//
// ************************************************************************
// global stuff
// ************************************************************************
//
#ifdef USE_WWWSERVER
  // create a local web server on port WWW_LISTENPORT
// WiFiServer server(WWW_LISTENPORT);
ESP8266WebServer server(WWW_LISTENPORT);

String pageContent;
int serverStatusCode;

#define SERVER_METHOD_GET       1
#define SERVER_METHOD_POST      2

#define ARGS_ADMIN_PAGE         2
#endif // USE_WWWSERVER

  // Initialize 1wire bus
OneWire oneWire(ONE_WIRE_BUS);

#ifdef USE_DS18B20
  // create DS18B20 object on the bus
DallasTemperature DS18B20(&oneWire);

#endif // USE_DS18B20
  // temperature - phase 1, one DS18B20 only
  // will go to a structure/array in one of the next steps
float temp;

//
// ************************************************************************
// EEPROM access
// ************************************************************************
//
//
// store field length to a specific position
//
int eeStoreFieldLength( char* len, int dataIndex )
{
  int retVal = 0;

  Serial.print("write LEN byte [");
  Serial.print(len[0],HEX);
  Serial.print("] to pos ");
  Serial.println(dataIndex);
  
  EEPROM.write(dataIndex, len[0]);

  Serial.print("write LEN byte [");
  Serial.print(len[1],HEX);
  Serial.print("] to pos ");
  Serial.println(dataIndex+1);
  
  
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

  Serial.print("got LEN byte [");
  Serial.print(len[0],HEX);
  Serial.print("] from pos ");
  Serial.println(dataIndex);

  len[1] = EEPROM.read(dataIndex+1);

  Serial.print("got LEN byte [");
  Serial.print(len[1],HEX);
  Serial.print("] from pos ");
  Serial.println(dataIndex+1);  

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
    Serial.print("Wrote: ");
    Serial.println(data[i]); 
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
  
  Serial.print("restore data from eeprom: Address is [");
  Serial.print(dataIndex);
  Serial.print("] - maxlen = ");
  Serial.println(maxLen);

  eeRestoreFieldLength( (char*) &len, dataIndex );

  Serial.print("len = ");
  Serial.println(len);

  if( len > 0 )
  {
    data = "";
    for( int i=0; i < len && i < maxLen; i++ )
    {
      c = EEPROM.read(dataIndex + LEN_TRAILING_LENGTH + i);
      Serial.print(c);
      data += c;
    }
  }
  Serial.println(" - done!");
    

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
    Serial.print("wrong magic: ");
    Serial.println( magicByte );
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
// ************************************************************************
// setup module ...
// ************************************************************************
//
void setup() 
{
  String n_ssid = "";
  String n_password = "";

  // startup serial console ...
  Serial.begin(SERIAL_BAUD);
  delay(10);
 
  // prepare access to EEPROM 
  EEPROM.begin(EEPROM_BLOCK_SIZE);

  // note: If you want to read ssid and password from EEPROM, just change
  //       n_ssid to ssid and n_password to password.
  //       I didn't do that because I will play around with EEPROM
  //       contents in the next future
  if( eeIsValid() )
  {
    Serial.println("eeprom content is valid");
    eeRestoreString( n_ssid, DATA_POS_SSID, LEN_SSID_MAX );
    eeRestoreString( n_password, DATA_POS_PASSWORD, LEN_PASSWORD_MAX );
  }
  else
  {
    Serial.println("INVALID eeprom content!");
  }

  Serial.print("ssid from eeprom is ");
  Serial.println(n_ssid);

  Serial.print("password from eeprom is ");
  Serial.println(n_password);

#ifdef USE_WIFICLIENT
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  // Connect to WiFi network
//  WiFi.begin((const char*) ssid.c_str(), (const char*) password.c_str());
  WiFi.begin( ssid.c_str(), password.c_str());
   
  // loop until connection is established
  // note that the program stays here if 
  // joining the network fails for any
  // reason
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
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

#ifdef USE_WWWSERVER
  // start www-server
  server.begin();
  Serial.println("Webserver started");
 
// #if defined(USE_WIFICLIENT) || defined(USE_WWWSERVER)

  // Print the IP address
  Serial.print("Webserver URL is: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
#endif // USE_WWWSERVER
    
}

//
// ************************************************************************
// page handling ...
// ************************************************************************
//
// handle /admin page containing a form with a submit button
//
void handleAdminPage()
{
  
  String n_ssid = "";
  String n_password = "";

  Serial.println("AdminIndex page");

  Serial.println( server.method() );
  Serial.println( server.args() );

   // WiFiClient client() { return _currentClient; }
  // String arg(const char* name);   // get request argument value by name
  // String arg(int i);              // get request argument value by number
  // String argName(int i);          // get request argument name by number
  // int args();                     // get arguments count
  // bool hasArg(const char* name);  // check if argument exists

  if( server.method() == SERVER_METHOD_GET )
  {
    // form was requested initially
    // set input fields to empty strings
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent +=   "<form method='post' action='admin'>";
    pageContent +=     "<label>SSID: </label>";
    pageContent +=      "<input name='ssid' value="+ssid+" length=32> (max. 32 chars)";
    pageContent +=     "<label>PASSWD: </label>";
    pageContent +=      "<input name='pass' value="+password+" length=64> (max. 64 chars)";
    pageContent +=      "<br>";
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

      if( server.args() == ARGS_ADMIN_PAGE )
      {
        for(int i = 0; i < server.args(); i++ )
        {
          Serial.print( server.argName(i) );
          Serial.print( " = " );
          Serial.println( server.arg(i) );
        }

        n_ssid = server.arg("ssid");
        n_password = server.arg("pass");

        if (n_ssid.length() > 0 && n_password.length() > 0)
        {

          //
          // here we go to store SSID and PASSWD to EEPROM
          //
          eeStoreString( n_ssid, DATA_POS_SSID );
          eeStoreString( n_password, DATA_POS_PASSWORD );
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
          pageContent += "<br>empty strings not allowed!<br>";
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
  Serial.println("Index page");

  // send response to client
  pageContent  = "<!DOCTYPE HTML>\r\n<html>";
  pageContent += "Temperature is now: ";
  pageContent += String(temp);  
  pageContent += " &deg;C";
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
    lastMillis = millis();
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    // info output to serial console ...
    Serial.print("Temperature: ");
    Serial.println(temp);
  }
#endif // USE_DS18B20
 
  server.handleClient();

}

