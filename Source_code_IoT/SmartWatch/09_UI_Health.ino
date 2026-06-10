// ==============================================================
// 09_UI_Health.ino
// màn hình nhịp tim, SpO2, biểu đồ
// ==============================================================

// =======================================================
// DRAW UI - HEALTH SCREEN V4 - PREMIUM DETAIL
// Màn hình Health đẹp hơn cho TFT 240x280
// - WiFi icon cùng kích thước với pin
// - Icon Heart/SpO2 nằm gọn trong khung, không vượt card
// - Chỉ cập nhật vùng động để hạn chế nhấp nháy
// =======================================================

#define H_BG              ST77XX_BLACK
#define H_FRAME_1         0x4208
#define H_FRAME_2         0x2104
#define H_PANEL           0x1082
#define H_PANEL_2         0x0841
#define H_RED_BG          0x1803
#define H_RED_BG_2        0x2805
#define H_RED_BORDER      0xF986
#define H_RED_SOFT        0x7808
#define H_BLUE_BG         0x0012
#define H_BLUE_BG_2       0x0216
#define H_BLUE_BORDER     0x04BF
#define H_BLUE_SOFT       0x039F
#define H_GREEN           0x07E0
#define H_YELLOW          0xFFE0
#define H_TEXT_DIM        0xA514
#define H_GRID            0x31A6
#define H_DARK_GRID       0x18E3
#define H_GRAPH_BG        0x0800
#define H_GRAPH_BG_2      0x1800
#define H_GRAPH_GRID      0x3963
#define H_GRAPH_GRID_RED  0x6000
#define H_GRAPH_FILL      0x4000
#define H_GRAPH_LINE      ST77XX_RED

const int H_CARD_X  = 16;
const int H_CARD_W  = 208;
const int H_HR_Y    = 48;
const int H_HR_H    = 58;
const int H_SPO2_Y  = 112;
const int H_SPO2_H  = 52;
const int H_GRAPH_Y = 172;
const int H_GRAPH_H = 86;

// =======================
// HEALTH GRAPH LAYOUT
// =======================
const int HG_GX      = 46;                // dịch đồ thị sang phải
const int HG_GY      = H_GRAPH_Y + 13;
const int HG_GW      = 122;               // giảm nhẹ chiều rộng để chừa vùng MAX/MIN
const int HG_GH      = 55;
const int HG_RIGHT_X = 182;               // vùng MAX / MIN bên phải

void drawHealthSoftGlowCard(int x, int y, int w, int h, uint16_t bg, uint16_t border) {
  tft.fillRoundRect(x + 1, y + 2, w, h, 14, 0x0001);      // bóng nhẹ
  tft.fillRoundRect(x, y, w, h, 14, bg);
  tft.drawRoundRect(x, y, w, h, 14, border);
  tft.drawRoundRect(x + 2, y + 2, w - 4, h - 4, 12, H_FRAME_2);
}


void drawTinyWifiEqualBox(int x, int y, bool connected) {
  // Box 28x16: cùng kích thước thị giác với icon pin
  uint16_t c = connected ? H_GREEN : ST77XX_RED;
  tft.fillRoundRect(x, y, 28, 16, 5, H_PANEL_2);
  tft.drawRoundRect(x, y, 28, 16, 5, H_FRAME_1);

  int cx = x + 14;
  int cy = y + 12;
  // Vẽ 3 cung WiFi nhỏ, nằm gọn trong box
  for (int r = 4; r <= 10; r += 3) {
    for (int a = 215; a <= 325; a += 12) {
      float r0 = radians((float)a);
      float r1 = radians((float)(a + 10));
      int x0 = cx + roundf(cosf(r0) * r);
      int y0 = cy + roundf(sinf(r0) * r);
      int x1 = cx + roundf(cosf(r1) * r);
      int y1 = cy + roundf(sinf(r1) * r);
      tft.drawLine(x0, y0, x1, y1, c);
    }
  }
  tft.fillCircle(cx, cy, 2, c);

  if (!connected) {
    tft.drawLine(x + 7, y + 4, x + 21, y + 13, ST77XX_RED);
  }
}


void drawTinyBatteryEqualBox(int x, int y, int percent) {
  // Box tổng 28x16 giống WiFi
  percent = constrain(percent, 0, 100);
  uint16_t c = H_GREEN;
  if (percent < 20) c = ST77XX_RED;
  else if (percent < 50) c = H_YELLOW;

  tft.fillRoundRect(x, y, 28, 16, 5, H_PANEL_2);
  tft.drawRoundRect(x, y, 28, 16, 5, H_FRAME_1);

  tft.drawRoundRect(x + 4, y + 4, 18, 8, 2, ST77XX_WHITE);
  tft.fillRect(x + 22, y + 7, 2, 3, ST77XX_WHITE);
  int fillW = map(percent, 0, 100, 0, 14);
  tft.fillRoundRect(x + 6, y + 6, fillW, 4, 1, c);
}


void drawHealthTopBar() {
  tft.setFont(NULL);
  tft.fillRect(18, 17, 204, 25, H_BG);

  updateClock();

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(24, 23);
  if (currentTimeText.length() >= 5) tft.print(currentTimeText);
  else tft.print("--:--");

  // WiFi và pin cùng kích thước 28x16, cân hàng với nhau
  drawTinyWifiEqualBox(161, 20, wifiConnected);
  drawTinyBatteryEqualBox(193, 20, batteryPercent);
}


void drawHealthStatusPill(int x, int y, const char *text, uint16_t color) {
  int w = strlen(text) * 6 + 18;
  tft.fillRoundRect(x, y, w, 15, 7, H_PANEL_2);
  tft.drawRoundRect(x, y, w, 15, 7, H_FRAME_1);
  tft.fillCircle(x + 7, y + 7, 3, color);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(x + 13, y + 4);
  tft.print(text);
}


void drawHeartCircleIcon(int cx, int cy) {
  // Bán kính tối đa 20px, nằm gọn trong card HR 58px cao
  tft.fillCircle(cx, cy, 22, H_RED_BG_2);
  tft.drawCircle(cx, cy, 22, H_RED_BORDER);
  tft.drawCircle(cx, cy, 18, H_RED_SOFT);
  tft.drawCircle(cx, cy, 14, 0x5805);

  // Heart nhỏ nằm trong vòng tròn
  drawHeartIcon(cx, cy - 4, H_RED_BORDER);
  tft.drawLine(cx - 16, cy + 5, cx - 8, cy + 5, ST77XX_WHITE);
  tft.drawLine(cx - 8, cy + 5, cx - 4, cy - 2, ST77XX_WHITE);
  tft.drawLine(cx - 4, cy - 2, cx, cy + 8, ST77XX_WHITE);
  tft.drawLine(cx, cy + 8, cx + 5, cy + 1, ST77XX_WHITE);
  tft.drawLine(cx + 5, cy + 1, cx + 16, cy + 1, ST77XX_WHITE);
}


void drawSpO2CircleIcon(int cx, int cy) {
  // Bán kính tối đa 20px, nằm gọn trong card SpO2 52px cao
  tft.fillCircle(cx, cy, 21, H_BLUE_BG_2);
  tft.drawCircle(cx, cy, 21, H_BLUE_BORDER);
  tft.drawCircle(cx, cy, 17, H_BLUE_SOFT);
  tft.drawCircle(cx, cy, 13, 0x0254);

  // Giọt O2 nhỏ, không vượt vòng tròn
  tft.fillCircle(cx, cy + 5, 11, H_BLUE_BORDER);
  tft.fillTriangle(cx, cy - 17, cx - 11, cy + 5, cx + 11, cy + 5, H_BLUE_BORDER);
  tft.fillCircle(cx - 4, cy - 2, 3, H_BLUE_BG);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(cx - 10, cy - 2);
  tft.print("O");
  tft.setTextSize(1);
  tft.setCursor(cx + 4, cy + 9);
  tft.print("2");
}


void drawHealthGraphGrid() {
  const int gx = HG_GX;
  const int gy = HG_GY;
  const int gw = HG_GW;
  const int gh = HG_GH;

  // Grid nằm trong khung đồ thị, không vẽ đè lên chữ thời gian bên dưới.
  // Grid bắt đầu sau trục Y 1px để không che trục Y.
  for (int i = 0; i <= 3; i++) {
    int y = gy + i * (gh / 3);
    tft.drawFastHLine(gx + 1, y, gw - 1, H_GRAPH_GRID);
  }

  // Grid dọc: chỉ vẽ trong vùng plot, không kéo xuống phần chữ -60s/-30s/-15s/Now.
  int x30  = gx + gw / 2;
  int x15  = gx + (gw * 3) / 4;
  int xNow = gx + gw;

  tft.drawFastVLine(x30,  gy, gh, H_DARK_GRID);
  tft.drawFastVLine(x15,  gy, gh, H_DARK_GRID);
  tft.drawFastVLine(xNow, gy, gh, H_GRAPH_GRID_RED);
}


void drawHealthGraphAxes(bool drawLabels) {
  const int gx = HG_GX;
  const int gy = HG_GY;
  const int gw = HG_GW;
  const int gh = HG_GH;

  int y120 = gy;
  int y90  = gy + gh / 3;
  int y60  = gy + (gh * 2) / 3;
  int y30  = gy + gh;

  uint16_t axisColor = ST77XX_WHITE;

  if (drawLabels) {
    tft.setFont(NULL);
    tft.setTextSize(1);

    // Xóa nền riêng vùng nhãn Y, tránh chữ bị nhòe khi refresh.
    // Không xóa vào trong vùng plot nên không làm mất graph.
    tft.fillRect(gx - 25, gy - 6, 22, gh + 13, H_GRAPH_BG);

    tft.setTextColor(H_TEXT_DIM);
    tft.setCursor(gx - 23, y120 - 3); tft.print("120");
    tft.setCursor(gx - 17, y90  - 3); tft.print("90");
    tft.setCursor(gx - 17, y60  - 3); tft.print("60");
    tft.setCursor(gx - 17, y30  - 3); tft.print("30");

    // Xóa nền riêng vùng nhãn X để grid không che chữ thời gian.
    int labelY = H_GRAPH_Y + H_GRAPH_H - 13;
    tft.fillRect(gx - 6, labelY - 2, gw + 20, 12, H_GRAPH_BG);

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(gx - 4,               labelY); tft.print("-60s");
    tft.setCursor(gx + gw / 2 - 10,     labelY); tft.print("-30s");
    tft.setCursor(gx + (gw * 3) / 4 - 10, labelY); tft.print("-15s");
    tft.setCursor(gx + gw - 8,          labelY); tft.print("Now");
  }

  // Vẽ trục SAU CÙNG để graph không che mất trục.
  tft.drawFastVLine(gx, gy, gh + 1, axisColor);        // trục Y
  tft.drawFastHLine(gx, gy + gh, gw + 1, axisColor);   // trục X

  // Tick nhỏ của trục Y
  tft.drawFastHLine(gx - 3, y120, 4, axisColor);
  tft.drawFastHLine(gx - 3, y90,  4, axisColor);
  tft.drawFastHLine(gx - 3, y60,  4, axisColor);
  tft.drawFastHLine(gx - 3, y30,  4, axisColor);

  // Tick nhỏ của trục X
  tft.drawFastVLine(gx,                  gy + gh, 4, axisColor);
  tft.drawFastVLine(gx + gw / 2,         gy + gh, 4, axisColor);
  tft.drawFastVLine(gx + (gw * 3) / 4,   gy + gh, 4, axisColor);
  tft.drawFastVLine(gx + gw,             gy + gh, 4, axisColor);
}


void drawStaticHealthScreen() {
  drawWatchCaseFrame();
  drawHealthTopBar();

  // HR Card
  drawHealthSoftGlowCard(H_CARD_X, H_HR_Y, H_CARD_W, H_HR_H, H_RED_BG, H_RED_BORDER);
  drawHeartCircleIcon(43, H_HR_Y + 29);

  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(76, H_HR_Y + 11);
  tft.print("Heart Rate");

  // SpO2 Card
  drawHealthSoftGlowCard(H_CARD_X, H_SPO2_Y, H_CARD_W, H_SPO2_H, H_BLUE_BG, H_BLUE_BORDER);
  drawSpO2CircleIcon(43, H_SPO2_Y + 26);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(76, H_SPO2_Y + 9);
  tft.print("Blood Oxygen");

  // Graph card
  drawHealthSoftGlowCard(H_CARD_X, H_GRAPH_Y, H_CARD_W, H_GRAPH_H, H_GRAPH_BG, H_RED_SOFT);

  // Vẽ nền + grid + trục ban đầu
  tft.fillRoundRect(HG_GX + 1, HG_GY + 1, HG_GW - 1, HG_GH - 1, 5, H_GRAPH_BG);
  drawHealthGraphGrid();
  drawHealthGraphAxes(true);

  // Page indicator
  tft.fillCircle(114, 270, 3, COLOR_DIM);
  tft.fillCircle(126, 270, 3, ST77XX_WHITE);
}


void drawHealthDynamicValues() {
  int bpmValue = 0;
  if (fingerDetected && validHeartRate && displayHeartRate >= 40 && displayHeartRate <= 160) {
    bpmValue = displayHeartRate;
  }

  int spo2Value = 0;
  if (fingerDetected && validSPO2 && displaySpO2 >= 80 && displaySpO2 <= 100) {
    spo2Value = displaySpO2;
  }

  // HR value area
  tft.fillRect(76, H_HR_Y + 23, 132, 25, H_RED_BG);
  tft.setFont(NULL);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(76, H_HR_Y + 22);
  if (bpmValue > 0) tft.print(bpmValue);
  else tft.print("--");

  tft.setTextSize(2);
  tft.setTextColor(H_RED_BORDER);
  tft.setCursor(130, H_HR_Y + 27);
  tft.print("BPM");

  // Badge Live/Finger
  tft.fillRect(174, H_HR_Y + 24, 44, 18, H_RED_BG);
  if (fingerDetected) drawHealthStatusPill(178, H_HR_Y + 25, "Live", H_GREEN);
  else drawHealthStatusPill(178, H_HR_Y + 25, "Wait", H_YELLOW);

  // SpO2 value area
  tft.fillRect(76, H_SPO2_Y + 22, 132, 24, H_BLUE_BG);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(76, H_SPO2_Y + 22);
  if (spo2Value > 0) tft.print(spo2Value);
  else tft.print("--");

  tft.setTextSize(2);
  tft.setTextColor(H_BLUE_BORDER);
  tft.setCursor(130, H_SPO2_Y + 25);
  tft.print("%");

  tft.fillRect(174, H_SPO2_Y + 21, 44, 18, H_BLUE_BG);
  if (spo2Value > 0) drawHealthStatusPill(178, H_SPO2_Y + 22, "Live", H_BLUE_BORDER);
  else drawHealthStatusPill(178, H_SPO2_Y + 24, "Wait", H_YELLOW);
}


int healthGraphMapYDynamic(uint32_t value, uint32_t minV, uint32_t maxV, int gy, int gh) {
  if (maxV <= minV) maxV = minV + 1;
  if (value < minV) value = minV;
  if (value > maxV) value = maxV;

  float ratio = (float)(value - minV) / (float)(maxV - minV);
  int y = gy + gh - 4 - (int)(ratio * (gh - 8));
  return constrain(y, gy + 3, gy + gh - 3);
}


void drawPremiumHealthGraph() {
  const int gx = HG_GX;
  const int gy = HG_GY;
  const int gw = HG_GW;
  const int gh = HG_GH;
  const int rightX = HG_RIGHT_X;

  // Vùng vẽ đường nằm cách trục Y vài pixel để không che trục.
  const int plotX = gx + 4;
  const int plotW = gw - 6;

  // Chỉ xóa bên trong vùng plot, không xóa trục Y, trục X và nhãn.
  tft.fillRoundRect(gx + 1, gy + 1, gw - 1, gh - 1, 5, H_GRAPH_BG);
  tft.fillRect(rightX - 2, gy - 1, 40, gh + 4, H_GRAPH_BG);

  // Nền nhẹ bên trong graph
  for (int y = 1; y < gh - 1; y += 2) {
    uint16_t c = (y > gh / 2) ? H_GRAPH_BG_2 : H_GRAPH_BG;
    tft.drawFastHLine(gx + 1, gy + y, gw - 1, c);
  }

  drawHealthGraphGrid();

  // Nếu chưa đặt ngón tay
  if (!fingerDetected) {

    tft.setTextColor(H_RED_BORDER);
    tft.setCursor(rightX, gy + 2);  tft.print("MAX");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(rightX, gy + 13); tft.print("--");

    tft.setTextColor(H_BLUE_BORDER);
    tft.setCursor(rightX, gy + 38); tft.print("MIN");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(rightX, gy + 49); tft.print("--");

    // Trục được vẽ cuối cùng nên luôn nổi lên trên.
    drawHealthGraphAxes(true);
    return;
  }

  // Tự scale tín hiệu
  int start = GRAPH_DATA_SIZE - plotW;
  if (start < 0) start = 0;

  uint32_t minV = 0xFFFFFFFF;
  uint32_t maxV = 0;
  int validCount = 0;

  for (int i = 0; i < plotW; i++) {
    int idx = start + i;
    if (idx < 0 || idx >= GRAPH_DATA_SIZE) continue;

    uint32_t v = graphData[idx];
    if (v <= GRAPH_MIN + 10) continue;

    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
    validCount++;
  }

  if (validCount < 8 || maxV <= minV) {
    tft.setFont(NULL);
    tft.setTextSize(1);
    tft.setTextColor(H_TEXT_DIM);
    tft.setCursor(gx + 18, gy + 23);
    tft.print("Waiting signal...");

    drawHealthGraphAxes(true);
    return;
  }

  uint32_t range = maxV - minV;
  if (range < 500) {
    minV = (minV > 250) ? minV - 250 : 0;
    maxV = maxV + 250;
  }

  int lastY = gy + gh / 2;

  for (int i = 0; i < plotW - 1; i++) {
    int idx1 = start + i;
    int idx2 = start + i + 1;
    if (idx2 >= GRAPH_DATA_SIZE) break;

    uint32_t v1 = graphData[idx1];
    uint32_t v2 = graphData[idx2];

    int x1p = plotX + i;
    int x2p = plotX + i + 1;
    int y1p = healthGraphMapYDynamic(v1, minV, maxV, gy, gh);
    int y2p = healthGraphMapYDynamic(v2, minV, maxV, gy, gh);

    // Đường graph mảnh hơn, không tô vùng xuống trục nên không che trục Y.
    tft.drawLine(x1p, y1p + 1, x2p, y2p + 1, H_RED_SOFT);
    tft.drawLine(x1p, y1p,     x2p, y2p,     H_GRAPH_LINE);

    lastY = y2p;
  }

  // Trục được vẽ sau graph để không bị graph che.
  drawHealthGraphAxes(true);

  // MAX / MIN
  int showMax = hrTrendMax;
  int showMin = hrTrendMin;
  if (showMax <= 0 && validHeartRate) showMax = displayHeartRate;
  if (showMin <= 0 && validHeartRate) showMin = displayHeartRate;

  tft.setFont(NULL);
  tft.setTextSize(1);

  tft.setTextColor(H_RED_BORDER);
  tft.setCursor(rightX, gy + 2);
  tft.print("MAX");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(rightX, gy + 13);
  if (showMax > 0) tft.print(showMax);
  else tft.print("--");

  tft.setTextColor(H_BLUE_BORDER);
  tft.setCursor(rightX, gy + 38);
  tft.print("MIN");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(rightX, gy + 49);
  if (showMin > 0) tft.print(showMin);
  else tft.print("--");
}


void drawHealthScreen() {
  if (standbyMode) return;

  if (!uiDrawn) {
    drawStaticHealthScreen();
    uiDrawn = true;
  }

  drawHealthTopBar();
  drawHealthDynamicValues();
  drawPremiumHealthGraph();
}


