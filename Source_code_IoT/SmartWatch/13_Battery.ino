// ==============================================================
// 13_Battery.ino
// Theo doi pin LiPo 1 cell cho mach TP4056
// ==============================================================
//
// Mach hien tai cua ban:
// BAT- -> GND ESP32
// BAT+ -> chan 1 cong tac
// chan 2 cong tac -> 5V ESP32
//
// Cach noi nay cap nguon duoc, nhung ESP32 khong do truc tiep duoc dien ap pin.
// Vi vay BATTERY_USE_ADC = 0 se uoc luong % theo thoi gian dung va luu vao flash.
// Khi sac day, giu CA HAI NUT A+B trong luc bat cong tac de reset ve 100%.
//
// Neu sau nay co dien tro, co the doi BATTERY_USE_ADC = 1 va noi:
// BAT+ -> R1 220k -> GPIO1 -> R2 100k -> GND.

const unsigned long BATTERY_UPDATE_INTERVAL_MS = 2000;
const unsigned long BATTERY_PERSIST_INTERVAL_MS = 60000;

// Pin LiPo 500mAh. Dong tieu thu uoc tinh cua ESP32-S3 + TFT + WiFi + MAX30102 + MPU
// thuong khoang 180-250mA, nen thoi gian thuc te co the nam quanh 2-3 gio.
// Neu test thay sai, chi can chinh BATTERY_EST_RUNTIME_HOURS.
const float BATTERY_EST_RUNTIME_HOURS = 2.5f;
const float BATTERY_LOW_WARN_PERCENT = 15.0f;

#if BATTERY_USE_ADC
const float BAT_ADC_REF_V = 3.30f;
const float BAT_ADC_MAX = 4095.0f;
const float BAT_R1_OHM = 220000.0f;
const float BAT_R2_OHM = 100000.0f;
const float BAT_DIVIDER_RATIO = (BAT_R1_OHM + BAT_R2_OHM) / BAT_R2_OHM;
#endif


int batteryPercentFromVoltage(float v) {
  if (v >= 4.20f) return 100;
  if (v >= 4.10f) return map((int)(v * 1000), 4100, 4200, 90, 100);
  if (v >= 4.00f) return map((int)(v * 1000), 4000, 4100, 80, 90);
  if (v >= 3.92f) return map((int)(v * 1000), 3920, 4000, 70, 80);
  if (v >= 3.85f) return map((int)(v * 1000), 3850, 3920, 60, 70);
  if (v >= 3.79f) return map((int)(v * 1000), 3790, 3850, 50, 60);
  if (v >= 3.73f) return map((int)(v * 1000), 3730, 3790, 40, 50);
  if (v >= 3.68f) return map((int)(v * 1000), 3680, 3730, 30, 40);
  if (v >= 3.62f) return map((int)(v * 1000), 3620, 3680, 20, 30);
  if (v >= 3.50f) return map((int)(v * 1000), 3500, 3620, 10, 20);
  if (v >= 3.30f) return map((int)(v * 1000), 3300, 3500, 0, 10);
  return 0;
}


float estimateVoltageFromPercent(float percent) {
  percent = constrain(percent, 0.0f, 100.0f);
  if (percent >= 90.0f) return 4.10f + (percent - 90.0f) * 0.010f;
  if (percent >= 70.0f) return 3.92f + (percent - 70.0f) * 0.009f;
  if (percent >= 50.0f) return 3.79f + (percent - 50.0f) * 0.0065f;
  if (percent >= 30.0f) return 3.68f + (percent - 30.0f) * 0.0055f;
  if (percent >= 10.0f) return 3.50f + (percent - 10.0f) * 0.0060f;
  return 3.30f + percent * 0.020f;
}


#if BATTERY_USE_ADC
float readBatteryVoltage() {
  uint32_t sum = 0;
  const int samples = 16;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(BAT_ADC_PIN);
    delay(1);
  }

  float raw = (float)sum / (float)samples;
  float adcVoltage = (raw / BAT_ADC_MAX) * BAT_ADC_REF_V;
  return adcVoltage * BAT_DIVIDER_RATIO;
}
#endif


bool shouldResetEstimatedBatteryToFull() {
  pinMode(BTN_A_PIN, INPUT_PULLUP);
  pinMode(BTN_B_PIN, INPUT_PULLUP);
  delay(30);
  return digitalRead(BTN_A_PIN) == LOW && digitalRead(BTN_B_PIN) == LOW;
}


void saveEstimatedBatteryPercent() {
  batteryPrefs.putFloat("percent", batteryRuntimePercent);
  lastBatteryPersist = millis();
}


void setupBatteryMonitor() {
  batteryPrefs.begin("battery", false);

#if BATTERY_USE_ADC
  pinMode(BAT_ADC_PIN, INPUT);
  analogReadResolution(12);
#if defined(ARDUINO_ARCH_ESP32)
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
#endif

  batteryVoltage = readBatteryVoltage();
  batteryFilteredVoltage = batteryVoltage;
  batteryPercent = batteryPercentFromVoltage(batteryFilteredVoltage);
  batteryRuntimePercent = batteryPercent;
#else
  if (shouldResetEstimatedBatteryToFull()) {
    batteryRuntimePercent = 100.0f;
    saveEstimatedBatteryPercent();
  } else {
    batteryRuntimePercent = batteryPrefs.getFloat("percent", 100.0f);
  }

  batteryRuntimePercent = constrain(batteryRuntimePercent, 0.0f, 100.0f);
  batteryPercent = (int)(batteryRuntimePercent + 0.5f);
  batteryVoltage = estimateVoltageFromPercent(batteryRuntimePercent);
  batteryFilteredVoltage = batteryVoltage;
#endif

  lastBatteryUpdate = millis();
  lastBatteryPersist = millis();
}


void updateBatteryMonitor() {
  unsigned long now = millis();
  if (lastBatteryUpdate > 0 && now - lastBatteryUpdate < BATTERY_UPDATE_INTERVAL_MS) return;

  unsigned long elapsedMs = now - lastBatteryUpdate;
  lastBatteryUpdate = now;

#if BATTERY_USE_ADC
  batteryVoltage = readBatteryVoltage();

  if (batteryFilteredVoltage <= 0.1f) {
    batteryFilteredVoltage = batteryVoltage;
  } else {
    batteryFilteredVoltage = batteryFilteredVoltage * 0.90f + batteryVoltage * 0.10f;
  }

  batteryPercent = constrain(batteryPercentFromVoltage(batteryFilteredVoltage), 0, 100);
  batteryRuntimePercent = batteryPercent;
#else
  float elapsedHours = (float)elapsedMs / 3600000.0f;
  float dropPercent = (elapsedHours / BATTERY_EST_RUNTIME_HOURS) * 100.0f;
  batteryRuntimePercent -= dropPercent;
  batteryRuntimePercent = constrain(batteryRuntimePercent, 0.0f, 100.0f);

  batteryPercent = (int)(batteryRuntimePercent + 0.5f);
  batteryVoltage = estimateVoltageFromPercent(batteryRuntimePercent);
  batteryFilteredVoltage = batteryVoltage;
#endif

  if (now - lastBatteryPersist >= BATTERY_PERSIST_INTERVAL_MS) {
    saveEstimatedBatteryPercent();
  }
}
