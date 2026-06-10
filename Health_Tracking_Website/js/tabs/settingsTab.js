export function renderSettingsTab() {
  const page = document.getElementById("pageContent");

  page.className = "page-content settings-page";

  page.innerHTML = `
    <div class="grid-12">
      <div class="card span-6">
        <h2 class="section-title">Cài đặt thiết bị</h2>
        <p class="section-desc">Các thông số này dùng cho cấu trúc Firebase hiện tại.</p>

        <div class="insight-list">
          <div class="insight-item">
            <div class="insight-title">Device ID</div>
            <div class="insight-desc">watch_001</div>
          </div>

          <div class="insight-item">
            <div class="insight-title">Firebase Latest Path</div>
            <div class="insight-desc">devices/watch_001/latest</div>
          </div>

          <div class="insight-item">
            <div class="insight-title">Firebase History Path</div>
            <div class="insight-desc">devices/watch_001/history/YYYY-MM-DD</div>
          </div>
        </div>
      </div>

      <div class="card span-6">
        <h2 class="section-title">Cài đặt thống kê</h2>
        <p class="section-desc">Chu kỳ lưu lịch sử đang được cấu hình trong code ESP32, tab này dùng để mô phỏng cấu hình.</p>

        <div class="filter-row" style="margin-top:18px;">
          <label>
            <div class="kpi-label">Chu kỳ lưu dữ liệu đề xuất</div>
            <select class="select">
              <option>1 phút</option>
              <option>30 giây</option>
              <option>10 giây</option>
              <option>5 giây</option>
            </select>
          </label>

          <button class="btn" type="button">Lưu cài đặt</button>
        </div>
      </div>
    </div>
  `;
}
