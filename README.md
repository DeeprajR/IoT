
# Projects 
## Radar scanner — ESP8266 (NodeMCU / Wemos D1 Mini) + HC-SR04 + SG90 + SSD1306 OLED
(ESP8266 Radar)[/ESP8266_Radar]
**Wiring:**
 - HC-SR04 VCC  -> Vin (5V)
 - HC-SR04 GND  -> GND
 - HC-SR04 Trig -> D5
 - HC-SR04 Echo -> 1K -> D6, D6 -> 2.2K -> GND   (voltage divider, 5V -> 3.44V)
 - SG90 Red     -> Vin (5V)    use external 5V if the board browns out
 - SG90 Brown   -> GND
 - SG90 Orange  -> D7
 - OLED VCC     -> 3V3
 - OLED GND     -> GND
 - OLED SDA     -> D2
 - OLED SCL     -> D1
 
 ## Libraries (install via Library Manager):
- Adafruit GFX
- Adafruit SSD1306


Servo: SG90 or MG90S
Board Config: in Arduino IDE select "NodeMCU 1.0 (ESP-12E Module)" or "LOLIN(WEMOS) D1 R2 & mini"
