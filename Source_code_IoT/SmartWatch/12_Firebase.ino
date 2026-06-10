// ==============================================================
// 12_Firebase.ino
// Gui du lieu Smart Health Watch len Firebase Realtime Database
// Duong dan ghi moi nhat: /devices/watch_001/latest.json
// Duong dan luu lich su: /devices/watch_001/history/YYYY-MM-DD/<auto-id>.json
// Ban nay chi gui HR/SpO2 len Firebase sau khi cua so MAX30102 da do xong va BPM hop le.
// Rieng buoc chan duoc cap nhat rieng bang PATCH, khong ghi de bpm/spo2 thanh null tren web.
// ============================================================== 

const char* FIREBASE_HOST = "smart-health-watch-2446b-default-rtdb.asia-southeast1.firebasedatabase.app";
const char* FIREBASE_PATH = "/devices/watch_001/latest.json";
const char* FIREBASE_HISTORY_BASE_PATH = "/devices/watch_001/history";
const char* FIREBASE_DAILY_BASE_PATH = "/devices/watch_001/daily";

unsigned long lastFirebaseSend = 0;
unsigned long lastFirebaseHistorySend = 0;
unsigned long lastFirebaseDailySend = 0;

const unsigned long FIREBASE_MIN_SEND_INTERVAL         = 7000;     // chi gui sau moi cua so BPM hop le, toi thieu 7s/lan
const unsigned long FIREBASE_RETRY_INTERVAL            = 30000;    // neu loi HTTP, doi 30s moi thu lai de tranh lag
const unsigned long FIREBASE_HISTORY_INTERVAL          = 30000;    // luu history BPM moi 30s de web thong ke
const unsigned long FIREBASE_DAILY_INTERVAL            = 30000;    // cap nhat tong ket ngay sau khi co BPM hop le
const unsigned long FIREBASE_FRESH_BPM_MAX_AGE         = 2500;     // BPM phai vua duoc tinh trong cua so 500 mau
const unsigned long FIREBASE_STEP_UPLOAD_INTERVAL       = 7000;     // cap nhat buoc chan toi da moi 7s de web thay doi realtime
const int FIREBASE_MIN_STEP_DELTA                       = 3;        // di them it nhat 3 buoc moi day step len web

String lastFirebaseDataKey = "";
String lastFirebaseUrgentKey = "";
bool firebaseHealthWindowReady = false;
bool firebaseForceNext = true;
unsigned long lastFirebaseFailTime = 0;

// Dung de tranh POST history toan null/lap lai khi chua co mau do huu ich.
int lastFirebaseHistoryStepCount = -1;
int lastFirebaseUploadedStepCount = -1;
int lastFirebaseUploadedCadenceSPM = -1;
bool lastFirebaseUploadedSedentaryWarning = false;
unsigned long lastFirebaseStepSend = 0;


String firebaseEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", " ");
  value.replace("\r", " ");
  return value;
}


String getFirebaseDateKey() {
  time_t nowSec = time(nullptr);

  if (nowSec > 100000) {
    struct tm timeinfo;
    localtime_r(&nowSec, &timeinfo);

    char buf[12];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday);

    return String(buf);
  }

  return "unknown-date";
}


String getFirebaseTimeKey() {
  time_t nowSec = time(nullptr);

  if (nowSec > 100000) {
    struct tm timeinfo;
    localtime_r(&nowSec, &timeinfo);

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);

    return String(buf);
  }

  return String(millis());
}


String getFirebaseTimestampMs() {
  time_t nowSec = time(nullptr);

  if (nowSec > 100000) {
    // Tao timestamp 13 chu so dang milliseconds de JavaScript ve thong ke dung thoi gian.
    return String((long)nowSec) + "000";
  }

  return String(millis());
}



bool hasValidFirebaseBPM() {
  return fingerDetected && validHeartRate && displayHeartRate >= 40 && displayHeartRate <= 180;
}


bool hasFreshFirebaseBPM() {
  // Chi coi la "do xong BPM" neu gia tri BPM vua duoc khoa trong cua so MAX30102 moi.
  // Dieu nay ngan Firebase gui lai BPM cu trong luc dang nap buffer hoac khi vua mat tiep xuc.
  return hasValidFirebaseBPM() &&
         lastValidHeartRateMs > 0 &&
         (millis() - lastValidHeartRateMs <= FIREBASE_FRESH_BPM_MAX_AGE);
}


bool hasValidFirebaseSpO2() {
  return fingerDetected && validSPO2 && displaySpO2 >= 80 && displaySpO2 <= 100;
}


// Chi luu history khi da co cua so BPM hop le hoac co canh bao.
// Khong luu cac payload null trong luc dang nap buffer.
bool shouldSaveFirebaseHistory() {
  if (hasValidFirebaseBPM() || hasValidFirebaseSpO2()) return true;
  if (stepCount != lastFirebaseHistoryStepCount) return true;
  if (alertActive && (alertType == ALERT_FALL || alertType == ALERT_SOS)) return true;
  if (sedentaryWarning) return true;
  return false;
}


String firebaseValueBPMKey() {
  if (hasValidFirebaseBPM()) {
    return String(displayHeartRate);
  }
  return "null";
}


String firebaseValueSpO2Key() {
  if (hasValidFirebaseSpO2()) {
    return String(displaySpO2);
  }
  return "null";
}


String buildFirebaseDataKey() {
  // Key nay dung de so sanh co du lieu moi hay khong.
  // Co tinh bo qua acc/pitch/roll/rssi/uptime/updatedAt vi cac gia tri nay dao dong lien tuc,
  // neu dua vao key thi Firebase van gui lien tuc va lam lag cam bien.
  String key = "";

  key += "bpm="; key += firebaseValueBPMKey();
  key += "|spo2="; key += firebaseValueSpO2Key();
  key += "|finger="; key += fingerDetected ? "1" : "0";
  key += "|signal="; key += signalOK ? "1" : "0";
  key += "|fall="; key += (alertActive && alertType == ALERT_FALL) ? "1" : "0";
  key += "|sos="; key += (alertActive && alertType == ALERT_SOS) ? "1" : "0";
  key += "|wifi="; key += (WiFi.status() == WL_CONNECTED) ? "1" : "0";
  key += "|time="; key += currentTimeText;
  key += "|date="; key += currentDateText;
  key += "|battery="; key += String(batteryPercent);
  key += "|steps="; key += String(stepCount);
  key += "|cadence="; key += String(stepCadenceSPM);
  key += "|sedentary="; key += sedentaryWarning ? "1" : "0";
  key += "|weather=";
  if (weatherReady) {
    key += weatherTempText; key += ",";
    key += weatherHumidityText; key += ",";
    key += weatherWindText; key += ",";
    key += weatherDescText;
  } else {
    key += "null";
  }

  return key;
}


String buildFirebaseUrgentKey() {
  // Cac trang thai nay can gui nhanh, ke ca khi dang do HR.
  // Khong dua finger=true vao urgent de tranh gui payload null ngay luc moi dat tay.
  String key = "";
  key += "fall="; key += (alertActive && alertType == ALERT_FALL) ? "1" : "0";
  key += "|sos="; key += (alertActive && alertType == ALERT_SOS) ? "1" : "0";
  key += "|wifi="; key += (WiFi.status() == WL_CONNECTED) ? "1" : "0";
  return key;
}


void markFirebaseHealthWindowReady() {
  // Goi khi da tinh xong 1 cua so MAX30102 500 mau.
  // Nhờ vậy Firebase chỉ gửi sau khi có kết quả mới, không gửi payload null trong lúc đang nap buffer.
  firebaseHealthWindowReady = true;
}


void forceFirebaseSendOnNextChange() {
  firebaseForceNext = true;
}


String buildFirebasePayload() {
  String payload = "{";

  payload += "\"bpm\":";
  if (hasValidFirebaseBPM()) {
    payload += String(displayHeartRate);
  } else {
    payload += "null";
  }

  payload += ",\"spo2\":";
  if (hasValidFirebaseSpO2()) {
    payload += String(displaySpO2);
  } else {
    payload += "null";
  }

  payload += ",\"finger\":";
  payload += fingerDetected ? "true" : "false";

  payload += ",\"signalOK\":";
  payload += signalOK ? "true" : "false";

  payload += ",\"acc\":";
  payload += String(fallAccMag, 2);

  payload += ",\"pitch\":";
  payload += String(fallPitch, 1);

  payload += ",\"roll\":";
  payload += String(fallRoll, 1);

  payload += ",\"fall\":";
  payload += (alertActive && alertType == ALERT_FALL) ? "true" : "false";

  payload += ",\"sos\":";
  payload += (alertActive && alertType == ALERT_SOS) ? "true" : "false";

  payload += ",\"wifi\":";
  payload += (WiFi.status() == WL_CONNECTED) ? "true" : "false";

  payload += ",\"rssi\":";
  if (WiFi.status() == WL_CONNECTED) payload += String(WiFi.RSSI());
  else payload += "null";

  payload += ",\"timeSynced\":";
  payload += timeSynced ? "true" : "false";

  payload += ",\"timeText\":\"";
  payload += firebaseEscape(currentTimeText);
  payload += "\"";

  payload += ",\"dateText\":\"";
  payload += firebaseEscape(currentDateText);
  payload += "\"";

  payload += ",\"dateKey\":\"";
  payload += getFirebaseDateKey();
  payload += "\"";

  payload += ",\"timeKey\":\"";
  payload += firebaseEscape(getFirebaseTimeKey());
  payload += "\"";

  payload += ",\"battery\":";
  payload += String(batteryPercent);

  payload += ",\"steps\":";
  payload += String(stepCount);

  payload += ",\"stepGoal\":";
  payload += String(stepGoal);

  payload += ",\"stepGoalPercent\":";
  payload += String(getStepGoalPercent());

  payload += ",\"distanceKm\":";
  payload += String(getStepDistanceKm(), 2);

  payload += ",\"calories\":";
  payload += String(getStepCalories());

  payload += ",\"cadenceSPM\":";
  payload += String(stepCadenceSPM);

  payload += ",\"sedentaryWarning\":";
  payload += sedentaryWarning ? "true" : "false";

  payload += ",\"weatherTemp\":";
  if (weatherReady) payload += weatherTempText;
  else payload += "null";

  payload += ",\"weatherHumidity\":";
  if (weatherReady) payload += weatherHumidityText;
  else payload += "null";

  payload += ",\"weatherWind\":";
  if (weatherReady) payload += weatherWindText;
  else payload += "null";

  payload += ",\"weatherDesc\":\"";
  payload += firebaseEscape(weatherDescText);
  payload += "\"";

  payload += ",\"updatedAt\":";
  time_t nowSec = time(nullptr);
  if (nowSec > 100000) payload += String((long)nowSec);
  else payload += "null";

  payload += ",\"createdAt\":";
  payload += getFirebaseTimestampMs();

  payload += ",\"uptimeMs\":";
  payload += String(millis());

  payload += "}";
  return payload;
}


String buildFirebaseStepPatchPayload() {
  // Payload PATCH chi cap nhat truong lien quan buoc chan.
  // Khong gui bpm/spo2/finger/signalOK de tranh lam web bi mat BPM khi dang do HR.
  String payload = "{";

  payload += "\"steps\":";
  payload += String(stepCount);

  payload += ",\"stepGoal\":";
  payload += String(stepGoal);

  payload += ",\"stepGoalPercent\":";
  payload += String(getStepGoalPercent());

  payload += ",\"distanceKm\":";
  payload += String(getStepDistanceKm(), 2);

  payload += ",\"calories\":";
  payload += String(getStepCalories());

  payload += ",\"cadenceSPM\":";
  payload += String(stepCadenceSPM);

  payload += ",\"sedentaryWarning\":";
  payload += sedentaryWarning ? "true" : "false";

  payload += ",\"battery\":";
  payload += String(batteryPercent);

  payload += ",\"wifi\":";
  payload += (WiFi.status() == WL_CONNECTED) ? "true" : "false";

  payload += ",\"rssi\":";
  if (WiFi.status() == WL_CONNECTED) payload += String(WiFi.RSSI());
  else payload += "null";

  payload += ",\"updatedAt\":";
  time_t nowSec = time(nullptr);
  if (nowSec > 100000) payload += String((long)nowSec);
  else payload += "null";

  payload += ",\"createdAt\":";
  payload += getFirebaseTimestampMs();

  payload += ",\"uptimeMs\":";
  payload += String(millis());

  payload += "}";
  return payload;
}


bool sendTelemetryToFirebase() {
  wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (!wifiConnected) {
    Serial.println("Firebase skipped: WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(3500);      // TLS Firebase can thoi gian hon 1.2s tren ESP32
  http.setReuse(false);

  String url = "https://" + String(FIREBASE_HOST) + String(FIREBASE_PATH);
  String payload = buildFirebasePayload();

  Serial.print("Firebase payload NEW: ");
  Serial.println(payload);

  if (!http.begin(client, url)) {
    Serial.println("Firebase HTTP begin failed");
    client.stop();
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");

  int httpCode = http.PUT(payload);

  Serial.print("Firebase HTTP code: ");
  Serial.println(httpCode);

  bool ok = (httpCode >= 200 && httpCode < 300);
  if (ok) {
    Serial.println("Firebase telemetry OK");
  } else {
    Serial.println("Firebase telemetry FAILED");
    String response = http.getString();
    if (response.length() > 0) Serial.println(response);
  }

  http.end();
  client.stop();

  return ok;
}


bool firebaseSendJson(String url, String payload, const char *methodName, const char *logLabel) {
  wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (!wifiConnected) {
    Serial.print(logLabel);
    Serial.println(" skipped: WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(3500);      // TLS Firebase can thoi gian hon 1.2s tren ESP32
  http.setReuse(false);

  if (!http.begin(client, url)) {
    Serial.print(logLabel);
    Serial.println(" HTTP begin failed");
    client.stop();
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");

  int httpCode;
  String method = String(methodName);
  if (method == "POST") {
    httpCode = http.POST(payload);
  } else if (method == "PATCH") {
    httpCode = http.sendRequest("PATCH", payload);
  } else {
    httpCode = http.PUT(payload);
  }

  Serial.print(logLabel);
  Serial.print(" HTTP code: ");
  Serial.println(httpCode);

  bool ok = (httpCode >= 200 && httpCode < 300);
  if (ok) {
    Serial.print(logLabel);
    Serial.println(" OK");
  } else {
    Serial.print(logLabel);
    Serial.println(" FAILED");
    String response = http.getString();
    if (response.length() > 0) Serial.println(response);
  }

  http.end();
  client.stop();

  return ok;
}


bool sendHistoryToFirebase() {
  String dateKey = getFirebaseDateKey();

  String url = "https://" + String(FIREBASE_HOST) +
               String(FIREBASE_HISTORY_BASE_PATH) + "/" + dateKey + ".json";

  String payload = buildFirebasePayload();

  return firebaseSendJson(url, payload, "POST", "Firebase history");
}


bool sendStepPatchToFirebase() {
  String url = "https://" + String(FIREBASE_HOST) + String(FIREBASE_PATH);
  String payload = buildFirebaseStepPatchPayload();

  Serial.print("Firebase step PATCH: ");
  Serial.println(payload);

  return firebaseSendJson(url, payload, "PATCH", "Firebase steps");
}


bool sendDailySummaryToFirebase() {
  String dateKey = getFirebaseDateKey();

  String url = "https://" + String(FIREBASE_HOST) +
               String(FIREBASE_DAILY_BASE_PATH) + "/" + dateKey + "/summary.json";

  String payload = "{";

  payload += "\"steps\":";
  payload += String(stepCount);

  payload += ",\"stepGoal\":";
  payload += String(stepGoal);

  payload += ",\"stepGoalPercent\":";
  payload += String(getStepGoalPercent());

  payload += ",\"distanceKm\":";
  payload += String(getStepDistanceKm(), 2);

  payload += ",\"calories\":";
  payload += String(getStepCalories());

  payload += ",\"battery\":";
  payload += String(batteryPercent);

  payload += ",\"fall\":";
  payload += (alertActive && alertType == ALERT_FALL) ? "true" : "false";

  payload += ",\"sos\":";
  payload += (alertActive && alertType == ALERT_SOS) ? "true" : "false";

  payload += ",\"updatedAt\":";
  time_t nowSec = time(nullptr);
  if (nowSec > 100000) payload += String((long)nowSec);
  else payload += "null";

  payload += ",\"createdAt\":";
  payload += getFirebaseTimestampMs();

  payload += "}";

  return firebaseSendJson(url, payload, "PUT", "Firebase daily");
}


void updateFirebaseIfNeeded() {
  unsigned long now = millis();

  bool urgentActive = alertActive && (alertType == ALERT_FALL || alertType == ALERT_SOS);
  String urgentKey = buildFirebaseUrgentKey();
  bool firstRun = (lastFirebaseDataKey.length() == 0);
  bool urgentChanged = urgentActive && (firstRun || urgentKey != lastFirebaseUrgentKey);

  // HR/SpO2: chi gui khi da tinh xong cua so MAX30102 500 mau va BPM vua hop le.
  bool bpmWindowDone = firebaseHealthWindowReady;
  bool bpmReadyToUpload = bpmWindowDone && hasFreshFirebaseBPM();

  // Steps: cap nhat rieng bang PATCH de web thay doi so buoc realtime,
  // nhung khong ghi de bpm/spo2 thanh null trong luc dang do HR.
  bool stepChangedEnough = false;
  if (lastFirebaseUploadedStepCount < 0) {
    stepChangedEnough = true;
  } else if (abs(stepCount - lastFirebaseUploadedStepCount) >= FIREBASE_MIN_STEP_DELTA) {
    stepChangedEnough = true;
  } else if (stepCadenceSPM != lastFirebaseUploadedCadenceSPM) {
    stepChangedEnough = true;
  } else if (sedentaryWarning != lastFirebaseUploadedSedentaryWarning) {
    stepChangedEnough = true;
  }

  bool stepIntervalOK = (lastFirebaseStepSend == 0) ||
                        (now - lastFirebaseStepSend >= FIREBASE_STEP_UPLOAD_INTERVAL);
  bool stepReadyToUpload = stepChangedEnough && stepIntervalOK;

  // Neu cua so HR vua xong nhung BPM khong hop le, chi bo qua phan HR.
  // Neu co step moi thi van cho gui step PATCH ben duoi.
  if (bpmWindowDone && !bpmReadyToUpload) {
    Serial.println("Firebase HR skipped: HR window done but BPM invalid -> khong gui bpm:null");
    firebaseHealthWindowReady = false;
  }

  if (!bpmReadyToUpload && !urgentChanged && !stepReadyToUpload) {
    return;
  }

  // Neu vua loi HTTP thi khong thu lai lien tuc, tranh lam lag loop.
  // Rieng Fall/SOS van duoc gui ngay khi co thay doi trang thai.
  if (lastFirebaseFailTime > 0 &&
      now - lastFirebaseFailTime < FIREBASE_RETRY_INTERVAL &&
      !urgentChanged) {
    return;
  }

  String dataKey = buildFirebaseDataKey();
  bool dataChanged = firebaseForceNext || firstRun || (dataKey != lastFirebaseDataKey);

  bool historyDue = (now - lastFirebaseHistorySend >= FIREBASE_HISTORY_INTERVAL);
  bool dailyDue = (now - lastFirebaseDailySend >= FIREBASE_DAILY_INTERVAL);

  // 1) Gui full latest khi co BPM hop le hoac Fall/SOS.
  // Full payload co bpm/spo2, vi luc nay ket qua suc khoe da san sang.
  bool latestDue = urgentChanged || bpmReadyToUpload;
  bool latestIntervalOK = firstRun || urgentChanged ||
                          (now - lastFirebaseSend >= FIREBASE_MIN_SEND_INTERVAL);

  if (latestDue && latestIntervalOK) {
    lastFirebaseSend = now;

    bool ok = sendTelemetryToFirebase();
    if (ok) {
      lastFirebaseDataKey = dataKey;
      lastFirebaseUrgentKey = urgentKey;
      firebaseForceNext = false;
      firebaseHealthWindowReady = false;
      lastFirebaseFailTime = 0;

      // Full payload da gom step, nen cap nhat moc step de khong PATCH lap lai ngay.
      lastFirebaseUploadedStepCount = stepCount;
      lastFirebaseUploadedCadenceSPM = stepCadenceSPM;
      lastFirebaseUploadedSedentaryWarning = sedentaryWarning;
      lastFirebaseStepSend = now;
    } else {
      lastFirebaseFailTime = now;
      return;
    }
  } else if (bpmReadyToUpload && !latestIntervalOK) {
    // Da co BPM nhung vua gui gan day, bo cua so nay de tranh spam Firebase.
    firebaseHealthWindowReady = false;
  }

  // 2) Neu khong co BPM moi nhung buoc chan thay doi, PATCH rieng cac truong step.
  // PATCH khong cham vao bpm/spo2 nen web khong bi mat gia tri HR cu.
  if (!bpmReadyToUpload && !urgentChanged && stepReadyToUpload) {
    if (sendStepPatchToFirebase()) {
      lastFirebaseUploadedStepCount = stepCount;
      lastFirebaseUploadedCadenceSPM = stepCadenceSPM;
      lastFirebaseUploadedSedentaryWarning = sedentaryWarning;
      lastFirebaseStepSend = now;
      lastFirebaseFailTime = 0;
    } else {
      lastFirebaseFailTime = now;
      return;
    }
  }

  // 3) History chi POST khi da co BPM hop le sau mot cua so moi, hoac Fall/SOS.
  // Khong POST history cho tung buoc chan de tranh database phinh to.
  if ((bpmReadyToUpload || urgentChanged) && historyDue && shouldSaveFirebaseHistory()) {
    if (sendHistoryToFirebase()) {
      lastFirebaseHistorySend = now;
      lastFirebaseHistoryStepCount = stepCount;
    } else {
      lastFirebaseFailTime = now;
      return;
    }
  }

  // 4) Daily summary co the cap nhat khi co BPM/Fall/SOS hoac khi step thay doi.
  if ((bpmReadyToUpload || urgentChanged || stepReadyToUpload) && dailyDue) {
    if (sendDailySummaryToFirebase()) {
      lastFirebaseDailySend = now;
    } else {
      lastFirebaseFailTime = now;
    }
  }
}
