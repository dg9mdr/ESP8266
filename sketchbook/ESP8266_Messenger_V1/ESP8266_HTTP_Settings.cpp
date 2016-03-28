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


