export function renderDeviceTab(state) {
  const d = state.latestData || {};
  const page = document.getElementById("pageContent");

  page.className = "page-content device-page";

  page.innerHTML = `
    <div class="grid-12">
      <div class="card span-4">
        <h2 class="section-title">Watch_001</h2>
        <p class="section-desc">Thiết bị vòng tay sức khỏe IoT.</p>
        <div class="pill ${d.wifi ? "pill-ok" : "pill-alert"}">${d.wifi ? "Online" : "Waiting data"}</div>
      </div>

      <div class="card span-4">
        <h2 class="section-title">Firebase Latest</h2>
        <p class="section-desc">devices/watch_001/latest</p>
        <div class="pill pill-ok">Realtime connected</div>
      </div>

      <div class="card span-4">
        <h2 class="section-title">Firebase History</h2>
        <p class="section-desc">devices/watch_001/history/YYYY-MM-DD</p>
        <div class="pill">Statistics storage</div>
      </div>

      <div class="card span-12">
        <h2 class="section-title">Kiến trúc dữ liệu hiện tại</h2>
        <p class="section-desc">ESP32 gửi latest để xem realtime, đồng thời lưu history/daily để thống kê và xuất PDF.</p>

        <div class="device-flow">
          <div class="flow-node">
            <div>
              <div class="flow-title cyan-text">ESP32-S3</div>
              <div class="flow-desc">Đọc MAX30102 + MPU6500</div>
            </div>
          </div>

          <div class="flow-node">
            <div>
              <div class="flow-title cyan-text">Firebase latest</div>
              <div class="flow-desc">Hiển thị realtime</div>
            </div>
          </div>

          <div class="flow-node">
            <div>
              <div class="flow-title purple-text">Firebase history</div>
              <div class="flow-desc">Lưu dữ liệu đo theo thời gian</div>
            </div>
          </div>

          <div class="flow-node">
            <div>
              <div class="flow-title green-text">Dashboard</div>
              <div class="flow-desc">Thống kê, biểu đồ, xuất PDF</div>
            </div>
          </div>
        </div>
      </div>

      <div class="card span-6">
        <h2 class="section-title">Thông tin cảm biến</h2>
        <div class="insight-list">
          <div class="insight-item">
            <div class="insight-title">MAX30102</div>
            <div class="insight-desc">Heart Rate: ${d.bpm ?? "--"} BPM · SpO2: ${d.spo2 ?? "--"}% · Finger: ${d.finger ? "YES" : "NO"}</div>
          </div>

          <div class="insight-item">
            <div class="insight-title">MPU6500</div>
            <div class="insight-desc">Acceleration: ${d.acc ?? "--"}g · Pitch: ${d.pitch ?? "--"}° · Roll: ${d.roll ?? "--"}°</div>
          </div>
        </div>
      </div>

      <div class="card span-6">
        <h2 class="section-title">Kết nối</h2>
        <div class="insight-list">
          <div class="insight-item">
            <div class="insight-title">WiFi</div>
            <div class="insight-desc">${d.wifi ? "Đang kết nối" : "Chưa có trạng thái WiFi"} · RSSI: ${d.rssi ?? "--"} dBm</div>
          </div>

          <div class="insight-item">
            <div class="insight-title">NTP</div>
            <div class="insight-desc">${d.timeSynced ? "Đã đồng bộ thời gian" : "Chưa đồng bộ thời gian"}</div>
          </div>
        </div>
      </div>
    </div>
  `;
}
