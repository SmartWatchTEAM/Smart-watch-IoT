// ==============================================================
// 05_MPU6500.ino
// đọc MPU6500 raw I2C, phát hiện té ngã, setup MPU
// ==============================================================

bool mpuWrite8(byte reg, byte value) {
  Wire.beginTransmission(mpuI2CAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}


bool mpuReadBytes(byte reg, byte count, byte *data) {
  Wire.beginTransmission(mpuI2CAddress);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  byte received = Wire.requestFrom(mpuI2CAddress, count);

  if (received != count) {
    return false;
  }

  for (byte i = 0; i < count; i++) {
    data[i] = Wire.read();
  }

  return true;
}


byte mpuRead8(byte reg) {
  byte data = 0xFF;

  if (mpuReadBytes(reg, 1, &data)) {
    return data;
  }

  return 0xFF;
}


int16_t combine16(byte highByte, byte lowByte) {
  return (int16_t)((highByte << 8) | lowByte);
}


bool readMPU6500() {
  byte data[14];

  if (!mpuReadBytes(MPU6500_REG_ACCEL_XOUT_H, 14, data)) {
    return false;
  }

  int16_t rawAX = combine16(data[0], data[1]);
  int16_t rawAY = combine16(data[2], data[3]);
  int16_t rawAZ = combine16(data[4], data[5]);

  int16_t rawGX = combine16(data[8], data[9]);
  int16_t rawGY = combine16(data[10], data[11]);
  int16_t rawGZ = combine16(data[12], data[13]);
// ACCEL_CONFIG = 0x10 tương ứng thang đo ±8g => 4096 LSB/g
  accX = rawAX / 4096.0;
  accY = rawAY / 4096.0;
  accZ = rawAZ / 4096.0;

  // GYRO_CONFIG = 0x08 tương ứng ±500 deg/s => 65.5 LSB/(deg/s)
  // Đổi sang rad/s để giữ nguyên ngưỡng GYRO_STILL_THRESHOLD = 1.0 như code cũ.
  gyroX = (rawGX / 65.5) * PI / 180.0;
  gyroY = (rawGY / 65.5) * PI / 180.0;
  gyroZ = (rawGZ / 65.5) * PI / 180.0;

  return true;
}


void updateFallDetection() {
  if (!mpuReady) return;

  if (!readMPU6500()) {
    return;
  }

  fallAccMag = sqrt(accX * accX + accY * accY + accZ * accZ);

  fallRoll = atan2(accY, accZ) * 180.0 / PI;
  fallPitch = atan2(-accX, sqrt(accY * accY + accZ * accZ)) * 180.0 / PI;

  float gyroMag = sqrt(
    gyroX * gyroX +
    gyroY * gyroY +
    gyroZ * gyroZ
  );

  if (millis() - lastMPUPrint >= 500) {
    lastMPUPrint = millis();

    Serial.print("MPU | ax=");
    Serial.print(accX, 2);
    Serial.print("g ay=");
    Serial.print(accY, 2);
    Serial.print("g az=");
    Serial.print(accZ, 2);
    Serial.print("g | mag=");
    Serial.print(fallAccMag, 2);
    Serial.print("g | pitch=");
    Serial.print(fallPitch, 1);
    Serial.print(" roll=");
    Serial.println(fallRoll, 1);
  }

  unsigned long now = millis();

  switch (fallState) {
    case FALL_NORMAL:
      if (fallAccMag < FREE_FALL_THRESHOLD) {
        fallState = FALL_FREE;
        fallStateTime = now;
      }
      break;

    case FALL_FREE:
      if (fallAccMag > IMPACT_THRESHOLD) {
        fallState = FALL_IMPACT;
        fallStateTime = now;
      }

      if (now - fallStateTime > 1000) {
        fallState = FALL_NORMAL;
      }
      break;

    case FALL_IMPACT:
      if (fabs(fallPitch) > ANGLE_THRESHOLD || fabs(fallRoll) > ANGLE_THRESHOLD) {
        fallState = FALL_CHECK_STILL;
        fallStateTime = now;
        stillStartTime = now;
      }

      if (now - fallStateTime > 2000) {
        fallState = FALL_NORMAL;
      }
      break;

    case FALL_CHECK_STILL:
      if (gyroMag < GYRO_STILL_THRESHOLD && fabs(fallAccMag - 1.0) < 0.4) {
        if (now - stillStartTime > STILL_TIME) {
            Serial.println("========== FALL DETECTED ==========");

            alertActive = true;
            alertType = ALERT_FALL;

            currentScreen = SCREEN_HEALTH;   // tự chuyển sang màn hình Health

            fallState = FALL_CONFIRMED;

            standbyMode = false;
            digitalWrite(TFT_BL, HIGH);
            uiDrawn = false;
        }
      } else {
        stillStartTime = now;
      }

      if (now - fallStateTime > 6000 && !alertActive) {
        fallState = FALL_NORMAL;
      }
      break;

    case FALL_CONFIRMED:
      alertActive = true;
      alertType = ALERT_FALL;
      break;
  }
}


void setupMPU6500() {
  Serial.println("Khoi tao MPU6500...");

  if (checkI2CDevice(Wire, 0x68)) {
    mpuI2CAddress = 0x68;
    Serial.println("Da thay MPU6500 tai 0x68");
  } else if (checkI2CDevice(Wire, 0x69)) {
    mpuI2CAddress = 0x69;
    Serial.println("Da thay MPU6500 tai 0x69");
  } else {
    Serial.println("KHONG thay MPU6500 tai 0x68 hoac 0x69!");
    mpuReady = false;
    return;
  }

  byte whoami = mpuRead8(MPU6500_REG_WHO_AM_I);

  Serial.print("MPU WHO_AM_I = 0x");
  if (whoami < 16) Serial.print("0");
  Serial.println(whoami, HEX);
// Module của bạn đang trả 0x70. Một số chip tương thích có thể trả ID khác,
  // nhưng với trường hợp hiện tại ta chấp nhận 0x70.
  if (whoami != 0x70) {
    Serial.println("CANH BAO: WHO_AM_I khong phai 0x70, van thu khoi tao raw I2C.");
  }

  // Wake up sensor
  mpuWrite8(MPU6500_REG_PWR_MGMT_1, 0x00);
  delay(100);

  // Low pass filter
  mpuWrite8(MPU6500_REG_CONFIG, 0x03);

  // Gyro ±500 deg/s
  mpuWrite8(MPU6500_REG_GYRO_CONFIG, 0x08);

  // Accel ±8g
  mpuWrite8(MPU6500_REG_ACCEL_CONFIG, 0x10);

  // Accel DLPF
  mpuWrite8(MPU6500_REG_ACCEL_CONFIG2, 0x03);

  mpuReady = true;
  Serial.println("MPU6500 SUCCESS");
}


