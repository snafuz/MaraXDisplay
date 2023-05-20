# MaraX Monitior - Visualisation for the Lelit Mara X (V2) with an automatic Shot-Timer
Here is my attempt for a Mara X v2 Shot Timer. 
It uses the Serial interface from the Mara X to visualize the Stats on a 1.3" Display.

![Display](https://github.com/DiSanzes/Mara-X2-ShotTimer/blob/main/assets/MainView.png?raw=true)

![Display](https://github.com/DiSanzes/Mara-X2-ShotTimer/blob/main/assets/TimerMode.png?raw=true)
 
# Parts

- WEMOS D1 mini (You can use any other equivalent board)
- 1,3" SSH1106 OLED Display

# The Interface

The MaraX has a 6-PIN Connector for the serial interface on the bottom. We only use **PIN-3** (Mara RX - Black Wire - Green Rectangle) and **PIN-4** (Mara TX - Orange Wire - White Rectangle). For testing you can use the jumper-wires later you should replace them with temperature protected cables. 
 - **PIN3 Mara** to  **D6**
 - **PIN4 Mara** to **D5**

RX Mara to TX Arudino
TX Mara to RX Arduino

## The Data

Example Data: **C1.06,116,124,093,0840,1,0\n**
- The delimitter is: **,**
- The end of data is: **\n**
- Length: **26**

|Data|Description |
|--|--|
| C | Machine-Mode: C = CoffeeMode; V = Vapour/SteamMode |
| 1.06 | Firmware |
| 116 | Current Steam Temperature in Celsius |
| 124| Target Steam Temperature in Celsius |
| 093| Curent Hx Temperature in Celsius |
| 0840| Countdown Boost-Mode |
| 1| Heat state (0 = off; 1= on) |
| 0| Pump state (0 = off; 1= on) |


# Display Wiring
|Pin Display|Pin Arduino Nano|
|--|--|
| VIN | 3,3V |
| GND| GND |
| SCL| D1 |
| SDA| D2 |

## Libaries
 - Wire.h
 - Adafruit_GFX.h
 - Adafruit_SH110X.h
 - SoftwareSerial.h

## Fonts used 
 - GothicA1_Light6pt7b
 - GothicA1_Light9pt7b
 - Org_01

## Bootup Logo
 - Create your Logo with the dimensions: 128x64px 
 - Convert your image to a binary file for example with this Tool https://javl.github.io/image2cpp/ 
 - Replace the data for logo_bmp in images.h


# Special Thanks goes to: 
 - SaibotFlow: https://github.com/SaibotFlow/marax-monitor
 - RedNomis: https://github.com/RedNomis/MaraXObserver