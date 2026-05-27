#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "Free_Fonts.h"
#include "dial240.h"
#include "fonts.h"
#include "images.h"

// =======================================================
// PIN CONFIG - ESP32-S3 SUPER MINI
// =======================================================

// I2C cho MAX30102 + MPU6050
#define I2C_SDA 8
#define I2C_SCL 9

// TFT ST7789V3 SPI
#define TFT_WIDTH   240
#define TFT_HEIGHT  280

#define TFT_SCLK 12
#define TFT_MOSI 11
#define TFT_CS   10
#define TFT_DC   7
#define TFT_RST  6
#define TFT_BL   5

// Buzzer
#define BUZZER_PIN 13

// Nút bấm
// Một chân nút nối GPIO, một chân nút nối GND
#define BTN_POWER_PIN 4
#define BTN_SOS_PIN   14

// =======================================================
// TFT
// =======================================================

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
bool watchFaceDrawn = false;

// =======================================================
// MAX30102 / MAX30105
// =======================================================

MAX30105 particleSensor;

const int BUFFER_LENGTH = 100;

uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

int bufferIndex = 0;

int32_t spo2 = 0;
int8_t validSPO2 = 0;

int32_t heartRate = 0;
int8_t validHeartRate = 0;

bool fingerDetected = false;

const uint32_t FINGER_THRESHOLD = 50000;

// =======================================================
// MPU6050 - FALL DETECTION
// =======================================================

Adafruit_MPU6050 mpu;

float fallAccMag = 0;
float fallPitch = 0;
float fallRoll = 0;

enum FallState {
  FALL_NORMAL,
  FALL_FREE,
  FALL_IMPACT,
  FALL_CHECK_STILL,
  FALL_CONFIRMED
};

FallState fallState = FALL_NORMAL;

unsigned long fallStateTime = 0;
unsigned long stillStartTime = 0;

// Ngưỡng té ngã
const float FREE_FALL_THRESHOLD = 0.55;
const float IMPACT_THRESHOLD = 2.4;
const float ANGLE_THRESHOLD = 60.0;
const float GYRO_STILL_THRESHOLD = 1.0;
const unsigned long STILL_TIME = 2000;

// =======================================================
// ALERT SYSTEM
// =======================================================

enum AlertType {
  ALERT_NONE,
  ALERT_FALL,
  ALERT_SOS
};

AlertType alertType = ALERT_NONE;

bool alertActive = false;
bool standbyMode = false;

// =======================================================
// GRAPH
// =======================================================

const int GRAPH_X = 10;
const int GRAPH_Y = 190;
const int GRAPH_W = 220;
const int GRAPH_H = 75;

const int GRAPH_DATA_SIZE = GRAPH_W;

uint32_t graphData[GRAPH_DATA_SIZE];

const uint32_t GRAPH_MIN = 50000;
const uint32_t GRAPH_MAX = 120000;

unsigned long lastScreenUpdate = 0;
const unsigned long SCREEN_UPDATE_INTERVAL = 200;

// =======================================================
// BUTTON DEBOUNCE
// =======================================================

struct Button {
  int pin;
  bool lastReading;
  bool stableState;
  unsigned long lastDebounceTime;
  unsigned long pressStartTime;
  bool shortPressEvent;
  bool longPressEvent;
  bool longFired;
};

Button powerBtn;
Button sosBtn;

const unsigned long DEBOUNCE_DELAY = 40;
const unsigned long LONG_PRESS_TIME = 2000;

void initButton(Button &btn, int pin) {
  btn.pin = pin;
  pinMode(pin, INPUT_PULLUP);

  btn.lastReading = digitalRead(pin);
  btn.stableState = btn.lastReading;
  btn.lastDebounceTime = 0;
  btn.pressStartTime = 0;
  btn.shortPressEvent = false;
  btn.longPressEvent = false;
  btn.longFired = false;
}

void updateButton(Button &btn) {
  btn.shortPressEvent = false;
  btn.longPressEvent = false;

  bool reading = digitalRead(btn.pin);

  if (reading != btn.lastReading) {
    btn.lastDebounceTime = millis();
  }

  if ((millis() - btn.lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != btn.stableState) {
      btn.stableState = reading;

      // Nút được nhấn vì dùng INPUT_PULLUP nên LOW là nhấn
      if (btn.stableState == LOW) {
        btn.pressStartTime = millis();
        btn.longFired = false;
      } else {
        if (!btn.longFired) {
          btn.shortPressEvent = true;
        }
      }
    }

    if (btn.stableState == LOW && !btn.longFired) {
      if (millis() - btn.pressStartTime >= LONG_PRESS_TIME) {
        btn.longPressEvent = true;
        btn.longFired = true;
      }
    }
  }

  btn.lastReading = reading;
}

// =======================================================
// RESET DATA
// =======================================================

void resetHealthBuffers() {
  bufferIndex = 0;

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    irBuffer[i] = 0;
    redBuffer[i] = 0;
  }

  for (int i = 0; i < GRAPH_DATA_SIZE; i++) {
    graphData[i] = GRAPH_MIN;
  }

  heartRate = 0;
  spo2 = 0;
  validHeartRate = 0;
  validSPO2 = 0;
}

void clearAlert() {
  alertActive = false;
  alertType = ALERT_NONE;

  fallState = FALL_NORMAL;
  fallStateTime = 0;
  stillStartTime = 0;

  digitalWrite(BUZZER_PIN, LOW);
}

// =======================================================
// TFT MESSAGE
// =======================================================

void showMessage(String line1, String line2 = "") {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(25, 95);
  tft.println(line1);

  if (line2 != "") {
    tft.setTextSize(2);
    tft.setCursor(35, 140);
    tft.println(line2);
  }
}

// =======================================================
// BUTTON HANDLER
// =======================================================

void handleButtons() {
  updateButton(powerBtn);
  updateButton(sosBtn);

  // Nút SOS: bấm là cảnh báo khẩn cấp
  if (sosBtn.shortPressEvent || sosBtn.longPressEvent) {
    alertActive = true;
    alertType = ALERT_SOS;

    standbyMode = false;
    digitalWrite(TFT_BL, HIGH);
  }

  // Nút nguồn bấm ngắn: tắt cảnh báo
  if (powerBtn.shortPressEvent) {
    if (alertActive) {
      clearAlert();
    }
  }

  // Nút nguồn giữ 2 giây: tắt/bật màn hình
  if (powerBtn.longPressEvent) {
    standbyMode = !standbyMode;

    if (standbyMode) {
      tft.fillScreen(ST77XX_BLACK);
      digitalWrite(TFT_BL, LOW);
    } else {
      digitalWrite(TFT_BL, HIGH);
      showMessage("WAKE UP");
      delay(500);
    }
  }
}

// =======================================================
// BUZZER
// =======================================================

void updateBuzzer() {
  if (alertActive) {
    digitalWrite(BUZZER_PIN, (millis() / 200) % 2);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// =======================================================
// FALL DETECTION
// =======================================================

void updateFallDetection() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x / 9.80665;
  float ay = a.acceleration.y / 9.80665;
  float az = a.acceleration.z / 9.80665;

  fallAccMag = sqrt(ax * ax + ay * ay + az * az);

  fallRoll = atan2(ay, az) * 180.0 / PI;
  fallPitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  float gyroMag = sqrt(
    g.gyro.x * g.gyro.x +
    g.gyro.y * g.gyro.y +
    g.gyro.z * g.gyro.z
  );

  unsigned long now = millis();

  switch (fallState) {
    case FALL_NORMAL:
      if (fallAccMag < FREE_FALL_THRESHOLD) {
        fallState = FALL_FREE;
        fallStateTime = now;
      }
      break;

    case FALL_FREE:
      if (fallAccMag > IMPACT_THRESHOLD) {
        fallState = FALL_IMPACT;
        fallStateTime = now;
      }

      if (now - fallStateTime > 1000) {
        fallState = FALL_NORMAL;
      }
      break;

    case FALL_IMPACT:
      if (fabs(fallPitch) > ANGLE_THRESHOLD || fabs(fallRoll) > ANGLE_THRESHOLD) {
        fallState = FALL_CHECK_STILL;
        fallStateTime = now;
        stillStartTime = now;
      }

      if (now - fallStateTime > 2000) {
        fallState = FALL_NORMAL;
      }
      break;

    case FALL_CHECK_STILL:
      if (gyroMag < GYRO_STILL_THRESHOLD && fabs(fallAccMag - 1.0) < 0.4) {
        if (now - stillStartTime > STILL_TIME) {
          alertActive = true;
          alertType = ALERT_FALL;

          fallState = FALL_CONFIRMED;

          standbyMode = false;
          digitalWrite(TFT_BL, HIGH);
        }
      } else {
        stillStartTime = now;
      }

      if (now - fallStateTime > 6000 && !alertActive) {
        fallState = FALL_NORMAL;
      }
      break;

    case FALL_CONFIRMED:
      alertActive = true;
      alertType = ALERT_FALL;
      break;
  }
}

// =======================================================
// MAX30102 PROCESS
// =======================================================

void pushGraphData(uint32_t value) {
  for (int i = 0; i < GRAPH_DATA_SIZE - 1; i++) {
    graphData[i] = graphData[i + 1];
  }

  graphData[GRAPH_DATA_SIZE - 1] = value;
}

void processHealthSensor() {
  particleSensor.check();

  while (particleSensor.available()) {
    uint32_t redValue = particleSensor.getRed();
    uint32_t irValue = particleSensor.getIR();

    particleSensor.nextSample();

    if (irValue < FINGER_THRESHOLD) {
      if (fingerDetected) {
        resetHealthBuffers();
      }

      fingerDetected = false;
      return;
    }

    if (!fingerDetected) {
      fingerDetected = true;
      resetHealthBuffers();
    }

    pushGraphData(irValue);

    irBuffer[bufferIndex] = irValue;
    redBuffer[bufferIndex] = redValue;
    bufferIndex++;

    if (bufferIndex >= BUFFER_LENGTH) {
      maxim_heart_rate_and_oxygen_saturation(
        irBuffer,
        BUFFER_LENGTH,
        redBuffer,
        &spo2,
        &validSPO2,
        &heartRate,
        &validHeartRate
      );

      bufferIndex = 0;

      Serial.print("HR = ");
      Serial.print(heartRate);
      Serial.print(" | Valid HR = ");
      Serial.print(validHeartRate);
      Serial.print(" | SpO2 = ");
      Serial.print(spo2);
      Serial.print(" | Valid SpO2 = ");
      Serial.print(validSPO2);
      Serial.print(" | Acc = ");
      Serial.print(fallAccMag, 2);
      Serial.println("g");
    }
  }
}

// =======================================================
// DRAW UI
// =======================================================

void drawHeader() {
  tft.fillRect(0, 0, TFT_WIDTH, 35, ST77XX_BLUE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(25, 9);
  tft.print("SMART HEALTH");
}

void drawHealthValues() {
  tft.setTextColor(ST77XX_WHITE);

  // BPM
  tft.drawRoundRect(10, 45, 220, 42, 8, ST77XX_RED);

  tft.setTextSize(2);
  tft.setCursor(20, 58);
  tft.print("BPM:");

  tft.setTextSize(3);
  tft.setCursor(95, 53);

  if (fingerDetected && validHeartRate && heartRate >= 40 && heartRate <= 160) {
    tft.print(heartRate);
  } else {
    tft.print("--");
  }

  // SpO2
  tft.drawRoundRect(10, 95, 220, 42, 8, ST77XX_GREEN);

  tft.setTextSize(2);
  tft.setCursor(20, 108);
  tft.print("SpO2:");

  tft.setTextSize(3);
  tft.setCursor(105, 103);

  if (fingerDetected && validSPO2 && spo2 >= 80 && spo2 <= 100) {
    tft.print(spo2);
    tft.print("%");
  } else {
    tft.print("--");
  }

  if (!fingerDetected) {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(55, 142);
    tft.print("Place finger on MAX30102");
  }
}

void drawAlertStatus() {
  if (alertActive && alertType == ALERT_FALL) {
    tft.fillRoundRect(10, 153, 220, 30, 6, ST77XX_RED);

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(40, 161);
    tft.print("FALL ALERT!");
  } 
  else if (alertActive && alertType == ALERT_SOS) {
    tft.fillRoundRect(10, 153, 220, 30, 6, ST77XX_MAGENTA);

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(55, 161);
    tft.print("SOS ALERT!");
  } 
  else {
    tft.drawRoundRect(10, 153, 220, 30, 6, ST77XX_YELLOW);

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(50, 161);
    tft.print("Fall: OK");
  }
}

int graphMapY(uint32_t value) {
  if (value < GRAPH_MIN) value = GRAPH_MIN;
  if (value > GRAPH_MAX) value = GRAPH_MAX;

  int y = GRAPH_Y + GRAPH_H - 2 -
          ((value - GRAPH_MIN) * (GRAPH_H - 4)) / (GRAPH_MAX - GRAPH_MIN);

  return constrain(y, GRAPH_Y + 2, GRAPH_Y + GRAPH_H - 2);
}

void drawGraph() {
  tft.drawRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, ST77XX_WHITE);

  for (int i = 0; i < GRAPH_DATA_SIZE - 1; i++) {
    int y1 = graphMapY(graphData[i]);
    int y2 = graphMapY(graphData[i + 1]);

    tft.drawLine(GRAPH_X + i, y1, GRAPH_X + i + 1, y2, ST77XX_CYAN);
  }

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(12, 268);
  tft.print("IR Wave | Acc: ");
  tft.print(fallAccMag, 2);
  tft.print("g");
}

void drawWatchFace() {
  if (standbyMode) return;

  if (!watchFaceDrawn) {
    tft.fillScreen(ST77XX_BLACK);

    // Vẽ ảnh mặt đồng hồ 240x240
    // Màn hình bạn 240x280 nên để y = 20 cho nằm giữa theo chiều dọc
    tft.drawRGBBitmap(0, 20, dial240, 240, 240);

    watchFaceDrawn = true;
  }

  // Hiển thị thêm trạng thái té ngã phía dưới
  tft.fillRect(0, 260, 240, 20, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 266);

  if (alertActive && alertType == ALERT_FALL) {
    tft.setTextColor(ST77XX_RED);
    tft.print("FALL ALERT!");
  } else if (alertActive && alertType == ALERT_SOS) {
    tft.setTextColor(ST77XX_MAGENTA);
    tft.print("SOS ALERT!");
  } else {
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Fall: OK");
  }

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(150, 266);
  tft.print("Acc:");
  tft.print(fallAccMag, 1);
  tft.print("g");
}

// void drawMainScreen() {
//   if (standbyMode) {
//     return;
//   }

//   tft.fillScreen(ST77XX_BLACK);

//   drawHeader();
//   drawHealthValues();
//   drawAlertStatus();
//   drawGraph();
// }

void drawMainScreen() {
  drawWatchFace();
}

// =======================================================
// SETUP
// =======================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  initButton(powerBtn, BTN_POWER_PIN);
  initButton(sosBtn, BTN_SOS_PIN);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.init(240, 280);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  showMessage("Welcome");
  delay(1000);

  showMessage("Health", "Monitor");
  delay(1000);

  // MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Khong tim thay MAX30102!");

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(20, 110);
    tft.println("MAX30102 FAILED");
    tft.setCursor(20, 140);
    tft.println("Check wiring");

    while (1);
  }

  Serial.println("MAX30102 SUCCESS");

  byte ledBrightness = 0x1F;
  byte sampleAverage = 4;
  byte ledMode = 2;
  int sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);

  // MPU6050
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("Khong tim thay MPU6050!");

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(30, 110);
    tft.println("MPU FAILED");
    tft.setCursor(20, 140);
    tft.println("Check wiring");

    while (1);
  }

  Serial.println("MPU6050 SUCCESS");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  resetHealthBuffers();

  showMessage("SETUP", "DONE");
  delay(1000);
}

// =======================================================
// LOOP
// =======================================================

void loop() {
  handleButtons();

  updateFallDetection();
  processHealthSensor();

  updateBuzzer();

  if (millis() - lastScreenUpdate >= SCREEN_UPDATE_INTERVAL) {
    lastScreenUpdate = millis();
    drawMainScreen();
  }

  delay(10);
}