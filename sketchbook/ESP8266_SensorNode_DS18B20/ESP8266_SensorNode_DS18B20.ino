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
// 1st version: 02/09/16
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
#define WWW_LISTENPORT       80
//
// ************************************************************************
// include necessary lib header
// ************************************************************************
//
#include <ESP8266WiFi.h>

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
#define SERIAL_BAUD      115200
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
WiFiServer server(WWW_LISTENPORT);
#endif // USE_WWWSERVER

  // Initialize 1wire bus
OneWire oneWire(ONE_WIRE_BUS);

#ifdef USE_DS18B20
  // create DS18B20 object on the bus
DallasTemperature DS18B20(&oneWire);
#endif // USE_DS18B20
//
// ************************************************************************
// setup module ...
// ************************************************************************
//
void setup() 
{
  // startup serial console ...
  Serial.begin(SERIAL_BAUD);
  delay(10);
  
#ifdef USE_WIFICLIENT
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  // Connect to WiFi network
  WiFi.begin(ssid, password);
   
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
 
void loop() {
  static float temp;
  static long lastMillis;

#ifdef USE_DS18B20
  // check for next read interval has reached
  if( (millis() - lastMillis) >= DS18B20_READINTVAL )
  {
    lastMillis = millis();
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    // info output do serial console ...
    Serial.print("Temperature: ");
    Serial.println(temp);
  }
#endif // USE_DS18B20
  

  // check for client-connection 
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // info output do serial console ...
  Serial.println("new client");

  // wait for request
  while(!client.available())
  {
    delay(1);
  }
   
  // read request upon CR
  String request = client.readStringUntil('\r');

  // info output do serial console ...
  Serial.println(request);

  // discard remaining bytes of client request
  client.flush();
   
  // send response to client
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
   
  client.print("Temperature is now: ");
  client.print(temp);  
  client.print(" &deg;C");

  client.println("<br><br>");
  client.println("</html>");
 
  delay(1);

  // info output do serial console ...
  Serial.println("Client disonnected");
  Serial.println("");
 
}
 
