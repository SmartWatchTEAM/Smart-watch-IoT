// ==============================================================
// 03_WiFi_Time_Weather.ino
// WiFi, NTP, thời tiết Open-Meteo
// ==============================================================

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
  if (getLocalTime(&timeinfo, 2000)) {
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
  http.setTimeout(1500);

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
  if (millis() - lastWeatherUpdate < WEATHER_UPDATE_INTERVAL) {
    return;
  }

  // Khong goi HTTP thoi tiet khi MAX30102 dang nap buffer, vi HTTPS co the lam loop bi khung/lost FIFO.
  if (fingerDetected && bufferIndex > 0 && bufferIndex < BUFFER_LENGTH) {
    return;
  }

  lastWeatherUpdate = millis();
  updateWeatherNow();
}


