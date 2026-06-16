// ==============================================================
// 07_UI_Common.ino
// icon, khung đồng hồ, message/error, hàm vẽ chung, router màn hình
// ==============================================================

void showMessage(String line1, String line2) {
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


void showMessage(String line1) {
  showMessage(line1, "");
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



// Vẽ chữ canh giữa. Dùng được cả font mặc định và FreeSans.
void drawCenteredTextSmart(const char *text, int16_t y, const GFXfont *font,
                           uint8_t size, uint16_t color) {
  tft.setFont(font);
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.setTextWrap(false);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (TFT_WIDTH - (int16_t)w) / 2 - x1;

  tft.setCursor(x, y);
  tft.print(text);
}


void drawAlertGlowCard(int x, int y, int w, int h, uint16_t accent) {
  // Nhiều lớp viền tạo cảm giác glow nhưng vẫn nhẹ cho ESP32.
  tft.fillRoundRect(x, y, w, h, 22, 0x0800);
  tft.drawRoundRect(x - 3, y - 3, w + 6, h + 6, 25, 0x3000);
  tft.drawRoundRect(x - 2, y - 2, w + 4, h + 4, 24, 0x5800);
  tft.drawRoundRect(x, y, w, h, 22, accent);
  tft.drawRoundRect(x + 2, y + 2, w - 4, h - 4, 20, 0x4208);

  // Mảng sáng nhẹ phía trên card.
  tft.drawFastHLine(x + 22, y + 9, w - 44, 0x7800);
  tft.drawFastHLine(x + 28, y + 10, w - 56, 0x3800);
}


void drawAlertSignalWaves(uint16_t accent) {
  // Sóng tín hiệu 2 bên chữ SOS.
  drawArcSegments(73, 82, 28, 130, 230, 0x5800);
  drawArcSegments(73, 82, 38, 130, 230, 0x7800);
  drawArcSegments(73, 82, 48, 130, 230, accent);

  drawArcSegments(167, 82, 28, 310, 410, 0x5800);
  drawArcSegments(167, 82, 38, 310, 410, 0x7800);
  drawArcSegments(167, 82, 48, 310, 410, accent);
}


void drawWarningIcon(int cx, int cy, uint16_t accent) {
  // Vòng tròn cảnh báo.
  tft.fillCircle(cx, cy, 27, 0x1800);
  tft.drawCircle(cx, cy, 28, 0x3800);
  tft.drawCircle(cx, cy, 25, accent);
  tft.drawCircle(cx, cy, 22, 0x8410);

  // Tia sáng ngang.
  tft.drawFastHLine(45, cy, 52, 0x3800);
  tft.drawFastHLine(143, cy, 52, 0x3800);
  tft.drawFastHLine(70, cy, 27, accent);
  tft.drawFastHLine(143, cy, 27, accent);

  // Tam giác cảnh báo.
  tft.drawTriangle(cx, cy - 15, cx - 17, cy + 15, cx + 17, cy + 15, ST77XX_WHITE);
  tft.drawTriangle(cx, cy - 14, cx - 16, cy + 14, cx + 16, cy + 14, accent);
  tft.drawTriangle(cx, cy - 13, cx - 15, cy + 13, cx + 15, cy + 13, accent);

  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(cx - 5, cy - 7);
  tft.print("!");
}


void drawTouchHintPill(const char *hint, uint16_t accent) {
  const int x = 24;
  const int y = 199;
  const int w = 192;
  const int h = 37;

  tft.fillRoundRect(x, y, w, h, 14, 0x0000);
  tft.drawRoundRect(x, y, w, h, 14, 0x3186);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 13, 0x1082);

  // Icon ngón tay / nút bấm đơn giản.
  int ix = x + 29;
  int iy = y + 18;
  tft.drawCircle(ix, iy - 7, 5, accent);
  tft.drawLine(ix, iy - 2, ix, iy + 12, accent);
  tft.drawLine(ix, iy + 12, ix + 11, iy + 12, accent);
  tft.drawLine(ix + 11, iy + 12, ix + 8, iy + 5, accent);
  tft.drawLine(ix, iy + 5, ix - 7, iy + 1, accent);
  tft.drawCircle(ix, iy - 7, 2, accent);

  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(false);
  tft.setCursor(x + 57, y + 15);
  tft.print(hint);
}


void drawAlertScreen() {
  if (uiDrawn) return;

  drawWatchCaseFrame();

  bool isSOS = (alertType == ALERT_SOS || alertType == ALERT_FALL);
  uint16_t accent = ST77XX_RED;
  const char *title = "SOS";
  const char *line1 = (alertType == ALERT_FALL) ? "DANG TE NGA" : "DANG GUI CANH BAO";
  const char *hint  = "Nhan nut de tat canh bao";

  // Card chính: nằm gọn trong màn 240x280, không bị cắt chữ.
  drawAlertGlowCard(22, 34, 196, 212, accent);

  // Sóng cảnh báo quanh chữ SOS/FALL.
  drawAlertSignalWaves(accent);

  // Chữ lớn phía trên.
  drawCenteredTextSmart(title, 94, &FreeSansBold24pt7b, 1, accent);

  // Icon cảnh báo ở giữa.
  drawWarningIcon(120, 126, accent);

  // Dòng trạng thái chính, dùng font nhỏ hơn để không bị tràn sang phải.
  drawCenteredTextSmart(line1, 170, &FreeSansBold9pt7b, 1, ST77XX_WHITE);

  // Divider đỏ nhỏ ở giữa.
  tft.drawFastHLine(58, 183, 48, 0x5800);
  tft.drawFastHLine(134, 183, 48, 0x5800);
  tft.fillCircle(120, 183, 4, accent);
  tft.fillCircle(120, 183, 2, ST77XX_WHITE);

  // Hướng dẫn tắt cảnh báo.
  drawTouchHintPill(hint, accent);

  // 3 chấm trạng thái nhỏ giống giao diện smartwatch.
  tft.fillCircle(105, 244, 3, 0x4208);
  tft.fillCircle(120, 244, 4, accent);
  tft.fillCircle(135, 244, 3, 0x4208);

  tft.setFont(NULL);
  tft.setTextWrap(true);
  uiDrawn = true;
}



void drawMainScreen() {
  if (standbyMode) return;

  if (alertActive) {
    drawAlertScreen();
    return;
  }

  if (currentScreen == SCREEN_HOME) {
    drawHomeWatchFace();
  } else if (currentScreen == SCREEN_HEALTH) {
    drawHealthScreen();
  } else if (currentScreen == SCREEN_STEPS) {
    drawStepScreen();
  }
}

