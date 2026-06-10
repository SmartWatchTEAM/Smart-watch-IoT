// ==============================================================
// 10_UI_Steps.ino
// màn hình đếm bước chân
// ==============================================================

// =======================================================
// STEP TRACKER UI - MODERN SMARTWATCH SCREEN
// =======================================================

#define S_BLUE        0x04FF
#define S_CYAN        0x07FF
#define S_PURPLE      0xA81F
#define S_ORANGE      0xFD20
#define S_ORANGE_DARK 0x3200
#define S_CARD_BG     0x0841
#define S_CARD_BG_2   0x0008
#define S_DIM         0x8410
#define S_GREEN       0x07E0
#define S_RED         0xF800

const int S_RING_CX = 120;
const int S_RING_CY = 104;
const int S_RING_R  = 50;

void drawThickArc(int16_t cx, int16_t cy, int16_t radius,
                  int startDeg, int endDeg, int thick, uint16_t color) {
  if (endDeg < startDeg) return;

  for (int r = radius; r > radius - thick; r--) {
    for (int a = startDeg; a < endDeg; a += 4) {
      float a0 = radians((float)a);
      float a1 = radians((float)(a + 4));
      int16_t x0 = cx + (int16_t)roundf(cosf(a0) * r);
      int16_t y0 = cy + (int16_t)roundf(sinf(a0) * r);
      int16_t x1 = cx + (int16_t)roundf(cosf(a1) * r);
      int16_t y1 = cy + (int16_t)roundf(sinf(a1) * r);
      tft.drawLine(x0, y0, x1, y1, color);
    }
  }
}


void drawStepFootIcon(int cx, int cy, uint16_t color) {
  // Thu nhỏ icon bước chân để giao diện gọn và cân đối hơn.
  tft.fillCircle(cx - 5, cy - 2, 3, color);
  tft.fillCircle(cx + 5, cy - 4, 3, color);
  tft.fillRoundRect(cx - 7, cy + 1, 5, 7, 2, color);
  tft.fillRoundRect(cx + 2, cy - 1, 5, 7, 2, color);
}


void drawLocationIcon(int cx, int cy, uint16_t color) {
  tft.fillCircle(cx, cy - 2, 6, color);
  tft.fillTriangle(cx - 6, cy + 1, cx + 6, cy + 1, cx, cy + 11, color);
  tft.fillCircle(cx, cy - 2, 2, S_CARD_BG_2);
}


void drawFlameIcon(int cx, int cy, uint16_t color) {
  tft.fillCircle(cx, cy + 4, 7, color);
  tft.fillTriangle(cx - 7, cy + 4, cx + 7, cy + 4, cx, cy - 10, color);
  tft.fillTriangle(cx - 3, cy + 4, cx + 4, cy + 4, cx + 1, cy - 4, H_YELLOW);
}


void drawClockIconSmall(int cx, int cy, uint16_t color) {
  tft.drawCircle(cx, cy, 9, color);
  tft.drawLine(cx, cy, cx, cy - 5, color);
  tft.drawLine(cx, cy, cx + 5, cy + 3, color);
  tft.fillCircle(cx, cy, 2, color);
}


void drawTargetIcon(int cx, int cy, uint16_t color) {
  tft.drawCircle(cx, cy, 9, color);
  tft.drawCircle(cx, cy, 5, color);
  tft.fillCircle(cx, cy, 2, color);
  tft.drawLine(cx + 4, cy - 4, cx + 11, cy - 11, color);
}


void drawSittingIcon(int cx, int cy, uint16_t color) {
  tft.fillCircle(cx - 2, cy - 10, 3, color);
  tft.drawLine(cx - 2, cy - 6, cx - 2, cy + 2, color);
  tft.drawLine(cx - 2, cy + 2, cx + 8, cy + 2, color);
  tft.drawLine(cx + 8, cy + 2, cx + 8, cy + 10, color);
  tft.drawLine(cx - 2, cy + 10, cx + 10, cy + 10, color);
  tft.drawLine(cx - 8, cy - 1, cx - 8, cy + 12, color);
  tft.drawLine(cx - 8, cy + 12, cx + 6, cy + 12, color);
}


void drawWalkingIcon(int cx, int cy, uint16_t color) {
  tft.fillCircle(cx, cy - 10, 3, color);
  tft.drawLine(cx, cy - 6, cx - 2, cy + 2, color);
  tft.drawLine(cx - 2, cy + 2, cx - 8, cy + 10, color);
  tft.drawLine(cx - 2, cy + 2, cx + 7, cy + 8, color);
  tft.drawLine(cx - 1, cy - 3, cx - 9, cy + 1, color);
  tft.drawLine(cx, cy - 3, cx + 8, cy - 1, color);
}


void drawStepMetricCard(int x, int y, int w, int h, const char *label, uint16_t accent, byte iconType) {
  tft.fillRoundRect(x + 1, y + 2, w, h, 11, 0x0001);
  tft.fillRoundRect(x, y, w, h, 11, S_CARD_BG_2);
  tft.drawRoundRect(x, y, w, h, 11, H_FRAME_1);
  tft.drawFastHLine(x + 8, y + h - 2, w - 16, accent);

  int cx = x + 15;
  tft.fillCircle(cx, y + 15, 10, S_CARD_BG);
  tft.drawCircle(cx, y + 15, 10, accent);

  if (iconType == 0) drawLocationIcon(cx, y + 15, accent);
  else if (iconType == 1) drawFlameIcon(cx, y + 15, accent);
  else if (iconType == 2) drawClockIconSmall(cx, y + 15, accent);
  else drawTargetIcon(cx, y + 15, accent);

  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(H_TEXT_DIM);
  tft.setCursor(x + 29, y + 8);
  tft.print(label);
}


void drawStepProgressRing() {
  int percent = getStepGoalPercent();
  int startDeg = 135;
  int endDeg = 405;
  int progressEnd = startDeg + (int)((long)(endDeg - startDeg) * percent / 100L);

  drawThickArc(S_RING_CX, S_RING_CY, S_RING_R, startDeg, endDeg, 8, 0x2108);
  drawThickArc(S_RING_CX, S_RING_CY, S_RING_R, startDeg, progressEnd, 8, S_BLUE);
  drawThickArc(S_RING_CX, S_RING_CY, S_RING_R - 2, startDeg + 3, progressEnd, 4, S_CYAN);

  if (progressEnd < endDeg) {
    drawThickArc(S_RING_CX, S_RING_CY, S_RING_R, progressEnd, endDeg, 5, S_PURPLE);
  }

  // Hai chấm ở đầu/cuối vòng để giống mặt đồng hồ thể thao.
  float a0 = radians((float)startDeg);
  float a1 = radians((float)progressEnd);
  tft.fillCircle(S_RING_CX + (int)roundf(cosf(a0) * S_RING_R),
                 S_RING_CY + (int)roundf(sinf(a0) * S_RING_R), 3, S_DIM);
  if (percent > 0) {
    tft.fillCircle(S_RING_CX + (int)roundf(cosf(a1) * S_RING_R),
                   S_RING_CY + (int)roundf(sinf(a1) * S_RING_R), 4, S_CYAN);
  }
}


void drawStaticStepScreen() {
  drawWatchCaseFrame();
  drawHealthTopBar();

  // Card STEP chính to hơn
  tft.fillRoundRect(16, 48, 208, 108, 20, ST77XX_BLACK);
  tft.drawRoundRect(16, 48, 208, 108, 20, H_FRAME_2);
  tft.drawRoundRect(18, 50, 204, 104, 18, H_FRAME_1);

  // 4 card thống kê xích xuống
  drawStepMetricCard(16, 160, 101, 44, "KM", S_BLUE, 0);
  drawStepMetricCard(123, 160, 101, 44, "KCAL", S_ORANGE, 1);
  drawStepMetricCard(16, 209, 101, 44, "ACTIVE", S_GREEN, 2);
  drawStepMetricCard(123, 209, 101, 44, "GOAL", S_PURPLE, 3);

  // Page indicator
  tft.fillCircle(102, 263, 3, COLOR_DIM);
  tft.fillCircle(114, 263, 3, COLOR_DIM);
  tft.fillCircle(126, 263, 3, S_CYAN);
  tft.fillCircle(138, 263, 3, COLOR_DIM);
}


void drawStepMainValues() {
  // Xóa vùng động bên trong card chính
  tft.fillRoundRect(18, 50, 204, 104, 18, ST77XX_BLACK);

  drawStepProgressRing();

  // Icon chân nằm gọn trong vòng
  tft.fillCircle(120, 72, 11, H_PANEL_2);
  tft.drawCircle(120, 72, 11, S_CYAN);
  drawStepFootIcon(120, 71, S_CYAN);

  String stepText = formatNumberWithComma(stepCount);
  byte textSize = 4;
  if (stepText.length() > 5) textSize = 3;

  tft.setFont(NULL);
  tft.setTextSize(textSize);
  tft.setTextColor(ST77XX_WHITE);

  int stepW = stepText.length() * 6 * textSize;
  tft.setCursor((240 - stepW) / 2, 94);
  tft.print(stepText);

  drawCenteredDefaultText("steps", 58, 126, 124, 13, 2, S_CYAN);

  int pct = getStepGoalPercent();

  tft.setTextSize(1);
  tft.setTextColor(H_TEXT_DIM);
}


void drawMetricValueText(int x, int y, int w, String value, String unit, uint16_t color) {
  tft.fillRect(x + 30, y + 18, w - 35, 14, S_CARD_BG_2);
  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(x + 33, y + 18);
  tft.print(value);

  tft.setTextSize(1);
  tft.setTextColor(color);
  int ux = x + w - (unit.length() * 6) - 8;
  tft.setCursor(ux, y + 26);
  tft.print(unit);
}


void drawStepMetricValues() {
  float dist = getStepDistanceKm();
  int cal = getStepCalories();
  int activeMin = activeWalkMs / 60000UL;
  int pct = getStepGoalPercent();

  String distText = String(dist, 1);
  if (dist >= 10.0f) distText = String(dist, 0);

  // Giá trị phải trùng y với 4 card mới
  drawMetricValueText(16, 160, 101, distText, "km", S_BLUE);
  drawMetricValueText(123, 160, 101, String(cal), "cal", S_ORANGE);
  drawMetricValueText(16, 209, 101, String(activeMin), "min", S_GREEN);
  drawMetricValueText(123, 209, 101, String(pct), "%", S_PURPLE);
}


void drawStepScreen() {
  if (standbyMode) return;

  if (!uiDrawn) {
    drawStaticStepScreen();
    uiDrawn = true;
  }

  drawHealthTopBar();
  drawStepMainValues();
  drawStepMetricValues();
}


