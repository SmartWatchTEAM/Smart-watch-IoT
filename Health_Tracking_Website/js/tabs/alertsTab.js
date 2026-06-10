export function renderAlertsTab(state) {
  const d = state.latestData || {};
  const page = document.getElementById("pageContent");

  page.className = "page-content alerts-page";

  page.innerHTML = `
    <div class="grid-12">
      <div class="card span-6">
        <h2 class="section-title">Fall Detection</h2>
        <p class="section-desc">Theo dõi cảnh báo té ngã từ MPU6500.</p>
        <div class="value-row">
          <span class="value ${d.fall ? "red-text" : "orange-text"}">${d.fall ? "ALERT" : "OK"}</span>
        </div>
        <div class="pill ${d.fall ? "pill-alert" : "pill-ok"}">
          ${d.fall ? "Fall detected" : "No fall detected"}
        </div>
      </div>

      <div class="card span-6">
        <h2 class="section-title">SOS Alert</h2>
        <p class="section-desc">Theo dõi tín hiệu khẩn cấp từ thiết bị.</p>
        <div class="value-row">
          <span class="value red-text">${d.sos ? "ON" : "OFF"}</span>
        </div>
        <div class="pill ${d.sos ? "pill-alert" : "pill-ok"}">
          ${d.sos ? "SOS active" : "Normal"}
        </div>
      </div>

      <div class="card span-12">
        <h2 class="section-title">Nhật ký cảnh báo</h2>
        <p class="section-desc">Sau khi kết nối MySQL, phần này có thể hiển thị lịch sử cảnh báo theo thời gian.</p>

        <div class="insight-list">
          <div class="insight-item">
            <div class="insight-title green-text">Hệ thống sẵn sàng</div>
            <div class="insight-desc">Chưa có dữ liệu lịch sử cảnh báo từ MySQL.</div>
          </div>
        </div>
      </div>
    </div>
  `;
}
