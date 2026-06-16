#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <stdlib.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

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

// Nút bấm chức năng
// Nút A: nhấn ngắn chuyển sang màn hình kế tiếp: Home -> Health -> Steps -> Home.
// Nút B: nhấn ngắn chuyển về màn hình trước đó: Home -> Steps -> Health -> Home.
// Nhấn giữ A hoặc B >= 2 giây: kích hoạt SOS.
// Đấu nút: 1 chân vào GPIO, 1 chân vào GND vì code dùng INPUT_PULLUP.
#define BTN_A_PIN 8
#define BTN_B_PIN 9

// Mac dinh khong do dien ap pin vi mach hien tai chi cap nguon:
// BAT- -> GND, BAT+ -> cong tac -> 5V ESP32.
// Khong co day ADC/cau chia ap thi chi uoc luong % theo thoi gian dung.
#define BATTERY_USE_ADC 0
#define BAT_ADC_PIN 1

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
  SCREEN_HEALTH,
  SCREEN_STEPS
};

ScreenPage currentScreen = SCREEN_HOME;

// Dữ liệu giao diện chính. Giờ/ngày sẽ lấy từ NTP sau khi kết nối WiFi.
String currentTimeText = "--:--";
String currentDateText = "--, --/--";
String weatherCity = "TP.HCM";
String weatherText = "--C";
String spo2HomeText = "--";
int batteryPercent = 0;
float batteryVoltage = 0.0f;
float batteryFilteredVoltage = 0.0f;
unsigned long lastBatteryUpdate = 0;
unsigned long lastBatteryPersist = 0;
float batteryRuntimePercent = 100.0f;
Preferences batteryPrefs;
int quickBPM = 0;
bool wifiConnected = false;

unsigned long lastClockDemoUpdate = 0;
int demoHour = 14;
int demoMinute = 35;

// =======================================================
// MAX30102 / MAX30105 - THUAT TOAN DO CO TAY
// =======================================================

MAX30105 particleSensor;

// Che do do CO TAY: giu 500 mau @25Hz = khoang 20 giay.
// Co tay co song PPG yeu hon dau ngon tay, nen can cua so du dai + loc chuyen dong.
// Sau khi du 500 mau, moi lan them 200 mau moi se tinh lai tren cua so truot 20 giay.
// Nghia la giu 300 mau cu + nap 200 mau moi, cap nhat nhanh hon so voi do lai tu dau.
const int MAX_SAMPLE_RATE_HZ = 25;
const int SAMPLE_PERIOD_MS = 1000 / MAX_SAMPLE_RATE_HZ;              // 40ms/mau @25Hz
const int HR_MEASURE_SECONDS = 20;
const int BUFFER_LENGTH = 500;                                      // 500 mau = 20 giay @25Hz
const int NEW_SAMPLES = 200;                                      // cua so truot: giu 300 mau cu + nap 200 mau moi
const int SPO2_WINDOW_LENGTH = 100;                                 // thu vien Maxim chay an toan voi 100 mau
const int SPO2_WINDOW_COUNT = BUFFER_LENGTH / SPO2_WINDOW_LENGTH;    // 5 cua so SpO2 trong 20s
const int MAX_DETECTED_POINTS = 120;                                // du cho HR toi 160 BPM trong 20s

uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];
uint32_t sampleTimeBuffer[BUFFER_LENGTH];  // thoi diem tung mau, dung de tinh BPM dung khi toc do doc khong dung 25Hz

// Buffer phu dat global de tranh tran stack va tiet kiem RAM stack.
uint32_t irSortedBuffer[BUFFER_LENGTH];
uint32_t redSortedBuffer[BUFFER_LENGTH];
int32_t irACBuffer[BUFFER_LENGTH];
int32_t acAbsSortedBuffer[BUFFER_LENGTH];

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

// Che do co tay: can tiep xuc ap sat hon dau ngon tay.
// Log cua ban: khong tiep xuc co luc IR ~16k-30k, tiep xuc tot ~82k-90k.
const uint32_t NO_FINGER_THRESHOLD = 35000;
const uint32_t FINGER_THRESHOLD    = 55000;
const uint32_t TARGET_IR_LOW       = 80000;   // vung muc tieu cho co tay
const uint32_t TARGET_IR_HIGH      = 145000;  // tranh bao hoa va tranh tang LED qua cao
const uint32_t SATURATION_LEVEL    = 245000;  // gan tran 262143 la bao hoa ADC

// LED co tay can manh hon dau ngon tay mot chut, nhung se bi khoa khi dang nap buffer.
byte maxLedLevel = 0x3C;
unsigned long lastLedAdjust = 0;
unsigned long lastHealthPrint = 0;
unsigned long ignoreSamplesUntil = 0;
unsigned long lastBufferProgressPrint = 0;

// Watchdog MAX30102: tự wake/reset khi mất LED, mất FIFO hoặc mất I2C.
unsigned long lastMaxSampleTime = 0;
unsigned long lastMaxWatchdogCheck = 0;
unsigned long lastMaxResetAttempt = 0;
uint8_t maxI2CFailCount = 0;
uint16_t maxZeroSampleCount = 0;

const unsigned long MAX_WATCHDOG_INTERVAL = 1000;  // kiểm tra MAX mỗi 1 giây
const unsigned long MAX_NO_SAMPLE_TIMEOUT = 15000; // tránh reset nhầm khi WiFi/Firebase làm loop bị chậm
const unsigned long MAX_RESET_COOLDOWN    = 10000; // tránh reset liên tục

const unsigned long FINGER_STABILIZE_MS = 3000; // co tay can 2-3 giay de ap luc/anh sang on dinh
const unsigned long LED_SETTLE_MS       = 1200; // bo qua mau sau khi doi LED lau hon vi co tay de bi troi DC

const byte HR_SMOOTH_SIZE = 9;
int hrHistory[HR_SMOOTH_SIZE];
byte hrHistoryCount = 0;
byte hrHistoryIndex = 0;
// Khong ep HR ve 70-100. Khoang nay de loc gia tri phi ly, van chap nhan HR cao/thap that neu song PPG du tot.
const int HR_ALGO_MIN_BPM = 40;
const int HR_ALGO_MAX_BPM = 135;

// Vung hien thi on dinh cho nguoi binh thuong luc nghi.
// Code KHONG ep gia tri gia; no uu tien/giu on dinh trong vung nay khi tin hieu PPG du tot.
// Neu BPM cao that va lap lai nhieu cua so voi chat luong cao, code van cho hien thi de canh bao.
const int HR_STABLE_LOW_BPM   = 71;
const int HR_STABLE_HIGH_BPM  = 90;
const int HR_HIGH_CONFIRM_BPM = 100;
const int HR_DANGER_BPM       = 120;
const int HR_ALERT_HIGH_BPM   = 110;
const int HR_ALERT_LOW_BPM    = 50;

const int SPO2_STABLE_LOW     = 95;
const int SPO2_STABLE_HIGH    = 100;
const int SPO2_LOW_CONFIRM    = 94;

const int HR_START_LOCK_COUNT = 1;
int hrPendingValue = 0;
byte hrPendingCount = 0;
int lastHRAuto = 0;
int lastHRExtrema = 0;
int lastHRCandidate = 0;
int lastHRWindowMedian = 0;
int lastHRWindowValidCount = 0;
float lastHRActualFs = 0;
uint32_t lastHRDurationMs = 0;
int lastHRAcRange = 0;
int lastHRThreshold = 0;
int lastHRPeakCount = 0;
int lastHRValleyCount = 0;
float lastHRAutoScore = 0;
int lastHRSpectrum = 0;
float lastHRSpectrumScore = 0;
float lastHRSpectrumSeparation = 0;
unsigned long lastLiveSpO2EstimateMs = 0;
int lastLiveSpO2Estimate = 0;
int lastLiveSpO2Quality = 0;
int lastRawRatioX100 = 0;
int lastAdjustedRatioX100 = 0;
int lastHRQuality = 0;
int lastHRRejectReason = 0;
float lastWristMotionScore = 0;
uint8_t wristBadSampleCount = 0;
uint16_t wristDroppedSamples = 0;
uint32_t lastGoodIRValue = 0;
uint32_t lastGoodREDValue = 0;

// Chong reset buffer qua nhay khi do co tay.
// 60 mau ~= 2.4 giay @25Hz, tranh reset buffer qua som khi day deo/co tay dao dong nhe.
const uint8_t WRIST_BAD_SAMPLE_RESET_COUNT = 60;

// Khi mat tiep xuc ngan/doi LED, chi giu ket qua cu vai giay roi an khoi man hinh/web.
unsigned long lastValidHeartRateMs = 0;
unsigned long lastValidSpO2Ms = 0;
unsigned long lastFingerLostMs = 0;
const unsigned long KEEP_LAST_HEALTH_VALUE_MS = 5000;

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
unsigned long fallFreeStartTime = 0;
bool fallTiltSeen = false;

const float FREE_FALL_THRESHOLD = 0.65;
const float IMPACT_THRESHOLD = 2.05;
const float HARD_IMPACT_THRESHOLD = 2.75;
const float ANGLE_THRESHOLD = 50.0;
const float GYRO_STILL_THRESHOLD = 0.85;
const unsigned long FREE_FALL_MIN_TIME = 60;
const unsigned long FREE_FALL_MAX_TIME = 900;
const unsigned long IMPACT_TO_TILT_TIMEOUT = 1800;
const unsigned long STILL_TIME = 1800;

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
// STEP TRACKER / PEDOMETER - SMARTWATCH QUALITY
// =======================================================
// Lưu ý: đặt module MPU chắc vào cổ tay. Nếu cầm lắc trên tay, mọi thuật toán đều dễ sai.
int stepCount = 0;
int stepGoal = 10000;

// Hieu chinh theo nguoi deo de distance/kcal sat thuc te hon.
// Neu chua co ho so nguoi dung, 165 cm / 60 kg la moc trung binh de demo.
const float USER_HEIGHT_CM = 165.0f;
const float USER_WEIGHT_KG = 60.0f;
const float STEP_LENGTH_M = (USER_HEIGHT_CM * 0.415f) / 100.0f;
const float WALKING_KCAL_PER_KG_KM = 0.75f;  // kcal ~= can nang(kg) * km * 0.75

// Bộ lọc gia tốc cho thuật toán đếm bước
float stepGravity = 1.0f;             // nền gia tốc quanh 1g
float stepSignalFiltered = 0.0f;      // tín hiệu bước sau khi lọc cao/thấp
float lastStepSignal = 0.0f;
float stepPeakValue = 0.0f;
float stepValleyValue = 0.0f;
float stepNoiseAvg = 0.04f;
float stepDynamicThreshold = 0.12f;
float stepAmplitudeAvg = 0.18f;
float stepGyroBaseline = 0.0f;
float stepWristSignal = 0.0f;
bool stepAboveThreshold = false;

unsigned long lastStepTime = 0;             // lần cuối đã cộng bước thật
unsigned long lastStepCandidateTime = 0;    // lần cuối thấy đỉnh bước ứng viên
unsigned long stepPeakStartTime = 0;
unsigned long lastStepUpdateTime = 0;
unsigned long lastStepSampleTime = 0;
unsigned long activeWalkMs = 0;

byte stepWalkConfirmCount = 0;        // yêu cầu ít nhất 2 nhịp liên tiếp mới xác nhận đang đi
int stepCadenceSPM = 0;               // steps per minute để hiển thị/troubleshoot

unsigned long sedentaryStartTime = 0;
bool sedentaryWarning = false;

int hourlySteps[24];
int lastStepDayOfYear = -1;

// Thông số chống đếm nhầm. Có thể hiệu chỉnh sau khi test 100 bước thật.
const unsigned long STEP_SAMPLE_INTERVAL_MS = 20;    // 50Hz xử lý thuật toán
const unsigned long STEP_MIN_INTERVAL = 480;         // tranh 1 buoc bi dem thanh 3-4 buoc
const unsigned long STEP_MAX_INTERVAL = 1700;        // buoc cham vua phai
const unsigned long STEP_ACTIVE_WINDOW = 2500;
const unsigned long STEP_BURST_TIMEOUT = 2200;
const unsigned long STEP_PEAK_MIN_WIDTH_MS = 25;
const unsigned long STEP_PEAK_MAX_WIDTH_MS = 900;

const float STEP_THRESHOLD_MIN = 0.085f;   // can vung tay ro hon, bot dem lap
const float STEP_THRESHOLD_MAX = 0.32f;
const float STEP_MIN_AMPLITUDE = 0.130f;
const float STEP_MAX_SIGNAL = 1.60f;

// Cảnh báo ngồi lâu. Để test nhanh có thể đổi 60 phút thành 1-2 phút.
const unsigned long SEDENTARY_LIMIT_MS = 60UL * 60UL * 1000UL;

// =======================================================
// GRAPH
// =======================================================

const int GRAPH_X = 10;
const int GRAPH_Y = 168;
const int GRAPH_W = 220;
const int GRAPH_H = 90;

const int GRAPH_DATA_SIZE = GRAPH_W;

uint32_t graphData[GRAPH_DATA_SIZE];

// Lưu 60 giá trị BPM gần nhất để hiện MAX/MIN giống giao diện smartwatch.
const int HR_TREND_SIZE = 60;
int hrTrendData[HR_TREND_SIZE];
byte hrTrendCount = 0;
int hrTrendMax = 0;
int hrTrendMin = 0;

const uint32_t GRAPH_MIN = 20000;
const uint32_t GRAPH_MAX = 120000;

unsigned long lastScreenUpdate = 0;
const unsigned long SCREEN_UPDATE_INTERVAL = 500;

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

Button buttonA;
Button buttonB;

const unsigned long DEBOUNCE_DELAY = 40;
const unsigned long LONG_PRESS_TIME = 2000;   // nhấn giữ 2s để SOS

// =======================================================
// FUNCTION PROTOTYPES - giup Arduino IDE khong loi thu tu tab
// =======================================================
bool checkI2CDevice(TwoWire &bus, byte address);
void scanI2C(TwoWire &bus, const char *busName);
void initButton(Button &btn, int pin);
void updateButton(Button &btn);
void triggerSOSFromButton(const char *buttonName);
void wakeScreenForButtonAction();
void goToNextScreen();
void goToPreviousScreen();
void resetHealthBuffers();
void resetHealthAcquisitionOnly();
void clearAlert();
void showMessage(String line1, String line2);
void showMessage(String line1);
void showError(String title, String line2);
void handleButtons();
void updateBuzzer();
void setupBatteryMonitor();
void updateBatteryMonitor();
bool mpuWrite8(byte reg, byte value);
bool mpuReadBytes(byte reg, byte count, byte *data);
byte mpuRead8(byte reg);
int16_t combine16(byte highByte, byte lowByte);
bool readMPU6500();
void updateFallDetection();
String formatNumberWithComma(long value);
float getStepDistanceKm();
int getStepCalories();
int getStepGoalPercent();
int getCurrentHourForSteps();
void resetStepDataForNewDay();
void resetStepsDailyIfNeeded();
void addOneStep();
void registerStepCandidate(unsigned long now);
void updateStepTracker();
void pushGraphData(uint32_t value);
void pushHeartRateTrend(int bpm);
int medianSmall(int *values, byte count);
bool isHealthResultReady();
bool isHeartRateAlertActive();
String heartRateAlertLevel();
void clearExpiredHealthDisplay();
void addHRValue(int bpm);
void addSpO2Value(int value);
void sortU32Small(uint32_t *arr, int n);
SignalStats calculateSignalStats();
bool isSignalGood(const SignalStats &s);
void applyLedLevel();
void autoTuneLed(uint32_t irValue, uint32_t redValue);
int calculateBPMFromExtrema(int32_t *ac, int threshold, bool detectPeak);
int calculateBPMByAutocorrelation(int32_t *ac);
int calculateBPMBySpectrum(int32_t *ac);
int estimateLiveSpO2FromPartialBuffer();
void updateLiveSpO2Preview();
int calculateHeartRateFromIRBuffer();
void processCompletedHealthWindow();
bool wakeMAX30102();
bool resetMAX30102(const char *reason);
void updateMAX30102Watchdog();
void processHealthSensor();
void connectWiFi();
void syncTimeFromNTP();
void updateClock();
String weatherCodeToText(int code);
String getJsonObject(const String &payload, const char *objectName);
float getJsonFloatValue(const String &json, const char *key, float fallback);
void setWeatherOffline(String reason);
void updateWeatherNow();
void updateWeatherIfNeeded();
String firebaseEscape(String value);
String buildFirebasePayload();
bool sendTelemetryToFirebase();
void markFirebaseHealthWindowReady();
void markFirebaseHealthCleared();
void forceFirebaseSendOnNextChange();
void updateFirebaseIfNeeded();
void drawBatteryIcon(int x, int y, int percent);
void drawArcSegments(int16_t x, int16_t y, int16_t radius, int startDeg, int endDeg, uint16_t color);
void drawWifiIcon(int16_t x, int16_t y, float scale, bool connected);
void drawLinkIcon(int x, int y);
void drawWatchCaseFrame();
void drawWeatherIcon(int16_t x, int16_t y, float scale);
void drawHeartIcon(int x, int y, uint16_t color);
void drawThermoIcon(int x, int y, uint16_t color);
void drawECGLine(int x, int y, uint16_t color);
void drawCenteredDefaultText(String text, int boxX, int boxY, int boxW, int boxH, uint8_t textSize, uint16_t color);
void drawStatusCard(int x, int y, int w, int h);
void drawSpO2Icon(int x, int y, uint16_t color);
void drawAlertScreen();
void drawWeatherValues();
void drawHomeWatchFace();
void drawHealthSoftGlowCard(int x, int y, int w, int h, uint16_t bg, uint16_t border);
void drawTinyWifiEqualBox(int x, int y, bool connected);
void drawTinyBatteryEqualBox(int x, int y, int percent);
void drawHealthTopBar();
void drawHealthStatusPill(int x, int y, const char *text, uint16_t color);
void drawHeartCircleIcon(int cx, int cy);
void drawSpO2CircleIcon(int cx, int cy);
void drawHealthGraphGrid();
void drawHealthGraphAxes(bool drawLabels);
void drawStaticHealthScreen();
void drawHealthDynamicValues();
int healthGraphMapYDynamic(uint32_t value, uint32_t minV, uint32_t maxV, int gy, int gh);
void drawPremiumHealthGraph();
void drawHealthScreen();
void drawThickArc(int16_t cx, int16_t cy, int16_t radius, int startDeg, int endDeg, int thick, uint16_t color);
void drawStepFootIcon(int cx, int cy, uint16_t color);
void drawLocationIcon(int cx, int cy, uint16_t color);
void drawFlameIcon(int cx, int cy, uint16_t color);
void drawClockIconSmall(int cx, int cy, uint16_t color);
void drawTargetIcon(int cx, int cy, uint16_t color);
void drawSittingIcon(int cx, int cy, uint16_t color);
void drawWalkingIcon(int cx, int cy, uint16_t color);
void drawStepMetricCard(int x, int y, int w, int h, const char *label, uint16_t accent, byte iconType);
void drawStepProgressRing();
void drawStaticStepScreen();
void drawStepMainValues();
void drawMetricValueText(int x, int y, int w, String value, String unit, uint16_t color);
void drawStepMetricValues();
void drawStepScreen();
void drawMainScreen();
void setupMAX30102();
void setupMPU6500();

// ==============================================================
// SETUP
// file chính: include, biến global, setup(), loop()
// ==============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  setupBatteryMonitor();

  initButton(buttonA, BTN_A_PIN);
  initButton(buttonB, BTN_B_PIN);

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

  resetHealthBuffers();
}


void loop() {
  handleButtons(); 
  updateBatteryMonitor();
  updateClock();
  updateWeatherIfNeeded();

  updateFallDetection();
  updateStepTracker();
  resetStepsDailyIfNeeded();

  // Doc MAX30102 truoc, sau do watchdog moi kiem tra.
  // Neu WiFi/Firebase lam loop cham, FIFO co mau se duoc doc truoc khi watchdog quyet dinh reset.
  processHealthSensor();
  updateMAX30102Watchdog();

  // Gửi dữ liệu mới nhất lên Firebase Realtime Database
  updateFirebaseIfNeeded();

  updateBuzzer();

  if (millis() - lastScreenUpdate >= SCREEN_UPDATE_INTERVAL) {
    lastScreenUpdate = millis();
    drawMainScreen();
  }

  delay(10);
}
