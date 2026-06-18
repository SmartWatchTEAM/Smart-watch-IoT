# IoT-based Medical Alert System for Elderly People

## 1. Giới thiệu dự án

**IoT-based Medical Alert System for Elderly People** là đồ án xây dựng mô hình **đồng hồ giám sát sức khỏe thông minh cho người cao tuổi**. Hệ thống sử dụng **ESP32-S3 Super Mini** làm bộ xử lý trung tâm, kết hợp với cảm biến **MAX30102**, cảm biến chuyển động **MPU6500**, màn hình **TFT ST7789**, nút SOS, buzzer và nền tảng **Firebase Realtime Database** để theo dõi dữ liệu sức khỏe theo thời gian thực.

Thiết bị được thiết kế theo hướng nhỏ gọn, có thể hiển thị trực tiếp thông tin trên màn hình TFT và đồng thời gửi dữ liệu lên web dashboard để người thân theo dõi từ xa. Khi phát hiện tình huống bất thường như té ngã hoặc người dùng nhấn giữ nút SOS, hệ thống sẽ kích hoạt cảnh báo tại thiết bị và cập nhật trạng thái lên Firebase để hỗ trợ gửi thông báo cảnh báo.

> **Lưu ý:** Đây là mô hình phục vụ học tập, nghiên cứu và minh họa ứng dụng IoT trong chăm sóc sức khỏe. Kết quả đo BPM và SpO2 chỉ mang tính tham khảo, không thay thế thiết bị y tế chuyên dụng.

---

## 2. Thông tin

### Thành viên thực hiện

| Họ và tên | MSSV |
|---|---|
| Trần Hải Đạt | 24110197 |
| Nguyễn Lê Huy | 24110221 |
| Ninh Nguyễn Minh Tuyên | 24110372 |

---

## 3. Mục tiêu hệ thống

Mục tiêu của đề tài là xây dựng một mô hình đồng hồ sức khỏe IoT có khả năng hỗ trợ theo dõi một số thông số cơ bản của người cao tuổi và cảnh báo khi có tình huống nguy hiểm.

Các mục tiêu chính:

1. Đo nhịp tim BPM bằng cảm biến MAX30102.
2. Đo nồng độ oxy trong máu SpO2 bằng cảm biến MAX30102.
3. Đếm số bước chân bằng dữ liệu chuyển động từ MPU6500.
4. Phát hiện té ngã dựa trên gia tốc, góc nghiêng và trạng thái bất động.
5. Hiển thị thời gian, thời tiết, Wi-Fi, pin, BPM, SpO2 và số bước chân trên màn hình TFT ST7789.
6. Kích hoạt cảnh báo khi phát hiện té ngã hoặc khi người dùng nhấn giữ nút SOS.
7. Gửi dữ liệu sức khỏe và trạng thái cảnh báo lên Firebase Realtime Database.
8. Hiển thị dữ liệu realtime, lịch sử đo và thống kê trên web dashboard.
9. Hỗ trợ xuất báo cáo thống kê sức khỏe ra file PDF.

---

## 4. Chức năng chính

### 4.1. Đo nhịp tim và SpO2

Cảm biến **MAX30102** đọc tín hiệu ánh sáng đỏ và hồng ngoại từ người dùng. Sau khi thu thập đủ mẫu, chương trình xử lý tín hiệu để tính toán:

- Nhịp tim theo đơn vị **BPM**.
- Nồng độ oxy trong máu **SpO2**.
- Trạng thái đặt tay lên cảm biến.
- Chất lượng tín hiệu đo.

Khi tín hiệu chưa ổn định hoặc chưa đặt tay đúng vị trí, giao diện sẽ hiển thị giá trị dạng `--` để tránh hiển thị sai.

### 4.2. Đếm số bước chân

Cảm biến **MPU6500** cung cấp dữ liệu gia tốc theo ba trục. Chương trình phân tích dao động gia tốc để xác định bước đi, từ đó cập nhật:

- Tổng số bước trong ngày.
- Mục tiêu bước chân.
- Phần trăm hoàn thành mục tiêu.
- Quãng đường ước tính.
- Lượng calo ước tính.

### 4.3. Phát hiện té ngã

Hệ thống sử dụng MPU6500 để đo gia tốc, góc nghiêng và trạng thái chuyển động của thiết bị. Thuật toán phát hiện té ngã được xây dựng theo chuỗi trạng thái nhằm hạn chế báo động giả:

```text
Hoạt động bình thường
→ Nghi ngờ rơi tự do
→ Phát hiện va chạm mạnh
→ Kiểm tra góc nghiêng
→ Kiểm tra bất động sau va chạm
→ Xác nhận té ngã
```

Khi té ngã được xác nhận, thiết bị sẽ:

- Hiển thị trạng thái cảnh báo trên màn hình TFT.
- Bật buzzer cảnh báo tại chỗ.
- Cập nhật trạng thái `fall` lên Firebase.
- Cho phép dashboard hiển thị cảnh báo để người thân theo dõi.

### 4.4. Cảnh báo SOS

Nút SOS được dùng để người dùng chủ động gửi cảnh báo khẩn cấp. Để hạn chế bấm nhầm, chức năng SOS được thiết kế theo dạng **nhấn giữ**.

Khi SOS được kích hoạt, hệ thống sẽ:

- Hiển thị cảnh báo SOS trên màn hình.
- Phát âm thanh bằng buzzer.
- Gửi trạng thái SOS lên Firebase.
- Hỗ trợ hệ thống web/email thông báo cho người thân.

### 4.5. Hiển thị trên màn hình TFT

Màn hình **TFT ST7789 1.69 inch** hiển thị giao diện đồng hồ thông minh với các thông tin:

- Thời gian và ngày tháng.
- Trạng thái Wi-Fi.
- Dung lượng pin.
- Thời tiết hiện tại.
- Nhịp tim BPM.
- SpO2.
- Số bước chân.
- Trạng thái té ngã và SOS.

### 4.6. Web dashboard và Firebase

ESP32-S3 gửi dữ liệu lên **Firebase Realtime Database**. Web dashboard đọc dữ liệu từ Firebase và hiển thị trên trình duyệt máy tính hoặc điện thoại.

Dashboard hỗ trợ các nhóm chức năng:

| Tab | Chức năng |
|---|---|
| Tổng quát | Hiển thị realtime BPM, SpO2, bước chân, trạng thái cảnh báo |
| Thống kê | Lọc dữ liệu theo ngày/tháng, biểu đồ BPM và SpO2, xuất PDF |
| Thiết bị | Hiển thị trạng thái cảm biến, kết nối, Firebase latest/history |
| Cảnh báo | Theo dõi Fall Detection, SOS Alert và bất thường sức khỏe |
| Cài đặt | Cấu hình ngưỡng cảnh báo và thông tin hệ thống |

---

## 5. Linh kiện sử dụng

| STT | Linh kiện | Chức năng |
|---|---|---|
| 1 | ESP32-S3 Super Mini | Bộ xử lý trung tâm, xử lý dữ liệu cảm biến và kết nối Wi-Fi |
| 2 | MAX30102 | Đo nhịp tim và SpO2 |
| 3 | MPU6500 | Đo gia tốc, con quay hồi chuyển, đếm bước chân và phát hiện té ngã |
| 4 | TFT ST7789 1.69 inch | Hiển thị giao diện đồng hồ sức khỏe |
| 5 | Nút chức năng | Chuyển màn hình hoặc điều khiển một số chức năng thiết bị |
| 6 | Nút SOS | Kích hoạt cảnh báo khẩn cấp khi nhấn giữ |
| 7 | Buzzer | Phát cảnh báo âm thanh khi có té ngã hoặc SOS |
| 8 | Pin Li-Po 3.7V | Cấp nguồn cho thiết bị |
| 9 | TP4056 + DW01 | Sạc và bảo vệ pin Li-Po |
| 10 | Dây nối / nguồn cấp | Kết nối phần cứng và cấp nguồn thử nghiệm |

---

## 6. Sơ đồ khối hệ thống

```text
+-------------------+        I2C        +-------------------+
|    MAX30102       | ----------------> |                   |
|  BPM / SpO2       |                   |                   |
+-------------------+                   |                   |
                                        |                   |
+-------------------+        I2C        |     ESP32-S3      |       SPI       +------------------+
|     MPU6500       | ----------------> |    Super Mini     | --------------> |   TFT ST7789     |
| Steps / Fall      |                   |                   |                 |    Display       |
+-------------------+                   |                   |                 +------------------+
                                        |                   |
+-------------------+       GPIO        |                   |       GPIO      +------------------+
|  Nút chức năng    | ----------------> |                   | --------------> |      Buzzer      |
|  Nút SOS          |                   |                   |                 |   Cảnh báo       |
+-------------------+                   +---------+---------+                 +------------------+
                                                  |
                                                  | Wi-Fi / HTTPS
                                                  v
                                      +------------------------+
                                      | Firebase Realtime DB   |
                                      | latest / history /daily|
                                      +-----------+------------+
                                                  |
                                                  v
                                      +------------------------+
                                      | Web Dashboard          |
                                      | Realtime / Chart / PDF |
                                      +------------------------+
```

---

## 7. Sơ đồ kết nối phần cứng

> Bảng dưới đây được trình bày theo cấu hình phần cứng trong báo cáo đồ án. Nếu thay đổi chân GPIO trong firmware, cần cập nhật lại bảng này cho đồng bộ.

### 7.1. Cảm biến MAX30102

| MAX30102 | ESP32-S3 Super Mini |
|---|---|
| SDA | GPIO 10 |
| SCL | GPIO 11 |
| VCC | 3.3V |
| GND | GND |

### 7.2. Cảm biến MPU6500

| MPU6500 | ESP32-S3 Super Mini |
|---|---|
| SDA | GPIO 12 |
| SCL | GPIO 13 |
| VCC | 3.3V |
| GND | GND |

### 7.3. Màn hình TFT ST7789

| TFT ST7789 | ESP32-S3 Super Mini |
|---|---|
| SCLK | GPIO 2 |
| MOSI | GPIO 3 |
| CS | GPIO 6 |
| RST | GPIO 4 |
| DC | GPIO 5 |
| BL | GPIO 7 |
| VCC | 3.3V |
| GND | GND |

### 7.4. Nút nhấn

| Thiết bị | ESP32-S3 Super Mini |
|---|---|
| Nút chức năng | GPIO 9 |
| Nút SOS | GPIO 15 |

---

## 8. Kiến trúc dữ liệu Firebase

Hệ thống sử dụng **Firebase Realtime Database** để lưu dữ liệu realtime và dữ liệu lịch sử.

```text
devices/
└── watch_001/
    ├── latest/
    │   ├── bpm
    │   ├── spo2
    │   ├── steps
    │   ├── fall
    │   ├── sos
    │   ├── battery
    │   ├── wifi
    │   ├── timeText
    │   ├── dateText
    │   └── updatedAt
    │
    ├── history/
    │   └── YYYY-MM-DD/
    │       └── auto-id hoặc timestamp
    │           ├── bpm
    │           ├── spo2
    │           ├── steps
    │           ├── fall
    │           ├── sos
    │           └── createdAt
    │
    └── daily/
        └── YYYY-MM-DD/
            └── summary/
                ├── steps
                ├── stepGoal
                ├── distanceKm
                ├── calories
                └── updatedAt
```

Ý nghĩa các nhánh chính:

| Nhánh | Chức năng |
|---|---|
| `latest` | Lưu trạng thái mới nhất của thiết bị để dashboard hiển thị realtime |
| `history/YYYY-MM-DD` | Lưu lịch sử đo theo ngày để vẽ biểu đồ và thống kê |
| `daily/YYYY-MM-DD/summary` | Lưu tổng hợp dữ liệu vận động trong ngày |

---

## 9. Nguyên lý hoạt động

Khi thiết bị được cấp nguồn, ESP32-S3 tiến hành khởi tạo màn hình TFT, cảm biến MAX30102, cảm biến MPU6500, nút nhấn, buzzer và kết nối Wi-Fi.

Nếu Wi-Fi kết nối thành công, hệ thống đồng bộ thời gian NTP và lấy dữ liệu thời tiết từ Internet. Nếu Wi-Fi chưa kết nối, thiết bị vẫn có thể đo và hiển thị dữ liệu cục bộ trên màn hình.

Trong vòng lặp chính, hệ thống thực hiện các tác vụ:

1. Đọc trạng thái nút chức năng và nút SOS.
2. Đọc tín hiệu IR/Red từ MAX30102.
3. Kiểm tra người dùng đã đặt tay lên cảm biến hay chưa.
4. Thu thập đủ mẫu và tính BPM, SpO2.
5. Đọc dữ liệu gia tốc và góc nghiêng từ MPU6500.
6. Đếm bước chân và kiểm tra điều kiện té ngã.
7. Cập nhật giao diện TFT theo màn hình hiện tại.
8. Bật buzzer nếu có cảnh báo Fall hoặc SOS.
9. Gửi dữ liệu realtime và lịch sử lên Firebase khi Wi-Fi hoạt động.
10. Web dashboard đọc dữ liệu Firebase để hiển thị và xuất báo cáo.

---

## 10. Cấu trúc firmware ESP32

Firmware được chia thành nhiều file `.ino` để dễ quản lý theo từng chức năng:

| File | Vai trò |
|---|---|
| `SmartWatch.ino` | File chính, khai báo biến toàn cục, `setup()` và `loop()` |
| `01_Button.ino` | Xử lý nút chức năng, nút SOS và thao tác người dùng |
| `02_I2C.ino` | Khởi tạo và kiểm tra bus I2C |
| `03_WiFi_Time_Weather.ino` | Kết nối Wi-Fi, đồng bộ thời gian NTP và lấy dữ liệu thời tiết |
| `04_MAX30102.ino` | Đọc MAX30102, xử lý BPM và SpO2 |
| `05_MPU6500.ino` | Đọc MPU6500, tính gia tốc và góc nghiêng |
| `06_StepTracker.ino` | Đếm bước chân, tính quãng đường và calo |
| `07_UI_Common.ino` | Các hàm vẽ giao diện chung |
| `08_UI_Home.ino` | Giao diện màn hình chính |
| `09_UI_Health.ino` | Giao diện đo sức khỏe BPM và SpO2 |
| `10_UI_Steps.ino` | Giao diện bước chân |
| `11_Alert_Buzzer.ino` | Xử lý cảnh báo và buzzer |
| `12_Firebase.ino` | Gửi dữ liệu latest, history và daily summary lên Firebase |

---

## 11. Cấu trúc web dashboard

Web dashboard được xây dựng bằng **HTML, CSS và JavaScript**, kết nối trực tiếp với Firebase Realtime Database.

Cấu trúc thư mục đề xuất:

```text
web-dashboard/
├── index.html
├── css/
│   ├── base.css
│   ├── layout.css
│   ├── sidebar.css
│   ├── cards.css
│   ├── tabs.css
│   └── responsive.css
├── js/
│   ├── app.js
│   ├── config/
│   │   └── firebaseConfig.js
│   ├── core/
│   │   ├── header.js
│   │   ├── router.js
│   │   └── sidebar.js
│   ├── services/
│   │   ├── firebaseService.js
│   │   └── statsService.js
│   ├── tabs/
│   │   ├── overviewTab.js
│   │   ├── statsTab.js
│   │   ├── deviceTab.js
│   │   ├── alertsTab.js
│   │   └── settingsTab.js
│   └── utils/
│       └── format.js
└── assets/
    └── icons/
```

---

## 12. Cài đặt phần mềm

### 12.1. Phần mềm cần có

- Arduino IDE 2.x.
- ESP32 board package by Espressif Systems.
- Trình duyệt web hiện đại như Chrome, Edge hoặc Firefox.
- Firebase project có bật Realtime Database.
- Node.js hoặc Firebase CLI nếu cần deploy web dashboard.

### 12.2. Thư viện Arduino cần cài

Trong Arduino IDE, vào:

```text
Sketch > Include Library > Manage Libraries
```

Cài các thư viện:

| Thư viện | Chức năng |
|---|---|
| Adafruit GFX Library | Thư viện đồ họa cơ bản |
| Adafruit ST7735 and ST7789 Library | Điều khiển màn hình TFT ST7789 |
| SparkFun MAX3010x Pulse and Proximity Sensor Library | Đọc cảm biến MAX30102 |
| Wire | Giao tiếp I2C |
| WiFi | Kết nối Wi-Fi ESP32 |
| HTTPClient | Gửi request HTTP/HTTPS |

### 12.3. Cài ESP32 board package

Trong Arduino IDE:

```text
File > Preferences
```

Thêm URL sau vào `Additional Boards Manager URLs`:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Sau đó vào:

```text
Tools > Board > Boards Manager
```

Tìm và cài:

```text
esp32 by Espressif Systems
```

---

## 13. Cách chạy firmware

### Bước 1: Mở project firmware

Mở file chính:

```text
SmartWatch.ino
```

Arduino IDE sẽ tự mở các file `.ino` còn lại trong cùng thư mục.

### Bước 2: Chọn board

```text
Tools > Board > ESP32S3 Dev Module
```

### Bước 3: Chọn cổng COM

```text
Tools > Port > COMx
```

### Bước 4: Cấu hình Wi-Fi và Firebase

Trong firmware, cập nhật các thông tin:

```cpp
const char* WIFI_SSID = "TEN_WIFI";
const char* WIFI_PASSWORD = "MAT_KHAU_WIFI";
```

Cấu hình Firebase theo project của nhóm:

```text
Firebase Realtime Database URL
Firebase Auth / Secret hoặc cấu hình xác thực tương ứng
Device ID: watch_001
```

### Bước 5: Nạp chương trình

Bấm `Verify`, sau đó bấm `Upload`.

Nếu upload bị dừng ở `Connecting...`, giữ nút `BOOT` trên ESP32-S3 trong lúc upload, sau đó thả ra khi quá trình nạp bắt đầu.

### Bước 6: Kiểm tra Serial Monitor

Mở Serial Monitor ở baudrate:

```text
115200
```

Kết quả mong đợi:

```text
MAX30102 SUCCESS
MPU6500 SUCCESS
WiFi SUCCESS
Firebase telemetry OK
```

---

## 14. Cách chạy web dashboard

### 14.1. Chạy bằng Live Server

Mở thư mục web dashboard trong VS Code, sau đó mở file:

```text
index.html
```

Chọn:

```text
Open with Live Server
```

### 14.2. Deploy bằng Firebase Hosting

Cài Firebase CLI:

```bash
npm install -g firebase-tools
```

Đăng nhập Firebase:

```bash
firebase login
```

Khởi tạo hosting nếu chưa có:

```bash
firebase init hosting
```

Deploy:

```bash
firebase deploy
```

Sau khi deploy, có thể truy cập dashboard bằng link Firebase Hosting của project.

---

## 15. Kiểm thử hệ thống

### 15.1. Kiểm thử MAX30102

| Trường hợp | Kết quả mong đợi |
|---|---|
| Chưa đặt tay lên cảm biến | Hiển thị `BPM: --`, `SpO2: --` |
| Đặt tay đúng vị trí và giữ yên | Sau khi đủ mẫu, hiển thị BPM và SpO2 |
| Tay rung hoặc đặt lệch | Có thể hiển thị `--` hoặc báo tín hiệu chưa ổn định |
| Nhấc tay ra khỏi cảm biến | Hệ thống dừng đo và chờ đặt tay lại |

### 15.2. Kiểm thử MPU6500

| Trường hợp | Kết quả mong đợi |
|---|---|
| Thiết bị đứng yên | Không báo té ngã |
| Di chuyển nhẹ | Không báo té ngã, có thể đếm bước |
| Mô phỏng rơi/va chạm/nghiêng mạnh | Có thể kích hoạt kiểm tra té ngã |
| Sau va chạm thiết bị nằm yên | Xác nhận `Fall Alert` nếu thỏa điều kiện |

### 15.3. Kiểm thử SOS và buzzer

| Trường hợp | Kết quả mong đợi |
|---|---|
| Nhấn giữ nút SOS | Kích hoạt SOS Alert |
| Có Fall hoặc SOS | Buzzer phát cảnh báo |
| Tắt cảnh báo theo thao tác nút | Buzzer dừng cảnh báo |

### 15.4. Kiểm thử Firebase và dashboard

| Trường hợp | Kết quả mong đợi |
|---|---|
| ESP32 có Wi-Fi | Dữ liệu cập nhật vào `latest` |
| Đo xong BPM/SpO2 hợp lệ | Lưu bản ghi vào `history/YYYY-MM-DD` |
| Có Fall/SOS | Dashboard hiển thị cảnh báo |
| Chọn ngày/tháng trong tab Thống kê | Biểu đồ và bảng lịch sử được hiển thị |
| Bấm xuất PDF | Tạo báo cáo thống kê sức khỏe |

---

## 16. Kết quả đạt được

Hệ thống đã đạt được các chức năng chính của đề tài:

- Đọc và xử lý tín hiệu từ cảm biến MAX30102.
- Hiển thị nhịp tim BPM và SpO2 trên màn hình TFT.
- Đọc dữ liệu MPU6500 để tính chuyển động, số bước chân và hỗ trợ phát hiện té ngã.
- Xây dựng giao diện đồng hồ sức khỏe gồm Home, Health và Steps.
- Kết nối Wi-Fi để đồng bộ thời gian và lấy dữ liệu thời tiết.
- Gửi dữ liệu sức khỏe và trạng thái thiết bị lên Firebase.
- Xây dựng web dashboard theo dõi realtime, lịch sử đo và cảnh báo.
- Hỗ trợ xuất báo cáo thống kê sức khỏe ra PDF.

---

## 17. Khó khăn và cách khắc phục

| Khó khăn | Cách khắc phục |
|---|---|
| Chọn sai board trong Arduino IDE | Cài ESP32 board package và chọn đúng ESP32S3 Dev Module |
| Thiếu thư viện TFT hoặc MAX30102 | Cài thư viện trong Library Manager |
| MAX30102 dễ bị nhiễu | Điều chỉnh LED, giữ tay ổn định, kiểm tra IR/Red và lọc tín hiệu |
| Té ngã dễ báo sai nếu chỉ dùng một ngưỡng | Xây dựng thuật toán nhiều trạng thái gồm rơi tự do, va chạm, góc nghiêng và bất động |
| Wi-Fi không ổn định | Cho thiết bị chạy cục bộ khi mất mạng và reconnect khi cần gửi Firebase |
| Dữ liệu lịch sử chưa đủ | Chỉ lưu history khi có dữ liệu đo hợp lệ và duy trì gửi định kỳ |

---

## 18. Hạn chế

- Kết quả BPM và SpO2 chỉ mang tính tham khảo, chưa đạt chuẩn thiết bị y tế.
- Cảm biến MAX30102 nhạy với vị trí đặt tay, ánh sáng bên ngoài và chuyển động.
- Thuật toán phát hiện té ngã cần tiếp tục hiệu chỉnh khi thay đổi vị trí đeo thực tế.
- Mô hình chưa có vỏ đồng hồ hoàn chỉnh và chưa tối ưu hoàn toàn về năng lượng.
- Chưa tích hợp GPS thực tế để gửi vị trí khi có cảnh báo.

---

## 19. Hướng phát triển

Trong các phiên bản tiếp theo, hệ thống có thể được mở rộng:

1. Thiết kế vỏ đồng hồ và dây đeo hoàn chỉnh.
2. Tối ưu thuật toán lọc nhiễu MAX30102 để tăng độ ổn định BPM/SpO2.
3. Bổ sung GPS để gửi vị trí khi có Fall Alert hoặc SOS.
4. Tối ưu tiêu thụ năng lượng để kéo dài thời lượng pin.
5. Tích hợp Firebase Cloud Messaging hoặc ứng dụng mobile để gửi push notification.
6. Thêm eSIM hoặc kết nối dữ liệu di động để hoạt động khi không có Wi-Fi.
7. Thử nghiệm với nhiều người dùng hơn để đánh giá độ ổn định.
8. Thiết kế PCB riêng cho mô hình thiết bị đeo.
9. Bổ sung cảm biến nhiệt độ hoặc cảm biến huyết áp nếu điều kiện cho phép.

---

## 20. Lưu ý an toàn về nguồn pin

Module **TP4056 + DW01** có chức năng sạc và bảo vệ pin Li-Po 1 cell. Khi sử dụng cần lưu ý:

- Không đấu ngược cực pin.
- Không sạc khi pin hoặc module nóng bất thường.
- Không để pin bị chập mạch.
- Cần kiểm tra mức điện áp phù hợp trước khi cấp cho ESP32-S3 và các cảm biến.
- Với sản phẩm thực tế, nên sử dụng mạch quản lý nguồn chuyên dụng và vỏ bảo vệ an toàn.

---

## 21. Tài liệu tham khảo chính

Một số nhóm tài liệu được sử dụng trong quá trình thực hiện:

- Tài liệu về Internet of Things và kiến trúc hệ thống IoT.
- Tài liệu ESP32-S3 và lập trình Arduino IDE.
- Datasheet MAX30102.
- Tài liệu MPU6500.
- Tài liệu màn hình TFT ST7789 và thư viện Adafruit.
- Tài liệu Firebase Realtime Database.
- Tài liệu về phát hiện té ngã và chăm sóc sức khỏe người cao tuổi.

---

## 22. License

Dự án được phát triển cho mục đích học tập và nghiên cứu. Người dùng có thể tham khảo, chỉnh sửa và mở rộng với điều kiện ghi rõ nguồn khi chia sẻ lại.
