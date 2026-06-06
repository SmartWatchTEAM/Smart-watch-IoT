#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// =======================================================
// PIN CONFIG - ESP32-S3 SUPER MINI
// =======================================================

// I2C bus 0 cho MPU6050
#define MPU_SDA 12
#define MPU_SCL 13

// I2C bus 1 cho MAX30102
#define MAX_SDA 10
#define MAX_SCL 11

// SOS

// TFT ST7789V3 SPI
#define TFT_WIDTH   240
#define TFT_HEIGHT  280

#define TFT_SCLK 2
#define TFT_MOSI 3
#define TFT_CS   6
#define TFT_RST  4
#define TFT_DC   5
#define TFT_BL   7

// Buzzer
#define BUZZER_PIN 14

// Nút bấm
#define BTN_FUNCTION 9
#define BTN_SOS_PIN 15

// Địa chỉ I2C
#define MAX30102_ADDR 0x57

// =======================================================
// TFT
// =======================================================

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

bool uiDrawn = false;

// =======================================================
// UI COLORS
// =======================================================
#define COLOR_BG          ST77XX_BLACK
#define COLOR_PANEL       0x1082
#define COLOR_CARD_BLUE   0x0254
#define COLOR_CARD_RED    0x2800
#define COLOR_CARD_GREEN  0x0200
#define COLOR_BORDER      0x8410
#define COLOR_DIM         0x4208
#define COLOR_ACCENT      ST77XX_CYAN
#define COLOR_SUN         0xFD20
#define COLOR_SUN_CORE    ST77XX_YELLOW
#define COLOR_CLOUD       ST77XX_WHITE
#define COLOR_CLOUD_SHA   0xC618



// =======================================================
// WIFI / REAL TIME CLOCK - NTP
// =======================================================

const char* WIFI_SSID = "Redmi Note 11";
const char* WIFI_PASSWORD = "sos1676cc";

const long GMT_OFFSET_SEC = 7 * 3600;      // Viet Nam GMT+7
const int DAYLIGHT_OFFSET_SEC = 0;

unsigned long lastClockUpdate = 0;
const unsigned long CLOCK_UPDATE_INTERVAL = 1000;
bool timeSynced = false;

// =======================================================
// REAL WEATHER - OPEN-METEO
// =======================================================
// Tọa độ TP.HCM. Có thể đổi theo vị trí bạn muốn hiển thị thời tiết.
const float WEATHER_LAT = 10.8231;
const float WEATHER_LON = 106.6297;

unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_UPDATE_INTERVAL = 10UL * 60UL * 1000UL; // cập nhật mỗi 10 phút

bool weatherReady = false;
String weatherTempText = "--";
String weatherDescText = "Offline";
String weatherHumidityText = "--";
String weatherWindText = "--";

// =======================================================
// WATCH FACE / MINI OS
// =======================================================

enum ScreenPage {
  SCREEN_HOME,
  SCREEN_HEALTH
};

ScreenPage currentScreen = SCREEN_HOME;

// Dữ liệu giao diện chính. Giờ/ngày sẽ lấy từ NTP sau khi kết nối WiFi.
String currentTimeText = "--:--";
String currentDateText = "--, --/--";
String weatherCity = "TP.HCM";
String weatherText = "--C";
String spo2HomeText = "--";
int batteryPercent = 82;
int quickBPM = 0;
bool wifiConnected = false;

unsigned long lastClockDemoUpdate = 0;
int demoHour = 14;
int demoMinute = 35;

// =======================================================
// MAX30102 / MAX30105 - THUAT TOAN ON DINH
// =======================================================

MAX30105 particleSensor;

// Dung 100 mau @25Hz = khoang 4 giay. Sau moi lan tinh giu lai 75 mau cu,
// them 25 mau moi de cap nhat gan moi giay.
const int BUFFER_LENGTH = 100;
const int MAX_SAMPLE_RATE_HZ = 25;
const int NEW_SAMPLES = 25;

uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

int bufferIndex = 0;

int32_t spo2 = 0;
int8_t validSPO2 = 0;

int32_t heartRate = 0;
int8_t validHeartRate = 0;

// Gia tri da loc de hien thi len UI
int32_t displayHeartRate = 0;
int32_t displaySpO2 = 0;

bool fingerDetected = false;
bool maxReady = false;
bool mpuReady = false;

// Theo log cua ban: khong dat tay IR < 10k, dat tay tot ~70k-160k.
const uint32_t NO_FINGER_THRESHOLD = 12000;
const uint32_t FINGER_THRESHOLD    = 25000;
const uint32_t TARGET_IR_LOW       = 60000;
const uint32_t TARGET_IR_HIGH      = 160000;
const uint32_t SATURATION_LEVEL    = 245000;  // gan tran 262143 la bao hoa ADC

// LED bat dau vua phai, sau do auto LED tu tang/giam theo IR/RED.
byte maxLedLevel = 0x38;
unsigned long lastLedAdjust = 0;
unsigned long lastHealthPrint = 0;
unsigned long ignoreSamplesUntil = 0;
unsigned long lastBufferProgressPrint = 0;

const unsigned long FINGER_STABILIZE_MS = 2500; // bo qua mau luc moi dat tay
const unsigned long LED_SETTLE_MS       = 1500; // bo qua mau sau khi doi LED

const byte HR_SMOOTH_SIZE = 5;
int hrHistory[HR_SMOOTH_SIZE];
byte hrHistoryCount = 0;
byte hrHistoryIndex = 0;

const byte SPO2_SMOOTH_SIZE = 5;
int spo2History[SPO2_SMOOTH_SIZE];
byte spo2HistoryCount = 0;
byte spo2HistoryIndex = 0;

struct SignalStats {
  uint32_t irMin;
  uint32_t irMax;
  uint32_t redMin;
  uint32_t redMax;
  uint32_t irAvg;
  uint32_t redAvg;
  uint32_t irAC;
  uint32_t redAC;
  float irACPercent;
  float redACPercent;
  uint8_t saturatedSamples;
};

SignalStats lastSignalStats;
bool signalOK = false;

// =======================================================
// MPU6500 - FALL DETECTION
// =======================================================

// Chip của bạn trả WHO_AM_I = 0x70, nên không dùng Adafruit_MPU6050.
// Đọc MPU6500 trực tiếp bằng thanh ghi raw I2C.
byte mpuI2CAddress = 0x68;

const byte MPU6500_REG_WHO_AM_I      = 0x75;
const byte MPU6500_REG_PWR_MGMT_1    = 0x6B;
const byte MPU6500_REG_CONFIG        = 0x1A;
const byte MPU6500_REG_GYRO_CONFIG   = 0x1B;
const byte MPU6500_REG_ACCEL_CONFIG  = 0x1C;
const byte MPU6500_REG_ACCEL_CONFIG2 = 0x1D;
const byte MPU6500_REG_ACCEL_XOUT_H  = 0x3B;

float accX = 0;
float accY = 0;
float accZ = 0;

float gyroX = 0;
float gyroY = 0;
float gyroZ = 0;

unsigned long lastMPUPrint = 0;

TwoWire I2C_MAX = TwoWire(1);

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

const uint32_t GRAPH_MIN = 20000;
const uint32_t GRAPH_MAX = 120000;

unsigned long lastScreenUpdate = 0;
const unsigned long SCREEN_UPDATE_INTERVAL = 1000;

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
const unsigned long LONG_PRESS_TIME = 5000;   // nhấn giữ 5s để SOS / sleep

// =======================================================
// I2C CHECK
// =======================================================

bool checkI2CDevice(TwoWire &bus, byte address) {
  bus.beginTransmission(address);
  byte error = bus.endTransmission();

  return error == 0;
}

void scanI2C(TwoWire &bus, const char *busName) {
  Serial.println();
  Serial.print("===== I2C SCANNER: ");
  Serial.print(busName);
  Serial.println(" =====");

  bool foundAny = false;
for (byte address = 1; address < 127; address++) {
    bus.beginTransmission(address);
    byte error = bus.endTransmission();

    if (error == 0) {
      foundAny = true;
      Serial.print("Found I2C device at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (!foundAny) {
    Serial.println("Khong tim thay thiet bi I2C nao!");
  }

  Serial.println("=======================");
  Serial.println();
}

// =======================================================
// BUTTON
// =======================================================

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
  displayHeartRate = 0;
  displaySpO2 = 0;

  hrHistoryCount = 0;
  hrHistoryIndex = 0;
  spo2HistoryCount = 0;
  spo2HistoryIndex = 0;

  for (int i = 0; i < HR_SMOOTH_SIZE; i++) hrHistory[i] = 0;
  for (int i = 0; i < SPO2_SMOOTH_SIZE; i++) spo2History[i] = 0;

  signalOK = false;
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
  tft.setFont(NULL);
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

  uiDrawn = false;
}

void showError(String title, String line2) {
  tft.setFont(NULL);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(20, 105);
tft.println(title);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 140);
  tft.println(line2);

  uiDrawn = false;
}

// =======================================================
// BUTTON HANDLER
// =======================================================

void handleButtons() {
  updateButton(powerBtn);
  updateButton(sosBtn);

  // Nút A: nhấn ngắn để đổi màn hình
  if (powerBtn.shortPressEvent) {
    if (alertActive) {
      clearAlert();
    } else {
      currentScreen = (currentScreen == SCREEN_HOME) ? SCREEN_HEALTH : SCREEN_HOME;
      uiDrawn = false;
    }
  }

  // Nút A: nhấn giữ để bật/tắt màn hình tiết kiệm pin
  if (powerBtn.longPressEvent) {
    standbyMode = !standbyMode;

    if (standbyMode) {
      tft.fillScreen(ST77XX_BLACK);
      digitalWrite(TFT_BL, LOW);
      uiDrawn = false;
    } else {
      digitalWrite(TFT_BL, HIGH);
      showMessage("WAKE UP");
      delay(500);
      uiDrawn = false;
    }
  }

  // Nút B: nhấn giữ 5s để SOS
  if (sosBtn.longPressEvent) {
    alertActive = true;
    alertType = ALERT_SOS;

    standbyMode = false;
    digitalWrite(TFT_BL, HIGH);
    uiDrawn = false;
  }

  // Nút B: nhấn ngắn để quay về màn hình chính
  if (sosBtn.shortPressEvent && !alertActive) {
    currentScreen = SCREEN_HOME;
    uiDrawn = false;
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
// MPU6500 RAW I2C DRIVER
// =======================================================

bool mpuWrite8(byte reg, byte value) {
  Wire.beginTransmission(mpuI2CAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool mpuReadBytes(byte reg, byte count, byte *data) {
  Wire.beginTransmission(mpuI2CAddress);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  byte received = Wire.requestFrom(mpuI2CAddress, count);

  if (received != count) {
    return false;
  }

  for (byte i = 0; i < count; i++) {
    data[i] = Wire.read();
  }

  return true;
}

byte mpuRead8(byte reg) {
  byte data = 0xFF;

  if (mpuReadBytes(reg, 1, &data)) {
    return data;
  }

  return 0xFF;
}

int16_t combine16(byte highByte, byte lowByte) {
  return (int16_t)((highByte << 8) | lowByte);
}

bool readMPU6500() {
  byte data[14];

  if (!mpuReadBytes(MPU6500_REG_ACCEL_XOUT_H, 14, data)) {
    return false;
  }

  int16_t rawAX = combine16(data[0], data[1]);
  int16_t rawAY = combine16(data[2], data[3]);
  int16_t rawAZ = combine16(data[4], data[5]);

  int16_t rawGX = combine16(data[8], data[9]);
  int16_t rawGY = combine16(data[10], data[11]);
  int16_t rawGZ = combine16(data[12], data[13]);
// ACCEL_CONFIG = 0x10 tương ứng thang đo ±8g => 4096 LSB/g
  accX = rawAX / 4096.0;
  accY = rawAY / 4096.0;
  accZ = rawAZ / 4096.0;

  // GYRO_CONFIG = 0x08 tương ứng ±500 deg/s => 65.5 LSB/(deg/s)
  // Đổi sang rad/s để giữ nguyên ngưỡng GYRO_STILL_THRESHOLD = 1.0 như code cũ.
  gyroX = (rawGX / 65.5) * PI / 180.0;
  gyroY = (rawGY / 65.5) * PI / 180.0;
  gyroZ = (rawGZ / 65.5) * PI / 180.0;

  return true;
}

// =======================================================
// FALL DETECTION
// =======================================================

void updateFallDetection() {
  if (!mpuReady) return;

  if (!readMPU6500()) {
    return;
  }

  fallAccMag = sqrt(accX * accX + accY * accY + accZ * accZ);

  fallRoll = atan2(accY, accZ) * 180.0 / PI;
  fallPitch = atan2(-accX, sqrt(accY * accY + accZ * accZ)) * 180.0 / PI;

  float gyroMag = sqrt(
    gyroX * gyroX +
    gyroY * gyroY +
    gyroZ * gyroZ
  );

  if (millis() - lastMPUPrint >= 500) {
    lastMPUPrint = millis();

    Serial.print("MPU | ax=");
    Serial.print(accX, 2);
    Serial.print("g ay=");
    Serial.print(accY, 2);
    Serial.print("g az=");
    Serial.print(accZ, 2);
    Serial.print("g | mag=");
    Serial.print(fallAccMag, 2);
    Serial.print("g | pitch=");
    Serial.print(fallPitch, 1);
    Serial.print(" roll=");
    Serial.println(fallRoll, 1);
  }

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
            Serial.println("========== FALL DETECTED ==========");

            alertActive = true;
            alertType = ALERT_FALL;

            currentScreen = SCREEN_HEALTH;   // tự chuyển sang màn hình Health

            fallState = FALL_CONFIRMED;

            standbyMode = false;
            digitalWrite(TFT_BL, HIGH);
            uiDrawn = false;
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
// MAX30102 PROCESS - THUAT TOAN TU FILE MAX30102
// =======================================================

void pushGraphData(uint32_t value) {
  for (int i = 0; i < GRAPH_DATA_SIZE - 1; i++) {
    graphData[i] = graphData[i + 1];
  }

  graphData[GRAPH_DATA_SIZE - 1] = value;
}

int medianSmall(int *values, byte count) {
  if (count == 0) return 0;

  int tmp[8];
  if (count > 8) count = 8;

  for (byte i = 0; i < count; i++) tmp[i] = values[i];

  for (byte i = 0; i < count - 1; i++) {
    for (byte j = i + 1; j < count; j++) {
      if (tmp[j] < tmp[i]) {
        int t = tmp[i];
        tmp[i] = tmp[j];
        tmp[j] = t;
      }
    }
  }

  return tmp[count / 2];
}

void addHRValue(int bpm) {
  if (bpm < 45 || bpm > 130) return;

  // Nhịp tim thật không đổi đột ngột quá mạnh; bỏ mẫu nhiễu.
  if (displayHeartRate > 0 && abs(bpm - displayHeartRate) > 25) return;

  hrHistory[hrHistoryIndex++] = bpm;
  hrHistoryIndex %= HR_SMOOTH_SIZE;
  if (hrHistoryCount < HR_SMOOTH_SIZE) hrHistoryCount++;

  displayHeartRate = medianSmall(hrHistory, hrHistoryCount);
  heartRate = displayHeartRate;
  validHeartRate = (hrHistoryCount >= 2) ? 1 : 0;
}

void addSpO2Value(int value) {
  if (value < 85 || value > 100) return;

  if (displaySpO2 > 0 && abs(value - displaySpO2) > 4) return;

  spo2History[spo2HistoryIndex++] = value;
  spo2HistoryIndex %= SPO2_SMOOTH_SIZE;
  if (spo2HistoryCount < SPO2_SMOOTH_SIZE) spo2HistoryCount++;

  displaySpO2 = medianSmall(spo2History, spo2HistoryCount);
  spo2 = displaySpO2;
  validSPO2 = (spo2HistoryCount >= 2) ? 1 : 0;
}

void sortU32Small(uint32_t *arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (arr[j] < arr[i]) {
        uint32_t t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
      }
    }
  }
}

SignalStats calculateSignalStats() {
  SignalStats s;
  s.irMin = 0xFFFFFFFF;
  s.redMin = 0xFFFFFFFF;
  s.irMax = 0;
  s.redMax = 0;
  s.saturatedSamples = 0;

  uint64_t irSum = 0;
  uint64_t redSum = 0;

  uint32_t irSorted[BUFFER_LENGTH];
  uint32_t redSorted[BUFFER_LENGTH];

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    uint32_t ir = irBuffer[i];
    uint32_t red = redBuffer[i];

    irSorted[i] = ir;
    redSorted[i] = red;

    if (ir < s.irMin) s.irMin = ir;
    if (ir > s.irMax) s.irMax = ir;
    if (red < s.redMin) s.redMin = red;
    if (red > s.redMax) s.redMax = red;

    if (ir >= SATURATION_LEVEL || red >= SATURATION_LEVEL) {
      s.saturatedSamples++;
    }

    irSum += ir;
    redSum += red;
  }

  s.irAvg = irSum / BUFFER_LENGTH;
  s.redAvg = redSum / BUFFER_LENGTH;

  // Dung P90-P10 thay vi max-min de tranh 1-2 mau loi lam AC% ao.
  sortU32Small(irSorted, BUFFER_LENGTH);
  sortU32Small(redSorted, BUFFER_LENGTH);

  uint32_t irP10  = irSorted[10];
  uint32_t irP90  = irSorted[90];
  uint32_t redP10 = redSorted[10];
  uint32_t redP90 = redSorted[90];

  s.irAC = irP90 - irP10;
  s.redAC = redP90 - redP10;
  s.irACPercent = (s.irAvg > 0) ? (s.irAC * 100.0f / s.irAvg) : 0;
  s.redACPercent = (s.redAvg > 0) ? (s.redAC * 100.0f / s.redAvg) : 0;

  return s;
}

bool isSignalGood(const SignalStats &s) {
  if (s.irAvg < 45000 || s.irAvg > 220000) return false;
  if (s.redAvg < 30000 || s.redAvg > 240000) return false;
  if (s.saturatedSamples > 0) return false;
  if (s.irACPercent < 0.25 || s.irACPercent > 8.0) return false;
  if (s.redACPercent < 0.15 || s.redACPercent > 10.0) return false;
  return true;
}

void applyLedLevel() {
  particleSensor.setPulseAmplitudeRed(maxLedLevel);
  particleSensor.setPulseAmplitudeIR(maxLedLevel);
  particleSensor.setPulseAmplitudeGreen(0);
}

void autoTuneLed(uint32_t irValue, uint32_t redValue) {
  unsigned long now = millis();
  if (now - lastLedAdjust < 900) return;

  byte oldLevel = maxLedLevel;

  if (irValue >= SATURATION_LEVEL || redValue >= SATURATION_LEVEL) {
    if (maxLedLevel > 0x20) maxLedLevel -= 0x08;
  } else if (irValue >= FINGER_THRESHOLD && irValue < TARGET_IR_LOW) {
    if (maxLedLevel < 0x80) maxLedLevel += 0x02;
  } else if (irValue > TARGET_IR_HIGH || redValue > 200000) {
    if (maxLedLevel > 0x20) maxLedLevel -= 0x02;
  }

  if (oldLevel != maxLedLevel) {
    lastLedAdjust = now;
    applyLedLevel();

    // Sau khi doi LED, DC/AC se nhay manh nen reset cua so do va cho on dinh.
    bufferIndex = 0;
    ignoreSamplesUntil = now + LED_SETTLE_MS;
    validHeartRate = 0;
    validSPO2 = 0;

    Serial.print("Auto LED -> 0x");
    Serial.print(maxLedLevel, HEX);
    Serial.print(" | IR=");
    Serial.print(irValue);
    Serial.print(" RED=");
    Serial.println(redValue);
  }
}

int calculateHeartRateFromIRBuffer() {
  SignalStats s = lastSignalStats;

  if (!signalOK) return 0;
  if (s.irAC < 250) return 0;

  uint32_t threshold = s.irAvg + (s.irAC * 20UL) / 100UL;

  int peaks[12];
  int peakCount = 0;
  int lastPeak = -1000;

  const int minDistance = 10; // 10 mau @25Hz = 400ms => max 150 BPM
  const int maxDistance = 38; // 38 mau @25Hz = 1.52s => min 39 BPM

  for (int i = 2; i < BUFFER_LENGTH - 2; i++) {
    bool isPeak = irBuffer[i] > threshold &&
                  irBuffer[i] >= irBuffer[i - 1] &&
                  irBuffer[i] >  irBuffer[i + 1] &&
                  irBuffer[i] >= irBuffer[i - 2] &&
                  irBuffer[i] >  irBuffer[i + 2];

    if (!isPeak) continue;

    if (i - lastPeak < minDistance) {
      if (peakCount > 0 && irBuffer[i] > irBuffer[peaks[peakCount - 1]]) {
        peaks[peakCount - 1] = i;
        lastPeak = i;
      }
      continue;
    }

    peaks[peakCount++] = i;
    lastPeak = i;
    if (peakCount >= 12) break;
  }

  if (peakCount < 3) return 0;

  int bpms[10];
  int bpmCount = 0;

  for (int i = 1; i < peakCount; i++) {
    int d = peaks[i] - peaks[i - 1];
    if (d >= minDistance && d <= maxDistance) {
      int bpm = (60 * MAX_SAMPLE_RATE_HZ) / d;
      if (bpm >= 45 && bpm <= 130) {
        bpms[bpmCount++] = bpm;
      }
    }
  }

  if (bpmCount < 2) return 0;
  return medianSmall(bpms, bpmCount);
}

void processCompletedHealthWindow() {
  lastSignalStats = calculateSignalStats();
  signalOK = isSignalGood(lastSignalStats);

  int32_t rawSpO2 = 0;
  int8_t rawValidSpO2 = 0;
  int32_t rawHRIgnored = 0;
  int8_t rawValidHRIgnored = 0;

  if (signalOK) {
    maxim_heart_rate_and_oxygen_saturation(
      irBuffer,
      BUFFER_LENGTH,
      redBuffer,
      &rawSpO2,
      &rawValidSpO2,
      &rawHRIgnored,
      &rawValidHRIgnored
    );

    if (rawValidSpO2 && rawSpO2 >= 85 && rawSpO2 <= 100) {
      addSpO2Value(rawSpO2);
    } else {
      validSPO2 = 0;
    }

    int hr = calculateHeartRateFromIRBuffer();
    if (hr > 0) {
      addHRValue(hr);
    } else {
      validHeartRate = 0;
    }
  } else {
    validHeartRate = 0;
    validSPO2 = 0;
  }

  if (millis() - lastHealthPrint > 1000) {
    lastHealthPrint = millis();

    Serial.println();
    Serial.println("========== MAX30102 ==========");
    Serial.print("LED=0x"); Serial.print(maxLedLevel, HEX);
    Serial.print(" | IRavg="); Serial.print(lastSignalStats.irAvg);
    Serial.print(" | REDavg="); Serial.print(lastSignalStats.redAvg);
    Serial.print(" | IRac%="); Serial.print(lastSignalStats.irACPercent, 2);
    Serial.print(" | REDac%="); Serial.print(lastSignalStats.redACPercent, 2);
    Serial.print(" | Sat="); Serial.print(lastSignalStats.saturatedSamples);
    Serial.print(" | SignalOK="); Serial.println(signalOK ? "YES" : "NO");

    Serial.print("Raw SpO2="); Serial.print(rawSpO2);
    Serial.print(" valid="); Serial.print(rawValidSpO2);
    Serial.print(" | Raw HR ignored="); Serial.print(rawHRIgnored);
    Serial.print(" valid="); Serial.println(rawValidHRIgnored);

    Serial.print("Display HR=");
    if (validHeartRate) Serial.print(heartRate); else Serial.print("--");
    Serial.print(" | Display SpO2=");
    if (validSPO2) Serial.print(spo2); else Serial.print("--");
    Serial.println();

    if (!signalOK) {
      Serial.println("TIN HIEU CHUA TOT: giu yen tay, che kin cam bien, khong an qua manh.");
      if (lastSignalStats.saturatedSamples > 0) {
        Serial.println("CANH BAO: IR/RED gan 262143 => bao hoa ADC, dat tay nhe hon hoac doi auto LED.");
      }
    }
    Serial.println("==============================");
  }
}

void processHealthSensor() {
  if (!maxReady) return;

  particleSensor.check();

  while (particleSensor.available()) {
    uint32_t redValue = particleSensor.getRed();
    uint32_t irValue = particleSensor.getIR();
    particleSensor.nextSample();

    unsigned long now = millis();

    if (irValue < NO_FINGER_THRESHOLD) {
      if (fingerDetected) {
        Serial.println("Mat ngon tay -> reset buffer MAX30102");
        resetHealthBuffers();
      }
      fingerDetected = false;
      return;
    }

    if (irValue >= FINGER_THRESHOLD && !fingerDetected) {
      fingerDetected = true;
      resetHealthBuffers();
      ignoreSamplesUntil = now + FINGER_STABILIZE_MS;
      lastBufferProgressPrint = 0;
      Serial.println("Da phat hien ngon tay. Giu yen 10-15 giay de on dinh HR/SpO2...");
    }

    if (!fingerDetected) return;

    autoTuneLed(irValue, redValue);

    // Moi dat tay hoac vua doi LED: bo qua mau de tranh IRac% ao 30-100%.
    if (now < ignoreSamplesUntil) {
      if (now - lastBufferProgressPrint > 1000) {
        lastBufferProgressPrint = now;
        Serial.print("Dang doi tin hieu on dinh... LED=0x");
        Serial.print(maxLedLevel, HEX);
        Serial.print(" | IR=");
        Serial.print(irValue);
        Serial.print(" RED=");
        Serial.println(redValue);
      }
      return;
    }

    if (irValue >= SATURATION_LEVEL || redValue >= SATURATION_LEVEL) {
      validHeartRate = 0;
      validSPO2 = 0;
      bufferIndex = 0;
      ignoreSamplesUntil = now + LED_SETTLE_MS;
      Serial.println("DANG BAO HOA ADC -> bo mau, giam luc an tay hoac cho auto LED giam.");
      return;
    }

    pushGraphData(irValue);

    irBuffer[bufferIndex] = irValue;
    redBuffer[bufferIndex] = redValue;
    bufferIndex++;

    if (now - lastBufferProgressPrint > 1000 && bufferIndex < BUFFER_LENGTH) {
      lastBufferProgressPrint = now;
      Serial.print("Dang nap buffer MAX30102: ");
      Serial.print(bufferIndex);
      Serial.print("/");
      Serial.print(BUFFER_LENGTH);
      Serial.print(" | LED=0x");
      Serial.print(maxLedLevel, HEX);
      Serial.print(" | IR=");
      Serial.print(irValue);
      Serial.print(" | RED=");
      Serial.println(redValue);
    }

    if (bufferIndex >= BUFFER_LENGTH) {
      processCompletedHealthWindow();

      for (int i = NEW_SAMPLES; i < BUFFER_LENGTH; i++) {
        irBuffer[i - NEW_SAMPLES] = irBuffer[i];
        redBuffer[i - NEW_SAMPLES] = redBuffer[i];
      }
      bufferIndex = BUFFER_LENGTH - NEW_SAMPLES;
    }
  }
}

// =======================================================
// SMARTWATCH HOME UI - PREMIUM WATCH FACE
// =======================================================

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Ket noi WiFi: ");
  Serial.println(WIFI_SSID);

  unsigned long startAttempt = millis();
  const unsigned long WIFI_TIMEOUT = 15000;

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("WiFi SUCCESS");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("WiFi FAILED");
  }
}

void syncTimeFromNTP() {
  if (!wifiConnected) {
    timeSynced = false;
    return;
  }

  Serial.println("Dong bo thoi gian NTP...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov", "time.google.com");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10000)) {
    timeSynced = true;
    Serial.println("NTP OK");
  } else {
    timeSynced = false;
    Serial.println("NTP FAILED");
  }
}

void updateClock() {
  if (millis() - lastClockUpdate < CLOCK_UPDATE_INTERVAL) {
    return;
  }

  lastClockUpdate = millis();
  wifiConnected = (WiFi.status() == WL_CONNECTED);

  struct tm timeinfo;
  if (wifiConnected && getLocalTime(&timeinfo)) {
    char timeBuf[6];
    char dateBuf[12];

    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
    strftime(dateBuf, sizeof(dateBuf), "%a, %d/%m", &timeinfo);

    currentTimeText = String(timeBuf);
    currentDateText = String(dateBuf);
    timeSynced = true;
  }
}


String weatherCodeToText(int code) {
  if (code == 0) return "Clear";
  if (code == 1 || code == 2) return "Partly";
  if (code == 3) return "Cloudy";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Shower";
  if (code >= 95 && code <= 99) return "Storm";
  return "Weather";
}

String getJsonObject(const String &payload, const char *objectName) {
  String pattern = String("\"") + objectName + "\":";
  int keyIndex = payload.indexOf(pattern);

  if (keyIndex < 0) {
    return "";
  }

  int start = payload.indexOf('{', keyIndex);
  if (start < 0) {
    return "";
  }

  int depth = 0;
  for (int i = start; i < payload.length(); i++) {
    if (payload[i] == '{') depth++;
    else if (payload[i] == '}') {
      depth--;
      if (depth == 0) {
        return payload.substring(start, i + 1);
      }
    }
  }

  return "";
}

float getJsonFloatValue(const String &json, const char *key, float fallback) {
  String pattern = String("\"") + key + "\":";
  int idx = json.indexOf(pattern);

  if (idx < 0) {
    return fallback;
  }

  idx += pattern.length();

  while (idx < json.length() && (json[idx] == ' ' || json[idx] == '\"')) {
    idx++;
  }

  int end = idx;
  while (end < json.length()) {
    char c = json[end];
    if ((c >= '0' && c <= '9') || c == '-' || c == '+'
        || c == '.' || c == 'e' || c == 'E') {
      end++;
    } else {
      break;
    }
  }

  if (end <= idx) {
    return fallback;
  }

  return json.substring(idx, end).toFloat();
}

void setWeatherOffline(String reason) {
  weatherReady = false;
  weatherTempText = "--";
  weatherDescText = reason;
  weatherHumidityText = "--";
  weatherWindText = "--";
  weatherText = "--C";
}

void updateWeatherNow() {
  wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (!wifiConnected) {
    setWeatherOffline("Offline");
    Serial.println("Weather skipped: WiFi not connected");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(8000);

  String url = "https://api.open-meteo.com/v1/forecast?"
               "latitude=10.8231&longitude=106.6297"
               "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code"
               "&timezone=Asia%2FBangkok";

  Serial.println("Lay nhiet do thoi tiet thuc te...");
  Serial.println(url);

  if (!http.begin(client, url)) {
    setWeatherOffline("HTTP Err");
    Serial.println("Weather HTTP begin failed");
    return;
  }

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    String currentJson = getJsonObject(payload, "current");

    float tempC = getJsonFloatValue(currentJson, "temperature_2m", NAN);
    float humidity = getJsonFloatValue(currentJson, "relative_humidity_2m", NAN);
    float wind = getJsonFloatValue(currentJson, "wind_speed_10m", NAN);
    int weatherCode = (int)getJsonFloatValue(currentJson, "weather_code", -1);

    if (!isnan(tempC)) {
      weatherReady = true;
      weatherTempText = String((int)round(tempC));
      weatherDescText = weatherCodeToText(weatherCode);

      if (!isnan(humidity)) weatherHumidityText = String((int)round(humidity));
      else weatherHumidityText = "--";

      if (!isnan(wind)) weatherWindText = String((int)round(wind));
      else weatherWindText = "--";

      weatherText = weatherTempText + "C " + weatherDescText;

      Serial.print("Weather OK | Temp=");
      Serial.print(weatherTempText);
      Serial.print("C | Hum=");
      Serial.print(weatherHumidityText);
      Serial.print("% | Wind=");
      Serial.print(weatherWindText);
      Serial.print(" km/h | Code=");
      Serial.println(weatherCode);
    } else {
      setWeatherOffline("No data");
      Serial.println("Weather parse failed: no temperature_2m");
    }
  } else {
    setWeatherOffline("HTTP Fail");
    Serial.print("Weather HTTP error: ");
    Serial.println(httpCode);
  }

  http.end();
}

void updateWeatherIfNeeded() {
  if (millis() - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
    lastWeatherUpdate = millis();
    updateWeatherNow();
  }
}


void drawBatteryIcon(int x, int y, int percent) {
  // Icon pin nhỏ, nằm gọn trong top bar
  percent = constrain(percent, 0, 100);

  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.drawRoundRect(x, y, 25, 12, 3, ST77XX_WHITE);
  tft.fillRect(x + 25, y + 4, 3, 4, ST77XX_WHITE);

  int fillW = map(percent, 0, 100, 0, 21);
  uint16_t color = ST77XX_GREEN;
  if (percent < 20) color = ST77XX_RED;
  else if (percent < 50) color = ST77XX_YELLOW;

  tft.fillRoundRect(x + 2, y + 2, fillW, 8, 2, color);

  // phần trăm đặt bên dưới để không bị đè icon
  tft.setTextSize(1);

  char battText[6];
  sprintf(battText,"%d%%",percent);

  tft.setCursor(x + 1, y + 14);
  tft.print(battText);
}



void drawArcSegments(int16_t x, int16_t y, int16_t radius,
                    int startDeg, int endDeg, uint16_t color) {
  const int stepDeg = 10;
for (int a = startDeg; a < endDeg; a += stepDeg) {
    float r0 = radians((float)a);
    float r1 = radians((float)(a + stepDeg));
    int16_t x0 = x + (int16_t)roundf(cosf(r0) * radius);
    int16_t y0 = y + (int16_t)roundf(sinf(r0) * radius);
    int16_t x1 = x + (int16_t)roundf(cosf(r1) * radius);
    int16_t y1 = y + (int16_t)roundf(sinf(r1) * radius);
    tft.drawLine(x0, y0, x1, y1, color);
  }
}

void drawWifiIcon(int16_t x, int16_t y, float scale, bool connected) {
  // Icon WiFi nhỏ, cân giữa trong ô trạng thái
  uint16_t color = connected ? COLOR_ACCENT : ST77XX_RED;

  int r1 = max(3, (int)roundf(5.0f * scale));
  int r2 = max(5, (int)roundf(9.0f * scale));
  int r3 = max(7, (int)roundf(13.0f * scale));

  drawArcSegments(x, y, r1, 220, 320, color);
  drawArcSegments(x, y, r2, 220, 320, color);
  drawArcSegments(x, y, r3, 220, 320, color);

  tft.fillCircle(x, y + max(4, (int)roundf(7.0f * scale)),
                max(2, (int)roundf(2.0f * scale)), color);

  if (!connected) {
    int slash = max(8, (int)roundf(11.0f * scale));
    tft.drawLine(x - slash, y - slash + 3, x + slash, y + slash, ST77XX_RED);
  }
}




void drawLinkIcon(int x, int y) {
  // Icon liên kết ở giữa top bar, tạo cảm giác đồng hồ thông minh
  tft.drawRoundRect(x, y, 15, 8, 4, ST77XX_GREEN);
  tft.drawRoundRect(x + 12, y, 15, 8, 4, ST77XX_GREEN);
  tft.drawLine(x + 10, y + 4, x + 17, y + 4, ST77XX_GREEN);
}

void drawWatchCaseFrame() {
  // Nền ngoài giả vỏ đồng hồ
  tft.fillScreen(ST77XX_BLACK);

  // Bóng ngoài
  tft.drawRoundRect(0, 0, 240, 280, 30, 0x4208);
  tft.drawRoundRect(1, 1, 238, 278, 29, 0x8410);
  tft.drawRoundRect(3, 3, 234, 274, 27, ST77XX_WHITE);
  tft.drawRoundRect(5, 5, 230, 270, 25, 0x8410);
  tft.drawRoundRect(8, 8, 224, 264, 23, 0x4208);

  // Mặt kính bên trong
  tft.fillRoundRect(12, 12, 216, 256, 20, ST77XX_BLACK);
  tft.drawRoundRect(12, 12, 216, 256, 20, 0x2104);

}

void drawWeatherIcon(int16_t x, int16_t y, float scale) {
  // Icon thời tiết nhỏ gọn: mặt trời + mây, không vẽ vòng tròn lớn
  int sunR = max(5, (int)roundf(7.0f * scale));
  int rayIn = sunR + max(1, (int)roundf(2.0f * scale));
  int rayOut = sunR + max(3, (int)roundf(5.0f * scale));

  int16_t sunX = x - (int16_t)roundf(7.0f * scale);
  int16_t sunY = y - (int16_t)roundf(6.0f * scale);

  // Tia nắng ngắn, không bị rối
  for (int a = 0; a < 360; a += 45) {
    float r = radians((float)a);
    int16_t x0 = sunX + (int16_t)roundf(cosf(r) * rayIn);
    int16_t y0 = sunY + (int16_t)roundf(sinf(r) * rayIn);
    int16_t x1 = sunX + (int16_t)roundf(cosf(r) * rayOut);
int16_t y1 = sunY + (int16_t)roundf(sinf(r) * rayOut);
    tft.drawLine(x0, y0, x1, y1, COLOR_SUN_CORE);
  }

  tft.fillCircle(sunX, sunY, sunR, COLOR_SUN);
  tft.fillCircle(sunX - 2, sunY - 2, max(3, sunR - 3), COLOR_SUN_CORE);

  // Cụm mây nhỏ nằm phía trước mặt trời
  int16_t cloudX = x + (int16_t)roundf(4.0f * scale);
  int16_t cloudY = y + (int16_t)roundf(4.0f * scale);

  int c1 = max(4, (int)roundf(5.0f * scale));
  int c2 = max(5, (int)roundf(7.0f * scale));
  int c3 = max(4, (int)roundf(5.0f * scale));
  int baseW = max(18, (int)roundf(27.0f * scale));
  int baseH = max(6, (int)roundf(9.0f * scale));

  tft.fillCircle(cloudX - (int)roundf(8.0f * scale), cloudY, c1, COLOR_CLOUD);
  tft.fillCircle(cloudX, cloudY - (int)roundf(3.0f * scale), c2, COLOR_CLOUD);
  tft.fillCircle(cloudX + (int)roundf(9.0f * scale), cloudY, c3, COLOR_CLOUD);
  tft.fillRoundRect(cloudX - (int)roundf(14.0f * scale), cloudY,
                    baseW, baseH, 5, COLOR_CLOUD);

  tft.drawFastHLine(cloudX - (int)roundf(11.0f * scale),
                    cloudY + baseH, max(16, (int)roundf(22.0f * scale)),
                    COLOR_CLOUD_SHA);
}




void drawHeartIcon(int x, int y, uint16_t color) {
  // Dời tâm 2 hình tròn và mép tam giác về cùng một trục ngang (y - 2)
  // để các khối hình học hòa vào nhau mượt mà, không bị lộ góc cạnh
  tft.fillCircle(x - 4, y - 2, 4, color);
  tft.fillCircle(x + 4, y - 2, 4, color);

  tft.fillTriangle(
      x - 8, y - 2, // Điểm trên cùng bên trái
      x + 8, y - 2, // Điểm trên cùng bên phải
      x, y + 8,     // Điểm nhọn dưới cùng
      color
  );
}

void drawThermoIcon(int x, int y, uint16_t color) {
  // 1. Vẽ viền ống nhiệt kế phía trên (Vẽ trước để phần dưới bị bầu tròn che đi cho tự nhiên)
  // Đỉnh ống ở y - 10, rộng 6, cao 14, bo góc 3
  tft.drawRoundRect(x - 3, y - 10, 6, 14, 3, color);

  // 2. Vẽ bầu nhiệt độ phía dưới 
  // Tâm ở y + 5, bán kính 5 (Tổng chiều ngang là 10px)
  // Đáy của bầu sẽ nằm ở (y + 5) + 5 = y + 10. Tổng chiều cao icon là 20px.
  tft.fillCircle(x, y + 5, 5, color);

  // 3. Khoét một chấm đen ở giữa bầu để tạo cảm giác bóng bẩy/chân thực
  tft.fillCircle(x, y + 5, 2, ST77XX_BLACK);

  // 4. Vẽ cột chất lỏng (thủy ngân) bên trong ống
  tft.fillRect(x - 1, y - 5, 2, 9, color);
}

void drawECGLine(int x, int y, uint16_t color) {
  tft.drawLine(x, y, x + 12, y, color);
  tft.drawLine(x + 12, y, x + 16, y - 8, color);
  tft.drawLine(x + 16, y - 8, x + 20, y + 6, color);
  tft.drawLine(x + 20, y + 6, x + 26, y, color);
  tft.drawLine(x + 26, y, x + 42, y, color);
  tft.drawLine(x + 42, y, x + 46, y - 5, color);
  tft.drawLine(x + 46, y - 5, x + 50, y + 5, color);
  tft.drawLine(x + 50, y + 5, x + 60, y, color);
}


void drawCenteredDefaultText(String text, int boxX, int boxY, int boxW, int boxH,
                            uint8_t textSize, uint16_t color) {
  tft.setFont(NULL);
  tft.setTextSize(textSize);
  tft.setTextColor(color);
int textW = text.length() * 6 * textSize;
  int textH = 8 * textSize;

  int x = boxX + (boxW - textW) / 2;
  int y = boxY + (boxH - textH) / 2;

  tft.setCursor(x, y);
  tft.print(text);
}

void drawStatusCard(int x, int y, int w, int h) {
  tft.fillRoundRect(x, y, w, h, 10, 0x0841);
  tft.drawRoundRect(x, y, w, h, 10, COLOR_DIM);
}

void drawSpO2Icon(int x, int y, uint16_t color) {
  // Giọt máu
  tft.fillCircle(x, y + 3, 6, color);
  tft.fillTriangle(
    x, y - 9,
    x - 6, y + 3,
    x + 6, y + 3,
    color
  );

  // Tạo bóng bên trong
  tft.fillCircle(x - 2, y + 1, 2, ST77XX_BLACK);
}


void drawWeatherValues() {
  tft.fillRect(86, 156, 128, 52, COLOR_CARD_BLUE);

  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(88, 158);
  tft.print(weatherCity);

  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(88, 174);

  if (weatherReady) {
    tft.print(weatherTempText);
  } else {
    tft.print("--");
  }

  tft.setTextSize(2);
  tft.print("C");

  String desc = weatherDescText;
  if (desc.length() > 8) {
    desc = desc.substring(0, 8);
  }

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(158, 178);
  tft.print(desc);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(88, 199);
  tft.print("Hum ");
  tft.print(weatherHumidityText);
  tft.print("% Wind ");
  tft.print(weatherWindText);
}


void drawHomeWatchFace() {
  if (standbyMode) return;

  updateClock();

  int displayBPM = 0;
  if (fingerDetected && validHeartRate && displayHeartRate >= 45 && displayHeartRate <= 150) {
    displayBPM = displayHeartRate;
  }

  int homeSpO2 = 0;
  if (fingerDetected && validSPO2 && displaySpO2 >= 90 && displaySpO2 <= 100) {
    homeSpO2 = displaySpO2;
  }

  // CHỈ VẼ TOÀN BỘ HOME 1 LẦN
  if (!uiDrawn) {
    drawWatchCaseFrame();

    drawStatusCard(24, 24, 52, 27);
    drawWifiIcon(50, 36, 0.75, wifiConnected);

    tft.setFont(NULL);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(wifiConnected ? 40 : 43, 43);
    if (wifiConnected) tft.print("WiFi");
    else tft.print("OFF");

    drawLinkIcon(106, 29);

    drawStatusCard(164, 24, 52, 27);
    drawBatteryIcon(184, 30, batteryPercent);

    tft.fillRoundRect(28, 58, 184, 34, 14, COLOR_PANEL);
    tft.drawRoundRect(28, 58, 184, 34, 14, COLOR_DIM);
    tft.drawRoundRect(30, 60, 180, 30, 12, 0x2104);
    drawCenteredDefaultText(currentDateText, 28, 58, 184, 34, 2, ST77XX_WHITE);

    tft.fillRoundRect(20, 150, 200, 60, 15, COLOR_CARD_BLUE);
    tft.drawRoundRect(20, 150, 200, 60, 15, ST77XX_BLUE);
    tft.drawRoundRect(22, 152, 196, 58, 13, COLOR_ACCENT);
    drawWeatherIcon(48, 176, 0.95);

    drawWeatherValues();

    tft.fillRoundRect(20, 218, 96, 44, 12, COLOR_CARD_RED);
    tft.drawRoundRect(20, 218, 96, 44, 12, ST77XX_RED);
    tft.fillCircle(38, 240, 15, ST77XX_BLACK);
    tft.drawCircle(38, 240, 15, ST77XX_RED);
    drawHeartIcon(38, 238, ST77XX_RED);

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(56, 225);
    tft.print("Nhip tim");

    tft.fillRoundRect(124, 218, 96, 44, 12, COLOR_CARD_GREEN);
    tft.drawRoundRect(124, 218, 96, 44, 12, ST77XX_GREEN);
    tft.fillCircle(142, 240, 15, ST77XX_BLACK);
    tft.drawCircle(142, 240, 15, ST77XX_GREEN);
    drawSpO2Icon(142, 240, ST77XX_GREEN);

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(160, 225);
    tft.print("SpO2");

    tft.fillCircle(114, 270, 3, ST77XX_WHITE);
    tft.fillCircle(126, 270, 3, COLOR_DIM);

    uiDrawn = true;
  }

  // Cập nhật lại ô WiFi và ngày, vì WiFi/NTP có thể đổi sau khi màn hình đã vẽ.
  tft.fillRect(35, 42, 35, 8, 0x0841);
  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(wifiConnected ? 40 : 43, 43);

  tft.fillRoundRect(30, 60, 180, 30, 12, COLOR_PANEL);
  tft.drawRoundRect(30, 60, 180, 30, 12, 0x2104);
  drawCenteredDefaultText(currentDateText, 28, 58, 184, 34, 2, ST77XX_WHITE);

  // Cập nhật nhiệt độ thời tiết thật từ Internet
  drawWeatherValues();

  // CHỈ XÓA VÙNG DỮ LIỆU THAY ĐỔI, KHÔNG fillScreen
  tft.fillRect(45, 100, 150, 45, ST77XX_BLACK);
tft.setFont(NULL);
  tft.setTextSize(5);
  int timeW = currentTimeText.length() * 6 * 5;
  int timeX = (TFT_WIDTH - timeW) / 2;
  tft.setCursor(timeX, 105);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(currentTimeText.substring(0, 3));
  tft.setTextColor(COLOR_ACCENT);
  tft.print(currentTimeText.substring(3));

  tft.fillRect(56, 238, 56, 18, COLOR_CARD_RED);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(56, 239);
  if (displayBPM > 0) tft.print(displayBPM);
  else tft.print("--");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_RED);
  tft.print(" BPM");

  tft.fillRect(160, 235, 55, 18, COLOR_CARD_GREEN);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(160, 239);
  if (homeSpO2 > 0) {
    tft.print(homeSpO2);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_GREEN);
    tft.print(" %");
  } else {
    tft.print("--");
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_GREEN);
    tft.print(" %");
  }
}



// =======================================================
// DRAW UI
// =======================================================

void drawHeader() {
  tft.setFont(NULL);
  tft.fillRect(0, 0, TFT_WIDTH, 35, ST77XX_BLUE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(25, 9);
  tft.print("SMART HEALTH");
}

void drawHealthValues() {
  tft.setFont(NULL);
  tft.setTextColor(ST77XX_WHITE);

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

  if (!maxReady) {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(35, 142);
    tft.print("MAX30102 not found: check 0x57");
  } else if (!fingerDetected) {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(55, 142);
    tft.print("Place finger on MAX30102");
  }
}

void drawAlertStatus() {
  tft.setFont(NULL);
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
  tft.setFont(NULL);
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

void drawHealthScreen() {
  if (standbyMode) return;

  if (!uiDrawn) {
    tft.fillScreen(ST77XX_BLACK);
    drawHeader();
    uiDrawn = true;
  }

  tft.fillRect(0, 40, TFT_WIDTH, 145, ST77XX_BLACK);
  tft.fillRect(GRAPH_X, GRAPH_Y, GRAPH_W + 1, TFT_HEIGHT - GRAPH_Y, ST77XX_BLACK);

  drawHealthValues();
  drawAlertStatus();
  drawGraph();
}

void drawMainScreen() {
  if (standbyMode) return;

  if (currentScreen == SCREEN_HOME) {
    drawHomeWatchFace();
  } else if (currentScreen == SCREEN_HEALTH) {
    drawHealthScreen();
  }
}

// KIỂM TRA 

// =======================================================
// SETUP SENSOR
// =======================================================

void setupMAX30102() {
  Serial.println("Khoi tao MAX30102...");

  if (!checkI2CDevice(I2C_MAX, MAX30102_ADDR)) {
    Serial.println("KHONG thay dia chi 0x57 cua MAX30102 tren SDA=10 SCL=11!");
    maxReady = false;
    showError("MAX30102 FAILED", "Check SDA10 SCL11");
    delay(1500);
    return;
  }

  Serial.println("Da thay MAX30102 tai 0x57");

  if (!particleSensor.begin(I2C_MAX, I2C_SPEED_STANDARD)) {
    Serial.println("particleSensor.begin() FAILED!");
    maxReady = false;
    showError("MAX30102 FAILED", "Library init fail");
    delay(1500);
    return;
  }

  byte ledBrightness = maxLedLevel;
  byte sampleAverage = 1;              // Quan trong: FIFO ra dung 25 mau/giay
  byte ledMode = 2;                    // RED + IR
  int sampleRate = MAX_SAMPLE_RATE_HZ; // 25Hz: 100 mau = khoang 4 giay
  int pulseWidth = 411;
  int adcRange = 8192;                 // giam nguy co bao hoa so voi 4096

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  applyLedLevel();
  particleSensor.clearFIFO();

  maxReady = true;
  Serial.print("MAX30102 SUCCESS | sampleRate=");
  Serial.print(MAX_SAMPLE_RATE_HZ);
  Serial.print("Hz | LED=0x");
  Serial.println(maxLedLevel, HEX);
}
void setupMPU6500() {
  Serial.println("Khoi tao MPU6500...");

  if (checkI2CDevice(Wire, 0x68)) {
    mpuI2CAddress = 0x68;
    Serial.println("Da thay MPU6500 tai 0x68");
  } else if (checkI2CDevice(Wire, 0x69)) {
    mpuI2CAddress = 0x69;
    Serial.println("Da thay MPU6500 tai 0x69");
  } else {
    Serial.println("KHONG thay MPU6500 tai 0x68 hoac 0x69!");
    mpuReady = false;
    return;
  }

  byte whoami = mpuRead8(MPU6500_REG_WHO_AM_I);

  Serial.print("MPU WHO_AM_I = 0x");
  if (whoami < 16) Serial.print("0");
  Serial.println(whoami, HEX);
// Module của bạn đang trả 0x70. Một số chip tương thích có thể trả ID khác,
  // nhưng với trường hợp hiện tại ta chấp nhận 0x70.
  if (whoami != 0x70) {
    Serial.println("CANH BAO: WHO_AM_I khong phai 0x70, van thu khoi tao raw I2C.");
  }

  // Wake up sensor
  mpuWrite8(MPU6500_REG_PWR_MGMT_1, 0x00);
  delay(100);

  // Low pass filter
  mpuWrite8(MPU6500_REG_CONFIG, 0x03);

  // Gyro ±500 deg/s
  mpuWrite8(MPU6500_REG_GYRO_CONFIG, 0x08);

  // Accel ±8g
  mpuWrite8(MPU6500_REG_ACCEL_CONFIG, 0x10);

  // Accel DLPF
  mpuWrite8(MPU6500_REG_ACCEL_CONFIG2, 0x03);

  mpuReady = true;
  Serial.println("MPU6500 SUCCESS");
}

// =======================================================
// SETUP
// =======================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  initButton(powerBtn, BTN_FUNCTION);
  initButton(sosBtn, BTN_SOS_PIN);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  showMessage("WiFi...", "Dang ket noi");
  connectWiFi();
  syncTimeFromNTP();
  updateClock();
  updateWeatherNow();
  lastWeatherUpdate = millis();

  // Bus MPU6500
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(100000);

  // Bus MAX30102
  I2C_MAX.begin(MAX_SDA, MAX_SCL);
  I2C_MAX.setClock(100000);

  delay(500);

  scanI2C(Wire, "MPU6500 BUS");
  scanI2C(I2C_MAX, "MAX30102 BUS");

  setupMAX30102();
  setupMPU6500(); 

  resetHealthBuffers();
}

// =======================================================
// LOOP
// =======================================================

void loop() {
  handleButtons(); 
  updateClock();
  updateWeatherIfNeeded();

  updateFallDetection();
  processHealthSensor();

  updateBuzzer();

  if (millis() - lastScreenUpdate >= SCREEN_UPDATE_INTERVAL) {
    lastScreenUpdate = millis();
    drawMainScreen();
  }

  delay(10);
}