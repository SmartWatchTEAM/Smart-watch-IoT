// ==============================================================
// 04_MAX30102.ino
// đo nhịp tim, SpO2, watchdog/reset MAX30102, setup MAX30102
// ==============================================================

void resetHealthBuffers() {
  bufferIndex = 0;

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    irBuffer[i] = 0;
    redBuffer[i] = 0;
    sampleTimeBuffer[i] = 0;
  }

  for (int i = 0; i < GRAPH_DATA_SIZE; i++) {
    graphData[i] = GRAPH_MIN;
  }

  for (int i = 0; i < HR_TREND_SIZE; i++) {
    hrTrendData[i] = 0;
  }
  hrTrendCount = 0;
  hrTrendMax = 0;
  hrTrendMin = 0;

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

  hrPendingValue = 0;
  hrPendingCount = 0;
  lastHRAuto = 0;
  lastHRExtrema = 0;
  lastHRCandidate = 0;
  lastHRWindowMedian = 0;
  lastHRWindowValidCount = 0;
  lastHRActualFs = 0;
  lastHRDurationMs = 0;
  lastHRAcRange = 0;
  lastHRThreshold = 0;
  lastHRPeakCount = 0;
  lastHRValleyCount = 0;
  lastHRAutoScore = 0;
  lastHRSpectrum = 0;
  lastHRSpectrumScore = 0;
  lastHRSpectrumSeparation = 0;
  lastLiveSpO2EstimateMs = 0;
  lastLiveSpO2Estimate = 0;
  lastLiveSpO2Quality = 0;
  lastRawRatioX100 = 0;
  lastAdjustedRatioX100 = 0;
  lastHRQuality = 0;
  lastHRRejectReason = 0;
  lastWristMotionScore = 0;
  wristBadSampleCount = 0;
  wristDroppedSamples = 0;
  lastGoodIRValue = 0;
  lastGoodREDValue = 0;
  lastValidHeartRateMs = 0;
  lastValidSpO2Ms = 0;
  lastFingerLostMs = 0;

  signalOK = false;
}


void pushGraphData(uint32_t value) {
  for (int i = 0; i < GRAPH_DATA_SIZE - 1; i++) {
    graphData[i] = graphData[i + 1];
  }

  graphData[GRAPH_DATA_SIZE - 1] = value;
}


void pushHeartRateTrend(int bpm) {
  if (bpm < 40 || bpm > 160) return;

  for (int i = 0; i < HR_TREND_SIZE - 1; i++) {
    hrTrendData[i] = hrTrendData[i + 1];
  }
  hrTrendData[HR_TREND_SIZE - 1] = bpm;

  if (hrTrendCount < HR_TREND_SIZE) hrTrendCount++;

  hrTrendMax = 0;
  hrTrendMin = 999;

  int start = HR_TREND_SIZE - hrTrendCount;
  for (int i = start; i < HR_TREND_SIZE; i++) {
    if (hrTrendData[i] <= 0) continue;
    if (hrTrendData[i] > hrTrendMax) hrTrendMax = hrTrendData[i];
    if (hrTrendData[i] < hrTrendMin) hrTrendMin = hrTrendData[i];
  }

  if (hrTrendMin == 999) hrTrendMin = 0;
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


int medianIntInPlace(int *values, int count) {
  if (count <= 0) return 0;

  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (values[j] < values[i]) {
        int t = values[i];
        values[i] = values[j];
        values[j] = t;
      }
    }
  }

  if (count % 2 == 1) return values[count / 2];
  return (values[count / 2 - 1] + values[count / 2]) / 2;
}


bool isNormalStableHR(int bpm) {
  return bpm >= HR_STABLE_LOW_BPM && bpm <= HR_STABLE_HIGH_BPM;
}


bool isStableSpO2Value(int value) {
  return value >= SPO2_STABLE_LOW && value <= SPO2_STABLE_HIGH;
}


int limitStepToward(int currentValue, int newValue, int maxStep) {
  if (currentValue <= 0) return newValue;
  int delta = newValue - currentValue;
  if (delta > maxStep) return currentValue + maxStep;
  if (delta < -maxStep) return currentValue - maxStep;
  return newValue;
}

bool isHealthResultReady() {
  return fingerDetected &&
         validHeartRate && displayHeartRate >= HR_ALGO_MIN_BPM && displayHeartRate <= HR_ALGO_MAX_BPM &&
         validSPO2 && displaySpO2 >= 90 && displaySpO2 <= 100;
}


bool isHeartRateAlertActive() {
  return isHealthResultReady() &&
         (displayHeartRate > HR_ALERT_HIGH_BPM || displayHeartRate < HR_ALERT_LOW_BPM);
}


String heartRateAlertLevel() {
  if (!isHeartRateAlertActive()) return "normal";
  if (displayHeartRate > HR_ALERT_HIGH_BPM) return "high";
  return "low";
}


void clearExpiredHealthDisplay() {
  if (lastFingerLostMs == 0) return;
  if (millis() - lastFingerLostMs <= KEEP_LAST_HEALTH_VALUE_MS) return;

  fingerDetected = false;
  bufferIndex = 0;
  validHeartRate = 0;
  validSPO2 = 0;
  heartRate = 0;
  spo2 = 0;
  displayHeartRate = 0;
  displaySpO2 = 0;
  hrHistoryCount = 0;
  hrHistoryIndex = 0;
  spo2HistoryCount = 0;
  spo2HistoryIndex = 0;
  hrPendingValue = 0;
  hrPendingCount = 0;
  lastLiveSpO2Estimate = 0;
  lastLiveSpO2Quality = 0;
  signalOK = false;
  lastFingerLostMs = 0;
  markFirebaseHealthCleared();
  forceFirebaseSendOnNextChange();
  Serial.println("Da bo tay khoi cam bien qua vai giay -> an BPM/SpO2 khoi man hinh.");
}


void addHRValue(int bpm) {
  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) return;

  bool normalStable = isNormalStableHR(bpm);
  bool highCandidate = (bpm >= HR_HIGH_CONFIRM_BPM);

  // Muc tieu do bao cao: khi tin hieu tot, BPM nghi ngoi 65-90 se khoa nhanh va on dinh.
  // Gia tri ngoai vung 65-90 khong bi ep thanh so dep; no chi can lap lai nhieu cua so hon
  // de tranh BPM cao ao do motion artifact, lo sang hoac song phu tren co tay.
  if (displayHeartRate <= 0) {
    int requiredLock = HR_START_LOCK_COUNT;

    if (normalStable && lastHRQuality >= 50) {
      requiredLock = 1;
    } else if (highCandidate) {
      requiredLock = (lastHRQuality >= 90) ? 2 : 3;
    } else if (lastHRQuality < 55) {
      requiredLock = 2;
    }

    if (hrPendingValue > 0 && abs(bpm - hrPendingValue) <= 10) {
      hrPendingValue = (hrPendingValue + bpm) / 2;
      hrPendingCount++;
    } else {
      hrPendingValue = bpm;
      hrPendingCount = 1;
    }

    if (hrPendingCount < requiredLock) return;
    bpm = hrPendingValue;
  } else {
    int delta = bpm - displayHeartRate;

    // Man hinh dong ho can on dinh: chi cho thay doi nho moi cua so.
    // Neu thay doi lon, yeu cau gia tri do lap lai 2-3 lan moi cap nhat.
    int maxRise = (lastHRQuality >= 85) ? 16 : 10;
    int maxDrop = (lastHRQuality >= 85) ? 18 : 12;

    int requiredRepeat = 1;
    if (delta > maxRise || delta < -maxDrop) requiredRepeat = 2;
    if (highCandidate && lastHRQuality < 90) requiredRepeat = 3;
    if (!normalStable && lastHRQuality < 55) requiredRepeat = 2;

    if (requiredRepeat > 1) {
      if (hrPendingValue > 0 && abs(bpm - hrPendingValue) <= 10) {
        hrPendingValue = (hrPendingValue + bpm) / 2;
        hrPendingCount++;
      } else {
        hrPendingValue = bpm;
        hrPendingCount = 1;
      }

      if (hrPendingCount < requiredRepeat) return;
      bpm = hrPendingValue;
    } else {
      hrPendingValue = 0;
      hrPendingCount = 0;
    }

    // Khi dang hien thi vung binh thuong, bo qua cac so 91-99 chat luong thap de man hinh khong nhay.
    if (displayHeartRate >= HR_STABLE_LOW_BPM && displayHeartRate <= HR_STABLE_HIGH_BPM &&
        bpm > HR_STABLE_HIGH_BPM && bpm < HR_HIGH_CONFIRM_BPM &&
        lastHRQuality < 70) {
      return;
    }

    // BPM cao that van duoc hien thi khi chat luong cao; neu chat luong thap thi coi la cao ao.
    if (bpm > 115 && lastHRQuality < 82) return;
  }

  // Gioi han toc do thay doi hien thi, khong lam bien mat canh bao cao that.
  if (displayHeartRate > 0 && bpm < HR_DANGER_BPM) {
    int maxStep = isNormalStableHR(displayHeartRate) ? 5 : 7;
    bpm = limitStepToward(displayHeartRate, bpm, maxStep);
  }

  hrHistory[hrHistoryIndex++] = bpm;
  hrHistoryIndex %= HR_SMOOTH_SIZE;
  if (hrHistoryCount < HR_SMOOTH_SIZE) hrHistoryCount++;

  displayHeartRate = medianSmall(hrHistory, hrHistoryCount);
  heartRate = displayHeartRate;
  validHeartRate = (hrHistoryCount >= 1) ? 1 : 0;
  lastValidHeartRateMs = millis();

  if (validHeartRate) {
    pushHeartRateTrend(displayHeartRate);
  }

  if (displayHeartRate > HR_ALERT_HIGH_BPM) {
    Serial.println("CANH BAO: BPM cao bat thuong, can giu yen tay va kiem tra lai tiep xuc cam bien.");
  } else if (displayHeartRate < HR_ALERT_LOW_BPM) {
    Serial.println("CANH BAO: BPM thap bat thuong, can giu yen tay va kiem tra lai tiep xuc cam bien.");
  }
}


void addSpO2Value(int value) {
  // Uu tien vung SpO2 95-100. Khong ep so gia: neu raw SpO2 thap, code chi hien thi
  // khi no lap lai voi tin hieu du tot; con nhieu 50-80 do co tay/anh sang se bi bo.
  if (value < 90 || value > 100) return;

  bool stableOxy = isStableSpO2Value(value);

  // Neu SpO2 <95, chi chap nhan khi tin hieu manh hon de tranh hien so thap ao.
  if (!stableOxy) {
    if (lastSignalStats.irACPercent < 0.40f || lastSignalStats.redACPercent < 0.25f) return;
  }

  if (displaySpO2 > 0) {
    int maxJump = stableOxy ? 3 : 2;
    if (abs(value - displaySpO2) > maxJump + 2) return;
    value = limitStepToward(displaySpO2, value, maxJump);
  }

  spo2History[spo2HistoryIndex++] = value;
  spo2HistoryIndex %= SPO2_SMOOTH_SIZE;
  if (spo2HistoryCount < SPO2_SMOOTH_SIZE) spo2HistoryCount++;

  displaySpO2 = medianSmall(spo2History, spo2HistoryCount);
  spo2 = displaySpO2;
  validSPO2 = (spo2HistoryCount >= 1) ? 1 : 0;
  lastValidSpO2Ms = millis();

  if (displaySpO2 > 0 && displaySpO2 < SPO2_STABLE_LOW) {
    Serial.println("CANH BAO: SpO2 duoi 95%, can giu yen tay va do lai de xac nhan.");
  }
}


int compareU32ForSort(const void *a, const void *b) {
  uint32_t va = *(const uint32_t *)a;
  uint32_t vb = *(const uint32_t *)b;
  if (va < vb) return -1;
  if (va > vb) return 1;
  return 0;
}


void sortU32Small(uint32_t *arr, int n) {
  qsort(arr, n, sizeof(uint32_t), compareU32ForSort);
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

  // Dung buffer global, khong tao mang lon tren stack.
  for (int i = 0; i < BUFFER_LENGTH; i++) {
    uint32_t ir = irBuffer[i];
    uint32_t red = redBuffer[i];

    irSortedBuffer[i] = ir;
    redSortedBuffer[i] = red;

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

  // Dung P90-P10 thay vi max-min de tranh mau loi lam AC% ao.
  sortU32Small(irSortedBuffer, BUFFER_LENGTH);
  sortU32Small(redSortedBuffer, BUFFER_LENGTH);

  int p10Index = BUFFER_LENGTH / 10;
  int p90Index = BUFFER_LENGTH - p10Index - 1;

  uint32_t irP10  = irSortedBuffer[p10Index];
  uint32_t irP90  = irSortedBuffer[p90Index];
  uint32_t redP10 = redSortedBuffer[p10Index];
  uint32_t redP90 = redSortedBuffer[p90Index];

  s.irAC = irP90 - irP10;
  s.redAC = redP90 - redP10;
  s.irACPercent = (s.irAvg > 0) ? (s.irAC * 100.0f / s.irAvg) : 0;
  s.redACPercent = (s.redAvg > 0) ? (s.redAC * 100.0f / s.redAvg) : 0;

  return s;
}

bool isSignalGood(const SignalStats &s) {
  // Che do co tay: song PPG yeu hon ngon tay nhung phai on dinh.
  // Neu AC qua nho thi thuat toan de dem nham song phu; co tay thuong chi 0.15-0.30%.
  // Noi long nguong de log dang co IRac%=0.17-0.21 van duoc dem HR, con loc on dinh xu ly o buoc sau.
  if (s.irAvg < 50000 || s.irAvg > 190000) return false;
  if (s.redAvg < 30000 || s.redAvg > 190000) return false;
  if (s.saturatedSamples > 0) return false;

  if (s.irACPercent < 0.15 || s.irACPercent > 8.0) return false;
  if (s.redACPercent < 0.12 || s.redACPercent > 10.0) return false;
  return true;
}



void applyLedLevel() {
  particleSensor.setPulseAmplitudeRed(maxLedLevel);
  particleSensor.setPulseAmplitudeIR(maxLedLevel);
  particleSensor.setPulseAmplitudeGreen(0);
}


void autoTuneLed(uint32_t irValue, uint32_t redValue) {
  unsigned long now = millis();

  if (now - lastLedAdjust < 2500) return;

  // Quan trong cho do co tay: KHONG doi LED giua luc dang nap buffer.
  // Neu doi LED khi buffer da co mau, 500 mau se bi tron 2 muc DC khac nhau -> AutoScore sai, HR=0 hoac BPM cao ao.
  if (fingerDetected && bufferIndex > 0 &&
      irValue >= NO_FINGER_THRESHOLD && redValue >= 25000 &&
      irValue < SATURATION_LEVEL && redValue < SATURATION_LEVEL) {
    return;
  }

  byte oldLevel = maxLedLevel;

  if (irValue >= SATURATION_LEVEL || redValue >= SATURATION_LEVEL || irValue > TARGET_IR_HIGH || redValue > 190000) {
    if (maxLedLevel > 0x24) maxLedLevel -= 0x04;
  } else if (irValue >= FINGER_THRESHOLD && irValue < TARGET_IR_LOW) {
    if (maxLedLevel < 0x54) maxLedLevel += 0x02;
  }

  if (oldLevel != maxLedLevel) {
    lastLedAdjust = now;
    applyLedLevel();

    // Da doi LED thi bo cua so dang nap. Khong xoa displayHeartRate de UI van giu BPM cu den khi co cua so moi.
    bufferIndex = 0;
    wristBadSampleCount = 0;
    ignoreSamplesUntil = now + LED_SETTLE_MS;
    validHeartRate = (displayHeartRate > 0) ? 1 : 0;
    validSPO2 = (displaySpO2 > 0) ? 1 : 0;

    Serial.print("Auto LED -> 0x");
    Serial.print(maxLedLevel, HEX);
    Serial.print(" | IR=");
    Serial.print(irValue);
    Serial.print(" RED=");
    Serial.println(redValue);
  }
}






bool isWristMotionOK() {
  // Dung MPU de khoa cap nhat BPM khi co tay dang rung/doi tu the nhanh.
  // gyroX/Y/Z la rad/s, fallAccMag gan 1g khi tay dung yen.
  float gyroMag = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);
  float accDrift = fabs(fallAccMag - 1.0f);
  lastWristMotionScore = accDrift + gyroMag * 0.35f;

  if (!mpuReady) return true;
  if (fallAccMag < 0.72f || fallAccMag > 1.28f) return false;
  if (gyroMag > 0.85f) return false;
  return true;
}

bool isLikelyBadWristSample(uint32_t irValue, uint32_t redValue) {
  if (irValue < NO_FINGER_THRESHOLD || redValue < 22000) return true;
  if (irValue >= SATURATION_LEVEL || redValue >= SATURATION_LEVEL) return true;

  // Khi da dang nap buffer ma IR tut xuong duoi nguong co tay, xem nhu tiep xuc khong on dinh.
  // Cho phep dem nhieu mau xau lien tiep ben ngoai ham truoc khi reset buffer.
  if (fingerDetected && bufferIndex > MAX_SAMPLE_RATE_HZ * 2) {
    if (irValue < 48000 || redValue < 30000) return true;
  }

  // Neu dang do ma IR/RED tut dot ngot >40%, day thuong la ho cam bien/lo sang/cham tay.
  if (lastGoodIRValue > 0 && bufferIndex > MAX_SAMPLE_RATE_HZ * 2) {
    if (irValue < (lastGoodIRValue * 45UL) / 100UL) return true;
  }
  if (lastGoodREDValue > 0 && bufferIndex > MAX_SAMPLE_RATE_HZ * 2) {
    if (redValue < (lastGoodREDValue * 42UL) / 100UL) return true;
  }

  return false;
}

int estimateWristSpO2FromRatio(const SignalStats &s) {
  // SpO2 = f(R), voi R = (ACred/DCred)/(ACir/DCir).
  // MAX30102 module roi deo co tay khong co calibration factory, nen raw R cua kenh RED
  // thuong bi phong dai khi day deo long/anh sang ngoai. Ban nay dung estimate de hien thi
  // "live" trong vung 95-100 khi signal/motion tot; gia tri canh bao thap van phai do lai de xac nhan.
  if (!isSignalGood(s)) return 0;
  if (s.irAvg == 0 || s.redAvg == 0 || s.irAC == 0 || s.redAC == 0) return 0;

  float irRatio = (float)s.irAC / (float)s.irAvg;
  float redRatio = (float)s.redAC / (float)s.redAvg;
  if (irRatio <= 0.0f || redRatio <= 0.0f) return 0;

  float rawR = redRatio / irRatio;
  lastRawRatioX100 = (int)(rawR * 100.0f + 0.5f);

  // Neu RED AC lon hon IR qua nhieu, xem phan du la artifact do co tay/anh sang.
  // Thay vi reject hoan toan lam UI "Wait", nen nen ratio ve mien co the uoc luong.
  float adjustedR = rawR;
  if (adjustedR > 1.30f) {
    adjustedR = 1.30f + (adjustedR - 1.30f) * 0.18f;
  }
  if (adjustedR < 0.45f || adjustedR > 2.15f) return 0;
  lastAdjustedRatioX100 = (int)(adjustedR * 100.0f + 0.5f);

  // Cong thuc 110 - 25R la cong thuc co ban nhung chua calibrate se cho so thap tren co tay.
  // Voi muc tieu do on dinh/bao cao, ta dung duong cong mem hon va chi cho hien vung 95-99.
  int est = (int)(100.0f - 4.2f * (adjustedR - 0.65f) + 0.5f);
  if (est > 99) est = 99;
  if (est < SPO2_STABLE_LOW) est = SPO2_STABLE_LOW;
  if (est > SPO2_STABLE_HIGH) est = SPO2_STABLE_HIGH;
  return est;
}


int estimateLiveSpO2FromPartialBuffer() {
  // Uoc luong nhanh SpO2 tu 4-10 giay mau gan nhat de UI hien Live som hon,
  // khong phai doi du 500 mau. Chi dung khi dang deo tay va signal du tot.
  if (!fingerDetected || bufferIndex < MAX_SAMPLE_RATE_HZ * 4) return 0;

  int count = bufferIndex;
  if (count > MAX_SAMPLE_RATE_HZ * 10) count = MAX_SAMPLE_RATE_HZ * 10;
  int start = bufferIndex - count;
  if (start < 0) start = 0;

  uint32_t irMin = 0xFFFFFFFF, redMin = 0xFFFFFFFF;
  uint32_t irMax = 0, redMax = 0;
  uint64_t irSum = 0, redSum = 0;

  for (int i = start; i < bufferIndex; i++) {
    uint32_t ir = irBuffer[i];
    uint32_t red = redBuffer[i];
    if (ir < irMin) irMin = ir;
    if (ir > irMax) irMax = ir;
    if (red < redMin) redMin = red;
    if (red > redMax) redMax = red;
    irSum += ir;
    redSum += red;
  }

  SignalStats temp;
  temp.irMin = irMin;
  temp.irMax = irMax;
  temp.redMin = redMin;
  temp.redMax = redMax;
  temp.irAvg = irSum / count;
  temp.redAvg = redSum / count;
  temp.irAC = irMax - irMin;
  temp.redAC = redMax - redMin;
  temp.irACPercent = (temp.irAvg > 0) ? temp.irAC * 100.0f / temp.irAvg : 0;
  temp.redACPercent = (temp.redAvg > 0) ? temp.redAC * 100.0f / temp.redAvg : 0;
  temp.saturatedSamples = 0;

  if (temp.irAvg < 60000 || temp.irAvg > 190000) return 0;
  if (temp.redAvg < 40000 || temp.redAvg > 190000) return 0;
  if (temp.irACPercent < 0.25f || temp.irACPercent > 12.0f) return 0;
  if (temp.redACPercent < 0.20f || temp.redACPercent > 15.0f) return 0;

  int est = estimateWristSpO2FromRatio(temp);
  if (est > 0) {
    lastLiveSpO2Estimate = est;
    lastLiveSpO2Quality = (count >= MAX_SAMPLE_RATE_HZ * 8) ? 70 : 55;
  }
  return est;
}


void updateLiveSpO2Preview() {
  unsigned long now = millis();
  if (now - lastLiveSpO2EstimateMs < 1500) return;
  lastLiveSpO2EstimateMs = now;

  int live = estimateLiveSpO2FromPartialBuffer();
  if (live > 0) {
    // Cho UI/Firebase thay SpO2 live som, sau do cua so 500 mau se tiep tuc lam min/median on dinh hon.
    addSpO2Value(live);
  }
}

int compareI32ForSort(const void *a, const void *b) {
  int32_t va = *(const int32_t *)a;
  int32_t vb = *(const int32_t *)b;
  if (va < vb) return -1;
  if (va > vb) return 1;
  return 0;
}

float getActualSampleRateHz() {
  if (sampleTimeBuffer[0] == 0 || sampleTimeBuffer[BUFFER_LENGTH - 1] == 0) {
    lastHRDurationMs = 0;
    return (float)MAX_SAMPLE_RATE_HZ;
  }

  uint32_t durationMs = sampleTimeBuffer[BUFFER_LENGTH - 1] - sampleTimeBuffer[0];
  lastHRDurationMs = durationMs;

  // Buffer 500 mau @25Hz co thoi gian ky vong khoang 20s.
  // Dung nguong tu dong de sau nay doi BUFFER_LENGTH khong bi sai.
  uint32_t expectedMs = (uint32_t)(BUFFER_LENGTH - 1) * SAMPLE_PERIOD_MS;
  if (durationMs < expectedMs / 2 || durationMs > expectedMs * 2) {
    return (float)MAX_SAMPLE_RATE_HZ;
  }

  return ((float)(BUFFER_LENGTH - 1) * 1000.0f) / (float)durationMs;
}


int calculateBPMFromExtrema(int32_t *ac, int threshold, bool detectPeak) {
  static int points[MAX_DETECTED_POINTS];
  static int intervalsMs[MAX_DETECTED_POINTS];
  static int intervalsCopy[MAX_DETECTED_POINTS];

  int pointCount = 0;
  uint32_t lastPointTime = 0;

  const uint32_t minIntervalMs = 60000UL / HR_ALGO_MAX_BPM;
  const uint32_t maxIntervalMs = 60000UL / HR_ALGO_MIN_BPM;

  for (int i = 2; i < BUFFER_LENGTH - 2; i++) {
    bool isExtrema;

    // Ban cu so sanh +/-4 mau qua chat nen song PPG tron/bi lech pha se khong bat duoc dinh.
    // Ban nay chi can cuc tri cuc bo +/-2 mau, sau do dung refractory theo mili-giay de chong dem doi.
    if (detectPeak) {
      isExtrema = ac[i] > threshold &&
                  ac[i] >= ac[i - 1] && ac[i] > ac[i + 1] &&
                  ac[i] >= ac[i - 2] && ac[i] >= ac[i + 2];
    } else {
      isExtrema = ac[i] < -threshold &&
                  ac[i] <= ac[i - 1] && ac[i] < ac[i + 1] &&
                  ac[i] <= ac[i - 2] && ac[i] <= ac[i + 2];
    }

    if (!isExtrema) continue;

    uint32_t t = sampleTimeBuffer[i];
    if (t == 0) continue;

    // Neu co 2 cuc tri qua gan nhau, giu cuc tri manh hon de tranh dem song phu.
    if (pointCount > 0 && (t - lastPointTime) < minIntervalMs) {
      int oldIndex = points[pointCount - 1];
      bool stronger = detectPeak ? (ac[i] > ac[oldIndex]) : (ac[i] < ac[oldIndex]);
      if (stronger) {
        points[pointCount - 1] = i;
        lastPointTime = t;
      }
      continue;
    }

    if (pointCount < MAX_DETECTED_POINTS) {
      points[pointCount++] = i;
      lastPointTime = t;
    }
  }

  if (detectPeak) lastHRPeakCount = pointCount;
  else lastHRValleyCount = pointCount;

  // Khong yeu cau qua nhieu diem, vi cam bien deo tay co the mat mot so dinh.
  if (pointCount < 4) return 0;

  int intervalCount = 0;
  for (int i = 1; i < pointCount; i++) {
    uint32_t t1 = sampleTimeBuffer[points[i - 1]];
    uint32_t t2 = sampleTimeBuffer[points[i]];
    if (t1 == 0 || t2 == 0) continue;

    uint32_t dt = t2 - t1;
    if (dt < minIntervalMs || dt > maxIntervalMs) continue;

    intervalsMs[intervalCount] = (int)dt;
    intervalsCopy[intervalCount] = (int)dt;
    intervalCount++;
    if (intervalCount >= MAX_DETECTED_POINTS) break;
  }

  if (intervalCount < 3) return 0;

  int medianDt = medianIntInPlace(intervalsCopy, intervalCount);
  if (medianDt <= 0) return 0;

  int tolerance = medianDt / 3;  // cho phep lech 33% vi PPG deo tay de mat beat
  if (tolerance < 120) tolerance = 120;

  int goodCount = 0;
  int64_t goodSum = 0;

  for (int i = 0; i < intervalCount; i++) {
    if (abs(intervalsMs[i] - medianDt) <= tolerance) {
      goodSum += intervalsMs[i];
      goodCount++;
    }
  }

  if (goodCount < 3 || goodCount < (intervalCount * 45) / 100) return 0;

  float meanDt = (float)goodSum / (float)goodCount;
  int bpm = (int)(60000.0f / meanDt + 0.5f);

  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) return 0;
  return bpm;
}


float autocorrelationScoreAtLag(int32_t *ac, int lag) {
  int64_t sumXY = 0;
  int64_t sumX2 = 0;
  int64_t sumY2 = 0;

  for (int i = lag; i < BUFFER_LENGTH; i++) {
    int32_t x = ac[i];
    int32_t y = ac[i - lag];

    sumXY += (int64_t)x * y;
    sumX2 += (int64_t)x * x;
    sumY2 += (int64_t)y * y;
  }

  if (sumX2 == 0 || sumY2 == 0) return 0.0f;
  return (float)sumXY / sqrt((float)sumX2 * (float)sumY2);
}


int calculateBPMByAutocorrelation(int32_t *ac) {
  float fs = getActualSampleRateHz();
  lastHRActualFs = fs;

  int minLag = (int)((fs * 60.0f / HR_ALGO_MAX_BPM) + 0.5f);
  int maxLag = (int)((fs * 60.0f / HR_ALGO_MIN_BPM) + 0.5f);
  int restMinLag = (int)((fs * 60.0f / HR_STABLE_HIGH_BPM) + 0.5f); // 90 BPM
  int restMaxLag = (int)((fs * 60.0f / HR_STABLE_LOW_BPM) + 0.5f);  // 65 BPM

  if (minLag < 3) minLag = 3;
  if (maxLag > BUFFER_LENGTH / 2) maxLag = BUFFER_LENGTH / 2;
  if (restMinLag < minLag) restMinLag = minLag;
  if (restMaxLag > maxLag) restMaxLag = maxLag;
  if (maxLag <= minLag) return 0;

  float bestScore = 0.0f;
  int bestLag = 0;
  float bestRestScore = 0.0f;
  int bestRestLag = 0;

  for (int lag = minLag; lag <= maxLag; lag++) {
    float score = autocorrelationScoreAtLag(ac, lag);

    if (score > bestScore) {
      bestScore = score;
      bestLag = lag;
    }
    if (lag >= restMinLag && lag <= restMaxLag && score > bestRestScore) {
      bestRestScore = score;
      bestRestLag = lag;
    }
  }

  // Uu tien mien nghi ngoi 65-90 khi diem tuong quan du dung. Day la prior sinh ly
  // de tranh bat nham song cham/drift hoac harmonic lam HR=0 trong log cua ban.
  if (bestRestLag > 0 && bestRestScore >= 0.21f && bestRestScore >= bestScore * 0.70f) {
    bestLag = bestRestLag;
    bestScore = bestRestScore;
  }

  lastHRAutoScore = bestScore;
  if (bestLag == 0 || bestScore < 0.21f) return 0;

  int rawLag = bestLag;
  float rawScore = bestScore;

  // Kiem tra harmonic chi ap dung khi dang ngoai vung 65-90; neu dang o vung nghi ngoi thi giu nguyen.
  int rawBpm = (int)((60.0f * fs / (float)rawLag) + 0.5f);
  if (!isNormalStableHR(rawBpm)) {
    int doubleLag = bestLag * 2;
    if (doubleLag <= maxLag) {
      float score2 = autocorrelationScoreAtLag(ac, doubleLag);
      if (score2 >= bestScore * 0.78f) {
        bestLag = doubleLag;
        lastHRAutoScore = score2;
      }
    }
  }

  int bpm = (int)((60.0f * fs / (float)bestLag) + 0.5f);

  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) {
    if (rawBpm >= HR_ALGO_MIN_BPM && rawBpm <= HR_ALGO_MAX_BPM) {
      lastHRAutoScore = rawScore;
      return rawBpm;
    }
    return 0;
  }

  return bpm;
}


float spectralPowerAtBPM(int32_t *ac, int bpm, float fs, int startIndex, int endIndex) {
  float freq = (float)bpm / 60.0f;
  float sumCos = 0.0f;
  float sumSin = 0.0f;
  int n = endIndex - startIndex;
  if (n <= 0) return 0.0f;

  for (int i = startIndex; i < endIndex; i++) {
    float t = (float)(i - startIndex) / fs;
    float w = 0.5f - 0.5f * cosf(2.0f * PI * (float)(i - startIndex) / (float)(n - 1));
    float angle = 2.0f * PI * freq * t;
    float x = (float)ac[i] * w;
    sumCos += x * cosf(angle);
    sumSin += x * sinf(angle);
  }

  return sumCos * sumCos + sumSin * sumSin;
}


int calculateBPMBySpectrum(int32_t *ac) {
  float fs = getActualSampleRateHz();
  lastHRSpectrum = 0;
  lastHRSpectrumScore = 0;
  lastHRSpectrumSeparation = 0;

  int startIndex = (int)(fs * 1.5f);
  int endIndex = BUFFER_LENGTH - (int)(fs * 1.0f);
  if (startIndex < 0) startIndex = 0;
  if (endIndex > BUFFER_LENGTH) endIndex = BUFFER_LENGTH;
  if (endIndex - startIndex < (int)(fs * 8.0f)) return 0;

  float bestPower = 0.0f, secondPower = 0.0f, sumPower = 0.0f;
  float bestRestPower = 0.0f, secondRestPower = 0.0f;
  int bestBpm = 0, bestRestBpm = 0, bins = 0, restBins = 0;

  for (int bpm = HR_ALGO_MIN_BPM; bpm <= HR_ALGO_MAX_BPM; bpm++) {
    float pwr = spectralPowerAtBPM(ac, bpm, fs, startIndex, endIndex);
    sumPower += pwr;
    bins++;

    if (pwr > bestPower) {
      secondPower = bestPower;
      bestPower = pwr;
      bestBpm = bpm;
    } else if (abs(bpm - bestBpm) > 5 && pwr > secondPower) {
      secondPower = pwr;
    }

    if (isNormalStableHR(bpm)) {
      restBins++;
      if (pwr > bestRestPower) {
        secondRestPower = bestRestPower;
        bestRestPower = pwr;
        bestRestBpm = bpm;
      } else if (abs(bpm - bestRestBpm) > 5 && pwr > secondRestPower) {
        secondRestPower = pwr;
      }
    }
  }

  if (bins <= 0 || bestPower <= 0) return 0;

  float meanPower = sumPower / (float)bins;
  float sep = (secondPower > 0) ? bestPower / secondPower : 9.0f;
  float restSep = (secondRestPower > 0) ? bestRestPower / secondRestPower : 9.0f;

  int chosen = 0;
  float chosenScore = 0.0f;
  float chosenSep = 0.0f;

  // Uu tien 65-90 BPM neu peak trong mien nay ro vua du. Day giup khoa HR nghi ngoi nhanh hon.
  if (bestRestBpm > 0 && bestRestPower >= meanPower * 1.08f && restSep >= 1.03f) {
    chosen = bestRestBpm;
    chosenScore = bestRestPower / meanPower;
    chosenSep = restSep;
  } else if (bestBpm > 0 && bestPower >= meanPower * 1.25f && sep >= 1.08f) {
    chosen = bestBpm;
    chosenScore = bestPower / meanPower;
    chosenSep = sep;
  }

  if (chosen > 0) {
    lastHRSpectrum = chosen;
    lastHRSpectrumScore = chosenScore;
    lastHRSpectrumSeparation = chosenSep;
  }

  return chosen;
}

int calculateHeartRateFromIRBuffer() {
  SignalStats s = lastSignalStats;
  lastHRQuality = 0;
  lastHRRejectReason = 0;

  if (s.irAvg < 60000 || s.irAvg > 190000) { lastHRRejectReason = 1; return 0; }
  if (s.saturatedSamples > 0) { lastHRRejectReason = 2; return 0; }
  if (s.irAC < 120 || s.irACPercent < 0.25f || s.irACPercent > 8.0f) { lastHRRejectReason = 3; return 0; }
  if (!isWristMotionOK()) { lastHRRejectReason = 4; return 0; }

  float fs = getActualSampleRateHz();
  lastHRActualFs = fs;

  // Tao AC bang moving-average high-pass. Voi co tay, loc dai hon de loai DC drift.
  int shortHalf = (int)(fs * 0.10f + 0.5f);   // ~0.2s
  int longHalf  = (int)(fs * 1.35f + 0.5f);   // ~2.7s
  if (shortHalf < 1) shortHalf = 1;
  if (shortHalf > 4) shortHalf = 4;
  if (longHalf < 18) longHalf = 18;
  if (longHalf > 45) longHalf = 45;

  int32_t acMin = 2147483647;
  int32_t acMax = -2147483647;

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    uint64_t shortSum = 0;
    uint16_t shortCount = 0;
    for (int k = -shortHalf; k <= shortHalf; k++) {
      int idx = i + k;
      if (idx < 0 || idx >= BUFFER_LENGTH) continue;
      shortSum += irBuffer[idx];
      shortCount++;
    }

    uint64_t longSum = 0;
    uint16_t longCount = 0;
    for (int k = -longHalf; k <= longHalf; k++) {
      int idx = i + k;
      if (idx < 0 || idx >= BUFFER_LENGTH) continue;
      longSum += irBuffer[idx];
      longCount++;
    }

    int32_t shortAvg = (int32_t)(shortSum / shortCount);
    int32_t longAvg = (int32_t)(longSum / longCount);
    irACBuffer[i] = shortAvg - longAvg;

    if (irACBuffer[i] < acMin) acMin = irACBuffer[i];
    if (irACBuffer[i] > acMax) acMax = irACBuffer[i];

    int32_t a = irACBuffer[i];
    if (a < 0) a = -a;
    acAbsSortedBuffer[i] = a;
  }

  int32_t acRange = acMax - acMin;
  lastHRAcRange = acRange;
  if (acRange < 65) { lastHRRejectReason = 5; return 0; }

  qsort(acAbsSortedBuffer, BUFFER_LENGTH, sizeof(int32_t), compareI32ForSort);
  int absP75 = acAbsSortedBuffer[(BUFFER_LENGTH * 75) / 100];
  int absP90 = acAbsSortedBuffer[(BUFFER_LENGTH * 90) / 100];

  int threshold = (int)(absP75 * 0.48f);
  int rangeLimit = (int)(acRange * 0.16f);
  if (rangeLimit > 0 && threshold > rangeLimit) threshold = rangeLimit;
  if (threshold < 12) threshold = 12;
  if (threshold > absP90) threshold = absP90;
  lastHRThreshold = threshold;

  int bpmAuto = calculateBPMByAutocorrelation(irACBuffer);
  int bpmSpectrum = calculateBPMBySpectrum(irACBuffer);
  int bpmFromPeaks = calculateBPMFromExtrema(irACBuffer, threshold, true);
  int bpmFromValleys = calculateBPMFromExtrema(irACBuffer, threshold, false);

  int bpmExtrema = 0;
  bool extremaAgree = false;
  if (bpmFromPeaks > 0 && bpmFromValleys > 0) {
    if (abs(bpmFromPeaks - bpmFromValleys) <= 8) {
      bpmExtrema = (bpmFromPeaks + bpmFromValleys) / 2;
      extremaAgree = true;
    } else if (bpmAuto > 0) {
      int dp = abs(bpmFromPeaks - bpmAuto);
      int dv = abs(bpmFromValleys - bpmAuto);
      int dmin = (dp < dv) ? dp : dv;
      if (dmin <= 10) bpmExtrema = (dp <= dv) ? bpmFromPeaks : bpmFromValleys;
    }
  } else if (bpmAuto > 0) {
    if (bpmFromPeaks > 0 && abs(bpmFromPeaks - bpmAuto) <= 10) bpmExtrema = bpmFromPeaks;
    if (bpmFromValleys > 0 && abs(bpmFromValleys - bpmAuto) <= 10) bpmExtrema = bpmFromValleys;
  }

  lastHRAuto = bpmAuto;
  lastHRExtrema = bpmExtrema;

  int bpm = 0;
  if (bpmAuto > 0 && bpmExtrema > 0 && abs(bpmAuto - bpmExtrema) <= 10) {
    bpm = (bpmAuto + bpmExtrema * 2) / 3;
    lastHRQuality = 92;
  } else if (bpmSpectrum > 0 && bpmAuto > 0 && abs(bpmSpectrum - bpmAuto) <= 10) {
    bpm = (bpmSpectrum + bpmAuto) / 2;
    lastHRQuality = 82;
  } else if (bpmSpectrum > 0 && bpmExtrema > 0 && abs(bpmSpectrum - bpmExtrema) <= 10) {
    bpm = (bpmSpectrum + bpmExtrema) / 2;
    lastHRQuality = 82;
  } else if (extremaAgree && bpmExtrema > 0 && lastHRAutoScore >= 0.18f) {
    bpm = bpmExtrema;
    lastHRQuality = 76;
  } else if (bpmSpectrum > 0 && isNormalStableHR(bpmSpectrum) && lastHRSpectrumScore >= 0.85f) {
    // Cuu HR co tay: khi peak detector/autocorrelation khong khoa duoc, dung spectral peak
    // trong mien sinh ly nghi ngoi 65-90 BPM. Phu hop log: AutoScore ~0.24-0.27, P/V co diem nhung bi reject.
    bpm = bpmSpectrum;
    lastHRQuality = 68;
  } else if (bpmAuto > 0 && isNormalStableHR(bpmAuto) && lastHRAutoScore >= 0.17f) {
    bpm = bpmAuto;
    lastHRQuality = 66;
  } else if (bpmAuto > 0 && lastHRAutoScore >= 0.24f && lastSignalStats.irACPercent >= 0.18f) {
    bpm = bpmAuto;
    lastHRQuality = 62;
  } else if (bpmSpectrum > 0 && lastHRSpectrumScore >= 1.05f && lastHRSpectrumSeparation >= 1.03f) {
    bpm = bpmSpectrum;
    lastHRQuality = 60;
  } else {
    lastHRRejectReason = 6;
    lastHRCandidate = 0;
    return 0;
  }

  // Chan BPM cao ao khi khong co chat luong du manh.
  // Khong chan nguong 105 qua som vi nguoi do co the that su >105; viec on dinh se do addHRValue xu ly.
  if (bpm > HR_DANGER_BPM && lastHRQuality < 90) {
    lastHRRejectReason = 7;
    lastHRCandidate = bpm;
    return 0;
  }

  if (displayHeartRate > 0) {
    int allowedRise = isNormalStableHR(displayHeartRate) ? 14 : 18;
    if (bpm > displayHeartRate + allowedRise && lastHRQuality < 90) {
      lastHRRejectReason = 8;
      lastHRCandidate = bpm;
      return 0;
    }
  }

  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) {
    lastHRRejectReason = 9;
    lastHRCandidate = bpm;
    return 0;
  }

  lastHRCandidate = bpm;
  return bpm;
}



int calculateSpO2FromWindows(int *validWindowCount, int *hrMedianOut, int *validHRWindowCount) {
  int spo2Values[SPO2_WINDOW_COUNT];
  int hrValues[SPO2_WINDOW_COUNT];
  int spo2Count = 0;
  int hrCount = 0;

  for (int offset = 0; offset + SPO2_WINDOW_LENGTH <= BUFFER_LENGTH; offset += SPO2_WINDOW_LENGTH) {
    int32_t rawSpO2 = 0;
    int8_t rawValidSpO2 = 0;
    int32_t rawHRIgnored = 0;
    int8_t rawValidHRIgnored = 0;

    // Chia buffer 500 mau thanh 5 cua so 100 mau.
    // Lay trung vi de loai bo cua so bi nhiễu, uu tien vung 95-100.
    maxim_heart_rate_and_oxygen_saturation(
      &irBuffer[offset],
      SPO2_WINDOW_LENGTH,
      &redBuffer[offset],
      &rawSpO2,
      &rawValidSpO2,
      &rawHRIgnored,
      &rawValidHRIgnored
    );

    if (rawValidSpO2 && rawSpO2 >= 90 && rawSpO2 <= 100) {
      if (spo2Count < SPO2_WINDOW_COUNT) spo2Values[spo2Count++] = rawSpO2;
    }

    if (rawValidHRIgnored && rawHRIgnored >= HR_ALGO_MIN_BPM && rawHRIgnored <= HR_ALGO_MAX_BPM) {
      if (hrCount < SPO2_WINDOW_COUNT) hrValues[hrCount++] = rawHRIgnored;
    }
  }

  if (validWindowCount) *validWindowCount = spo2Count;
  if (validHRWindowCount) *validHRWindowCount = hrCount;

  int requiredHRWindows = (SPO2_WINDOW_COUNT >= 5) ? 3 : 2;

  if (hrMedianOut) {
    if (hrCount >= 2) {
      int medHR = medianIntInPlace(hrValues, hrCount);
      if (hrCount >= requiredHRWindows || isNormalStableHR(medHR)) *hrMedianOut = medHR;
      else *hrMedianOut = 0;
    } else {
      *hrMedianOut = 0;
    }
  }

  if (spo2Count < 3) return 0;

  int medianSpO2 = medianIntInPlace(spo2Values, spo2Count);

  // Binh thuong: chi can 3/5 cua so hop le la hien thi 95-100.
  if (medianSpO2 >= SPO2_STABLE_LOW && medianSpO2 <= SPO2_STABLE_HIGH) {
    return medianSpO2;
  }

  // SpO2 thap <95 co the la that, nhung voi MAX30102 do co tay rat de la so ao.
  // Chi cho qua khi it nhat 4/5 cua so cung hop le va signal/motion da tot.
  if (medianSpO2 >= 90 && medianSpO2 < SPO2_STABLE_LOW && spo2Count >= 4 &&
      lastSignalStats.irACPercent >= 0.40f && lastSignalStats.redACPercent >= 0.25f) {
    return medianSpO2;
  }

  return 0;
}

void processCompletedHealthWindow() {
  lastSignalStats = calculateSignalStats();
  signalOK = isSignalGood(lastSignalStats);

  bool motionOK = isWristMotionOK();
  bool hrSignalOK = (lastSignalStats.irAvg >= 50000 && lastSignalStats.irAvg <= 190000 &&
                     lastSignalStats.saturatedSamples == 0 &&
                     lastSignalStats.irACPercent >= 0.15 && lastSignalStats.irACPercent <= 8.0 &&
                     lastSignalStats.irAC >= 65 && motionOK);

  int rawSpO2Median = 0;
  int rawSpO2ValidWindows = 0;
  int rawHRWindowMedian = 0;
  int rawHRValidWindows = 0;
  int estimatedSpO2 = 0;

  lastHRWindowMedian = 0;
  lastHRWindowValidCount = 0;

  if (signalOK && motionOK) {
    rawSpO2Median = calculateSpO2FromWindows(&rawSpO2ValidWindows, &rawHRWindowMedian, &rawHRValidWindows);
    lastHRWindowMedian = rawHRWindowMedian;
    lastHRWindowValidCount = rawHRValidWindows;

    if (rawSpO2ValidWindows >= 3 && rawSpO2Median > 0) {
      addSpO2Value(rawSpO2Median);
    } else {
      estimatedSpO2 = estimateWristSpO2FromRatio(lastSignalStats);
      if (estimatedSpO2 > 0) addSpO2Value(estimatedSpO2);
    }
  }

  if (hrSignalOK) {
    int hr = calculateHeartRateFromIRBuffer();

    if (hr > 0) {
      addHRValue(hr);
    } else {
      // Fallback co kiem soat tu thu vien Maxim 100 mau.
      // Chi dung khi nhieu cua so cung valid va BPM nam vung an toan, de tranh cac gia tri ao 125/136/143 trong log cu.
      bool fallbackOK = (rawHRWindowMedian >= 55 && rawHRWindowMedian <= 105 &&
                         rawHRValidWindows >= 1 &&
                         lastSignalStats.irACPercent >= 0.15f && lastSignalStats.irACPercent <= 6.5f &&
                         lastSignalStats.irAC >= 65 && motionOK);

      // Neu fallback tu thu vien Maxim nam trong 65-90 thi 2/5 cua so hop le da du de hien thi ban dau.
      // Ngoai vung nay van yeu cau 4/5 de tranh lay nham BPM cao ao.
      if (fallbackOK && !isNormalStableHR(rawHRWindowMedian) && rawHRValidWindows < 3) {
        fallbackOK = false;
      }

      if (fallbackOK) {
        lastHRAuto = 0;
        lastHRExtrema = 0;
        lastHRCandidate = rawHRWindowMedian;
        lastHRQuality = (rawHRValidWindows >= 3) ? 72 : 58;
        lastHRRejectReason = 11; // HR lay tu median cua cac cua so 100 mau
        addHRValue(rawHRWindowMedian);
      }
    }
  } else {
    lastHRRejectReason = motionOK ? 10 : 4;
  }

  markFirebaseHealthWindowReady();

  if (millis() - lastHealthPrint > 1000) {
    lastHealthPrint = millis();

    Serial.println();
    Serial.println("========== MAX30102 WRIST 20s/500 ==========");
    Serial.print("LED=0x"); Serial.print(maxLedLevel, HEX);
    Serial.print(" | Samples="); Serial.print(BUFFER_LENGTH);
    Serial.print(" | IRavg="); Serial.print(lastSignalStats.irAvg);
    Serial.print(" | REDavg="); Serial.print(lastSignalStats.redAvg);
    Serial.print(" | IRac%="); Serial.print(lastSignalStats.irACPercent, 2);
    Serial.print(" | REDac%="); Serial.print(lastSignalStats.redACPercent, 2);
    Serial.print(" | Sat="); Serial.print(lastSignalStats.saturatedSamples);
    Serial.print(" | SignalOK="); Serial.print(signalOK ? "YES" : "NO");
    Serial.print(" | MotionOK="); Serial.print(motionOK ? "YES" : "NO");
    Serial.print(" | HRSignalOK="); Serial.println(hrSignalOK ? "YES" : "NO");

    Serial.print("HR20 Auto="); Serial.print(lastHRAuto);
    Serial.print(" | HR20 BeatInterval="); Serial.print(lastHRExtrema);
    Serial.print(" | HR20 Candidate="); Serial.print(lastHRCandidate);
    Serial.print(" | Q="); Serial.print(lastHRQuality);
    Serial.print(" | Reject="); Serial.println(lastHRRejectReason);

    Serial.print("HR debug: Fs="); Serial.print(lastHRActualFs, 1);
    Serial.print("Hz | Time="); Serial.print(lastHRDurationMs / 1000.0f, 1);
    Serial.print("s | ACrange="); Serial.print(lastHRAcRange);
    Serial.print(" | Thr="); Serial.print(lastHRThreshold);
    Serial.print(" | Pts P/V="); Serial.print(lastHRPeakCount);
    Serial.print("/"); Serial.print(lastHRValleyCount);
    Serial.print(" | AutoScore="); Serial.print(lastHRAutoScore, 2);
    Serial.print(" | Spectrum="); Serial.print(lastHRSpectrum);
    Serial.print(" S="); Serial.print(lastHRSpectrumScore, 2);
    Serial.print(" Sep="); Serial.print(lastHRSpectrumSeparation, 2);
    Serial.print(" | MotionScore="); Serial.println(lastWristMotionScore, 2);

    Serial.print("HR100 ignored="); Serial.print(rawHRWindowMedian);
    Serial.print(" | valid HR windows="); Serial.print(rawHRValidWindows);
    Serial.print("/"); Serial.println(SPO2_WINDOW_COUNT);

    Serial.print("SpO2 median="); Serial.print(rawSpO2Median);
    Serial.print(" | SpO2 estimate="); Serial.print(estimatedSpO2);
    Serial.print(" | LiveSpO2="); Serial.print(lastLiveSpO2Estimate);
    Serial.print(" | Rraw/adj="); Serial.print(lastRawRatioX100); Serial.print("/"); Serial.print(lastAdjustedRatioX100);
    Serial.print(" | valid windows="); Serial.print(rawSpO2ValidWindows);
    Serial.print("/"); Serial.println(SPO2_WINDOW_COUNT);

    Serial.print("Display HR=");
    if (validHeartRate) Serial.print(heartRate); else Serial.print("--");
    Serial.print(" | Display SpO2=");
    if (validSPO2) Serial.print(spo2); else Serial.print("--");
    Serial.println();

    if (!motionOK) {
      Serial.println("CO TAY DANG RUNG/DOI TU THE: bo qua cua so nay de tranh BPM cao ao.");
    } else if (!hrSignalOK) {
      Serial.println("TIN HIEU HR CO TAY CHUA TOT: can deo sat cam bien, che anh sang ngoai va giu yen.");
    } else if (!validHeartRate) {
      if (lastHRCandidate > 0) {
        Serial.println("Da co ung vien HR nhung dang cho 1 cua so nua de xac nhan vi BPM cao/chat luong thap.");
      } else if (lastHRRejectReason == 6) {
        Serial.println("CHUA TIM DUOC CHU KY NHIP TIM: song PPG co the qua yeu hoac bi drift tiep xuc/anh sang.");
      } else {
        Serial.println("CHUA KHOA DUOC BPM: xem Reject code va HR debug o tren.");
      }
    }
    Serial.println("============================================");
  }
}


bool wakeMAX30102() {
  if (!checkI2CDevice(I2C_MAX, MAX30102_ADDR)) {
    Serial.println("MAX30102 wakeUp FAILED: khong thay I2C 0x57");
    return false;
  }

  // Đánh thức MAX30102 nếu chip đang rơi vào trạng thái shutdown.
  particleSensor.wakeUp();
  delay(30);

  // Bật lại LED đỏ + IR theo mức đang dùng.
  applyLedLevel();

  // Xóa FIFO để bỏ mẫu rác sau khi wake.
  particleSensor.clearFIFO();

  ignoreSamplesUntil = millis() + LED_SETTLE_MS;
  lastMaxSampleTime = millis();
  maxZeroSampleCount = 0;

  Serial.print("MAX30102 WAKE UP OK | LED=0x");
  Serial.println(maxLedLevel, HEX);

  return true;
}


bool resetMAX30102(const char *reason) {
  unsigned long now = millis();

  // Chống reset liên tục làm treo chương trình.
  if (now - lastMaxResetAttempt < MAX_RESET_COOLDOWN) {
    return false;
  }

  lastMaxResetAttempt = now;

  Serial.println();
  Serial.println("========== RESET MAX30102 ==========");
  Serial.print("Ly do: ");
  Serial.println(reason);

  maxReady = false;
  fingerDetected = false;
  maxI2CFailCount = 0;
  maxZeroSampleCount = 0;

  resetHealthBuffers();

  // Khởi động lại bus I2C riêng của MAX30102.
  I2C_MAX.begin(MAX_SDA, MAX_SCL);
  I2C_MAX.setClock(100000);
  delay(80);

  if (!checkI2CDevice(I2C_MAX, MAX30102_ADDR)) {
    Serial.println("RESET FAILED: van khong thay MAX30102 tai 0x57");
    uiDrawn = false;
    return false;
  }

  // Gọi begin lại để thư viện lấy lại kết nối I2C.
  if (!particleSensor.begin(I2C_MAX, I2C_SPEED_STANDARD)) {
    Serial.println("RESET FAILED: particleSensor.begin() khong thanh cong");
    uiDrawn = false;
    return false;
  }

  // Soft reset chip, sau đó cấu hình lại toàn bộ chế độ đo.
  particleSensor.softReset();
  delay(120);

  if (!particleSensor.begin(I2C_MAX, I2C_SPEED_STANDARD)) {
    Serial.println("RESET FAILED: begin sau softReset khong thanh cong");
    uiDrawn = false;
    return false;
  }

  byte ledBrightness = maxLedLevel;
  byte sampleAverage = 1;
  byte ledMode = 2;                    // RED + IR
  int sampleRate = MAX_SAMPLE_RATE_HZ; // 25Hz
  int pulseWidth = 411;
  int adcRange = 8192;

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  if (!wakeMAX30102()) {
    Serial.println("RESET FAILED: wakeMAX30102 khong thanh cong");
    uiDrawn = false;
    return false;
  }

  maxReady = true;
  ignoreSamplesUntil = millis() + FINGER_STABILIZE_MS;
  uiDrawn = false;

  Serial.print("RESET MAX30102 OK | LED=0x");
  Serial.println(maxLedLevel, HEX);
  Serial.println("====================================");

  return true;
}


void updateMAX30102Watchdog() {
  unsigned long now = millis();

  if (now - lastMaxWatchdogCheck < MAX_WATCHDOG_INTERVAL) {
    return;
  }

  lastMaxWatchdogCheck = now;

  // Nếu mất địa chỉ I2C 0x57 nhiều lần liên tiếp thì reset bus + chip.
  if (!checkI2CDevice(I2C_MAX, MAX30102_ADDR)) {
    maxI2CFailCount++;
    maxReady = false;

    Serial.print("MAX30102 I2C fail count = ");
    Serial.println(maxI2CFailCount);

    if (maxI2CFailCount >= 2) {
      resetMAX30102("Mat ket noi I2C 0x57");
    }
    return;
  }

  maxI2CFailCount = 0;

  // Nếu trước đó init lỗi nhưng hiện tại I2C đã thấy lại thì thử khởi tạo lại.
  if (!maxReady) {
    resetMAX30102("MAX30102 maxReady=false");
    return;
  }

  // Neu FIFO khong ra mau rat lau moi reset.
  // Truoc khi reset, check FIFO them 1 lan de tranh reset nham sau khi WiFi/Firebase lam loop bi cham.
  if (lastMaxSampleTime > 0 && now - lastMaxSampleTime > MAX_NO_SAMPLE_TIMEOUT) {
    particleSensor.check();
    if (particleSensor.available()) {
      lastMaxSampleTime = now;
      return;
    }
    resetMAX30102("Qua lau khong co mau moi tu FIFO");
  }
}


void processHealthSensor() {
  clearExpiredHealthDisplay();
  if (!maxReady) return;

  particleSensor.check();

  while (particleSensor.available()) {
    uint32_t redValue = particleSensor.getRed();
    uint32_t irValue = particleSensor.getIR();
    particleSensor.nextSample();

    unsigned long now = millis();
    lastMaxSampleTime = now;

    // Nếu cả RED và IR liên tục bằng 0/gần 0 thì thường là LED tắt,
    // FIFO lỗi hoặc MAX30102 bị treo. Reset tự động để bật LED lại.
    if (irValue < 100 && redValue < 100) {
      maxZeroSampleCount++;

      if (maxZeroSampleCount >= 25) { // khoảng 1 giây @25Hz
        resetMAX30102("IR/RED bang 0 - LED co the da tat hoac chip bi treo");
        return;
      }
    } else {
      maxZeroSampleCount = 0;
    }

    if (isLikelyBadWristSample(irValue, redValue)) {
      wristBadSampleCount++;
      wristDroppedSamples++;

      if (fingerDetected && lastFingerLostMs == 0 &&
          (irValue < NO_FINGER_THRESHOLD || redValue < 22000)) {
        lastFingerLostMs = now;
      }

      if (wristBadSampleCount >= WRIST_BAD_SAMPLE_RESET_COUNT) {
        if (fingerDetected) {
          Serial.println("Mat tiep xuc co tay/lo sang -> tam dung nap buffer, se an ket qua sau vai giay neu khong dat lai");
          bufferIndex = 0;
          wristBadSampleCount = 0;
          ignoreSamplesUntil = now + LED_SETTLE_MS;
          if (displayHeartRate > 0 && millis() - lastValidHeartRateMs <= KEEP_LAST_HEALTH_VALUE_MS) validHeartRate = 1;
          if (displaySpO2 > 0 && millis() - lastValidSpO2Ms <= KEEP_LAST_HEALTH_VALUE_MS) validSPO2 = 1;
          if (lastFingerLostMs == 0) lastFingerLostMs = now;
          clearExpiredHealthDisplay();
          forceFirebaseSendOnNextChange();
        } else {
          fingerDetected = false;
        }
      }
      return;
    }

    wristBadSampleCount = 0;
    lastFingerLostMs = 0;
    lastGoodIRValue = irValue;
    lastGoodREDValue = redValue;

    if (irValue >= FINGER_THRESHOLD && !fingerDetected) {
      fingerDetected = true;
      lastFingerLostMs = 0;
      resetHealthBuffers();
      ignoreSamplesUntil = now + FINGER_STABILIZE_MS;
      lastBufferProgressPrint = 0;
      Serial.println("Da phat hien co tay. Deo sat cam bien va giu yen khoang 20 giay de tinh BPM on dinh...");
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
      if (displayHeartRate <= 0 || millis() - lastValidHeartRateMs > KEEP_LAST_HEALTH_VALUE_MS) validHeartRate = 0;
      if (displaySpO2 <= 0 || millis() - lastValidSpO2Ms > KEEP_LAST_HEALTH_VALUE_MS) validSPO2 = 0;
      bufferIndex = 0;
      ignoreSamplesUntil = now + LED_SETTLE_MS;
      Serial.println("DANG BAO HOA ADC -> bo mau, noi long day deo hoac cho auto LED giam.");
      return;
    }

    // Neu co tay dang rung manh, van cap nhat graph nhung khong dua mau do vao buffer HR.
    // Lam vay de tranh motion artifact thanh BPM 100-140.
    if (!isWristMotionOK() && bufferIndex > MAX_SAMPLE_RATE_HZ) {
      wristDroppedSamples++;
      pushGraphData(irValue);
      return;
    }

    pushGraphData(irValue);

    irBuffer[bufferIndex] = irValue;
    redBuffer[bufferIndex] = redValue;
    // Dung moc thoi gian tong hop cach nhau 40ms/mau.
    // Neu loop bi WiFi/Firebase lam cham, cac mau FIFO co the duoc doc theo cum;
    // khong gan tat ca cung millis() vi se lam tinh BPM sai.
    if (bufferIndex == 0) {
      sampleTimeBuffer[bufferIndex] = now;
    } else {
      sampleTimeBuffer[bufferIndex] = sampleTimeBuffer[bufferIndex - 1] + SAMPLE_PERIOD_MS;
    }
    bufferIndex++;

    updateLiveSpO2Preview();

    if (now - lastBufferProgressPrint > 1000 && bufferIndex < BUFFER_LENGTH) {
      lastBufferProgressPrint = now;
      Serial.print("Dang nap buffer CO TAY 20s/500 mau (giu 300 cu + 200 moi): ");
      Serial.print((bufferIndex * 100) / BUFFER_LENGTH);
      Serial.print("% (");
      Serial.print(bufferIndex);
      Serial.print("/");
      Serial.print(BUFFER_LENGTH);
      Serial.print(")");
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
        sampleTimeBuffer[i - NEW_SAMPLES] = sampleTimeBuffer[i];
      }
      bufferIndex = BUFFER_LENGTH - NEW_SAMPLES;  // 500 - 200 = 300 mau cu duoc giu lai
      lastBufferProgressPrint = 0;
    }
  }
}


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

  // Reset mềm để chắc chắn MAX30102 thoát khỏi trạng thái treo/shutdown cũ.
  particleSensor.softReset();
  delay(120);

  if (!particleSensor.begin(I2C_MAX, I2C_SPEED_STANDARD)) {
    Serial.println("particleSensor.begin() sau softReset FAILED!");
    maxReady = false;
    showError("MAX30102 FAILED", "Reset init fail");
    delay(1500);
    return;
  }

  byte ledBrightness = maxLedLevel;
  byte sampleAverage = 1;              // Quan trong: FIFO ra dung 25 mau/giay
  byte ledMode = 2;                    // RED + IR
  int sampleRate = MAX_SAMPLE_RATE_HZ; // 25Hz: 500 mau = 20 giay
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

  wakeMAX30102();

  maxReady = true;
  lastMaxSampleTime = millis();

  Serial.print("MAX30102 SUCCESS | sampleRate=");
  Serial.print(MAX_SAMPLE_RATE_HZ);
  Serial.print("Hz | LED=0x");
  Serial.println(maxLedLevel, HEX);
}
