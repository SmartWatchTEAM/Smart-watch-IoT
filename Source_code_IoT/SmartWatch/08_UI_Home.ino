// ==============================================================
// 08_UI_Home.ino
// màn hình chính
// ==============================================================

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


