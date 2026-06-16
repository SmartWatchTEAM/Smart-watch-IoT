// ==============================================================
// 11_Alert_Buzzer.ino
// xóa cảnh báo, điều khiển buzzer
// ==============================================================

void clearAlert() {
  alertActive = false;
  alertType = ALERT_NONE;

  fallState = FALL_NORMAL;
  fallStateTime = 0;
  fallFreeStartTime = 0;
  fallTiltSeen = false;
  stillStartTime = 0;

  digitalWrite(BUZZER_PIN, LOW);
}


void updateBuzzer() {
  if (alertActive) {
    digitalWrite(BUZZER_PIN, (millis() / 200) % 2);
  } else if (sedentaryWarning) {
    // Cảnh báo ngồi lâu: bíp ngắn mỗi vài giây, không kêu liên tục.
    digitalWrite(BUZZER_PIN, ((millis() % 5000UL) < 180UL) ? HIGH : LOW);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}


