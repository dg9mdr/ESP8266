//
// ************************************************************************
// A rework and enhancement to ESPs as sensor nodes
// (C) 2016 Dirk Schanz aka dreamshader
//
// Functional description:
// step by step add protocols, sensors, actors and functionalities
// to ESP modules. In opposition to the last solution try to hold
// all features modular.
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
//-------- Files and their meanings related to this project --------
//
// ESP8266_Config.h
//     All configuration for the firmware is done here.
//
// ESP8266_EEPROM_Access.h
// ESP8266_EEPROM_Access.cpp
//     High-level functions, declaration and macros for EEPROM access.
//
// ESP8266_EEPROM_Layout.h
//     Constants and structures to store/restore data
//
// ESP8266_HTTP.h
// ESP8266_HTTP.cpp
//     Misc. functions and declarations for HTTP-access and server
//     functionality
//
// ESP8266_HTTP_Settings.h
// ESP8266_HTTP_Settings.cpp
//     Handling and definition of the forms that are used for
//     node settings e.g. passwords.
// 
// ************************************************************************
//
//-------- History -------------------------------------------------
//
// 1st version: 03/28/16
//         Web forms for node administration, complete rework on EEPROM
//         layout.
// update:
//
//
// ************************************************************************
// first of all include configuration definitions
// ************************************************************************
//

#include "ESP8266_Config.h"

//
// ************************************************************************
// then include necessary lib header
// ************************************************************************
//
#include <EEPROM.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "ESP8266_EEPROM_Layout.h"
#include "ESP8266_EEPROM_Access.h"

//
// ************************************************************************
// define some default values
// ************************************************************************
//
//
// ************************************************************************
// the only configuration data located outside ESP8266_Config.h
// set these values to get network access
// ************************************************************************
// change to your SSID 
String ssid = "SSID";
// change to your passphrase
String password = "PASSPHRASE";
//
// ************************************************************************
// global stuff
// ************************************************************************
//

  // create a local web server on port WWW_LISTENPORT
ESP8266WebServer server(WWW_LISTENPORT);

String pageContent;



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
// main loop is quite simple ...
// ************************************************************************
//
void loop() 
{
  server.handleClient();
}

