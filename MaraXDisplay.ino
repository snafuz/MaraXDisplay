#include <ESP8266WiFi.h>

// Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Serial
#include <SoftwareSerial.h>

// Graphic
#include "GothicA1_Light6pt7b.h"
#include "GothicA1_Light9pt7b.h"
#include "images.h"

// MQTT
#include <PubSubClient.h>
// #include <ESPPubSubClientWrapper.h>

// Private config
#include "secrets.h"

// Serial Port Buffer Size
#define BUFFER_SIZE     32

//Enable simulator for debug
bool gbSim = true;
bool gbMaraOff = false;
int giSimTimer=0;
int gbSimInit=true;

bool gbPumpOn = false;
bool gbCoffeeMode = false;
bool gbHeaterOn = false;
bool gbShowShotTimer = false;

int giSteamTarget = 199;
int giSteamCurrent= 0;
int giCoffeeCurrent= 0;
int giBoostCountdown = 0;
int giCurrentSeconds = 0;
int giCurrentShotSeconds = 0;
int giLastShotSeconds = 0;
int giTimeThr = 5;
int giTimeoutThr = 3000;

long glLastPumpOnMillis;
long giCurrentOnSeconds = 0;
long glLastOnMillis = 0;
long glSerialTimeout = 0;
char buffer[BUFFER_SIZE];
int giIndex = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// initialize the serial port
SoftwareSerial mySerial(D5, D6);

// Some says that it shoud be inverted 
// SoftwareSerial mySerial(D5, D6, 1);

//MaraX Data
String maraData[7];

// Display 128x64
Adafruit_SSD1306 display(128, 64, &Wire, -1);

//////////////////////////////////////////////////////////////////////////////////////
void setup() {

  // disable wifi
  //WiFi.mode(WIFI_ON);
  setupWifi();

   client.setServer(mqtt_server, mqtt_port);

  

  Serial.begin(115200);
  Serial.println(F("\nMareX Display\n"));

  Serial.println("Initializing OLED Display 128x64");

  // initialize with the I2C addr 0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // display.display();
  // delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  // if (WiFi.isConnected())
  // {
  //   display.setTextColor(SSD1306_WHITE);
  //   display.setFont(&GothicA1_Light6pt7b);
  //   display.setTextSize(1);
  //   display.setCursor(5, 10);
  //   display.println("WiFi connected");
  //   display.println("IP address: ");
  //   display.println(WiFi.localIP());
    
  // }
  display.display();

  mySerial.begin(9600);
  memset(buffer, 0, BUFFER_SIZE);
  mySerial.write(0x11);
  showLogo();
  delay(1000);
  
  
}


//////////////////////////////////////////////////////////////////////////////////////
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() - glLastOnMillis >= 1000)
  {
    glLastOnMillis = millis();
    ++giSimTimer;
    ++giCurrentOnSeconds;
  }
  if(gbSim) {
    SetSim();
  }
  else{
    getMaraData();
  }
  
  castMaraData();
  if (gbPumpOn || gbShowShotTimer)
  {
    if (millis() - glLastPumpOnMillis >= 1000)
    {
      gbShowShotTimer = true;
      glLastPumpOnMillis = millis();
      ++giCurrentSeconds;
      if (giCurrentSeconds > 99)
        giCurrentSeconds = 0;
      if (gbPumpOn)
      {
        giCurrentShotSeconds = giCurrentSeconds;
      }
      else if (giCurrentSeconds - giCurrentShotSeconds > giTimeThr)
        gbShowShotTimer = false;
    }
  }
  else
  {
    giCurrentSeconds = 0;
  }
  if ((!gbMaraOff || gbSim) && giCurrentOnSeconds > 5)
    updateView();
  else 
    showLogo();
}


//////////////////////////////////////////////////////////////////////////////////////
void SetSim() {
  if (gbSim)
  {

    if(gbSimInit){
      //C1.10,116,124,093,0840,1,0
      gbSimInit = false;
      // version
      maraData[0] = String("+1.10");
      // current steam temperature
      maraData[1] = String("118");
      // target steam temperature
      maraData[2] = String("124");
      // current hx temperature
      maraData[3] = String("93");
      // countdown for 'boost-mode'
      maraData[4] = String("0840");
      // heating element on or off
      maraData[5] = String("0");
      // pump on(1) or off(0)
      maraData[6] = String("0");
    }
    else 
    {
    
    
      // version
      maraData[0] = String("+1.10");
      // current steam temperature
      maraData[1] = String("118");
      // target steam temperature
      maraData[2] = String("124");
      
      if(giSimTimer  == 3)
      {
        // current hx temperature
        maraData[3] = String("103");
        // heating element on or off
        maraData[5] = String("1");
      }
      // countdown for 'boost-mode'
      maraData[4] = String("0840");
    
      // pump on(1) or off(0)
      if(giSimTimer == 45 ){
        maraData[6] = String("0");
        giSimTimer = 0;
      }
      else if (giSimTimer  >= 12){
        maraData[6] = String("1");
      }
      else
      {
        maraData[6] = String("0");
      }
    
    
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void getMaraData()
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
  if (millis() - glSerialTimeout > giTimeoutThr)
  {
    gbMaraOff = true;
    SetSim();
    glSerialTimeout = millis();
    mySerial.write(0x11);

  }
}

//////////////////////////////////////////////////////////////////////////////////////
void castMaraData() {
      // Parse Data
      /*
        Example Data: C1.10,116,124,093,0840,1,0\n every ~400-500ms
        Length: 26
        [Pos] [Data] [Describtion]
        0)      C     Coffee Mode (+) or SteamMode (C) || ToDo: This might be wrong
        -        1.10 Software Version
        1)      116   current steam temperature (Celsisus)
        2)      124   target steam temperature (Celsisus)
        3)      093   current hx temperature (Celsisus)
        4)      0840  countdown for 'boost-mode'
        5)      1     heating element on or off
        6)      0     pump on or off
      */

      // ToDo: Switched this temporarily To check if the assignment is correct
      if (maraData[0].startsWith("C")) 
        gbCoffeeMode = false;
      else
        gbCoffeeMode = true;
        
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
  display.clearDisplay();
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
  display.setTextColor(SSD1306_WHITE);
  //Header
  display.setFont(&GothicA1_Light6pt7b);
  display.setCursor(0, 8);
  display.print(giSteamTarget);

  if (gbHeaterOn)
    display.drawBitmap(41, 0, heat_bmp, heat_width, heat_height,SSD1306_WHITE);

  display.setCursor(71, 8);
  display.print(giBoostCountdown);

  display.setCursor(106, 8);
  display.print(giLastShotSeconds);
  display.setFont();
  display.print("s");

  //Body
  display.setFont(&GothicA1_Light9pt7b);
  display.drawBitmap(22, 20, steam_bmp, steam_width, steam_height, SSD1306_WHITE);
  display.drawBitmap(81, 13, coffee_bmp, coffee_width, coffee_height, SSD1306_WHITE);

  if (!gbCoffeeMode)
    display.drawCircle(5, 30, 2, SSD1306_WHITE);
  else
    display.drawCircle(120, 30, 2, SSD1306_WHITE);

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

  display.setTextColor(SSD1306_WHITE);
  //Header
  display.setTextSize(1);
  display.setFont(&GothicA1_Light6pt7b);
  display.setCursor(22, 8);
  display.print(giCoffeeCurrent);
  
  //Body
  if (giCurrentShotSeconds%3 == 0)
    display.drawBitmap(22, 21, drips_1_bmp, drips_width, drips_height, SSD1306_WHITE);
  else if (giCurrentShotSeconds%3 == 1)
    display.drawBitmap(22, 21, drips_2_bmp, drips_width, drips_height, SSD1306_WHITE);
  else
    display.drawBitmap(22, 21, drips_3_bmp, drips_width, drips_height, SSD1306_WHITE);

  display.drawBitmap(15, 37, coffee_timer_bmp, coffee_timer_width, coffee_timer_height, SSD1306_WHITE);

  display.setTextSize(5);
  display.setTextColor(SSD1306_WHITE);
  

  //handle single vs double digit
  if (giCurrentShotSeconds < 10) {
    display.setCursor(92, 50);
    display.print(giCurrentShotSeconds);
  }
  else 
  {
    display.setCursor(59, 50);
    display.print(giCurrentShotSeconds/10);
    display.setCursor(92, 50);
    display.print(giCurrentShotSeconds % 10);
  }


  if(giCurrentShotSeconds > 12){
    giLastShotSeconds = giCurrentShotSeconds;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void showLogo()  {

  delay(250);
  display.clearDisplay();
  display.drawBitmap(22, 20, coffee_bmp, coffee_width, coffee_height, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);
  display.setFont(&GothicA1_Light6pt7b);
  display.setCursor(77, 57);
  display.print("MaraX Display\nby Snafuz");
  display.setFont();
  display.setCursor(109, 45);

  display.setFont(&GothicA1_Light6pt7b);

  display.display();
}

//////////////////////////////////////////////////////////////////////////////////////
bool setupWifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attemptIdx = 0;

  while (WiFi.status() != WL_CONNECTED) {
    if(attemptIdx>10) return false;
    delay(500);
    Serial.print(".");
    ++attemptIdx;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////
void reconnect() {
  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("testTopic", "MaraX Display connected"); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



