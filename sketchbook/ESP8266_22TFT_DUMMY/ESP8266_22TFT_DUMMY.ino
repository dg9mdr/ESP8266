
/***************************************************
  This is our GFX example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"
// https://github.com/glennirwin/Adafruit_ILI9340

// For the Adafruit shield, these are the default.
// #define TFT_DC 9
// #define TFT_CS 10
// #define TFT_MOSI  11
// #define TFT_CLK   13
// #define TFT_RST   8
// #define TFT_MISO  12



#define TFT_DC     5
#define TFT_CS     4
#define TFT_MOSI  13
#define TFT_CLK   14
#define TFT_RST   16
#define TFT_MISO  12

// use the default SPI pins SCK #14, MOSI #13, MISO #12 and CS #4, DC #5
// https://github.com/glennirwin/Adafruit_ILI9340

#define PIN_BACKLIGHT  2

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// If using the breakout, change pins as desired
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
// Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
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
int tft_rotation;
int tft_char_width;
int tft_char_height;
int tft_lines;
int tft_columns;
int tft_text_size;
int tft_hor_spacing;
int tft_vert_spacing;




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
// /////////////////////////////////////
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
// /////////////////////////////////////
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
// /////////////////////////////////////
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
// /////////////////////////////////////
//
// Pos Free mem: x = 73 y = 215
#define POS_FREE_MEM_X  73 
#define POS_FREE_MEM_Y 215
//
// Pos Free SD: x = 220 y = 215
#define POS_FREE_SD_X  220 
#define POS_FREE_SD_Y  215





void tft_SetTextCursor( int column, int line )
{
 
  tft.setCursor( ((tft_text_size * tft_char_width) + tft_hor_spacing) * (column-1),
                ((tft_text_size * tft_char_height) + tft_vert_spacing ) * (line-1) );
}


void tft_MapTFTParams( void )
{
  tft_columns = tft.width()/((tft_char_width * tft_text_size) + tft_hor_spacing);
  tft_lines = tft.height()/((tft_char_height * tft_text_size) + tft_vert_spacing);
}



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

  Serial.println("SD: ");
    tft.print("SD: ");
    
Serial.println("      M");
    tft.print("      M");

}


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
void tft_TempRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, int degree, int tenth )
{
  static int lastDegree, lastTenth;
  static long mSecs;
  static int DStatus;
  
  int savX, savY;
  char cOutBuf[3];


  if( millis() - mSecs >= interval )
  {
    if( degree != lastDegree ||
        tenth != lastTenth )
    {
      savX = tft.getCursorX();
      savY = tft.getCursorY();

      lastDegree = degree;
      lastTenth = tenth;

      tft.setTextSize( MEDIUM_TEXT_SIZE );

      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(  POS_TEMP_TEMP_X, POS_TEMP_TEMP_Y );
      tft.print(" ");
      sprintf(cOutBuf, "%2d", degree);
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
      sprintf(cOutBuf, "%2d", tenth);
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
void tft_LoadRefresh( int interval, int displayTextColor, int displayBgColor, int textColor, int bgColor, int load, int tenth )
{
  static int lastLoad, lastTenth;
  static long mSecs;
  int savX, savY;
  char cOutBuf[3];

  if( millis() - mSecs >= interval )
  {
    if( load != lastLoad ||
        tenth != lastTenth )
    {
      savX = tft.getCursorX();
      savY = tft.getCursorY();

      lastLoad = load;
      lastTenth = tenth;

      tft.setTextSize( MEDIUM_TEXT_SIZE );

      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(  POS_LOAD_LOAD_X, POS_LOAD_LOAD_Y );
      
      tft.print(" ");
      sprintf(cOutBuf, "%d", load);
      tft.setTextColor(displayTextColor, displayBgColor ); 
      tft.setCursor(  POS_LOAD_LOAD_X, POS_LOAD_LOAD_Y );
      tft.print(cOutBuf);


      tft.setTextColor(displayBgColor, displayBgColor); 
      tft.setCursor(  POS_LOAD_TENTH_X, POS_LOAD_TENTH_Y );
      tft.print(" ");
      sprintf(cOutBuf, "%2d", tenth);
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
      sprintf(cOutBuf, "%4d", freesd);
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
  static int day, month, year;
  static int wDay;
  static int hour, minute, second;
  static int degree, dgTenth;
  static int load, ldTenth;
  static int abyte, bbyte, cbyte, dbyte;
  static long freemem;
  static long freesd;
  static long mSecs, sSecs;

  if( mSecs == 0 )
  {
    day = 19;
    month = 5;
    year = 2016;

    wDay = 4;

    hour = 1;
    minute = 32;
    second = 16;

    degree = 57;
    dgTenth = 22;
    load = 2;
    ldTenth = 17;

    freemem = 1234;
    freesd = 56789;

    mSecs = millis();

  }

  if( millis() - mSecs >= 5000 )
  {
    day++;
    month++;
    year++;
    wDay++;
    hour++;
    minute++;
//    second += 5;
    degree++;
    dgTenth++;
    load++;
    ldTenth++;
    freemem++;
    freesd++;
  }
  
  if( millis() - sSecs >= 1000 )
  {
    second++;
  }

  if( millis() - mSecs >= 5000 )
  {
    if( day > 31 )
    {
      day = 1;
    }

    if( month > 12 )
    {
      month = 1;
    }

    if( year > 2050 )
    {
      year = 2001;
    }

    if( wDay > 6 )
    {
      wDay = 0;
    }

    if( hour > 24 )
    {
      hour = 0;
    }

    if( minute > 60 )
    {
      minute = 0;
    }

//    if( second > 60 )
//    {
//      second = 0;
//    }

    if( degree > 87 )
    {
      degree = 58;
    }

    if( dgTenth > 99 )
    {
      dgTenth = 17;
    }

    if( load > 3 )
    {
      load = 0;
    }

    if( ldTenth > 99 )
    {
      ldTenth = 17;
    }

    if( freemem > 1342 )
    {
      freemem = 1234;
    }

    if( freesd > 13547 )
    {
      freesd = 13490;
    }
    
    mSecs = millis();
  }
  
  if( millis() - sSecs >= 1000 )
  {
    if( second > 60 )
    {
      second = 0;
    }    
    sSecs = millis();
  }
  
  tft_DateRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, day, month, year );
  tft_DateDot1Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_DateDot2Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );

  tft_WDayRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, wDay );

  tft_TimeRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, hour, minute, second );
  tft_TimeQuoteRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_TimeDotRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );

  tft_TempRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, degree, dgTenth );
  tft_TempDotRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK);

  tft_LoadRefresh( 1000, ILI9340_YELLOW, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, load, ldTenth );
  tft_LoadDotRefresh( 1000, ILI9340_YELLOW, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK);

  tft_IPRefresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, 192, 168, 1, 23 );
  tft_IPDot1Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_IPDot2Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );
  tft_IPDot3Refresh( 1000, ILI9340_WHITE, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK );

  tft_FreeMemRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, freemem );

  tft_FreeSDRefresh( 1000, ILI9340_GREEN, ILI9340_BLACK, ILI9340_WHITE, ILI9340_BLACK, freesd );

  delay(10);

}


