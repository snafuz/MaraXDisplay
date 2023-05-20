// Copyright (C) 2023 Simon Ziehme
// This file is part of MaraXObserver <https://github.com/RedNomis/MaraXObserver>.
//
// MaraXObserver is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MaraXObserver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dogtag.  If not, see <http://www.gnu.org/licenses/>.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <Fonts/Org_01.h>
#include "GothicA1_Light6pt7b.h"
#include "GothicA1_Light9pt7b.h"
#include "images.h"

// Serial Port PINS ESP8266
#define D5              14
#define D6              12

// Serial Port Buffer Size
#define BUFFER_SIZE     32


// Display stuff
#define SCREEN_WIDTH    128// OLED display width, in pixels
#define SCREEN_HEIGHT    64  // OLED display height, in pixels
#define OLED_RESET      -1 // QT-PY / XIAO
#define i2c_Address   0x3c // initialize with the I2C addr 0x3C Typically eBay OLED's
#define WHITE   0xFFFF

  // initialize the 1.3 inch OLED display
  Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 1000000, 100000);

// initialize the serial port
SoftwareSerial mySerial(D5, D6);

//Internals
bool gbSim = true;
bool gbMaraOff = false;
bool gbPumpOn = false;
bool gbCoffeeMode = false;
bool gbHeaterOn = false;
int giSteamTarget = 199;
int giSteamCurrent= 0;
int giCoffeeCurrent= 0;
int giBoostCountdown = 0;
int giCurrentSeconds = 0;
int giLastSeconds = 0;
int giTimeThr = 5;
long glLastPumpOnMillis;
long glSerialTimeout = 0;
char buffer[BUFFER_SIZE];
int giIndex = 0;

//Mara Data
String maraData[7];

//////////////////////////////////////////////////////////////////////////////////////
void showLogo()  {
//////////////////////////////////////////////////////////////////////////////////////
  delay(250);
  display.clearDisplay();
  display.drawBitmap(0, 0, logo_bmp, logo_width, logo_height, 1);
  display.display();
  delay(1000);
}

//////////////////////////////////////////////////////////////////////////////////////
void setup()   {
//////////////////////////////////////////////////////////////////////////////////////
  WiFi.mode(WIFI_OFF);

  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  display.clearDisplay();

  Serial.begin(9600);
  mySerial.begin(9600);
  memset(buffer, 0, BUFFER_SIZE);
  mySerial.write(0x11);

  showLogo();
}

//////////////////////////////////////////////////////////////////////////////////////
void SetSim() {
//////////////////////////////////////////////////////////////////////////////////////
  if (gbSim)
  {
    //C1.06,116,124,093,0840,1,0
    maraData[0] = String("C1.06");
    maraData[1] = String("116");
    maraData[2] = String("124");
    maraData[3] = maraData[3].equals("093")? String("103") : String("093");
    maraData[4] = String("0840");
    maraData[5] = maraData[5].equals("1")? String("0") : String("1");
    maraData[6] = maraData[6].equals("1")? String("0") : String("1");
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void getMaraData()
//////////////////////////////////////////////////////////////////////////////////////
{
 while (mySerial.available())
  {
    gbMaraOff = false;
    glSerialTimeout = millis();
    char rcv = mySerial.read();
    if (rcv != '\n')
      buffer[giIndex++] = rcv;
    else 
    {
      giIndex = 0;
      Serial.println(buffer);
      char* ptr = strtok(buffer, ",");
      int idx = 0;
      while (ptr != NULL)
      {
        maraData[idx++] = String(ptr);
        ptr = strtok(NULL, ",");
      }
    }
  }
  if (millis() - glSerialTimeout > 6000)
  {
    gbMaraOff = true;
    SetSim();
    glSerialTimeout = millis();
    mySerial.write(0x11);

  }
}

//////////////////////////////////////////////////////////////////////////////////////
void castMaraData() {
//////////////////////////////////////////////////////////////////////////////////////
      // Parse Data
      /*
        Example Data: C1.10,116,124,093,0840,1,0\n every ~400-500ms
        Length: 26
        [Pos] [Data] [Describtion]
        0)      C     Coffee Mode (C) or SteamMode (V)
        -        1.10 Software Version
        1)      116   current steam temperature (Celsisus)
        2)      124   target steam temperature (Celsisus)
        3)      093   current hx temperature (Celsisus)
        4)      0840  countdown for 'boost-mode'
        5)      1     heating element on or off
        6)      0     pump on or off
      */

      if (maraData[0].startsWith("C"))
        gbCoffeeMode = true;
      else
        gbCoffeeMode = false;
        
      giSteamCurrent = maraData[1].toInt();
      giSteamTarget = maraData[2].toInt();
      giCoffeeCurrent = maraData[3].toInt();
      giBoostCountdown = maraData[4].toInt();

      if (maraData[5].toInt() > 0)
        gbHeaterOn = true;
      else
        gbHeaterOn = false;

      if (maraData[6].toInt() > 0)
        gbPumpOn = true;
      else
        gbPumpOn = false;
}

//////////////////////////////////////////////////////////////////////////////////////
void updateView() {
//////////////////////////////////////////////////////////////////////////////////////
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setFont();
  display.setTextSize(1);
  
  if (giCurrentSeconds >= giTimeThr) // Show Countdown
  {
    showCounter();
  } 
  else // Show Main View
  {
    showMain();
  }
  
  display.display();
}

//////////////////////////////////////////////////////////////////////////////////////
// show the main screen with the temperature information
void showMain()   {
//////////////////////////////////////////////////////////////////////////////////////

  //Header
  display.setFont(&GothicA1_Light6pt7b);
  display.setCursor(0, 8);
  display.print(giSteamTarget);

  if (gbHeaterOn)
    display.drawBitmap(41, 0, heat_bmp, heat_width, heat_height, SH110X_WHITE);

  display.setCursor(71, 8);
  display.print(giBoostCountdown);

  display.setCursor(106, 8);
  display.print(giLastSeconds);
  display.setFont();
  display.print("s");

  //Body
  display.setFont(&GothicA1_Light9pt7b);
  display.drawBitmap(22, 20, steam_bmp, steam_width, steam_height, SH110X_WHITE);
  display.drawBitmap(81, 13, coffee_bmp, coffee_width, coffee_height, SH110X_WHITE);

  if (!gbCoffeeMode)
    display.drawCircle(5, 30, 2, SH110X_WHITE);
  else
    display.drawCircle(120, 30, 2, SH110X_WHITE);

  display.setCursor(16, 57);
  display.print(giSteamCurrent);
  display.setFont();
  display.setCursor(48, 45);
  display.print((char)247);
  display.setFont(&GothicA1_Light9pt7b);

  if (giCoffeeCurrent >= 100) {
    display.setCursor(77, 57);
    display.print(giCoffeeCurrent);
    display.setFont();
    display.setCursor(109, 45);
  }
  else {
    display.setCursor(83, 57);
    display.print(giCoffeeCurrent);
    display.setFont();
    display.setCursor(105, 45);
  }
  display.print((char)247);
  display.setFont(&GothicA1_Light9pt7b);
}

//////////////////////////////////////////////////////////////////////////////////////
// show the counter screen; if pump is on this screen is active
void showCounter()   {
//////////////////////////////////////////////////////////////////////////////////////

  //Header
  display.setTextSize(1);
  display.setFont(&GothicA1_Light6pt7b);
  display.setCursor(22, 8);
  display.print(giSteamCurrent);

  display.setCursor(90, 8);
  display.print(giCoffeeCurrent);
  //display.setFont();

  //Body
  if (giCurrentSeconds%3 == 0)
    display.drawBitmap(22, 21, drips_1_bmp, drips_width, drips_height, SH110X_WHITE);
  else if (giCurrentSeconds%3 == 1)
    display.drawBitmap(22, 21, drips_2_bmp, drips_width, drips_height, SH110X_WHITE);
  else
    display.drawBitmap(22, 21, drips_3_bmp, drips_width, drips_height, SH110X_WHITE);

  display.drawBitmap(15, 37, coffee_timer_bmp, coffee_timer_width, coffee_timer_height, SH110X_WHITE);

  display.setFont(&Org_01);
  display.setTextSize(6);

  //Shift Digits to align right
  if (giCurrentSeconds == 11) {
    display.setCursor(84, 46);
    display.print(1);
    display.setCursor(120, 46);
    display.print(1);
  }
  else if (giCurrentSeconds < 10) {
    display.setCursor(96, 46);
    display.print(giCurrentSeconds);
  }
  else if (giCurrentSeconds % 10 == 1){
    display.setCursor(60, 46);
    display.print((giCurrentSeconds/10) % 10);
    display.setCursor(120, 46);
    display.print(1);
  }
  else if ((giCurrentSeconds/10) % 10 == 1) {
    display.setCursor(84, 46);
    display.print(1);
    display.setCursor(96, 46);
    display.print(giCurrentSeconds % 10);
  }
  else {
    display.setCursor(60, 46);
    display.print(giCurrentSeconds);
  }

  giLastSeconds = giCurrentSeconds;
}

//////////////////////////////////////////////////////////////////////////////////////
void loop() {
//////////////////////////////////////////////////////////////////////////////////////

  getMaraData();
  castMaraData();
  if (gbPumpOn)
  {
    if (millis() - glLastPumpOnMillis >= 1000)
    {
      glLastPumpOnMillis = millis();
      ++giCurrentSeconds;
      if (giCurrentSeconds > 99)
        giCurrentSeconds = 0;
    }
  }
  else
  {
    giCurrentSeconds = 0;
  }
  if (!gbMaraOff || gbSim)
    updateView();
  else 
    showLogo();
}