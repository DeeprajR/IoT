/*
 * Radar scanner — ESP8266 (NodeMCU / Wemos D1 Mini) + HC-SR04 + SG90 + SSD1306 OLED
 *
 * Wiring:
 *   HC-SR04 VCC  -> Vin (5V)
 *   HC-SR04 GND  -> GND
 *   HC-SR04 Trig -> D5
 *   HC-SR04 Echo -> 1K -> D6, D6 -> 2.2K -> GND   (voltage divider, 5V -> 3.44V)
 *   SG90 Red     -> Vin (5V)    use external 5V if the board browns out
 *   SG90 Brown   -> GND
 *   SG90 Orange  -> D7
 *   OLED VCC     -> 3V3
 *   OLED GND     -> GND
 *   OLED SDA     -> D2
 *   OLED SCL     -> D1
 *
 * Libraries (install via Library Manager):
 *   - Adafruit GFX
 *   - Adafruit SSD1306
 *   - Servo  (the standard one — it works on ESP8266, unlike on ESP32)
 *
 * Board: in Arduino IDE select "NodeMCU 1.0 (ESP-12E Module)" or "LOLIN(WEMOS) D1 R2 & mini"
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// ---------- Display ----------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDR   0x3C       // try 0x3D if 0x3C doesn't respond

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Hardware pins (ESP8266 D-labels) ----------
const int TRIG_PIN  = D5;
const int ECHO_PIN  = D6;
const int SERVO_PIN = D7;
// I2C uses D1 (SCL) and D2 (SDA) by default on ESP8266 — Wire.begin() picks them up.

Servo radarServo;

// ---------- Radar parameters ----------
const int   ANGLE_MIN    = 0;
const int   ANGLE_MAX    = 180;
const int   ANGLE_STEP   = 2;
const int   SETTLE_MS    = 30;         // ESP8266 is slower; a touch more settle
const int   MAX_DISTANCE = 80;         // cm
const unsigned long ECHO_TIMEOUT_US = 25000UL;

// Polar-to-pixel mapping on the 128x64 OLED
const int CX = 64;
const int CY = 60;
const int R  = 50;

uint8_t distances[181];

int currentAngle = ANGLE_MIN;
int sweepDir     = 1;

// ---------------------------------------------------------------
// Ultrasonic ranging
// ---------------------------------------------------------------
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1.0f;
  return duration * 0.0343f / 2.0f;
}

// ---------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------
void drawRadarFrame() {
  for (int r = R / 3; r <= R; r += R / 3) {
    for (int a = 0; a <= 180; a += 4) {
      float rad = a * DEG_TO_RAD;
      int x = CX + (int)(cosf(rad) * r);
      int y = CY - (int)(sinf(rad) * r);
      if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        display.drawPixel(x, y, SSD1306_WHITE);
      }
    }
  }
  for (int a = 0; a <= 180; a += 30) {
    float rad = a * DEG_TO_RAD;
    int x = CX + (int)(cosf(rad) * R);
    int y = CY - (int)(sinf(rad) * R);
    display.drawLine(CX, CY, x, y, SSD1306_WHITE);
  }
}

void drawBlips() {
  for (int a = 0; a <= 180; a++) {
    uint8_t d = distances[a];
    if (d == 0 || d > MAX_DISTANCE) continue;
    int pixelR = map(d, 0, MAX_DISTANCE, 0, R);
    float rad = a * DEG_TO_RAD;
    int x = CX + (int)(cosf(rad) * pixelR);
    int y = CY - (int)(sinf(rad) * pixelR);
    display.fillCircle(x, y, 1, SSD1306_WHITE);
  }
}

void drawSweepLine(int angle) {
  float rad = angle * DEG_TO_RAD;
  int x = CX + (int)(cosf(rad) * R);
  int y = CY - (int)(sinf(rad) * R);
  display.drawLine(CX, CY, x, y, SSD1306_WHITE);
}

void drawHud(int angle, uint8_t d) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print(angle);
  display.print((char)247);            // degree symbol

  display.setCursor(80, 0);
  if (d > 0 && d <= MAX_DISTANCE) {
    display.print(d);
    display.print(F("cm"));
  } else {
    display.print(F("---"));
  }
}

// ---------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(50);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  Wire.begin();   // uses D2 (SDA) and D1 (SCL) on ESP8266 by default

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
    Serial.println(F("SSD1306 init failed — check I2C wiring / address"));
    while (true) delay(10);
  }
  display.clearDisplay();
  display.display();

  radarServo.attach(SERVO_PIN, 500, 2400);  // SG90 pulse range
  radarServo.write(ANGLE_MIN);

  for (int i = 0; i <= 180; i++) distances[i] = 0;

  delay(500);
}

void loop() {
  radarServo.write(currentAngle);
  delay(SETTLE_MS);

  float dCm = readDistanceCm();
  uint8_t stored = 0;
  if (dCm > 0 && dCm <= MAX_DISTANCE) stored = (uint8_t)dCm;
  distances[currentAngle] = stored;

  display.clearDisplay();
  drawRadarFrame();
  drawBlips();
  drawSweepLine(currentAngle);
  drawHud(currentAngle, stored);
  display.display();

  yield();   // important on ESP8266 — keeps the WiFi/system stack happy

  currentAngle += sweepDir * ANGLE_STEP;
  if (currentAngle >= ANGLE_MAX) { currentAngle = ANGLE_MAX; sweepDir = -1; }
  else if (currentAngle <= ANGLE_MIN) { currentAngle = ANGLE_MIN; sweepDir = 1; }
}
