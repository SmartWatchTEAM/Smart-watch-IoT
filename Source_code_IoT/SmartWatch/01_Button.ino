// ==============================================================
// 01_Button.ino
// 2 nút chức năng: chuyển màn hình + SOS khi nhấn giữ 2 giây
// ==============================================================

void initButton(Button &btn, int pin) {
  btn.pin = pin;
  pinMode(pin, INPUT_PULLUP);

  btn.lastReading = digitalRead(pin);
  btn.stableState = btn.lastReading;
  btn.lastDebounceTime = 0;
  btn.pressStartTime = 0;
  btn.shortPressEvent = false;
  btn.longPressEvent = false;
  btn.longFired = false;
}


void updateButton(Button &btn) {
  btn.shortPressEvent = false;
  btn.longPressEvent = false;

  bool reading = digitalRead(btn.pin);

  // Chống dội nút: chỉ chấp nhận trạng thái khi đã ổn định đủ DEBOUNCE_DELAY.
  if (reading != btn.lastReading) {
    btn.lastDebounceTime = millis();
  }

  if ((millis() - btn.lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != btn.stableState) {
      btn.stableState = reading;

      if (btn.stableState == LOW) {
        // Nút được nhấn xuống. INPUT_PULLUP nên LOW = đang nhấn.
        btn.pressStartTime = millis();
        btn.longFired = false;
      } else {
        // Khi đã kích hoạt nhấn giữ thì lúc nhả nút không tính là nhấn ngắn.
        if (!btn.longFired) {
          btn.shortPressEvent = true;
        }
      }
    }

    if (btn.stableState == LOW && !btn.longFired) {
      if (millis() - btn.pressStartTime >= LONG_PRESS_TIME) {
        btn.longPressEvent = true;
        btn.longFired = true;
      }
    }
  }

  btn.lastReading = reading;
}


void wakeScreenForButtonAction() {
  standbyMode = false;
  digitalWrite(TFT_BL, HIGH);
}


void goToNextScreen() {
  wakeScreenForButtonAction();

  if (currentScreen == SCREEN_HOME) {
    currentScreen = SCREEN_HEALTH;
  } else if (currentScreen == SCREEN_HEALTH) {
    currentScreen = SCREEN_STEPS;
  } else {
    currentScreen = SCREEN_HOME;
  }

  uiDrawn = false;
}


void goToPreviousScreen() {
  wakeScreenForButtonAction();

  if (currentScreen == SCREEN_HOME) {
    currentScreen = SCREEN_STEPS;
  } else if (currentScreen == SCREEN_STEPS) {
    currentScreen = SCREEN_HEALTH;
  } else {
    currentScreen = SCREEN_HOME;
  }

  uiDrawn = false;
}


void triggerSOSFromButton(const char *buttonName) {
  alertActive = true;
  alertType = ALERT_SOS;

  // Nếu đang tắt màn hình/standby thì bật lại để hiện cảnh báo.
  wakeScreenForButtonAction();
  uiDrawn = false;

  Serial.print("SOS ACTIVATED by ");
  Serial.println(buttonName);

  // Gửi Firebase ngay vòng lặp kế tiếp, không đợi chu kỳ định kỳ.
  forceFirebaseSendOnNextChange();
}


void handleButtons() {
  updateButton(buttonA);
  updateButton(buttonB);

  // Nhấn giữ bất kỳ nút nào >= 2 giây để SOS.
  if (buttonA.longPressEvent) {
    triggerSOSFromButton("Button A");
  }

  if (buttonB.longPressEvent) {
    triggerSOSFromButton("Button B");
  }

  // Khi đang cảnh báo, nhấn ngắn bất kỳ nút nào để tắt cảnh báo.
  if (alertActive) {
    if (buttonA.shortPressEvent || buttonB.shortPressEvent) {
      clearAlert();
      uiDrawn = false;
      forceFirebaseSendOnNextChange();
    }
    return;
  }

  // Nút A: chuyển màn hình kế tiếp.
  if (buttonA.shortPressEvent) {
    goToNextScreen();
  }

  // Nút B: chuyển màn hình trước đó.
  if (buttonB.shortPressEvent) {
    goToPreviousScreen();
  }
}
