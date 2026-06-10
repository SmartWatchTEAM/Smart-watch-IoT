// ==============================================================
// 02_I2C.ino
// scan I2C, check I2C
// ==============================================================

bool checkI2CDevice(TwoWire &bus, byte address) {
  bus.beginTransmission(address);
  byte error = bus.endTransmission();

  return error == 0;
}


void scanI2C(TwoWire &bus, const char *busName) {
  Serial.println();
  Serial.print("===== I2C SCANNER: ");
  Serial.print(busName);
  Serial.println(" =====");

  bool foundAny = false;
for (byte address = 1; address < 127; address++) {
    bus.beginTransmission(address);
    byte error = bus.endTransmission();

    if (error == 0) {
      foundAny = true;
      Serial.print("Found I2C device at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (!foundAny) {
    Serial.println("Khong tim thay thiet bi I2C nao!");
  }

  Serial.println("=======================");
  Serial.println();
}

