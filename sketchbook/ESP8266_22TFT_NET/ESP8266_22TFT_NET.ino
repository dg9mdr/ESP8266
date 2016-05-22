/* ----------------------------------------------------------------------
  ESP8266_22TFT (c) 2015 by dreamshader aka Dirk Schanz

  A simple status-monitor for displaying "lifedata" from a host in a
  nice graphical outfit.

  The following values are displayed:
    current date
    current time
    current day of the week
    current CPU-temperature
    current load-average
    IP address of host
    free memory in MB
    free space of /root-fs in MB

  All Values are transmitted using HTTP-Requests, e.g.:
    http://IP:PORT/ipdaddress?a=192&b=168&c=1&d=122
    http://IP:PORT/datetime?day=22&month=5&year=2016&hour=6&minute=2&second=49&wday=0
    http://IP:PORT/cputemp?degree=47&tenth=00
    http://IP:PORT/systemload?load=0&tenth=01
    http://IP:PORT/freemem?freemem=1296&freesd=290918
  where IP is the IP address and PORT is the listening port
  of your ESP module.
   
 
   ----------------------------------------------------------------------
*/



#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"
// https://github.com/glennirwin/Adafruit_ILI9340

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define TFT_DC     5
#define TFT_CS     4
#define TFT_MOSI  13
#define TFT_CLK   14
#define TFT_RST   16
#define TFT_MISO  12

#define PIN_BACKLIGHT  2

Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_RST);

#define FIXED_FONT_CHAR_WIDTH   6
#define FIXED_FONT_CHAR_HEIGHT  8
//
#define ROTATION_NO             0
#define ROTATION_90             1
#define ROTATION_180            2
#define ROTATION_270            3
//
#define SMALLEST_TEXT_SIZE      1
#define SMALL_TEXT_SIZE         2
#define MEDIUM_TEXT_SIZE        3
#define BIG_TEXT_SIZE           4
#define BIGGEST_TEXT_SIZE       5
//
#define FIXED_HORIZONTAL_SPACING  1
#define FIXED_VERTICAL_SPACING    0


//
//////////////////////////


// Positions for frames and Labels
//
//
// Pos day: x = 10 y = 8
#define POS_DATE_DAY_X      10 
#define POS_DATE_DAY_Y       8
//
// Pos . x = 34 y = 8
#define POS_DATE_DOT1_X      34 
#define POS_DATE_DOT1_Y       8
//
// Pos month x = 46 y = 8
#define POS_DATE_MONTH_X      46 
#define POS_DATE_MONTH_Y       8
//
// Pos . x = 70 y = 8
#define POS_DATE_DOT2_X      70 
#define POS_DATE_DOT2_Y       8
//
// Pos year x = 82 y = 8
#define POS_DATE_YEAR_X      82 
#define POS_DATE_YEAR_Y       8
//
// Pos WDay: x = 149 y = 8
#define POS_WDAY_X     149 
#define POS_WDAY_Y       8
//
//
// Pos hour: x = 201 y = 8
#define POS_TIME_HOUR_X     201 
#define POS_TIME_HOUR_Y       8
//
// Pos : : x = 225 y = 8
#define POS_TIME_QUOTE_X     225 
#define POS_TIME_QUOTE_Y       8
//
// Pos minute: x = 237 y = 8
#define POS_TIME_MINUTE_X     237 
#define POS_TIME_MINUTE_Y       8
//
// Pos dot: x = 261 y = 8
#define POS_TIME_DOT_X     261 
#define POS_TIME_DOT_Y       8
//
// Pos second: x = 273 y = 8
#define POS_TIME_SECOND_X     273 
#define POS_TIME_SECOND_Y       8
//
//
// Pos Temp:
// Pos degree: x = 38 y = 112
// Pos Dot: x = 74 y = 112
// Pos Tenth: x = 92 y = 112
#define POS_TEMP_TEMP_X      38 
#define POS_TEMP_TEMP_Y     112
#define POS_TEMP_DOT_X       74 
#define POS_TEMP_DOT_Y      112
#define POS_TEMP_TENTH_X     92 
#define POS_TEMP_TENTH_Y    112
//
//
// Pos Load:
// Pos Load Full: x = 199 y = 112
// Pos Load Dot: x = 217 y = 112
// Pos Load Tenth: x = 235 y = 112
#define POS_LOAD_LOAD_X     199 
#define POS_LOAD_LOAD_Y     112

#define POS_LOAD_DOT_X      217
#define POS_LOAD_DOT_Y      112 

#define POS_LOAD_TENTH_X    235
#define POS_LOAD_TENTH_Y    112
//
//
// Pos AByte: x = 68 y = 184
#define POS_IP_ABYTE_X  68 
#define POS_IP_ABYTE_Y  184 
//
// Pos Dot1: x = 104 y = 184
#define POS_IP_DOT1_X  104 
#define POS_IP_DOT1_Y  184 
//
// Pos BByte: x = 116 y = 184
#define POS_IP_BBYTE_X  116 
#define POS_IP_BBYTE_Y  184 
//
// Pos Dot3: x = 152 y = 184
#define POS_IP_DOT2_X  152 
#define POS_IP_DOT2_Y  184 
//
// Pos CByte: x = 164 y = 184
#define POS_IP_CBYTE_X  164 
#define POS_IP_CBYTE_Y  184 
//
// Pos Dot3: x = 200 y = 184
#define POS_IP_DOT3_X  200 
#define POS_IP_DOT3_Y  184 
//
// Pos DByte: x = 212 y = 184
#define POS_IP_DBYTE_X  212 
#define POS_IP_DBYTE_Y  184 
//
//
// Pos Free mem: x = 73 y = 215
#define POS_FREE_MEM_X  73 
#define POS_FREE_MEM_Y 215
//
// Pos Free SD: x = 220 y = 215
#define POS_FREE_SD_X  202 
#define POS_FREE_SD_Y  215


// /////////////////////////////////////


// ************************************************************************
// set these values to get network access
// ************************************************************************
// change to your SSID 
String ssid = "YOUR_SSID_HERE";
// change to your passphrase
String password = "YOUR_PASSPHRASE_HERE";
// change to match listening port you want
#define WWW_SERVER_PORT   8080
//
// ************************************************************************
// global stuff
// ************************************************************************
//
//
int tft_rotation;
int tft_char_width;
int tft_char_height;
int tft_lines;
int tft_columns;
int tft_text_size;
int tft_hor_spacing;
int tft_vert_spacing;

// ///////////////////////
//

int gDay, gMonth, gYear;
int gWDay;
int gHour, gMinute, gSecond;
String gDegree, gDegreeTenth;
String gLoad, gLoadTenth;
int gAbyte, gBbyte, gCbyte, gDbyte;
long gFreemem;
long gFreesd;


  // create a local web server on port WWW_SERVER_PORT
ESP8266WebServer server(WWW_SERVER_PORT);

String pageContent;
#define SERVER_METHOD_GET       1
#define SERVER_METHOD_POST      2



void tft_SetTextCursor( int column, int line )
{
 
  tft.setCursor( ((tft_text_size * tft_char_width) + tft_hor_spacing) * (column-1),
                ((tft_text_size * tft_char_height) + tft_vert_spacing ) * (line-1) );
}
//
// ************
//
void tft_MapTFTParams( void )
{
  tft_columns = tft.width()/((tft_char_width * tft_text_size) + tft_hor_spacing);
  tft_lines = tft.height()/((tft_char_height * tft_text_size) + tft_vert_spacing);
}
//
// ************
//
void setup() 
{

  tft_char_width = FIXED_FONT_CHAR_WIDTH;
  tft_char_height = FIXED_FONT_CHAR_HEIGHT;

  tft_rotation = ROTATION_NO;
  tft_text_size = SMALL_TEXT_SIZE;
  tft_lines = 0;
  tft_columns = 0;

  tft_hor_spacing = FIXED_HORIZONTAL_SPACING;
  tft_vert_spacing = FIXED_VERTICAL_SPACING;

  Serial.begin(57600);
  Serial.println("ILI9341 Test!"); 
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  WiFi.begin( ssid.c_str(), password.c_str());
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // install handler for virtual pages
  server.on("/datetime", handleDateTimePage);
  server.on("/cputemp", handleCputempPage);
  server.on("/systemload", handleSystemloadPage);
  server.on("/ipdaddress", handleIpaddressPage);
  server.on("/freemem", handleFreememPage);

  // start www-server
  server.begin();
  Serial.println("Webserver started");
 

  // Print the IP address
  Serial.print("Webserver URL is: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");


  pinMode(PIN_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_BACKLIGHT, HIGH);
 
  tft.begin();

  Serial.println("display_out");
  tft_rotation = ROTATION_90;
  tft.setRotation(tft_rotation);
  tft_text_size = SMALL_TEXT_SIZE;
  tft.setTextSize(tft_text_size);
  tft_MapTFTParams();
  tft_drawFrames( ILI9340_BLACK, ILI9340_WHITE, ILI9340_CYAN );
  delay(3000);
  tft_drawFrameLabels();
}
//
// ************
//
void tft_drawFrameLabels(void)
{
  int textSizeSave;
  
  Serial.println("tft_SetTextCursor()");
    tft_SetTextCursor( 2, 2 );

//
//

  tft.setTextColor(ILI9340_WHITE); 
  textSizeSave = tft_text_size;
  tft_text_size = MEDIUM_TEXT_SIZE;
  tft.setTextSize(tft_text_size);  

Serial.println("tft_SetTextCursor()");
    tft_SetTextCursor( 4, 3 );

Serial.println("setCursor()");
    tft.setCursor( tft.getCursorX() + tft_char_width, tft.getCursorY() + 2*tft_char_height);

    
  tft_text_size = textSizeSave;
  tft.setTextSize(tft_text_size);      

Serial.println("O");
  tft.print("O");

  textSizeSave = tft_text_size;
  tft_text_size = MEDIUM_TEXT_SIZE;
  tft.setTextSize(tft_text_size);    

Serial.println("C");
  tft.print("C");


Serial.println("setCursor()");
    tft.setCursor( tft.getCursorX()+(6 * tft_text_size*tft_char_width), tft.getCursorY() );

  Serial.println("LOAD");
    tft.print("LOAD");

//
//


  tft_text_size = textSizeSave;
  tft.setTextSize(tft_text_size);  


Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 2, tft_lines );
  
  Serial.println("setCursor()");
    tft.setCursor( tft.getCursorX(), tft.getCursorY()-tft_char_height-1 );

Serial.println("mem: ");
    tft.print("mem: ");

Serial.println("     M");
    tft.print("     M");

Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 14, tft_lines );
        
Serial.println("setCursor()");
  tft.setCursor( tft.getCursorX()+tft_char_width/2, tft.getCursorY()-tft_char_height-1 );

  Serial.println("/: ");
    tft.print("/: ");
    
Serial.println("       M");
    tft.print("       M");

}
//
// ************
//
void tft_drawFrames( int background, int textforeground,  int foreground )
{
  int vy_top, vy_bottom;

Serial.println("clear");
  tft.fillScreen(background);
Serial.println("foreground");  
  tft.setTextColor(textforeground); 

//
// 1st Frame
Serial.println("Pos 1,1");
  tft_SetTextCursor( 1, 1 );
  
//  tft.drawFastHLine(0, tft.getCursorY(), tft.width(), foreground);
Serial.println("getCursorY()");
  vy_top = tft.getCursorY();

  tft_SetTextCursor( 1, 3 );
//  tft.drawFastHLine(0, tft.getCursorY()+7, tft.width(), foreground);
Serial.println("getCursorY()");
  vy_bottom = tft.getCursorY();

Serial.println("drawRoundRect()");
  tft.drawRoundRect(tft.getCursorX(), vy_top, tft.width()-1, vy_bottom - vy_top, 10, foreground);

//Serial.println("drawFastVLine()");
//  tft.drawFastVLine( (tft.width()/2) - 1, vy_top, vy_bottom - vy_top, foreground);

Serial.println("drawFastVLine()");
  tft.drawFastVLine( (tft.width()/2)-2*2*tft_char_width, vy_top, vy_bottom - vy_top, foreground);

Serial.println("drawFastVLine()");
  tft.drawFastVLine( (tft.width()/2)+2*2*tft_char_width, vy_top, vy_bottom - vy_top, foreground);


//
//2nd frame
Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 1, 4 );

Serial.println("getCursorY()");  
  vy_top = tft.getCursorY();

Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 1, tft_lines-3 );
  
Serial.println("getCursorY()");
  vy_bottom = tft.getCursorY();

Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 1, 4 );
  
Serial.println("drawRoundRect()");
  tft.drawRoundRect(tft.getCursorX(), vy_top, tft.width()-1, vy_bottom - vy_top, 10, foreground);

//
//new 3rd frame
Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 1, tft_lines-1 );

Serial.println("getCursorY()");  
  vy_top = tft.getCursorY();

Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 1, tft_lines+1 );
  
Serial.println("getCursorY()");
  vy_bottom = tft.getCursorY();

Serial.println("tft_SetTextCursor()");
  tft_SetTextCursor( 1, tft_lines-1 );
  
Serial.println("drawRoundRect()");
  tft.drawRoundRect(tft.getCursorX(), vy_top, tft.width()-1, vy_bottom - vy_top, 10, foreground);

Serial.println("drawFastVLine()");
  tft.drawFastVLine( (tft.width()/2) - 1, vy_top, vy_bottom - vy_top, foreground);
}

//
// ************
//
void tft_DateDot1Refresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  int savX, savY;
  static long mSecs;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor(POS_DATE_DOT1_X, POS_DATE_DOT1_Y);
    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor); 
    tft.setCursor(POS_DATE_DOT1_X, POS_DATE_DOT1_Y);
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_DateDot2Refresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  int savX, savY;
  static long mSecs;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor(POS_DATE_DOT2_X, POS_DATE_DOT2_Y);
    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor ); 
    tft.setCursor(POS_DATE_DOT2_X, POS_DATE_DOT2_Y);
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_DateRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, int day, int month, int year )
{
  static long mSecs;
  static int lastDay, lastMonth, lastYear;
  int savX, savY;
  char cOutput[5];


  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    if( day != lastDay )
    {
      lastDay = day;
      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_DATE_DAY_X, POS_DATE_DAY_Y);
      tft.print("  ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_DATE_DAY_X, POS_DATE_DAY_Y);
      sprintf(cOutput, "%02d", day);
      tft.print(cOutput);
      tft.setTextColor(textColor, bgColor); 
    }

    if( month != lastMonth )
    {
      lastMonth = month;
      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_DATE_MONTH_X, POS_DATE_MONTH_Y);
      tft.print("  ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_DATE_MONTH_X, POS_DATE_MONTH_Y);
      sprintf(cOutput, "%02d", month);
      tft.print(cOutput);
      tft.setTextColor(textColor, bgColor); 
    }

    if( year != lastYear )
    {
      lastYear = year;
      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_DATE_YEAR_X, POS_DATE_YEAR_Y);
      tft.println("    ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_DATE_YEAR_X, POS_DATE_YEAR_Y);
      sprintf(cOutput, "%04d", year);
      tft.print(cOutput);
      tft.setTextColor(textColor, bgColor); 
    }

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_WDayRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, int wDay )
{
  static long mSecs;
  static int lastWDay;
  int savX, savY;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    if( wDay != lastWDay )
    {
      lastWDay = wDay;
      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_WDAY_X, POS_WDAY_Y);
      tft.print("  ");

      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_WDAY_X, POS_WDAY_Y);

      switch( wDay )
      {
        case 0:
          tft.print("So");
          break;
        case 1:
          tft.print("Mo");
          break;
        case 2:
          tft.print("Di");
          break;
        case 3:
          tft.print("Mi");
          break;
        case 4:
          tft.print("Do");
          break;
        case 5:
          tft.print("Fr");
          break;
        case 6:
          tft.print("Sa");
          break;
        default:
          break;
      }

      tft.setTextColor(textColor, bgColor); 
    }

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_TimeRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, int hour, int minute, int second )
{
  static int lastHour, lastMinute, lastSecond;
  static long mSecs;

  char cOutBuf[3];
  int savX, savY;
//
//

  if( millis() - mSecs >= interval )
  {

    savX = tft.getCursorX();
    savY = tft.getCursorY();

    if( hour != lastHour )
    {
      lastHour = hour;
      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_TIME_HOUR_X, POS_TIME_HOUR_Y);
      tft.println("  ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_TIME_HOUR_X, POS_TIME_HOUR_Y);    
      sprintf(cOutBuf, "%02d", hour);
      tft.print(cOutBuf);
      tft.setTextColor(textColor, bgColor); 
    }

    if( minute != lastMinute )
    {
      lastMinute = minute;
      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_TIME_MINUTE_X, POS_TIME_MINUTE_Y);
      tft.println("  ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_TIME_MINUTE_X, POS_TIME_MINUTE_Y);    
      sprintf(cOutBuf, "%02d", minute);
      tft.print(cOutBuf);
      tft.setTextColor(textColor, bgColor); 
    }

    if( second != lastSecond )
    {
      lastSecond = second;
      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_TIME_SECOND_X, POS_TIME_SECOND_Y);
      tft.println("  ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_TIME_SECOND_X, POS_TIME_SECOND_Y);    
      sprintf(cOutBuf, "%02d", second);
      tft.print(cOutBuf);
      tft.setTextColor(textColor, bgColor); 
    }

    tft.setCursor( savX, savY );
    mSecs = millis();
  }
}
//
// ************
//
void tft_TimeQuoteRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  int savX, savY;
  static long mSecs;
  
  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();
    
    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor(POS_TIME_QUOTE_X, POS_TIME_QUOTE_Y);
    tft.print(" ");

    tft.setTextColor(displayTextColor, displayBgColor); 
    tft.setCursor(POS_TIME_QUOTE_X, POS_TIME_QUOTE_Y);
    tft.print(":");
    
    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_TimeDotRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  int savX, savY;
  static long mSecs;
  
  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor(POS_TIME_DOT_X, POS_TIME_DOT_Y);
    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor ); 
    tft.setCursor(POS_TIME_DOT_X, POS_TIME_DOT_Y);
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_TempRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, char *degree, char *tenth )
{
  static int lastDegree, lastTenth;
  static long mSecs;
  static int DStatus;
  
  int savX, savY;
  char cOutBuf[3];


  if( millis() - mSecs >= interval )
  {
    if( atoi(degree) != lastDegree ||
        atoi(tenth) != lastTenth )
    {
      savX = tft.getCursorX();
      savY = tft.getCursorY();

      lastDegree = atoi(degree);
      lastTenth = atoi(tenth);

      tft.setTextSize( MEDIUM_TEXT_SIZE );

      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(  POS_TEMP_TEMP_X, POS_TEMP_TEMP_Y );
      tft.print(" ");
      sprintf(cOutBuf, "%s", degree);
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(  POS_TEMP_TEMP_X, POS_TEMP_TEMP_Y );
      tft.print(cOutBuf);

      if( millis() - mSecs >= 1000 )
      {

        if( DStatus <= 0)
        {
          DStatus = 1;
          tft.print(".");
        }
        else
        {
          DStatus = 0;
          tft.print(" ");        
        }
      }
      
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(  POS_TEMP_TENTH_X, POS_TEMP_TENTH_Y );
      tft.print(" ");
      sprintf(cOutBuf, "%s", tenth);
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(  POS_TEMP_TENTH_X, POS_TEMP_TENTH_Y );
      tft.print(cOutBuf);

      tft.setTextSize( SMALL_TEXT_SIZE );

      tft.setCursor( savX, savY );
      tft.setTextColor(textColor, bgColor); 

    }

    mSecs = millis();
  }
}
//
// ************
//
void tft_TempDotRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  int savX, savY;
  static long mSecs;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextSize( MEDIUM_TEXT_SIZE );

    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor(POS_TEMP_DOT_X, POS_TEMP_DOT_Y);
    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor ); 
    tft.setCursor(POS_TEMP_DOT_X, POS_TEMP_DOT_Y);
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 

    tft.setTextSize( SMALL_TEXT_SIZE );

    mSecs = millis();
  }
}
//
// ************
//
void tft_LoadRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, char *load, char *tenth )
{
  static int lastLoad, lastTenth;
  static long mSecs;
  int savX, savY;
  char cOutBuf[3];

  if( millis() - mSecs >= interval )
  {
    if( atoi(load) != lastLoad ||
        atoi(tenth) != lastTenth )
    {
      savX = tft.getCursorX();
      savY = tft.getCursorY();

      lastLoad = atoi(load);
      lastTenth = atoi(tenth);

      tft.setTextSize( MEDIUM_TEXT_SIZE );

      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(  POS_LOAD_LOAD_X, POS_LOAD_LOAD_Y );
      
      tft.print(" ");
      sprintf(cOutBuf, "%s", load);
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(  POS_LOAD_LOAD_X, POS_LOAD_LOAD_Y );
      tft.print(cOutBuf);


      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(  POS_LOAD_TENTH_X, POS_LOAD_TENTH_Y );
      tft.print(" ");
      sprintf(cOutBuf, "%s", tenth);
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(  POS_LOAD_TENTH_X, POS_LOAD_TENTH_Y );
      tft.print(cOutBuf);

      tft.setCursor( savX, savY );
      tft.setTextColor(textColor, bgColor); 

      tft.setTextSize( SMALL_TEXT_SIZE );
    }

    mSecs = millis();
  }
}
//
// ************
//
void tft_LoadDotRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  int savX, savY;
  static long mSecs;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextSize( MEDIUM_TEXT_SIZE );


    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor(POS_LOAD_DOT_X, POS_LOAD_DOT_Y);

    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor ); 
    tft.setCursor(POS_LOAD_DOT_X, POS_LOAD_DOT_Y);
        
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 

    tft.setTextSize( SMALL_TEXT_SIZE );
    mSecs = millis();
  }
}
//
// ************
//
void tft_IPDot1Refresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  static long mSecs;
  int savX, savY;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor( POS_IP_DOT1_X, POS_IP_DOT1_Y );
    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor ); 
    tft.setCursor( POS_IP_DOT1_X, POS_IP_DOT1_Y );
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_IPDot2Refresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  static long mSecs;
  int savX, savY;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor( POS_IP_DOT2_X, POS_IP_DOT2_Y );
    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor ); 
    tft.setCursor( POS_IP_DOT2_X, POS_IP_DOT2_Y );
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_IPDot3Refresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor )
{
  static long mSecs;
  int savX, savY;

  if( millis() - mSecs >= interval )
  {
    savX = tft.getCursorX();
    savY = tft.getCursorY();

    tft.setTextColor(displayBgColor, displayBgColor); 
    tft.setCursor( POS_IP_DOT3_X, POS_IP_DOT3_Y );
    tft.print(" ");
    tft.setTextColor(displayTextColor, displayBgColor ); 
    tft.setCursor( POS_IP_DOT3_X, POS_IP_DOT3_Y );
    tft.print(".");

    tft.setCursor( savX, savY );
    tft.setTextColor(textColor, bgColor); 
    mSecs = millis();
  }
}
//
// ************
//
void tft_IPRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, int abyte, int bbyte, int cbyte, int dbyte )
{
  static int lastAByte, lastBByte, lastCByte, lastDByte;
  static long mSecs;
  int savX, savY;
  char cOutBuf[6];


  if( millis() - mSecs >= interval )
  {

    if( abyte != lastAByte ||
        bbyte != lastBByte ||
        cbyte != lastCByte ||
        dbyte != lastDByte )
    {
      savX = tft.getCursorX();
      savY = tft.getCursorY();

      lastAByte = abyte;
      lastBByte = bbyte;
      lastCByte = cbyte;
      lastDByte = dbyte;

      tft.setTextColor(displayBgColor, displayBgColor); 


      tft.setCursor( POS_IP_ABYTE_X, POS_IP_ABYTE_Y );
      tft.print("   ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor( POS_IP_ABYTE_X, POS_IP_ABYTE_Y );
      sprintf(cOutBuf, "%03d", abyte);
      tft.print(cOutBuf);

      tft.setCursor( POS_IP_BBYTE_X, POS_IP_BBYTE_Y );
      tft.print("   ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor( POS_IP_BBYTE_X, POS_IP_BBYTE_Y );
      sprintf(cOutBuf, "%03d", bbyte);
      tft.print(cOutBuf);

      tft.setCursor( POS_IP_CBYTE_X, POS_IP_CBYTE_Y );
      tft.print("   ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor( POS_IP_CBYTE_X, POS_IP_CBYTE_Y );
      sprintf(cOutBuf, "%03d", cbyte);
      tft.print(cOutBuf);

      tft.setCursor( POS_IP_DBYTE_X, POS_IP_DBYTE_Y );
      tft.print("   ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor( POS_IP_DBYTE_X, POS_IP_DBYTE_Y );
      sprintf(cOutBuf, "%03d", dbyte);
      tft.print(cOutBuf);



      // 192.168.001.110
      // 
      tft.setCursor( savX, savY );
      tft.setTextColor(textColor, bgColor); 
    }

    mSecs = millis();
  }
}
//
// ************
//
void tft_FreeMemRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, long freemem )
{
  static int lastFreemem;
  static long mSecs;
  int savX, savY;
  char cOutBuf[6];

  if( millis() - mSecs >= interval )
  {
    if( freemem != lastFreemem )
    {
      savX = tft.getCursorX();
      savY = tft.getCursorY();

      lastFreemem = freemem;

      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_FREE_MEM_X, POS_FREE_MEM_Y);
      tft.print("     ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_FREE_MEM_X, POS_FREE_MEM_Y);
      sprintf(cOutBuf, "%4d", freemem);
      tft.print(cOutBuf);

      // 684 M
      // 
      tft.setCursor( savX, savY );
      tft.setTextColor(textColor, bgColor); 
    }

    mSecs = millis();
  }
}
//
// ************
//
void tft_FreeSDRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, long freesd )
{
  static long mSecs;
  static int lastFreeSD;
  int savX, savY;
  char cOutBuf[6];


  if( millis() - mSecs >= interval )
  {
    if( freesd != lastFreeSD )
    {
      savX = tft.getCursorX();
      savY = tft.getCursorY();

      lastFreeSD = freesd;

      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(POS_FREE_SD_X, POS_FREE_SD_Y);
      tft.print("     ");
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(POS_FREE_SD_X, POS_FREE_SD_Y);
      sprintf(cOutBuf, "%5d", freesd);
      tft.print(cOutBuf);

      // 684 M
      // 
      tft.setCursor( savX, savY );
      tft.setTextColor(textColor, bgColor); 
    }

    mSecs = millis();
  }
}
//
// ************
//
void loop(void) 
{

  tft_DateRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, gDay, gMonth, gYear );
  tft_DateDot1Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_DateDot2Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );

  tft_WDayRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, gWDay );

  tft_TimeRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, gHour, gMinute, gSecond );
  tft_TimeQuoteRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_TimeDotRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );

  tft_TempRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, (char*)gDegree.c_str(), (char*)gDegreeTenth.c_str() );
  tft_TempDotRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK);

  tft_LoadRefresh( 1000, ILI9340_YELLOW, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, (char*)gLoad.c_str(), (char*)gLoadTenth.c_str() );
  tft_LoadDotRefresh( 1000, ILI9340_YELLOW, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK);

  tft_IPRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, gAbyte, gBbyte, gCbyte, gDbyte );
  tft_IPDot1Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_IPDot2Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_IPDot3Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );

  tft_FreeMemRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, gFreemem );

  tft_FreeSDRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, gFreesd );

  server.handleClient();

}


// ************************************************************************
// page handling ...
// ************************************************************************
//
// handle /datetime page 
//
void handleDateTimePage()
{
  String argGiven;

  Serial.println("DateTimePage");

  Serial.println( server.method() );
  Serial.println( server.args() );

  if( server.method() == SERVER_METHOD_GET )
  {
    Serial.println("METHOD GET");

    for(int i = 0; i < server.args(); i++ )
    {
      Serial.print( server.argName(i) );
      Serial.print( " = " );
      Serial.println( server.arg(i) );
    }

    argGiven = server.arg("day");
    gDay = argGiven.toInt();

    argGiven = server.arg("month");
    gMonth = argGiven.toInt();

    argGiven = server.arg("year");
    gYear = argGiven.toInt();

    argGiven = server.arg("hour");
    gHour = argGiven.toInt();

    argGiven = server.arg("minute");
    gMinute = argGiven.toInt();

    argGiven = server.arg("second");
    gSecond = argGiven.toInt();

    argGiven = server.arg("wday");
    gWDay = argGiven.toInt();

    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>0 OK<br>";
    pageContent += "</html>";

  }
  else
  {
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>-1 FAIL<br>";
    pageContent += "</html>";
  }

  server.send(200, "text/html", pageContent);  
}

void handleCputempPage()
{
  String argGiven;

  Serial.println("CputempPage");

  Serial.println( server.method() );
  Serial.println( server.args() );

  if( server.method() == SERVER_METHOD_GET )
  {
    Serial.println("METHOD GET");

    for(int i = 0; i < server.args(); i++ )
    {
      Serial.print( server.argName(i) );
      Serial.print( " = " );
      Serial.println( server.arg(i) );
    }

    gDegree = server.arg("degree");
    gDegreeTenth = server.arg("tenth");

    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>0 OK<br>";
    pageContent += "</html>";

  }
  else
  {
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>-1 FAIL<br>";
    pageContent += "</html>";
  }

  server.send(200, "text/html", pageContent);  
}


void handleSystemloadPage()
{
  String argGiven;

  Serial.println("handleSystemloadPage");

  Serial.println( server.method() );
  Serial.println( server.args() );

  if( server.method() == SERVER_METHOD_GET )
  {
    Serial.println("METHOD GET");

    for(int i = 0; i < server.args(); i++ )
    {
      Serial.print( server.argName(i) );
      Serial.print( " = " );
      Serial.println( server.arg(i) );
    }

    gLoad = server.arg("load");
    gLoadTenth = server.arg("tenth");

    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>0 OK<br>";
    pageContent += "</html>";

  }
  else
  {
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>-1 FAIL<br>";
    pageContent += "</html>";
  }

  server.send(200, "text/html", pageContent);  
}


void handleIpaddressPage()
{
  
  String argGiven;

  Serial.println("handleIpaddressPage");

  Serial.println( server.method() );
  Serial.println( server.args() );

  if( server.method() == SERVER_METHOD_GET )
  {
    Serial.println("METHOD GET");

    for(int i = 0; i < server.args(); i++ )
    {
      Serial.print( server.argName(i) );
      Serial.print( " = " );
      Serial.println( server.arg(i) );
    }

    argGiven = server.arg("a");
    gAbyte = argGiven.toInt();

    argGiven = server.arg("b");
    gBbyte = argGiven.toInt();

    argGiven = server.arg("c");
    gCbyte = argGiven.toInt();

    argGiven = server.arg("d");
    gDbyte = argGiven.toInt();

    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>0 OK<br>";
    pageContent += "</html>";

  }
  else
  {
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>-1 FAIL<br>";
    pageContent += "</html>";
  }

  server.send(200, "text/html", pageContent);  
}


void handleFreememPage()
{
  String argGiven;

  Serial.println("handleFreememPage");

  Serial.println( server.method() );
  Serial.println( server.args() );

  if( server.method() == SERVER_METHOD_GET )
  {
    Serial.println("METHOD GET");

    for(int i = 0; i < server.args(); i++ )
    {
      Serial.print( server.argName(i) );
      Serial.print( " = " );
      Serial.println( server.arg(i) );
    }

    argGiven = server.arg("freemem");
    gFreemem = atol(argGiven.c_str());

    argGiven = server.arg("freesd");
    gFreesd = atol(argGiven.c_str());

    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>0 OK<br>";
    pageContent += "</html>";

  }
  else
  {
    pageContent = "<!DOCTYPE HTML>\r\n";
    pageContent += "<html></p>";
    pageContent += "<br>-1 FAIL<br>";
    pageContent += "</html>";
  }

  server.send(200, "text/html", pageContent);  
}


