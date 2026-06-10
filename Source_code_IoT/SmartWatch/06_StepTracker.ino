// ==============================================================
// 06_StepTracker.ino
// thuật toán đếm bước, quãng đường, calo, cảnh báo ngồi lâu
// ==============================================================

String formatNumberWithComma(long value) {
  String s = String(value);
  String out = "";
  int count = 0;

  for (int i = s.length() - 1; i >= 0; i--) {
    out = String(s[i]) + out;
    count++;
    if (count == 3 && i > 0) {
      out = "," + out;
      count = 0;
    }
  }

  return out;
}


float getStepDistanceKm() {
  return (stepCount * STEP_LENGTH_M) / 1000.0f;
}


int getStepCalories() {
  return (int)(stepCount * KCAL_PER_STEP + 0.5f);
}


int getStepGoalPercent() {
  if (stepGoal <= 0) return 0;
  int p = (int)((stepCount * 100L) / stepGoal);
  return constrain(p, 0, 100);
}


int getCurrentHourForSteps() {
  struct tm timeinfo;
  if (timeSynced && getLocalTime(&timeinfo, 20)) {
    return constrain(timeinfo.tm_hour, 0, 23);
  }

  return (millis() / 3600000UL) % 24;
}


void resetStepDataForNewDay() {
  stepCount = 0;
  activeWalkMs = 0;
  lastStepTime = 0;
  lastStepCandidateTime = 0;
  lastStepUpdateTime = 0;
  lastStepSampleTime = 0;

  stepGravity = 1.0f;
  stepSignalFiltered = 0.0f;
  lastStepSignal = 0.0f;
  stepPeakValue = 0.0f;
  stepValleyValue = 0.0f;
  stepNoiseAvg = 0.04f;
  stepDynamicThreshold = 0.12f;
  stepAboveThreshold = false;
  stepWalkConfirmCount = 0;
  stepCadenceSPM = 0;

  sedentaryStartTime = millis();
  sedentaryWarning = false;

  for (int i = 0; i < 24; i++) {
    hourlySteps[i] = 0;
  }

  uiDrawn = false;
}


void resetStepsDailyIfNeeded() {
  struct tm timeinfo;
  if (!timeSynced || !getLocalTime(&timeinfo, 20)) return;

  int today = timeinfo.tm_yday;
  if (lastStepDayOfYear < 0) {
    lastStepDayOfYear = today;
    return;
  }

  if (today != lastStepDayOfYear) {
    lastStepDayOfYear = today;
    resetStepDataForNewDay();
    Serial.println("STEP TRACKER: Da reset buoc chan sang ngay moi.");
  }
}


void addOneStep() {
  stepCount++;

  int h = getCurrentHourForSteps();
  if (h >= 0 && h < 24) {
    hourlySteps[h]++;
  }

  lastStepTime = millis();
  sedentaryStartTime = millis();
  sedentaryWarning = false;
}


void registerStepCandidate(unsigned long now) {
  // Nếu đã quá lâu không có nhịp kế tiếp thì coi như bắt đầu một cụm đi mới.
  if (lastStepCandidateTime == 0 || now - lastStepCandidateTime > STEP_BURST_TIMEOUT) {
    stepWalkConfirmCount = 1;
    stepCadenceSPM = 0;
    lastStepCandidateTime = now;
    return;   // chưa cộng ngay để tránh rung tay đơn lẻ bị tính là 1 bước
  }

  unsigned long interval = now - lastStepCandidateTime;
  if (interval < STEP_MIN_INTERVAL) return;

  if (interval > STEP_MAX_INTERVAL) {
    stepWalkConfirmCount = 1;
    stepCadenceSPM = 0;
    lastStepCandidateTime = now;
    return;
  }

  stepCadenceSPM = (int)(60000UL / interval);

  // Nhịp người đi/chạy thường nằm trong khoảng này. Ngoài khoảng này dễ là nhiễu.
  if (stepCadenceSPM < 42 || stepCadenceSPM > 200) {
    stepWalkConfirmCount = 1;
    lastStepCandidateTime = now;
    return;
  }

  if (stepWalkConfirmCount == 1) {
    // Khi thấy bước thứ 2 hợp lệ, cộng bù cả bước thứ 1 và bước thứ 2.
    addOneStep();
    addOneStep();
    stepWalkConfirmCount = 2;
  } else {
    addOneStep();
    if (stepWalkConfirmCount < 250) stepWalkConfirmCount++;
  }

  lastStepCandidateTime = now;
}


void updateStepTracker() {
  if (!mpuReady) return;

  unsigned long now = millis();

  // Giới hạn tần số xử lý để bộ lọc ổn định hơn, tránh mỗi vòng loop 10ms bắt nhiều đỉnh giả.
  if (now - lastStepSampleTime < STEP_SAMPLE_INTERVAL_MS) return;
  lastStepSampleTime = now;

  if (lastStepUpdateTime == 0) lastStepUpdateTime = now;
  unsigned long dt = now - lastStepUpdateTime;
  lastStepUpdateTime = now;

  float accMag = sqrt(accX * accX + accY * accY + accZ * accZ);

  // Loại bỏ mẫu va đập / đọc lỗi quá mạnh.
  if (accMag < 0.45f || accMag > 3.20f) {
    stepAboveThreshold = false;
    stepWalkConfirmCount = 0;
    lastStepSignal = 0.0f;
    return;
  }

  // 1) Low-pass để ước lượng thành phần trọng lực quanh 1g.
  stepGravity = stepGravity * 0.94f + accMag * 0.06f;

  // 2) High-pass + smoothing: giữ dao động do bước chân, giảm rung nhanh.
  float highPass = accMag - stepGravity;
  stepSignalFiltered = stepSignalFiltered * 0.72f + highPass * 0.28f;

  // 3) Ngưỡng động theo mức nhiễu thực tế của người đeo.
  stepNoiseAvg = stepNoiseAvg * 0.96f + fabs(stepSignalFiltered) * 0.04f;
  stepDynamicThreshold = constrain(stepNoiseAvg * 1.85f, STEP_THRESHOLD_MIN, STEP_THRESHOLD_MAX);

  if (stepSignalFiltered < stepValleyValue) {
    stepValleyValue = stepSignalFiltered;
  }

  bool stepDetected = false;

  // Bắt đầu một đỉnh bước khi tín hiệu vượt ngưỡng.
  if (!stepAboveThreshold && stepSignalFiltered > stepDynamicThreshold) {
    stepAboveThreshold = true;
    stepPeakValue = stepSignalFiltered;
  }

  // Cập nhật đỉnh cao nhất trong cùng một nhịp.
  if (stepAboveThreshold && stepSignalFiltered > stepPeakValue) {
    stepPeakValue = stepSignalFiltered;
  }

  // Xác nhận bước khi tín hiệu rơi xuống dưới nửa ngưỡng: đủ một chu kỳ peak -> valley.
  if (stepAboveThreshold && stepSignalFiltered < (stepDynamicThreshold * 0.45f)) {
    float amplitude = stepPeakValue - stepValleyValue;

    if (amplitude >= STEP_MIN_AMPLITUDE &&
        stepPeakValue <= STEP_MAX_SIGNAL &&
        accMag > 0.70f && accMag < 2.60f) {
      stepDetected = true;
    }

    stepAboveThreshold = false;
    stepPeakValue = 0.0f;
    stepValleyValue = stepSignalFiltered;
  }

  if (stepDetected) {
    registerStepCandidate(now);
  }

  // Nếu chỉ có một rung động đơn lẻ, hủy ứng viên để không cộng sai.
  if (lastStepCandidateTime > 0 && now - lastStepCandidateTime > STEP_BURST_TIMEOUT) {
    stepWalkConfirmCount = 0;
  }

  unsigned long recentMoveTime = lastStepTime;
  if (lastStepCandidateTime > recentMoveTime) recentMoveTime = lastStepCandidateTime;

  if (recentMoveTime > 0 && (now - recentMoveTime) < STEP_ACTIVE_WINDOW) {
    if (dt < 1000) activeWalkMs += dt;
  }

  // Cảnh báo ngồi lâu: reset bộ đếm nếu có bước hoặc có chuyển động đủ rõ.
  float gyroMag = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);
  bool moving = stepDetected || fabs(stepSignalFiltered) > 0.055f || gyroMag > 0.25f;

  if (moving) {
    sedentaryStartTime = now;
    sedentaryWarning = false;
  } else {
    if (sedentaryStartTime == 0) sedentaryStartTime = now;
    if ((now - sedentaryStartTime) >= SEDENTARY_LIMIT_MS) {
      sedentaryWarning = true;
    }
  }

  lastStepSignal = stepSignalFiltered;
}


