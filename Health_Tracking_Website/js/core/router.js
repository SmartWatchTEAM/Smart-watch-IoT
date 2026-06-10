import { renderOverviewTab } from "../tabs/overviewTab.js";
import { renderStatsTab } from "../tabs/statsTab.js";
import { renderDeviceTab } from "../tabs/deviceTab.js";
import { renderAlertsTab } from "../tabs/alertsTab.js";
import { renderSettingsTab } from "../tabs/settingsTab.js";

export function router(state) {
  const page = document.getElementById("pageContent");

  if (!page) {
    console.error("Không tìm thấy #pageContent");
    return;
  }

  try {
    if (state.currentTab === "overview") {
      renderOverviewTab(state);
      return;
    }

    if (state.currentTab === "stats") {
      renderStatsTab(state);
      return;
    }

    if (state.currentTab === "device") {
      renderDeviceTab(state);
      return;
    }

    if (state.currentTab === "alerts") {
      renderAlertsTab(state);
      return;
    }

    if (state.currentTab === "settings") {
      renderSettingsTab(state);
      return;
    }

    renderOverviewTab(state);
  } catch (error) {
    console.error("Router render error:", error);

    page.innerHTML = `
      <div class="card span-12">
        <h2 class="section-title">Lỗi hiển thị giao diện</h2>
        <p class="section-desc">
          Có lỗi JavaScript khi render tab. Mở F12 → Console để xem chi tiết.
        </p>
      </div>
    `;
  }
}