# Smart Health Monitoring Device for Elderly People

## 1. Giới thiệu dự án

**Smart Health Monitoring Device for Elderly People** là thiết bị giám sát y tế thông minh dành cho người cao tuổi, được xây dựng trên nền tảng **ESP32-S3 Super Mini**.

Hệ thống có khả năng:

- Đo nhịp tim theo đơn vị BPM.
- Đo nồng độ oxy trong máu SpO2.
- Phát hiện té ngã bằng cảm biến gia tốc MPU6050.
- Hiển thị dữ liệu sức khỏe trên màn hình TFT ST

7789V3 1.69 inch.
- Phát cảnh báo bằng buzzer khi phát hiện té ngã hoặc khi người dùng bấm nút SOS.
- Sử dụng pin LiPo 3.7V kết hợp module sạc TP4056 + DW01.

Dự án hướng đến bài toán chăm sóc sức khỏe người cao tuổi tại nhà, đặc biệt trong các tình huống nguy hiểm như té ngã, bất động sau va chạm hoặc cần hỗ trợ khẩn cấp.

> Lưu ý: Thiết bị này được phát triển cho mục đích học tập, nghiên cứu và minh họa ý tưởng IoT. Kết quả đo nhịp tim và SpO2 chỉ mang tính tham khảo, không thay thế thiết bị y tế chuyên dụng.

---

## 2. Mục tiêu của hệ thống

Mục tiêu của dự án là xây dựng một thiết bị IoT nhỏ gọn, có khả năng giám sát một số thông số sức khỏe cơ bản và cảnh báo khi người cao tuổi gặp sự cố.

Các mục tiêu chính gồm:

1. Thu thập dữ liệu nhịp tim và SpO2 từ cảm biến MAX30102/MAX30105.
2. Theo dõi chuyển động cơ thể bằng cảm biến MPU6050.
3. Phát hiện tình huống té ngã dựa trên gia tốc, góc nghiêng và trạng thái bất động.
4. Hiển thị dữ liệu sức khỏe trực tiếp trên màn hình TFT.
5. Cảnh báo bằng âm thanh khi phát hiện té ngã hoặc khi người dùng bấm nút SOS.
6. Hỗ trợ sử dụng pin để thiết bị có thể hoạt động độc lập.
7. Có khả năng mở rộng thêm WiFi, Bluetooth, Telegram, Firebase, MQTT hoặc ứng dụng điện thoại trong các phiên bản tiếp theo.

---

## 3. Chức năng chính

### 3.1. Đo nhịp tim BPM

Cảm biến MAX30102/MAX30105 sử dụng ánh sáng hồng ngoại để nhận biết sự thay đổi lưu lượng máu ở đầu ngón tay. Từ tín hiệu thu được, hệ thống tính toán nhịp tim theo đơn vị BPM.

Khi dữ liệu hợp lệ, màn hình hiển thị ví dụ:

```text
BPM: 78
```

Nếu chưa đặt ngón tay hoặc tín hiệu không ổn định, màn hình hiển thị:

```text
BPM: --
```

### 3.2. Đo nồng độ oxy trong máu SpO2

MAX30102/MAX30105 sử dụng hai kênh ánh sáng đỏ và hồng ngoại để hỗ trợ tính toán nồng độ oxy trong máu.

Khi dữ liệu hợp lệ, màn hình hiển thị ví dụ:

```text
SpO2: 98%
```

Nếu chưa đủ dữ liệu hoặc tín hiệu không hợp lệ, màn hình hiển thị:

```text
SpO2: --
```

### 3.3. Phát hiện té ngã

Hệ thống sử dụng cảm biến MPU6050 để đo:

- Gia tốc theo trục X, Y, Z.
- Vận tốc góc theo trục X, Y, Z.
- Góc nghiêng của thiết bị thông qua pitch và roll.

Thuật toán phát hiện té ngã không chỉ dựa trên một giá trị tức thời, mà sử dụng chuỗi điều kiện nhằm hạn chế báo động giả:

```text
Bình thường
→ Rơi tự do
→ Va chạm mạnh
→ Thiết bị nghiêng mạnh
→ Bất động sau va chạm
→ Xác nhận té ngã
```

Khi té ngã được xác nhận, màn hình hiển thị:

```text
FALL ALERT!
```

Đồng thời buzzer sẽ kêu cảnh báo.

### 3.4. Nút SOS

Nút SOS cho phép người dùng chủ động kích hoạt cảnh báo trong trường hợp khẩn cấp, chẳng hạn:

- Khó thở.
- Chóng mặt.
- Đau ngực.
- Mệt đột ngột.
- Cần người thân hỗ trợ.
- Không thể tự di chuyển.

Khi bấm nút SOS, hệ thống hiển thị:

```text
SOS ALERT!
```

và buzzer phát cảnh báo.

### 3.5. Nút nguồn / điều khiển cảnh báo

Nút nguồn trong dự án được dùng như nút điều khiển chức năng:

| Thao tác | Chức năng |
|---|---|
| Bấm ngắn | Tắt cảnh báo hiện tại |
| Giữ khoảng 2 giây | Bật hoặc tắt màn hình TFT để tiết kiệm pin |

---

## 4. Phần cứng sử dụng

| STT | Linh kiện | Chức năng |
|---|---|---|
| 1 | ESP32-S3 Super Mini | Vi điều khiển trung tâm |
| 2 | MAX30102 / MAX30105 | Đo nhịp tim và SpO2 |
| 3 | MPU6050 | Đo gia tốc, vận tốc góc và phát hiện té ngã |
| 4 | TFT ST7789V3 1.69 inch 240x280 | Hiển thị thông tin sức khỏe |
| 5 | Buzzer | Cảnh báo âm thanh |
| 6 | Nút SOS | Kích hoạt cảnh báo khẩn cấp |
| 7 | Nút nguồn | Tắt cảnh báo hoặc bật/tắt màn hình |
| 8 | Pin LiPo 3.7V 1000mAh | Cấp nguồn di động |
| 9 | TP4056 + DW01 | Sạc và bảo vệ pin LiPo |
| 10 | Dây nối / breadboard / PCB | Kết nối mạch |

---

## 5. Sơ đồ hệ thống

```text
+-------------------------+
|        Pin LiPo         |
|     3.7V 1000mAh        |
+------------+------------+
             |
             v
+-------------------------+
|      TP4056 + DW01      |
|   Sạc và bảo vệ pin     |
+------------+------------+
             |
             v
+--------------------------------------+
|          ESP32-S3 Super Mini         |
|                                      |
|  +--------------------------------+  |
|  | MAX30102/MAX30105             |  |
|  | Đo nhịp tim và SpO2           |  |
|  +--------------------------------+  |
|                                      |
|  +--------------------------------+  |
|  | MPU6050                        |  |
|  | Đo gia tốc, phát hiện té ngã   |  |
|  +--------------------------------+  |
|                                      |
|  +--------------------------------+  |
|  | TFT ST7789V3                   |  |
|  | Hiển thị dữ liệu               |  |
|  +--------------------------------+  |
|                                      |
|  +--------------------------------+  |
|  | Buzzer + Nút SOS               |  |
|  | Cảnh báo khẩn cấp              |  |
|  +--------------------------------+  |
+--------------------------------------+
```

---

## 6. Sơ đồ nối chân

> Lưu ý: Bảng dưới đây dựa trên cấu hình chân trong firmware. Nếu mạch thực tế nối khác GPIO, cần chỉnh lại các dòng `#define` trong code.

### 6.1. Kết nối TFT ST7789V3 với ESP32-S3

| TFT ST7789V3 | ESP32-S3 Super Mini |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SCL / SCK | GPIO12 |
| SDA / MOSI | GPIO11 |
| CS | GPIO10 |
| DC | GPIO7 |
| RST | GPIO6 |
| BL / LED | GPIO5 |

### 6.2. Kết nối MAX30102/MAX30105 và MPU6050

Hai cảm biến MAX30102/MAX30105 và MPU6050 dùng chung bus I2C.

| MAX30102 / MPU6050 | ESP32-S3 Super Mini |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

### 6.3. Kết nối buzzer và nút bấm

| Thiết bị | ESP32-S3 Super Mini |
|---|---|
| Buzzer + | GPIO13 |
| Buzzer - | GND |
| Nút nguồn | GPIO4 và GND |
| Nút SOS | GPIO14 và GND |

---

## 7. Nguyên lý hoạt động tổng quát

Sau khi được cấp nguồn, ESP32-S3 khởi tạo các ngoại vi gồm màn hình TFT, cảm biến MAX30102/MAX30105, cảm biến MPU6050, buzzer và các nút bấm.

Nếu cảm biến không được tìm thấy, hệ thống sẽ hiển thị thông báo lỗi trên màn hình để người dùng kiểm tra lại dây nối.

Trong quá trình hoạt động, chương trình liên tục thực hiện các tác vụ:

```text
1. Đọc trạng thái nút nguồn và nút SOS.
2. Đọc dữ liệu gia tốc và gyro từ MPU6050.
3. Phân tích dữ liệu MPU6050 để phát hiện té ngã.
4. Đọc tín hiệu RED và IR từ MAX30102/MAX30105.
5. Tính nhịp tim và SpO2 khi đủ dữ liệu.
6. Hiển thị BPM, SpO2, trạng thái té ngã và biểu đồ IR lên màn hình.
7. Kích hoạt buzzer nếu có cảnh báo té ngã hoặc SOS.
```

---

## 8. Thuật toán đo nhịp tim và SpO2

Cảm biến MAX30102/MAX30105 phát ánh sáng đỏ và hồng ngoại vào ngón tay. Dựa vào tín hiệu phản xạ, hệ thống thu được hai giá trị chính:

| Tín hiệu | Ý nghĩa |
|---|---|
| RED | Tín hiệu ánh sáng đỏ |
| IR | Tín hiệu hồng ngoại |

Quy trình xử lý:

```text
Bước 1: Đọc giá trị RED và IR từ cảm biến.
Bước 2: Kiểm tra có đặt ngón tay lên cảm biến hay chưa.
Bước 3: Nếu chưa có ngón tay, reset dữ liệu và hiển thị yêu cầu đặt tay.
Bước 4: Nếu có ngón tay, lưu dữ liệu vào bộ đệm.
Bước 5: Khi đủ số mẫu, gọi thuật toán tính BPM và SpO2.
Bước 6: Hiển thị kết quả lên TFT.
```

Ngưỡng nhận biết ngón tay trong firmware:

```cpp
const uint32_t FINGER_THRESHOLD = 50000;
```

Nếu tín hiệu IR nhỏ hơn ngưỡng này, hệ thống xem như chưa có ngón tay đặt lên cảm biến.

---

## 9. Thuật toán phát hiện té ngã

### 9.1. Dữ liệu sử dụng

MPU6050 cung cấp dữ liệu gia tốc và gyro. Firmware chuyển gia tốc sang đơn vị g và tính tổng gia tốc:

```text
fallAccMag = sqrt(ax² + ay² + az²)
```

Ngoài ra, hệ thống tính thêm:

| Biến | Ý nghĩa |
|---|---|
| `fallAccMag` | Tổng gia tốc theo 3 trục |
| `fallPitch` | Góc nghiêng theo hướng pitch |
| `fallRoll` | Góc nghiêng theo hướng roll |
| `gyroMag` | Tổng vận tốc góc |
| `fallState` | Trạng thái hiện tại của thuật toán té ngã |

### 9.2. Các ngưỡng phát hiện

| Ngưỡng | Giá trị | Ý nghĩa |
|---|---|---|
| `FREE_FALL_THRESHOLD` | 0.55g | Nghi ngờ rơi tự do |
| `IMPACT_THRESHOLD` | 2.4g | Va chạm mạnh |
| `ANGLE_THRESHOLD` | 60 độ | Thiết bị nghiêng mạnh |
| `GYRO_STILL_THRESHOLD` | 1.0 | Ít chuyển động |
| `STILL_TIME` | 2000 ms | Bất động trong 2 giây |

### 9.3. Máy trạng thái phát hiện té ngã

Firmware sử dụng máy trạng thái gồm các trạng thái:

| Trạng thái | Ý nghĩa |
|---|---|
| `FALL_NORMAL` | Hoạt động bình thường |
| `FALL_FREE` | Nghi ngờ rơi tự do |
| `FALL_IMPACT` | Phát hiện va chạm mạnh |
| `FALL_CHECK_STILL` | Kiểm tra bất động sau va chạm |
| `FALL_CONFIRMED` | Xác nhận té ngã |

Luồng hoạt động:

```text
FALL_NORMAL
    |
    | Nếu fallAccMag < 0.55g
    v
FALL_FREE
    |
    | Nếu fallAccMag > 2.4g
    v
FALL_IMPACT
    |
    | Nếu |pitch| hoặc |roll| > 60 độ
    v
FALL_CHECK_STILL
    |
    | Nếu ít chuyển động trong 2 giây
    v
FALL_CONFIRMED
    |
    v
Kích hoạt FALL ALERT!
```

Cách xử lý này giúp giảm báo động giả so với việc chỉ dùng một ngưỡng gia tốc duy nhất.

---

## 10. Cảnh báo và điều khiển buzzer

Hệ thống có hai loại cảnh báo:

| Cảnh báo | Điều kiện kích hoạt |
|---|---|
| `FALL ALERT!` | Thuật toán xác nhận té ngã |
| `SOS ALERT!` | Người dùng bấm nút SOS |

Khi có cảnh báo, buzzer kêu theo chu kỳ bật/tắt:

```text
200 ms bật
200 ms tắt
```

Mục đích là tạo âm báo dễ nhận biết và tiết kiệm năng lượng hơn so với bật liên tục.

Người dùng có thể bấm ngắn nút nguồn để tắt cảnh báo hiện tại.

---

## 11. Giao diện hiển thị TFT

Giao diện chính trên màn hình TFT ST7789V3:

```text
SMART HEALTH

BPM: --
SpO2: --

Fall: OK

IR Wave | Acc: 1.00g
```

Ý nghĩa các thành phần:

| Thành phần | Ý nghĩa |
|---|---|
| `SMART HEALTH` | Tên hệ thống |
| `BPM` | Nhịp tim |
| `SpO2` | Nồng độ oxy trong máu |
| `Fall: OK` | Chưa phát hiện té ngã |
| `FALL ALERT!` | Đã phát hiện té ngã |
| `SOS ALERT!` | Người dùng đã bấm nút SOS |
| `IR Wave` | Biểu đồ tín hiệu hồng ngoại từ MAX30102/MAX30105 |
| `Acc` | Tổng gia tốc từ MPU6050 |

---

## 12. Cài đặt phần mềm

### 12.1. Yêu cầu

- Arduino IDE 2.x.
- ESP32 board package by Espressif Systems.
- Cáp USB có truyền dữ liệu.
- ESP32-S3 Super Mini.

### 12.2. Cài ESP32 board package

Trong Arduino IDE, vào:

```text
File > Preferences
```

Thêm URL sau vào mục `Additional Boards Manager URLs`:

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

### 12.3. Chọn board và cổng nạp

Trong Arduino IDE, chọn:

```text
Tools > Board > esp32 > ESP32S3 Dev Module
```

Sau đó chọn cổng COM tương ứng:

```text
Tools > Port > COMx
```

Nếu upload bị kẹt ở dòng `Connecting...`, giữ nút `BOOT` trên ESP32-S3 trong lúc upload, sau đó thả ra khi quá trình nạp bắt đầu.

---

## 13. Thư viện cần cài

Vào:

```text
Sketch > Include Library > Manage Libraries
```

Cài các thư viện sau:

| Thư viện | Chức năng |
|---|---|
| Adafruit GFX Library | Thư viện đồ họa cơ bản |
| Adafruit ST7735 and ST7789 Library | Điều khiển màn hình TFT ST7789 |
| Adafruit MPU6050 | Đọc cảm biến MPU6050 |
| Adafruit Unified Sensor | Thư viện cảm biến dùng chung |
| SparkFun MAX3010x Pulse and Proximity Sensor Library | Đọc cảm biến MAX30102/MAX30105 |

---

## 14. Cách chạy chương trình

### Bước 1: Kết nối phần cứng

Kết nối các module với ESP32-S3 theo sơ đồ chân trong mục 6.

### Bước 2: Mở firmware trong Arduino IDE

Mở file:

```text
smart_health_monitor.ino
```

### Bước 3: Chọn board

```text
Board: ESP32S3 Dev Module
Port: COMx
```

### Bước 4: Biên dịch

Bấm nút `Verify`.

Nếu không có lỗi, Arduino IDE sẽ báo:

```text
Done compiling
```

### Bước 5: Nạp code

Bấm nút `Upload`.

Nếu nạp thành công, Arduino IDE sẽ báo:

```text
Done uploading
```

### Bước 6: Kiểm tra Serial Monitor

Mở Serial Monitor với baudrate:

```text
115200
```

Kết quả mong đợi:

```text
MAX30102 SUCCESS
MPU6050 SUCCESS
HR = 78 | Valid HR = 1 | SpO2 = 98 | Valid SpO2 = 1
```

---

## 15. Cấu trúc thư mục đề xuất khi đưa lên GitHub

```text
smart-health-monitoring-device/
│
├── README.md
├── firmware/
│   └── smart_health_monitor.ino
│
├── images/
│   ├── circuit_diagram.png
│   ├── hardware_setup.jpg
│   └── demo_screen.jpg
│
├── docs/
│   ├── report.pdf
│   └── presentation.pdf
│
└── LICENSE
```

Gợi ý:

- Đặt ảnh mạch vào thư mục `images/`.
- Đặt code Arduino vào thư mục `firmware/`.
- Đặt báo cáo hoặc slide vào thư mục `docs/`.

---

## 16. Kiểm thử hệ thống

### 16.1. Kiểm thử cảm biến MAX30102/MAX30105

| Trường hợp kiểm thử | Kết quả mong đợi |
|---|---|
| Không đặt ngón tay | Hiển thị `BPM: --`, `SpO2: --` |
| Đặt ngón tay lên cảm biến | Sau một thời gian hiển thị BPM và SpO2 |
| Nhấc ngón tay ra | Dữ liệu đo được reset |
| Đặt tay lệch cảm biến | Kết quả có thể không ổn định |

### 16.2. Kiểm thử cảm biến MPU6050

| Trường hợp kiểm thử | Kết quả mong đợi |
|---|---|
| Thiết bị đứng yên | Hiển thị `Fall: OK` |
| Lắc nhẹ thiết bị | Không báo té ngã |
| Mô phỏng rơi và va chạm | Có thể chuyển sang trạng thái cảnh báo |
| Sau va chạm thiết bị nằm yên | Hiển thị `FALL ALERT!`, buzzer kêu |

### 16.3. Kiểm thử nút SOS và nút nguồn

| Trường hợp kiểm thử | Kết quả mong đợi |
|---|---|
| Bấm nút SOS | Hiển thị `SOS ALERT!`, buzzer kêu |
| Bấm ngắn nút nguồn khi đang cảnh báo | Tắt cảnh báo |
| Giữ nút nguồn khoảng 2 giây | Bật hoặc tắt màn hình TFT |

---

## 17. Kết quả đạt được

Hệ thống đã thực hiện được các chức năng cơ bản của một thiết bị giám sát y tế thông minh:

- Đọc dữ liệu từ cảm biến MAX30102/MAX30105.
- Tính toán và hiển thị nhịp tim BPM.
- Tính toán và hiển thị SpO2.
- Đọc dữ liệu gia tốc và gyro từ MPU6050.
- Phát hiện té ngã dựa trên nhiều điều kiện.
- Hiển thị dữ liệu trên màn hình TFT ST7789V3.
- Cảnh báo bằng buzzer khi phát hiện té ngã hoặc khi bấm SOS.
- Hỗ trợ nút tắt cảnh báo và bật/tắt màn hình.

---

## 18. Hạn chế của hệ thống

Dự án hiện tại vẫn còn một số hạn chế:

- Kết quả BPM và SpO2 chỉ mang tính tham khảo.
- Thuật toán té ngã cần hiệu chỉnh thêm theo vị trí đeo thiết bị thực tế.
- Có thể xảy ra báo động giả khi thiết bị bị rơi hoặc bị lắc mạnh.
- Chưa có chức năng gửi cảnh báo qua WiFi, Telegram, app điện thoại hoặc cloud server.
- Chưa có GPS để gửi vị trí người dùng khi xảy ra sự cố.
- Chưa có vỏ thiết bị hoàn chỉnh để đeo trên tay hoặc gắn trên người.
- Thiết kế nguồn cần được kiểm tra kỹ để đảm bảo an toàn khi dùng pin LiPo.

---

## 19. Hướng phát triển

Trong các phiên bản tiếp theo, hệ thống có thể được mở rộng:

1. Gửi cảnh báo té ngã qua WiFi.
2. Gửi tin nhắn Telegram cho người thân.
3. Gửi dữ liệu lên Firebase hoặc MQTT server.
4. Xây dựng ứng dụng điện thoại để theo dõi từ xa.
5. Thêm GPS để gửi vị trí khi có cảnh báo.
6. Lưu lịch sử BPM và SpO2.
7. Tối ưu thuật toán phát hiện té ngã bằng machine learning.
8. Thiết kế PCB riêng cho hệ thống.
9. Thiết kế vỏ đeo tay nhỏ gọn.
10. Tối ưu tiêu thụ năng lượng để kéo dài thời lượng pin.

---

## 20. Lưu ý về nguồn pin LiPo

Module TP4056 + DW01 có chức năng sạc và bảo vệ pin LiPo, nhưng không phải lúc nào cũng cung cấp điện áp ổn định cho toàn bộ hệ thống.

Cần lưu ý:

- Không cấp trực tiếp pin LiPo đầy 4.2V vào chân 3.3V của ESP32.
- Nếu cấp vào chân 5V/VIN, nên dùng mạch boost 5V ổn định.
- Cần kiểm tra cực tính pin trước khi cấp nguồn.
- Không sạc pin khi mạch bị chập hoặc nóng bất thường.
- Khi làm sản phẩm thực tế, nên dùng module quản lý pin chuyên dụng hơn.

---

## 21. Tiêu chí đánh giá đồ án IoT

Dự án đáp ứng các tiêu chí cơ bản của một đồ án IoT/Embedded System:

| Tiêu chí | Mô tả |
|---|---|
| Cảm biến | Có cảm biến sinh học MAX30102/MAX30105 và cảm biến chuyển động MPU6050 |
| Xử lý dữ liệu | Có xử lý BPM, SpO2, gia tốc, góc nghiêng và trạng thái cảnh báo |
| Thiết bị đầu ra | Có màn hình TFT và buzzer |
| Tương tác người dùng | Có nút SOS và nút nguồn |
| Ứng dụng thực tế | Hướng đến giám sát người cao tuổi |
| Tính tích hợp | Kết hợp nhiều module phần cứng trên ESP32-S3 |
| Tính mở rộng | Có thể mở rộng WiFi, cloud, app, GPS |
| Tính hoàn thiện | Có pin, sạc, cảm biến, hiển thị và cảnh báo | 

---

## 22. Thông tin tác giả

- Sinh viên thực hiện: `Nguyễn Lê Huy - 24110221`
                       `Trần Hải Đạt - 24110197`
                       `Ninh Nguyễn Minh Tuyên - 24110372`
- Môn học: `Vạn vạn kết nối - IoT`
- Giảng viên hướng dẫn: `Đinh Công Đoan`
- Khoa: `Công nghệ thông tin`

---

## 23. License

Dự án được phát triển cho mục đích học tập và nghiên cứu. Người dùng có thể tham khảo, chỉnh sửa và mở rộng dự án với điều kiện ghi rõ nguồn nếu chia sẻ lại.
