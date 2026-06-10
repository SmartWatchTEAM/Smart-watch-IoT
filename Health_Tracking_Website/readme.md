Ý nghĩa từng phần:

index.html là file chính để chạy giao diện.

css/ chứa toàn bộ giao diện: nền, sidebar, card, tab, responsive.

js/core/ chứa phần lõi: sidebar, header, chuyển tab.

js/tabs/ chứa từng giao diện riêng: Tổng quát, Thống kê, Thiết bị.

js/services/firebaseService.js đọc dữ liệu realtime từ Firebase.

js/services/statsService.js gọi API thống kê từ Node.js.

server/ là backend sau này để kết nối MySQL, lưu lịch sử và thống kê trung bình theo ngày/tháng.

# Smart Health Watch API Server

## 1. Cài thư viện

```bash
cd server
npm install
```

## 2. Tạo file cấu hình

Copy `.env.example` thành `.env`:

```bash
copy .env.example .env
```

Sau đó sửa thông tin MySQL nếu cần:

```env
PORT=3000
DB_HOST=localhost
DB_PORT=3306
DB_USER=root
DB_PASSWORD=
DB_NAME=smart_health_watch
```

## 3. Tạo database

Mở MySQL / phpMyAdmin / MySQL Workbench và chạy:

```sql
source server/sql/schema.sql;
```

Hoặc copy nội dung file `sql/schema.sql` rồi chạy thủ công.

Có thể thêm dữ liệu mẫu bằng file:

```sql
server/sql/sample_data.sql
```

## 4. Chạy server

```bash
npm run dev
```

Server chạy tại:

```txt
http://localhost:3000
```

## 5. API chính

### Kiểm tra API + MySQL

```txt
GET http://localhost:3000/api/health
```

### ESP32 gửi dữ liệu

```txt
POST http://localhost:3000/api/telemetry
```

Body JSON ví dụ:

```json
{
  "device_id": "watch_001",
  "bpm": 78,
  "spo2": 97,
  "acc": 0.98,
  "pitch": -51.2,
  "roll": 168.7,
  "battery": 82,
  "finger": true,
  "wifi": true,
  "timeSynced": true,
  "fall": false,
  "sos": false,
  "weatherTemp": 31,
  "weatherHumidity": 70,
  "weatherWind": 8,
  "weatherDesc": "Cloudy",
  "rssi": -62,
  "firmware": "v1.2.3"
}
```

### Lấy dữ liệu mới nhất

```txt
GET http://localhost:3000/api/telemetry/latest?device_id=watch_001
```

### Thống kê theo ngày

```txt
GET http://localhost:3000/api/stats/day?device_id=watch_001&date=2026-06-07
```

### Thống kê theo tháng

```txt
GET http://localhost:3000/api/stats/month?device_id=watch_001&month=2026-06
```
Không, chỉ chạy index.html thì không thể tự lấy dữ liệu MySQL trực tiếp được.

Lý do là trình duyệt chỉ chạy được HTML/CSS/JS frontend. MySQL không cho web frontend kết nối trực tiếp vì lý do bảo mật. Bắt buộc phải có một lớp trung gian:

index.html
   ↓ gọi fetch()
Node.js server
   ↓ query
MySQL

Nên muốn tab Thống kê lấy dữ liệu MySQL thật, bạn phải chạy song song 2 cái:

# Terminal 1: chạy backend
cd server
npm run dev

và:

Frontend:
Chuột phải index.html → Open with Live Server

Khi đó luồng chạy sẽ là:

Live Server chạy index.html
→ js/services/statsService.js gọi http://localhost:3000/api/stats/day
→ server Node.js nhận request
→ server query MySQL
→ trả dữ liệu thống kê về web

Nếu không chạy server Node.js, tab Thống kê vẫn mở được nhưng nó sẽ dùng dữ liệu demo, không phải dữ liệu MySQL thật.

Bạn có thể kiểm tra backend đã chạy chưa bằng cách mở trình duyệt:

http://localhost:3000/api/health

Nếu hiện kiểu như:

{
  "success": true,
  "api": "OK",
  "database": "OK"
}

thì lúc đó web mới lấy được dữ liệu MySQL thật.

Tóm lại:

index.html + Live Server: chỉ chạy giao diện
server/ + Node.js: lấy dữ liệu MySQL

Nếu iPhone không vào được, thường do Windows Firewall. Bạn cho phép Node.js và Visual Studio Code qua Firewall, hoặc khi Windows hiện bảng hỏi thì bấm Allow access.

Nếu iPhone không vào được, thường do **Windows Firewall**. Bạn cho phép `Node.js` và `Visual Studio Code` qua Firewall, hoặc khi Windows hiện bảng hỏi thì bấm **Allow access**.

làm sao
Thought for 5s

Bạn làm như này để mở quyền Firewall cho iPhone truy cập web trên laptop.

Cách nhanh nhất

Trên Windows, mở PowerShell bằng quyền Admin:

Bấm Start, gõ:

PowerShell

chuột phải chọn:

Run as administrator

Sau đó chạy 2 lệnh này:

netsh advfirewall firewall add rule name="Allow NodeJS 3000" dir=in action=allow protocol=TCP localport=3000
netsh advfirewall firewall add rule name="Allow Live Server 5500" dir=in action=allow protocol=TCP localport=5500

Xong.

Kiểm tra lại

Trên laptop vẫn phải đang chạy:

cd server
npm run dev

và VS Code đang mở Live Server.

Sau đó trên iPhone mở Safari:

http://IP_MAY_TINH:3000/api/health

Ví dụ:

http://192.168.1.27:3000/api/health

Nếu hiện JSON là backend OK.

Tiếp theo mở giao diện:

http://192.168.1.27:5500
Nếu Live Server không phải port 5500

Nhìn trên trình duyệt laptop, nếu Live Server đang là:

http://127.0.0.1:5501

thì bạn phải mở thêm port 5501:

netsh advfirewall firewall add rule name="Allow Live Server 5501" dir=in action=allow protocol=TCP localport=5501

và trên iPhone mở:

http://IP_MAY_TINH:5501
Lưu ý quan trọng

File này:

js/config/apiConfig.js

không được để localhost, phải đổi sang IP laptop:

export const API_BASE_URL = "http://192.168.1.27:3000";

export const API_ENDPOINTS = {
  statsDay: "/api/stats/day",
  statsMonth: "/api/stats/month",
  telemetry: "/api/telemetry",
};

Thay 192.168.1.27 bằng IP thật của máy bạn.

Firebase Online - Không dùng MySQL local

Thay các file này vào project:

1. js/services/firebaseService.js
2. js/services/statsService.js
3. js/config/apiConfig.js

Cấu trúc Firebase Database cần có:

devices/watch_001/latest
history/watch_001/2026-06-07/<push_id>

Ví dụ bản ghi history:

{
  "bpm": 78,
  "spo2": 97,
  "acc": 0.98,
  "pitch": -51.2,
  "roll": 168.7,
  "battery": 82,
  "fall": false,
  "sos": false,
  "finger": true,
  "wifi": true,
  "timeSynced": true,
  "createdAt": 1780810000000
}

Deploy:
firebase deploy
