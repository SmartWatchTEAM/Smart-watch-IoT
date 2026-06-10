SMART HEALTH WATCH - FIREBASE DASHBOARD UI

Các file trong gói:
1. index.html          - giao diện dashboard
2. style.css           - style giao diện dark/neon như hình mẫu
3. app.js              - đọc Firebase realtime + vẽ biểu đồ
4. firebase-config.js  - nơi dán firebaseConfig của bạn

Cách dùng:
1. Giải nén thư mục này.
2. Mở firebase-config.js.
3. Dán firebaseConfig thật từ Firebase Console vào.
   Đường dẫn:
   Firebase Console → Project settings → General → Your apps → Web app.
4. Mở index.html bằng VS Code Live Server.
5. Firebase cần có dữ liệu tại:
   devices/watch_001/latest
   devices/watch_001/history/YYYY-MM-DD
   devices/watch_001/daily/YYYY-MM-DD/summary

Ghi chú:
- Giao diện vẫn chạy nếu chưa có history, nhưng biểu đồ Today sẽ dùng dữ liệu demo từ latest.
- Biểu đồ Heart Rate Waveform sẽ vẽ dữ liệu thật nếu ESP32 gửi ppgIr và ppgRed dạng mảng.
- Nếu chưa gửi ppgIr/ppgRed, app sẽ tạo sóng demo theo BPM để không bị trống giao diện.
- Muốn sóng PPG thật 100%, cần sửa code ESP32 để gửi mảng raw IR/RED lên Firebase.
