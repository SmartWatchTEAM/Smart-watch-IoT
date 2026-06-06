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

#include <WiFi.h>
#include "time.h"

// =======================================================
// WIFI & THỜI GIAN
// =======================================================
const char* ssid = "Redmi Note 11";
const char* password = "sos1676cc";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; // Múi giờ Việt Nam (GMT+7)
const int daylightOffset_sec = 0;

// =======================================================
// PIN CONFIG - ESP32-S3 SUPER MINI
// =======================================================
#define I2C_SDA 8
#define I2C_SCL 9

#define TFT_WIDTH   240
#define TFT_HEIGHT  280

#define TFT_SCLK 12
#define TFT_MOSI 11
#define TFT_CS   10
#define TFT_DC   7
#define TFT_RST  6
#define TFT_BL   5

#define BUZZER_PIN 13

#define BTN_POWER_PIN 4
#define BTN_SOS_PIN   14

// =======================================================
// TFT & GIAO DIỆN
// =======================================================
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

enum ScreenMode {
  MODE_WATCHFACE,
  MODE_HEALTH_GRAPH
};
ScreenMode currentMode = MODE_WATCHFACE;

bool forceRedraw = true; // Cờ yêu cầu vẽ lại toàn bộ giao diện

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

enum FallState { FALL_NORMAL, FALL_FREE, FALL_IMPACT, FALL_CHECK_STILL, FALL_CONFIRMED };
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
enum AlertType { ALERT_NONE, ALERT_FALL, ALERT_SOS };
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
      if (btn.stableState == LOW) {
        btn.pressStartTime = millis();
        btn.longFired = false;
      } else {
        if (!btn.longFired) btn.shortPressEvent = true;
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
  forceRedraw = true;
}

// =======================================================
// MESSAGE & BUZZER
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

void updateBuzzer() {
  if (alertActive) {
    digitalWrite(BUZZER_PIN, (millis() / 200) % 2);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// =======================================================
// NÚT BẤM
// =======================================================
void handleButtons() {
  updateButton(powerBtn);
  updateButton(sosBtn);

  // Nút SOS: Báo động khẩn cấp
  if (sosBtn.shortPressEvent || sosBtn.longPressEvent) {
    alertActive = true;
    alertType = ALERT_SOS;
    standbyMode = false;
    digitalWrite(TFT_BL, HIGH);
    forceRedraw = true;
  }

  // Nút nguồn ngắn: Tắt cảnh báo HOẶC Đổi màn hình
  if (powerBtn.shortPressEvent) {
    if (alertActive) {
      clearAlert();
    } else {
      currentMode = (currentMode == MODE_WATCHFACE) ? MODE_HEALTH_GRAPH : MODE_WATCHFACE;
      forceRedraw = true;
    }
  }

  // Nút nguồn dài: Tắt/Bật màn hình
  if (powerBtn.longPressEvent) {
    standbyMode = !standbyMode;
    if (standbyMode) {
      tft.fillScreen(ST77XX_BLACK);
      digitalWrite(TFT_BL, LOW);
    } else {
      digitalWrite(TFT_BL, HIGH);
      forceRedraw = true;
      showMessage("WAKE UP");
      delay(500);
    }
  }
}

// =======================================================
// SENSOR PROCESSING
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

  float gyroMag = sqrt(g.gyro.x * g.gyro.x + g.gyro.y * g.gyro.y + g.gyro.z * g.gyro.z);
  unsigned long now = millis();

  switch (fallState) {
    case FALL_NORMAL:
      if (fallAccMag < FREE_FALL_THRESHOLD) { fallState = FALL_FREE; fallStateTime = now; }
      break;
    case FALL_FREE:
      if (fallAccMag > IMPACT_THRESHOLD) { fallState = FALL_IMPACT; fallStateTime = now; }
      if (now - fallStateTime > 1000) fallState = FALL_NORMAL;
      break;
    case FALL_IMPACT:
      if (fabs(fallPitch) > ANGLE_THRESHOLD || fabs(fallRoll) > ANGLE_THRESHOLD) {
        fallState = FALL_CHECK_STILL; fallStateTime = now; stillStartTime = now;
      }
      if (now - fallStateTime > 2000) fallState = FALL_NORMAL;
      break;
    case FALL_CHECK_STILL:
      if (gyroMag < GYRO_STILL_THRESHOLD && fabs(fallAccMag - 1.0) < 0.4) {
        if (now - stillStartTime > STILL_TIME) {
          alertActive = true; alertType = ALERT_FALL; fallState = FALL_CONFIRMED;
          standbyMode = false; digitalWrite(TFT_BL, HIGH); forceRedraw = true;
        }
      } else { stillStartTime = now; }
      if (now - fallStateTime > 6000 && !alertActive) fallState = FALL_NORMAL;
      break;
    case FALL_CONFIRMED:
      alertActive = true; alertType = ALERT_FALL;
      break;
  }
}

void pushGraphData(uint32_t value) {
  for (int i = 0; i < GRAPH_DATA_SIZE - 1; i++) graphData[i] = graphData[i + 1];
  graphData[GRAPH_DATA_SIZE - 1] = value;
}

void processHealthSensor() {
  particleSensor.check();
  while (particleSensor.available()) {
    uint32_t redValue = particleSensor.getRed();
    uint32_t irValue = particleSensor.getIR();
    particleSensor.nextSample();

    if (irValue < FINGER_THRESHOLD) {
      if (fingerDetected) resetHealthBuffers();
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
      maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_LENGTH, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
      bufferIndex = 0;
    }
  }
}

// =======================================================
// DRAW UI (WATCH FACE MODE)
// =======================================================
void drawWatchFace() {
  // Chỉ vẽ lại mặt đồng hồ 1 lần để tránh giật hình
  if (forceRedraw) {
    tft.fillScreen(ST77XX_BLACK);
    
    // --- CÁCH VẼ SIÊU TỐC KHÔNG LÀM TREO ESP32 ---
    tft.startWrite(); // Mở luồng ghi SPI
    tft.setAddrWindow(0, 0, 240, 240); // Khung tọa độ ảnh Casio (240x240)
    
    // Đổ toàn bộ mảng ảnh nguyên khối vào màn hình, bỏ qua vòng lặp
    tft.writePixels((const uint16_t*)dial240, 57600); 
    
    tft.endWrite(); // Đóng luồng ghi
    // ---------------------------------------------

    // Vẽ phần nền đen và vạch trắng chia cách khu vực đo sức khỏe
    tft.fillRect(0, 240, 240, 40, ST77XX_BLACK);
    tft.drawFastHLine(0, 240, 240, ST77XX_WHITE);
    forceRedraw = false;
  }

  // --- HIỂN THỊ THỜI GIAN THỰC TẾ ---
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) { // Timeout 10ms để không bị đứng mạch
    tft.fillRect(30, 95, 180, 60, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);

    char timeStr[15];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    tft.setTextSize(3);
    tft.setCursor(48, 100);
    tft.print(timeStr);

    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
    tft.setTextSize(2);
    tft.setCursor(60, 135);
    tft.print(dateStr);
  } else {
    tft.fillRect(30, 95, 180, 60, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(3);
    tft.setCursor(70, 110);
    tft.print("--:--");
  }

  // --- HIỂN THỊ TRẠNG THÁI SỨC KHỎE Ở MÉP DƯỚI ---
  tft.fillRect(0, 241, 240, 39, ST77XX_BLACK); 
  
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(10, 255);
  if (fingerDetected && validHeartRate) {
    tft.print("HR:"); tft.print(heartRate);
  } else {
    tft.print("HR:--");
  }

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(120, 255);
  if (fingerDetected && validSPO2) {
    tft.print("SpO2:"); tft.print(spo2);
  } else {
    tft.print("SpO2:--");
  }
}

// =======================================================
// DRAW UI (GRAPH MODE)
// =======================================================
int graphMapY(uint32_t value) {
  if (value < GRAPH_MIN) value = GRAPH_MIN;
  if (value > GRAPH_MAX) value = GRAPH_MAX;
  int y = GRAPH_Y + GRAPH_H - 2 - ((value - GRAPH_MIN) * (GRAPH_H - 4)) / (GRAPH_MAX - GRAPH_MIN);
  return constrain(y, GRAPH_Y + 2, GRAPH_Y + GRAPH_H - 2);
}

void drawHealthGraph() {
  if (forceRedraw) {
    tft.fillScreen(ST77XX_BLACK);
    tft.fillRect(0, 0, TFT_WIDTH, 35, ST77XX_BLUE);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(25, 9);
    tft.print("SMART HEALTH");
    
    tft.drawRoundRect(10, 45, 220, 42, 8, ST77XX_RED);
    tft.drawRoundRect(10, 95, 220, 42, 8, ST77XX_GREEN);
    forceRedraw = false;
  }

  tft.setTextColor(ST77XX_WHITE);

  // Xóa số liệu cũ
  tft.fillRect(90, 50, 100, 30, ST77XX_BLACK);
  tft.setTextSize(2); tft.setCursor(20, 58); tft.print("BPM:");
  tft.setTextSize(3); tft.setCursor(95, 53);
  if (fingerDetected && validHeartRate) tft.print(heartRate); else tft.print("--");

  tft.fillRect(90, 100, 100, 30, ST77XX_BLACK);
  tft.setTextSize(2); tft.setCursor(20, 108); tft.print("SpO2:");
  tft.setTextSize(3); tft.setCursor(105, 103);
  if (fingerDetected && validSPO2) { tft.print(spo2); tft.print("%"); } else tft.print("--");

  if (!fingerDetected) {
    tft.fillRect(20, 142, 200, 10, ST77XX_BLACK);
    tft.setTextSize(1); tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(55, 142); tft.print("Place finger on MAX30102");
  }

  // Vẽ biểu đồ
  tft.fillRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, ST77XX_BLACK);
  tft.drawRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, ST77XX_WHITE);
  for (int i = 0; i < GRAPH_DATA_SIZE - 1; i++) {
    int y1 = graphMapY(graphData[i]);
    int y2 = graphMapY(graphData[i + 1]);
    tft.drawLine(GRAPH_X + i, y1, GRAPH_X + i + 1, y2, ST77XX_CYAN);
  }
}

// =======================================================
// MAIN SCREEN ROUTER
// =======================================================
void drawMainScreen() {
  if (standbyMode) return;

  if (alertActive) {
    tft.fillScreen(ST77XX_RED);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(3);
    if (alertType == ALERT_FALL) { tft.setCursor(40, 120); tft.print("FALL ALERT!"); } 
    else { tft.setCursor(50, 120); tft.print("SOS ALERT!"); }
    forceRedraw = true;
    return;
  }

  if (currentMode == MODE_WATCHFACE) {
    drawWatchFace();
  } else {
    drawHealthGraph();
  }
}

// =======================================================
// WIFI & TIME SETUP
// =======================================================
void setupTime() {
  WiFi.begin(ssid, password);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 110);
  tft.println("Connecting WiFi");

  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 15) {
    delay(500);
    Serial.print(".");
    count++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Time synced");
    tft.setCursor(20, 140);
    tft.setTextColor(ST77XX_GREEN);
    tft.println("WiFi SUCCESS!");
  } else {
    Serial.println("WiFi failed");
    tft.setCursor(20, 140);
    tft.setTextColor(ST77XX_RED);
    tft.println("WiFi FAILED!");
  }
  delay(1000);
}

// =======================================================
// SETUP
// =======================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.init(240, 280);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  setupTime();

  currentMode = MODE_WATCHFACE;
  forceRedraw = true;
}

// =======================================================
// LOOP
// =======================================================
void loop() {
  if (millis() - lastScreenUpdate >= SCREEN_UPDATE_INTERVAL) {
    lastScreenUpdate = millis();
    drawWatchFace();
  }

  delay(10);
}