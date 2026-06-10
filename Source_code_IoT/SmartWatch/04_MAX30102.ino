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
  lastHRQuality = 0;
  lastHRRejectReason = 0;
  lastWristMotionScore = 0;
  wristBadSampleCount = 0;
  wristDroppedSamples = 0;
  lastGoodIRValue = 0;
  lastGoodREDValue = 0;

  signalOK = false;
}


// Reset rieng phan thu mau/phan tich HR, nhung GIU gia tri dang hien thi.
// Dung khi mat tiep xuc ngan, doi LED, hoac bo cua so loi.
// Khong nen goi resetHealthBuffers() trong cac truong hop nay vi no lam BPM/SpO2 nhay ve null ngay.
void resetHealthAcquisitionOnly() {
  bufferIndex = 0;

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    irBuffer[i] = 0;
    redBuffer[i] = 0;
    sampleTimeBuffer[i] = 0;
  }

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
  lastHRQuality = 0;
  lastHRRejectReason = 0;
  lastWristMotionScore = 0;

  wristBadSampleCount = 0;
  wristDroppedSamples = 0;
  lastGoodIRValue = 0;
  lastGoodREDValue = 0;

  // Van giu displayHeartRate/displaySpO2 neu con moi, de UI khong bi nhay.
  unsigned long now = millis();
  validHeartRate = (displayHeartRate > 0 && (now - lastValidHeartRateMs <= KEEP_LAST_HEALTH_VALUE_MS)) ? 1 : 0;
  validSPO2 = (displaySpO2 > 0 && (now - lastValidSpO2Ms <= KEEP_LAST_HEALTH_VALUE_MS)) ? 1 : 0;

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


void addHRValue(int bpm) {
  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) return;

  // Muc tieu: sau khi du 500 mau, neu tin hieu tot thi hien thi nhanh.
  // Neu BPM cao hoac chat luong trung binh/thap, bat xac nhan 2 cua so de tranh nhiem song phu.
  if (displayHeartRate <= 0) {
    int requiredLock = (bpm > 105 || lastHRQuality < 65) ? 2 : 1;

    if (requiredLock > 1) {
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
      hrPendingValue = 0;
      hrPendingCount = 0;
    }
  } else {
    int delta = bpm - displayHeartRate;

    // Khi da co BPM hien thi, khong cho nhay qua nhanh.
    // Neu cua so moi lech nhieu, yeu cau no lap lai 2 lan moi cap nhat.
    int maxRise = (lastHRQuality >= 85) ? 18 : 12;
    int maxDrop = (lastHRQuality >= 85) ? 22 : 16;

    if (delta > maxRise || delta < -maxDrop) {
      if (hrPendingValue > 0 && abs(bpm - hrPendingValue) <= 10) {
        hrPendingValue = (hrPendingValue + bpm) / 2;
        hrPendingCount++;
      } else {
        hrPendingValue = bpm;
        hrPendingCount = 1;
      }

      if (hrPendingCount < 2) return;
      bpm = hrPendingValue;
    } else {
      hrPendingValue = 0;
      hrPendingCount = 0;
    }

    // HR cao that van cho phep, nhung HR >115 voi chat luong thap thuong la dem doi/song phu.
    if (bpm > 115 && lastHRQuality < 80) return;
  }

  hrHistory[hrHistoryIndex++] = bpm;
  hrHistoryIndex %= HR_SMOOTH_SIZE;
  if (hrHistoryCount < HR_SMOOTH_SIZE) hrHistoryCount++;

  displayHeartRate = medianSmall(hrHistory, hrHistoryCount);
  heartRate = displayHeartRate;
  validHeartRate = (hrHistoryCount >= 1) ? 1 : 0;

  if (validHeartRate) {
    lastValidHeartRateMs = millis();
    pushHeartRateTrend(displayHeartRate);
  }
}


void addSpO2Value(int value) {
  // MAX30102 de bi ra SpO2 ao 50-80 khi tay chua on dinh/AC qua yeu.
  // Du an dong ho nen chi hien thi khi gia tri nam trong vung hop ly.
  if (value < 88 || value > 100) return;

  if (displaySpO2 > 0 && abs(value - displaySpO2) > 8) return; // Tang nguong cho phep dao dong nhẹ

  spo2History[spo2HistoryIndex++] = value;
  spo2HistoryIndex %= SPO2_SMOOTH_SIZE;
  if (spo2HistoryCount < SPO2_SMOOTH_SIZE) spo2HistoryCount++;

  int medianVal = medianSmall(spo2History, spo2HistoryCount);

  // Ap dung bo loc EMA de chong "nhay so"
  static float emaSpO2 = 0;
  if (displaySpO2 == 0) {
    emaSpO2 = medianVal; // Khoi tao ban dau
  } else {
    // Chon he so thuat toan EMA dua vao chat luong tin hieu
    float alpha = 0.15f; 
    if (lastSignalStats.irACPercent > 1.0f) {
      alpha = 0.3f; // Tin hieu manh, tin tuong hon
    }
    emaSpO2 = (alpha * medianVal) + ((1.0f - alpha) * emaSpO2);
  }

  displaySpO2 = (int)(emaSpO2 + 0.5f);
  spo2 = displaySpO2;
  validSPO2 = (spo2HistoryCount >= 1) ? 1 : 0;
  if (validSPO2) lastValidSpO2Ms = millis();
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
  // Neu AC qua nho (<0.25%) thi thuat toan se de dem nham song phu; neu qua lon thuong la rung/lo sang.
  if (s.irAvg < 60000 || s.irAvg > 190000) return false;
  if (s.redAvg < 40000 || s.redAvg > 190000) return false;
  if (s.saturatedSamples > 0) return false;

  if (s.irACPercent < 0.25 || s.irACPercent > 8.0) return false;
  if (s.redACPercent < 0.20 || s.redACPercent > 10.0) return false;
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

  if (irValue >= SATURATION_LEVEL || redValue >= SATURATION_LEVEL) {
    if (maxLedLevel > 0x14) maxLedLevel -= 0x10; // Giam manh ngay lap tuc de thoat bao hoa
  } else if (irValue > TARGET_IR_HIGH || redValue > 190000) {
    if (maxLedLevel > 0x24) maxLedLevel -= 0x04;
  } else if (irValue >= FINGER_THRESHOLD && irValue < TARGET_IR_LOW) {
    if (irValue < TARGET_IR_LOW / 2) {
      if (maxLedLevel < 0x50) maxLedLevel += 0x08; // Tang nhanh neu tin hieu qua yeu
    } else {
      if (maxLedLevel < 0x54) maxLedLevel += 0x02; // Tang cham khi gan den dich
    }
  }

  if (oldLevel != maxLedLevel) {
    lastLedAdjust = now;
    applyLedLevel();

    // Da doi LED thi bo cua so dang nap. Khong xoa displayHeartRate de UI van giu BPM cu den khi co cua so moi.
    resetHealthAcquisitionOnly();
    ignoreSamplesUntil = now + LED_SETTLE_MS;

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
  if (irValue < NO_FINGER_THRESHOLD || redValue < 25000) return true;
  if (irValue >= SATURATION_LEVEL || redValue >= SATURATION_LEVEL) return true;

  // Khi da dang nap buffer ma IR tut xuong duoi nguong co tay, xem nhu tiep xuc khong on dinh.
  // Cho phep dem nhieu mau xau lien tiep ben ngoai ham truoc khi reset buffer.
  if (fingerDetected && bufferIndex > MAX_SAMPLE_RATE_HZ * 2) {
    if (irValue < FINGER_THRESHOLD || redValue < 35000) return true;
  }

  // Neu dang do ma IR/RED tut dot ngot >40%, day thuong la ho cam bien/lo sang/cham tay.
  if (lastGoodIRValue > 0 && bufferIndex > MAX_SAMPLE_RATE_HZ * 2) {
    if (irValue < (lastGoodIRValue * 60UL) / 100UL) return true;
  }
  if (lastGoodREDValue > 0 && bufferIndex > MAX_SAMPLE_RATE_HZ * 2) {
    if (redValue < (lastGoodREDValue * 55UL) / 100UL) return true;
  }

  return false;
}

int estimateWristSpO2FromRatio(const SignalStats &s) {
  // SpO2 o co tay bang MAX30102 module roi rat kho chinh xac neu chua hieu chuan.
  if (!isSignalGood(s)) return 0;
  if (s.irAvg == 0 || s.redAvg == 0 || s.irAC == 0 || s.redAC == 0) return 0;

  float irRatio = (float)s.irAC / (float)s.irAvg;
  float redRatio = (float)s.redAC / (float)s.redAvg;
  if (irRatio <= 0.0f) return 0;

  float R = redRatio / irRatio;

  if (R < 0.40f || R > 1.50f) return 0;

  // Cong thuc bac 2 cho MAX30102 de tang do chinh xac
  float spo2Float = -45.060f * (R * R) + 30.354f * R + 94.845f;
  
  int est = (int)(spo2Float + 0.5f);
  if (est < 88 || est > 100) return 0;
  if (est > 99) est = 99;
  return est;
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
  if (pointCount < 5) return 0;

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

  if (intervalCount < 4) return 0;

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

  if (goodCount < 4 || goodCount < (intervalCount * 45) / 100) return 0;

  float meanDt = (float)goodSum / (float)goodCount;
  int bpm = (int)(60000.0f / meanDt + 0.5f);

  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) return 0;
  return bpm;
}



int calculateBPMFromZeroCrossings(int32_t *ac, int threshold) {
  static int intervalsMs[MAX_DETECTED_POINTS];
  static int intervalsCopy[MAX_DETECTED_POINTS];

  const uint32_t minIntervalMs = 60000UL / HR_ALGO_MAX_BPM;
  const uint32_t maxIntervalMs = 60000UL / HR_ALGO_MIN_BPM;

  int start = MAX_SAMPLE_RATE_HZ;
  int end = BUFFER_LENGTH - MAX_SAMPLE_RATE_HZ;
  if (start < 3) start = 3;
  if (end > BUFFER_LENGTH - 3) end = BUFFER_LENGTH - 3;

  bool armed = false;
  uint32_t lastCrossTime = 0;
  int intervalCount = 0;

  int armThreshold = threshold;
  if (armThreshold < 24) armThreshold = 24;

  for (int i = start + 1; i < end; i++) {
    // Phải đi xuống vùng âm trước rồi mới tính lần cắt lên 0.
    // Điều này giúp mỗi nhịp chỉ đếm 1 lần, không bị nhiễu nhỏ quanh 0.
    if (ac[i] < -armThreshold) {
      armed = true;
    }

    if (!armed) continue;

    bool risingCross = (ac[i - 1] < 0 && ac[i] >= 0 && ac[i] > ac[i - 1]);
    if (!risingCross) continue;

    uint32_t t = sampleTimeBuffer[i];
    if (t == 0) continue;

    if (lastCrossTime > 0) {
      uint32_t dt = t - lastCrossTime;

      if (dt < minIntervalMs) {
        // Nhiễu cắt 0 quá gần, bỏ qua.
        armed = false;
        continue;
      }

      if (dt <= maxIntervalMs && intervalCount < MAX_DETECTED_POINTS) {
        intervalsMs[intervalCount] = (int)dt;
        intervalsCopy[intervalCount] = (int)dt;
        intervalCount++;
      }
    }

    lastCrossTime = t;
    armed = false;
  }

  if (intervalCount < 3) return 0;

  int medianDt = medianIntInPlace(intervalsCopy, intervalCount);
  if (medianDt <= 0) return 0;

  int tolerance = medianDt / 3;
  if (tolerance < 140) tolerance = 140;

  int goodCount = 0;
  int64_t goodSum = 0;

  for (int i = 0; i < intervalCount; i++) {
    if (abs(intervalsMs[i] - medianDt) <= tolerance) {
      goodSum += intervalsMs[i];
      goodCount++;
    }
  }

  if (goodCount < 3) return 0;

  float meanDt = (float)goodSum / (float)goodCount;
  int bpm = (int)(60000.0f / meanDt + 0.5f);

  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) return 0;
  return bpm;
}

float autocorrelationScoreAtLag(int32_t *ac, int lag) {
  // Tính tương quan có trừ mean theo từng lag.
  // Bản cũ cộng trực tiếp x*y nên khi tín hiệu cổ tay bị trôi DC/áp lực, score dễ rơi về 0
  // dù vẫn còn chu kỳ tim. Bỏ 1 giây đầu/cuối để tránh biên lọc và tiếp xúc.
  int start = MAX_SAMPLE_RATE_HZ;
  int end = BUFFER_LENGTH - MAX_SAMPLE_RATE_HZ;

  if (start < 2) start = 2;
  if (end > BUFFER_LENGTH - 2) end = BUFFER_LENGTH - 2;
  if (start + lag >= end) {
    start = 2;
    end = BUFFER_LENGTH - 2;
  }

  int count = 0;
  int64_t sumX = 0;
  int64_t sumY = 0;

  for (int i = start + lag; i < end; i++) {
    sumX += ac[i];
    sumY += ac[i - lag];
    count++;
  }

  if (count < 30) return 0.0f;

  float meanX = (float)sumX / (float)count;
  float meanY = (float)sumY / (float)count;

  double sumXY = 0.0;
  double sumX2 = 0.0;
  double sumY2 = 0.0;

  for (int i = start + lag; i < end; i++) {
    float x = (float)ac[i] - meanX;
    float y = (float)ac[i - lag] - meanY;

    sumXY += (double)x * (double)y;
    sumX2 += (double)x * (double)x;
    sumY2 += (double)y * (double)y;
  }

  if (sumX2 <= 1.0 || sumY2 <= 1.0) return 0.0f;
  float score = (float)(sumXY / sqrt(sumX2 * sumY2));

  // Chỉ dùng tương quan dương. Tương quan âm thường là nửa chu kỳ, không phải chu kỳ tim.
  if (score < 0.0f) score = 0.0f;
  return score;
}


int calculateBPMByAutocorrelation(int32_t *ac) {
  float fs = getActualSampleRateHz();
  lastHRActualFs = fs;

  int minLag = (int)ceil((fs * 60.0f) / (float)HR_ALGO_MAX_BPM);
  int maxLag = (int)floor((fs * 60.0f) / (float)HR_ALGO_MIN_BPM);

  if (minLag < 3) minLag = 3;
  if (maxLag > BUFFER_LENGTH / 2) maxLag = BUFFER_LENGTH / 2;
  if (maxLag <= minLag) return 0;

  float bestAdjusted = 0.0f;
  float bestScore = 0.0f;
  int bestLag = 0;

  for (int lag = minLag; lag <= maxLag; lag++) {
    float score = autocorrelationScoreAtLag(ac, lag);
    if (score <= 0.0f) continue;

    int bpmTest = (int)((60.0f * fs / (float)lag) + 0.5f);
    if (bpmTest < HR_ALGO_MIN_BPM || bpmTest > HR_ALGO_MAX_BPM) continue;

    float adjusted = score;

    // Ưu tiên vùng nhịp tim nghỉ/đo cổ tay 50-115 khi chưa khóa BPM.
    // Không ép chết ngoài vùng này, chỉ giảm điểm để tránh bắt nhiễu tần số cao thành 120-135.
    if (displayHeartRate > 0) {
      int diff = abs(bpmTest - displayHeartRate);
      if (diff > 35) adjusted *= 0.55f;
      else if (diff > 25) adjusted *= 0.72f;
      else if (diff > 18) adjusted *= 0.86f;
    } else {
      if (bpmTest < 50 || bpmTest > 120) adjusted *= 0.72f;
      else if (bpmTest < 55 || bpmTest > 115) adjusted *= 0.88f;
      if (bpmTest >= 60 && bpmTest <= 100) adjusted *= 1.05f;
    }

    if (adjusted > bestAdjusted) {
      bestAdjusted = adjusted;
      bestScore = score;
      bestLag = lag;
    }
  }

  if (bestLag == 0) {
    lastHRAutoScore = 0.0f;
    return 0;
  }

  // Sửa lỗi bắt harmonic: nếu lag x2 hoặc x3 vẫn có tương quan đủ mạnh,
  // chọn chu kỳ dài hơn vì PPG cổ tay hay có 2 đỉnh nhỏ trong 1 nhịp.
  int chosenLag = bestLag;
  float chosenScore = bestScore;

  int doubleLag = bestLag * 2;
  if (doubleLag <= maxLag) {
    float score2 = autocorrelationScoreAtLag(ac, doubleLag);
    int bpm2 = (int)((60.0f * fs / (float)doubleLag) + 0.5f);
    if (bpm2 >= HR_ALGO_MIN_BPM && bpm2 <= HR_ALGO_MAX_BPM &&
        score2 >= bestScore * 0.62f) {
      chosenLag = doubleLag;
      chosenScore = score2;
    }
  }

  int tripleLag = bestLag * 3;
  if (tripleLag <= maxLag) {
    float score3 = autocorrelationScoreAtLag(ac, tripleLag);
    int bpm3 = (int)((60.0f * fs / (float)tripleLag) + 0.5f);
    if (bpm3 >= HR_ALGO_MIN_BPM && bpm3 <= HR_ALGO_MAX_BPM &&
        score3 >= bestScore * 0.78f && score3 >= chosenScore * 0.90f) {
      chosenLag = tripleLag;
      chosenScore = score3;
    }
  }

  int bpm = (int)((60.0f * fs / (float)chosenLag) + 0.5f);
  lastHRAutoScore = chosenScore;

  if (bpm < HR_ALGO_MIN_BPM || bpm > HR_ALGO_MAX_BPM) return 0;

  // Tín hiệu rất yếu cần score cao hơn; nếu đã có BPM cũ thì cho phép bám nhẹ để không mất số.
  float minScore = 0.24f;
  if (lastSignalStats.irACPercent < 0.35f) minScore = 0.34f;
  if (lastSignalStats.irACPercent >= 1.0f) minScore = 0.20f;
  if (displayHeartRate > 0) minScore -= 0.04f;
  if (minScore < 0.18f) minScore = 0.18f;

  if (chosenScore < minScore) return 0;

  return bpm;
}

int calculateHeartRateFromIRBuffer() {
  SignalStats s = lastSignalStats;
  lastHRQuality = 0;
  lastHRRejectReason = 0;

  // Nới nhẹ biên đo cổ tay. Log của bạn có IRavg ~86k-105k là vùng có thể đo được.
  if (s.irAvg < 52000 || s.irAvg > 210000) { lastHRRejectReason = 1; return 0; }
  if (s.saturatedSamples > 0) { lastHRRejectReason = 2; return 0; }
  if (s.irAC < 70 || s.irACPercent < 0.18f || s.irACPercent > 10.0f) { lastHRRejectReason = 3; return 0; }
  if (!isWristMotionOK()) { lastHRRejectReason = 4; return 0; }

  float fs = getActualSampleRateHz();
  lastHRActualFs = fs;

  // Lọc PPG cổ tay kiểu robust:
  // 1) trừ trung bình cục bộ ~3 giây để loại drift do dây đeo/áp lực/ánh sáng.
  // 2) làm mượt 5 điểm để giảm nhiễu cao tần.
  // Cách này ổn định hơn bộ IIR cũ, vì IIR cũ tạo ACrange rất lớn nhưng AutoScore=0 khi DC bị trôi.
  const int trendHalfWindow = MAX_SAMPLE_RATE_HZ * 3 / 2; // ~1.5s mỗi bên
  const int edgeIgnore = MAX_SAMPLE_RATE_HZ;              // bỏ 1s đầu/cuối khi phân tích

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    int start = i - trendHalfWindow;
    int end = i + trendHalfWindow;
    if (start < 0) start = 0;
    if (end >= BUFFER_LENGTH) end = BUFFER_LENGTH - 1;

    uint64_t localSum = 0;
    int count = 0;

    for (int j = start; j <= end; j++) {
      localSum += irBuffer[j];
      count++;
    }

    uint32_t localMean = (count > 0) ? (uint32_t)(localSum / count) : s.irAvg;
    irACBuffer[i] = (int32_t)irBuffer[i] - (int32_t)localMean;
  }

  int32_t acMin = 2147483647;
  int32_t acMax = -2147483647;

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    int32_t y;

    if (i >= 2 && i < BUFFER_LENGTH - 2) {
      y = (irACBuffer[i - 2] +
           2 * irACBuffer[i - 1] +
           3 * irACBuffer[i] +
           2 * irACBuffer[i + 1] +
           irACBuffer[i + 2]) / 9;
    } else {
      y = irACBuffer[i];
    }

    acAbsSortedBuffer[i] = y;
  }

  for (int i = 0; i < BUFFER_LENGTH; i++) {
    irACBuffer[i] = acAbsSortedBuffer[i];

    if (i >= edgeIgnore && i < BUFFER_LENGTH - edgeIgnore) {
      if (irACBuffer[i] < acMin) acMin = irACBuffer[i];
      if (irACBuffer[i] > acMax) acMax = irACBuffer[i];
    }

    int32_t a = irACBuffer[i];
    if (a < 0) a = -a;
    acAbsSortedBuffer[i] = a;
  }

  int32_t acRange = acMax - acMin;
  lastHRAcRange = acRange;
  if (acRange < 70) { lastHRRejectReason = 5; return 0; }

  qsort(acAbsSortedBuffer, BUFFER_LENGTH, sizeof(int32_t), compareI32ForSort);
  int absP60 = acAbsSortedBuffer[(BUFFER_LENGTH * 60) / 100];
  int absP75 = acAbsSortedBuffer[(BUFFER_LENGTH * 75) / 100];
  int absP90 = acAbsSortedBuffer[(BUFFER_LENGTH * 90) / 100];

  int threshold = (int)(absP75 * 0.55f);
  int threshold2 = (int)(absP90 * 0.22f);
  if (threshold < threshold2) threshold = threshold2;
  if (threshold < (int)(absP60 * 0.75f)) threshold = (int)(absP60 * 0.75f);

  int rangeLimit = (int)(acRange * 0.24f);
  if (rangeLimit > 0 && threshold > rangeLimit) threshold = rangeLimit;

  if (threshold < 24) threshold = 24;
  if (threshold > absP90) threshold = absP90;
  if (threshold < 1) threshold = 1;

  lastHRThreshold = threshold;

  int bpmAuto = calculateBPMByAutocorrelation(irACBuffer);
  int bpmFromPeaks = calculateBPMFromExtrema(irACBuffer, threshold, true);
  int bpmFromValleys = calculateBPMFromExtrema(irACBuffer, threshold, false);
  int bpmZero = calculateBPMFromZeroCrossings(irACBuffer, threshold);

  int bpmExtrema = 0;
  bool extremaAgree = false;

  if (bpmFromPeaks > 0 && bpmFromValleys > 0) {
    if (abs(bpmFromPeaks - bpmFromValleys) <= 12) {
      bpmExtrema = (bpmFromPeaks + bpmFromValleys) / 2;
      extremaAgree = true;
    } else if (bpmAuto > 0) {
      int dp = abs(bpmFromPeaks - bpmAuto);
      int dv = abs(bpmFromValleys - bpmAuto);
      int dz = (bpmZero > 0) ? abs(bpmZero - bpmAuto) : 999;
      int dmin = dp;
      bpmExtrema = bpmFromPeaks;
      if (dv < dmin) { dmin = dv; bpmExtrema = bpmFromValleys; }
      if (dz < dmin) { dmin = dz; bpmExtrema = bpmZero; }
      if (dmin > 14) bpmExtrema = 0;
    }
  } else if (bpmAuto > 0) {
    if (bpmFromPeaks > 0 && abs(bpmFromPeaks - bpmAuto) <= 14) bpmExtrema = bpmFromPeaks;
    if (bpmFromValleys > 0 && abs(bpmFromValleys - bpmAuto) <= 14) bpmExtrema = bpmFromValleys;
    if (bpmZero > 0 && abs(bpmZero - bpmAuto) <= 14) bpmExtrema = bpmZero;
  } else if (bpmZero > 0) {
    if (bpmFromPeaks > 0 && abs(bpmFromPeaks - bpmZero) <= 14) bpmExtrema = (bpmFromPeaks + bpmZero) / 2;
    else if (bpmFromValleys > 0 && abs(bpmFromValleys - bpmZero) <= 14) bpmExtrema = (bpmFromValleys + bpmZero) / 2;
    else bpmExtrema = bpmZero;
  }

  lastHRAuto = bpmAuto;
  lastHRExtrema = bpmExtrema;

  int bpm = 0;

  if (bpmAuto > 0 && bpmExtrema > 0 && abs(bpmAuto - bpmExtrema) <= 14) {
    bpm = (bpmAuto + bpmExtrema * 2) / 3;
    lastHRQuality = 88;
  } else if (extremaAgree && bpmExtrema > 0 && lastHRAutoScore >= 0.18f) {
    bpm = bpmExtrema;
    lastHRQuality = 76;
  } else if (bpmAuto > 0 && lastHRAutoScore >= 0.26f) {
    bpm = bpmAuto;
    lastHRQuality = 72;
  } else if (bpmAuto > 0 && lastHRAutoScore >= 0.20f &&
             lastSignalStats.irACPercent >= 0.45f &&
             bpmAuto >= 50 && bpmAuto <= 120) {
    // Cứu HR cổ tay: signal tốt nhưng đỉnh/valley khó bắt do PPG tròn/yếu.
    bpm = bpmAuto;
    lastHRQuality = 64;
  } else if (bpmExtrema > 0 && lastSignalStats.irACPercent >= 0.50f) {
    bpm = bpmExtrema;
    lastHRQuality = 62;
  } else {
    lastHRRejectReason = 6;
    lastHRCandidate = 0;
    return 0;
  }

  // Chặn BPM cao ảo khi chất lượng chưa đủ.
  if (bpm > 125 && lastHRQuality < 82) {
    lastHRRejectReason = 7;
    lastHRCandidate = bpm;
    return 0;
  }

  if (displayHeartRate > 0) {
    if (bpm > displayHeartRate + 18 && lastHRQuality < 82) {
      lastHRRejectReason = 8;
      lastHRCandidate = bpm;
      return 0;
    }
    if (bpm < displayHeartRate - 22 && lastHRQuality < 82) {
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

    // Khong goi thu vien Maxim voi ca buffer vi ban goc thuong cap phat mang noi bo 100 mau.
    // Chia buffer 20s thanh 5 cua so 100 mau, lay trung vi SpO2 hop le.
    // HR tu Maxim chi dung lam fallback khi thuat toan buffer dai khong bat duoc chu ky.
    maxim_heart_rate_and_oxygen_saturation(
      &irBuffer[offset],
      SPO2_WINDOW_LENGTH,
      &redBuffer[offset],
      &rawSpO2,
      &rawValidSpO2,
      &rawHRIgnored,
      &rawValidHRIgnored
    );

    if (rawValidSpO2 && rawSpO2 >= 88 && rawSpO2 <= 100) {
      if (spo2Count < SPO2_WINDOW_COUNT) spo2Values[spo2Count++] = rawSpO2;
    }

    if (rawValidHRIgnored && rawHRIgnored >= HR_ALGO_MIN_BPM && rawHRIgnored <= HR_ALGO_MAX_BPM) {
      if (hrCount < SPO2_WINDOW_COUNT) hrValues[hrCount++] = rawHRIgnored;
    }
  }

  if (validWindowCount) *validWindowCount = spo2Count;
  if (validHRWindowCount) *validHRWindowCount = hrCount;

  if (hrMedianOut) {
    *hrMedianOut = 0;

    // Với đo cổ tay, thư viện Maxim 100 mẫu thường chỉ valid 2/5 cửa sổ.
    // Nếu 2 cửa sổ này gần nhau thì vẫn có thể dùng làm fallback có kiểm soát.
    if (hrCount >= 3) {
      *hrMedianOut = medianIntInPlace(hrValues, hrCount);
    } else if (hrCount == 2) {
      if (abs(hrValues[0] - hrValues[1]) <= 18) {
        *hrMedianOut = (hrValues[0] + hrValues[1]) / 2;
      }
    }
  }

  if (spo2Count < 2) return 0;
  return medianIntInPlace(spo2Values, spo2Count);
}

void processCompletedHealthWindow() {
  lastSignalStats = calculateSignalStats();
  signalOK = isSignalGood(lastSignalStats);

  bool motionOK = isWristMotionOK();
  bool hrSignalOK = (lastSignalStats.irAvg >= 60000 && lastSignalStats.irAvg <= 190000 &&
                     lastSignalStats.saturatedSamples == 0 &&
                     lastSignalStats.irACPercent >= 0.25 && lastSignalStats.irACPercent <= 8.0 &&
                     lastSignalStats.irAC >= 120 && motionOK);

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
      bool fallbackOK = (rawHRWindowMedian >= 50 && rawHRWindowMedian <= 118 &&
                         rawHRValidWindows >= 2 &&
                         // Đo cổ tay thường chỉ đạt valid HR windows=2/5.
                         // Cho dùng 2/5 nếu hai cửa sổ gần nhau và tín hiệu IR đủ sạch.
                         lastSignalStats.irACPercent >= 0.25f && lastSignalStats.irACPercent <= 6.5f &&
                         lastSignalStats.irAC >= 90 && motionOK);

      if (fallbackOK && rawHRValidWindows == 2) {
        if (displayHeartRate > 0) {
          fallbackOK = (abs(rawHRWindowMedian - displayHeartRate) <= 18);
        } else {
          // Lần khóa đầu tiên với 2/5 chỉ nhận vùng nghỉ tương đối an toàn để tránh HR ảo.
          fallbackOK = (rawHRWindowMedian >= 55 && rawHRWindowMedian <= 105);
        }
      }

      if (fallbackOK) {
        lastHRAuto = 0;
        lastHRExtrema = 0;
        lastHRCandidate = rawHRWindowMedian;
        lastHRQuality = (rawHRValidWindows >= 4) ? 72 : 65;
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
    Serial.print(" | MotionScore="); Serial.println(lastWristMotionScore, 2);

    Serial.print("HR100 ignored="); Serial.print(rawHRWindowMedian);
    Serial.print(" | valid HR windows="); Serial.print(rawHRValidWindows);
    Serial.print("/"); Serial.println(SPO2_WINDOW_COUNT);

    Serial.print("SpO2 median="); Serial.print(rawSpO2Median);
    Serial.print(" | SpO2 estimate="); Serial.print(estimatedSpO2);
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

      // Khong reset qua som: co tay rat de bi nhiem 0.2-0.5s khi day deo/dong tac nho.
      if (wristBadSampleCount >= WRIST_BAD_SAMPLE_RESET_COUNT) {
        if (fingerDetected) {
          Serial.println("Mat tiep xuc co tay/lo sang >1s -> reset buffer, giu BPM cu tam thoi");
          resetHealthAcquisitionOnly();
          fingerDetected = false;
          ignoreSamplesUntil = now + FINGER_STABILIZE_MS;
          forceFirebaseSendOnNextChange();
        } else {
          fingerDetected = false;
        }
      }
      return;
    }

    wristBadSampleCount = 0;
    lastGoodIRValue = irValue;
    lastGoodREDValue = redValue;

    if (irValue >= FINGER_THRESHOLD && !fingerDetected) {
      fingerDetected = true;

      // Neu moi mat tiep xuc ngan roi co tay quay lai, chi reset buffer thu mau
      // va giu BPM/SpO2 cu trong thoi gian ngan de UI/Firebase khong nhay null.
      bool hasRecentHealthValue =
        (displayHeartRate > 0 && (now - lastValidHeartRateMs <= KEEP_LAST_HEALTH_VALUE_MS)) ||
        (displaySpO2 > 0 && (now - lastValidSpO2Ms <= KEEP_LAST_HEALTH_VALUE_MS));

      if (hasRecentHealthValue) resetHealthAcquisitionOnly();
      else resetHealthBuffers();

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
      resetHealthAcquisitionOnly();
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