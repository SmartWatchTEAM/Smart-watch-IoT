import {
  numberOrNull,
  clamp,
  getLastUpdateText,
  isTrue
} from "../utils/format.js";

const icons = {
  heart: `<svg viewBox="0 0 24 24" fill="none"><path d="M20.8 8.6c0 5.2-8.8 10.4-8.8 10.4S3.2 13.8 3.2 8.6A4.6 4.6 0 0 1 12 6.7a4.6 4.6 0 0 1 8.8 1.9Z" stroke="currentColor" fill="currentColor" fill-opacity=".12"/></svg>`,
  drop: `<svg viewBox="0 0 24 24" fill="none"><path d="M12 3s6 6.2 6 11a6 6 0 0 1-12 0c0-4.8 6-11 6-11Z" stroke="currentColor" fill="currentColor" fill-opacity=".12"/></svg>`,
  fall: `<svg viewBox="0 0 24 24" fill="none"><path d="M13 4.5a2 2 0 1 1-4 0 2 2 0 0 1 4 0Z" stroke="currentColor"/><path d="M10.5 7.5 8 11l3.5 2.5-2 5" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/><path d="m12 10 3 2 2.5-2.5" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/><path d="M15 18h5" stroke="currentColor" stroke-linecap="round"/></svg>`,
  siren: `<svg viewBox="0 0 24 24" fill="none"><path d="M7 19v-6a5 5 0 0 1 10 0v6" stroke="currentColor"/><path d="M5 19h14" stroke="currentColor" stroke-linecap="round"/><path d="M12 3v2M4.8 6.2 6.2 7.6M19.2 6.2l-1.4 1.4" stroke="currentColor" stroke-linecap="round"/></svg>`,
  shoe: `<svg viewBox="0 0 24 24" fill="none"><path d="M5 14c2 2 5 3 10 3h4c1 0 2-1 1-2l-2-3c-.4-.6-1-.9-1.8-.7l-3.5.8-3.7-5.5-2 1.2 2.4 5.2H5Z" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/></svg>`,
  route: `<svg viewBox="0 0 24 24" fill="none"><path d="M6 18c2-6 10-1 12-8" stroke="currentColor" stroke-linecap="round"/><circle cx="6" cy="18" r="2" stroke="currentColor"/><circle cx="18" cy="10" r="2" stroke="currentColor"/></svg>`,
  flame: `<svg viewBox="0 0 24 24" fill="none"><path d="M12 21a7 7 0 0 0 7-7c0-4-2.5-6.2-5-9 .2 3-1.3 4.2-3 5.7C9.5 12 8 13.5 8 16a4 4 0 0 0 4 5Z" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/></svg>`,
  run: `<svg viewBox="0 0 24 24" fill="none"><path d="M13 5a2 2 0 1 1-4 0 2 2 0 0 1 4 0Z" stroke="currentColor"/><path d="m10 8-2 4 4 2 2 5" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/><path d="m13 10 3 2 3-2" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/><path d="M7 20h4" stroke="currentColor" stroke-linecap="round"/></svg>`,
  clock: `<svg viewBox="0 0 24 24" fill="none"><circle cx="12" cy="12" r="8" stroke="currentColor"/><path d="M12 8v5l3 2" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/></svg>`,
  refresh: `<svg viewBox="0 0 24 24" fill="none"><path d="M20 7v5h-5" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/><path d="M4 17v-5h5" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/><path d="M19 12a7 7 0 0 0-12-5M5 12a7 7 0 0 0 12 5" stroke="currentColor" stroke-linecap="round"/></svg>`,
  signal: `<svg viewBox="0 0 24 24" fill="none"><path d="M5 18v-4M10 18v-7M15 18V8M20 18V5" stroke="currentColor" stroke-linecap="round"/></svg>`,
  chip: `<svg viewBox="0 0 24 24" fill="none"><rect x="7" y="7" width="10" height="10" rx="2" stroke="currentColor"/><path d="M10 3v4M14 3v4M10 17v4M14 17v4M3 10h4M3 14h4M17 10h4M17 14h4" stroke="currentColor" stroke-linecap="round"/></svg>`,
  battery: `<svg viewBox="0 0 24 24" fill="none"><rect x="4" y="7" width="14" height="10" rx="2" stroke="currentColor"/><path d="M20 10v4" stroke="currentColor" stroke-linecap="round"/><path d="M7 10v4M10 10v4M13 10v4" stroke="currentColor" stroke-linecap="round"/></svg>`,
  finger: `<svg viewBox="0 0 24 24" fill="none"><path d="M12 3a5 5 0 0 0-5 5v5" stroke="currentColor" stroke-linecap="round"/><path d="M12 6a2 2 0 0 0-2 2v8" stroke="currentColor" stroke-linecap="round"/><path d="M14 9v8M17 11v5" stroke="currentColor" stroke-linecap="round"/><path d="M7 17c1 3 3 4 6 4 4 0 6-2.5 6-6" stroke="currentColor" stroke-linecap="round"/></svg>`
};

export function renderOverviewTab(state) {
  const d = state.latestData || {};
  const hasLatest = Object.keys(d).length > 0;
  const page = document.getElementById("pageContent");

  if (!page) return;

  const bpm = numberOrNull(d.bpm ?? d.heartRate);
  const spo2 = numberOrNull(d.spo2);
  const fall = isTrue(d.fall);
  const sos = isTrue(d.sos);
  const finger = isTrue(d.finger);
  const heartAlert = isTrue(d.heartAlert) || (bpm !== null && (bpm > 110 || bpm < 50));

  const daily = state.dailySummary || {};
  const steps = numberOrNull(d.steps ?? daily.steps);
  const stepGoal = 5000;
  const distanceKm = getDistanceKm(d, daily, steps);
  const calories = getCalories(d, daily, steps);
  const cadence = getCadence(d, daily);

  const stepGoalPercent = steps !== null
    ? clamp((steps / stepGoal) * 100, 0, 100)
    : 0;

  const historyRows = Array.isArray(state.historyRows) ? state.historyRows : [];
  const safetyOk = hasLatest && !fall && !sos && !heartAlert;

  page.className = "page-content overview-page";

  page.innerHTML = `
    <div class="grid-12 overview-clean-grid">
      <div class="card span-3 overview-summary-card summary-heart">
        <div class="summary-card-top">
          <div class="icon red">${icons.heart}</div>
          <div>
            <div class="label">Nhịp tim</div>
            <div class="summary-sensor">MAX30102</div>
          </div>
        </div>

        <div class="summary-value-row">
          <span class="value heart">${bpm ?? "--"}</span>
          <span class="unit">bpm</span>
        </div>

        <div class="summary-status-line ${getBpmLevel(bpm)}">
          <span class="status-dot"></span>
          <span>${getBpmStatusText(bpm)}</span>
        </div>
      </div>

      <div class="card span-3 overview-summary-card summary-spo2">
        <div class="summary-card-top">
          <div class="icon cyan">${icons.drop}</div>
          <div>
            <div class="label">SpO2</div>
            <div class="summary-sensor">Blood Oxygen</div>
          </div>
        </div>

        <div class="summary-value-row">
          <span class="value cyan-text">${spo2 ?? "--"}</span>
          <span class="unit">%</span>
        </div>

        <div class="summary-status-line ${getSpo2Level(spo2)}">
          <span class="status-dot"></span>
          <span>${getSpo2StatusText(spo2)}</span>
        </div>
      </div>

      <div class="card span-3 overview-summary-card summary-steps">
        <div class="summary-card-top">
          <div class="icon green">${icons.shoe}</div>
          <div>
            <div class="label">Bước chân</div>
            <div class="summary-sensor">Activity</div>
          </div>
        </div>

        <div class="summary-value-row">
          <span class="value green-text">${steps !== null ? Math.round(steps).toLocaleString("en-US") : "--"}</span>
          <span class="unit">bước</span>
        </div>

        <div class="summary-status-line">
          <span class="status-dot"></span>
          <span>${steps !== null ? "Hoạt động tốt" : "Chờ dữ liệu"}</span>
        </div>
      </div>

      <div class="card span-3 overview-summary-card summary-safe">
        <div class="summary-card-top">
          <div class="icon purple">${icons.siren}</div>
          <div>
            <div class="label">Trạng thái</div>
            <div class="summary-sensor">Safety</div>
          </div>
        </div>

        <div class="summary-value-row">
          <span class="value ${safetyOk ? "green-text" : (hasLatest ? "red-text" : "")}">
            ${hasLatest ? (safetyOk ? "An toàn" : "Cảnh báo") : "--"}
          </span>
        </div>

        <div class="summary-status-line ${safetyOk ? "" : "danger"}">
          <span class="status-dot"></span>
          <span>${hasLatest ? (safetyOk ? "Không có cảnh báo" : "Cần kiểm tra") : "Chờ dữ liệu"}</span>
        </div>
      </div>

      <div class="card span-12 system overview-system-strip">
        <div class="system-grid">
          <div class="system-item">
            <div class="system-icon">${icons.signal}</div>
            <div>
              <div class="system-label">Kết nối</div>
              <div class="system-value">${hasLatest ? "Tốt" : "Chờ dữ liệu"}</div>
              <div class="system-sub">${hasLatest ? "Firebase realtime" : "Waiting"}</div>
            </div>
          </div>

          <div class="system-item">
            <div class="system-icon">${icons.battery}</div>
            <div>
              <div class="system-label">Pin thiết bị</div>
              <div class="system-value">${formatBattery(d.battery)}</div>
              <div class="system-sub">${hasLatest ? "Đang hoạt động" : "Waiting"}</div>
            </div>
          </div>

          <div class="system-item">
            <div class="system-icon">${icons.chip}</div>
            <div>
              <div class="system-label">Cảm biến</div>
              <div class="system-value">${finger ? "Hoạt động tốt" : (hasLatest ? "Chưa đặt tay" : "--")}</div>
              <div class="system-sub">MAX30102 + MPU6500</div>
            </div>
          </div>

          <div class="system-item">
            <div class="system-icon">${icons.refresh}</div>
            <div>
              <div class="system-label">Cập nhật lúc</div>
              <div class="system-value">${getLastUpdateText(state.lastUpdateTime)}</div>
              <div class="system-sub">Realtime</div>
            </div>
          </div>
        </div>
      </div>

      <div class="card span-6 activity-card-v2 overview-activity-panel">
        <div class="activity-title">Activity Summary <span>(Today)</span></div>

        <div class="activity-grid-modern">
          <div class="activity-metric activity-steps">
            <div class="activity-icon green-text">${icons.shoe}</div>
            <strong class="green-text">${steps !== null ? Math.round(steps).toLocaleString("en-US") : "--"}</strong>
            <small>Steps</small>

            <div class="metric-goal">Goal: ${Math.round(stepGoal).toLocaleString("en-US")}</div>
            <div class="metric-progress">
              <span style="width:${steps !== null ? stepGoalPercent : 0}%"></span>
            </div>
            <div class="metric-percent">${steps !== null ? Math.round(stepGoalPercent) : "--"}%</div>
          </div>

          <div class="activity-metric">
            <div class="activity-icon cyan-text">${icons.route}</div>
            <strong class="cyan-text">${distanceKm !== null ? distanceKm.toFixed(2) : "--"}</strong>
            <small>km</small>
            <div class="metric-name">Distance</div>
          </div>

          <div class="activity-metric">
            <div class="activity-icon orange-text">${icons.flame}</div>
            <strong class="orange-text">${calories !== null ? Math.round(calories) : "--"}</strong>
            <small>kcal</small>
            <div class="metric-name">Calories</div>
          </div>

          <div class="activity-metric">
            <div class="activity-icon purple-text">${icons.run}</div>
            <strong class="purple-text">${cadence !== null ? Math.round(cadence) : "--"}</strong>
            <small>SPM</small>
            <div class="metric-name">Cadence</div>
          </div>
        </div>
      </div>

      <div class="card span-6 overview-alert-panel">
        <div class="activity-title">Trạng thái cảnh báo</div>

        <div class="overview-alert-list">
          ${renderAlertRow({
            icon: icons.fall,
            title: "Ngã",
            desc: fall ? "Đã phát hiện" : "Không phát hiện",
            ok: hasLatest && !fall,
            waiting: !hasLatest
          })}

          ${renderAlertRow({
            icon: icons.siren,
            title: "SOS",
            desc: sos ? "Đang kích hoạt" : "Không kích hoạt",
            ok: hasLatest && !sos,
            waiting: !hasLatest
          })}

          ${renderAlertRow({
            icon: icons.heart,
            title: "Nhịp tim",
            desc: heartAlert ? (bpm > 110 ? "Cao hơn 110 bpm" : "Thấp hơn 50 bpm") : "Trong ngưỡng",
            ok: hasLatest && !heartAlert,
            waiting: !hasLatest
          })}

        </div>
      </div>

      <div class="card span-12 overview-history-card">
        <div class="history-card-head">
          <div class="activity-title">Lịch sử đo gần đây</div>
          <button class="history-more-btn" id="historyMoreBtn" type="button">Xem tất cả</button>
        </div>

        <div class="overview-history-table-wrap">
          <table class="overview-history-table">
            <thead>
              <tr>
                <th>Thời gian</th>
                <th>Nhịp tim (bpm)</th>
                <th>SpO2 (%)</th>
                <th>Bước chân</th>
                <th>Trạng thái</th>
                <th>Ghi chú</th>
              </tr>
            </thead>

            <tbody>
              ${renderHistoryRows(historyRows, d, hasLatest, state.lastUpdateTime)}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  `;

  const historyMoreBtn = document.getElementById("historyMoreBtn");

  if (historyMoreBtn) {
    historyMoreBtn.addEventListener("click", () => {
      const statsTabButton = document.querySelector('[data-tab="stats"]');

      if (statsTabButton) {
        statsTabButton.click();
      }
    });
  }
}

function getDistanceKm(d, daily, steps) {
  const explicit = numberOrNull(d.distanceKm ?? daily.distanceKm ?? d.distance ?? daily.distance);
  if (explicit !== null) return explicit;
  return steps !== null ? steps * 0.000709 : null;
}

function getCalories(d, daily, steps) {
  const explicit = numberOrNull(d.calories ?? daily.calories ?? d.kcal ?? daily.kcal);
  if (explicit !== null) return explicit;
  return steps !== null ? steps * 0.0394 : null;
}

function getCadence(d, daily) {
  return numberOrNull(
    d.cadenceSPM ??
    daily.cadenceSPM ??
    d.cadence ??
    daily.cadence ??
    d.spm ??
    daily.spm ??
    d.stepsPerMinute ??
    daily.stepsPerMinute
  );
}

function getBpmLevel(bpm) {
  if (!bpm) return "";
  if (bpm < 60 || bpm > 100) return "warning";
  return "";
}

function getSpo2Level(spo2) {
  if (!spo2) return "";
  if (spo2 < 95) return "warning";
  return "";
}

function getBpmStatusText(bpm) {
  if (!bpm) return "Chờ kết quả";
  if (bpm < 60) return "Nhịp tim thấp";
  if (bpm > 100) return "Nhịp tim cao";
  return "Bình thường";
}

function getSpo2StatusText(spo2) {
  if (!spo2) return "Chờ kết quả";
  if (spo2 < 92) return "SpO2 thấp";
  if (spo2 < 95) return "Cần theo dõi";
  return "Bình thường";
}

function formatBattery(battery) {
  if (battery === undefined || battery === null || battery === "") return "--";
  return `${battery}%`;
}

function renderAlertRow({ icon, title, desc, ok, waiting, goodText = "An toàn" }) {
  const badgeClass = waiting ? "" : (ok ? "pill-ok" : "pill-alert");
  const badgeText = waiting ? "Waiting" : (ok ? goodText : "Cảnh báo");

  return `
    <div class="overview-alert-row">
      <div class="overview-alert-icon ${ok ? "green" : "red"}">${icon}</div>

      <div>
        <div class="overview-alert-title">${title}</div>
        <div class="overview-alert-desc">${waiting ? "Chờ dữ liệu" : desc}</div>
      </div>

      <span class="pill ${badgeClass}">${badgeText}</span>
    </div>
  `;
}

function renderHistoryRows(historyRows, latestData, hasLatest, lastUpdateTime) {
  let rows = Array.isArray(historyRows) ? [...historyRows] : [];

  rows.sort((a, b) => getRowTime(b) - getRowTime(a));
  rows = rows.slice(0, 5);

  if (!rows.length && hasLatest) {
    rows = [{
      ...latestData,
      createdAt: lastUpdateTime || Date.now(),
      updatedAt: lastUpdateTime || Date.now()
    }];
  }

  if (!rows.length) {
    return `
      <tr>
        <td colspan="6" class="history-empty">Chưa có dữ liệu lịch sử đo.</td>
      </tr>
    `;
  }

  return rows.map((row) => {
    const bpm = numberOrNull(row.bpm ?? row.heartRate);
    const spo2 = numberOrNull(row.spo2);
    const steps = numberOrNull(row.steps);
    const fall = isTrue(row.fall);
    const sos = isTrue(row.sos);
    const status = getRowStatus(bpm, spo2, fall, sos);

    return `
      <tr>
        <td>${formatRowTime(row)}</td>
        <td class="${status.level === "warning" ? "orange-text" : "green-text"}">${bpm ?? "--"}</td>
        <td class="${spo2 !== null && spo2 < 95 ? "orange-text" : "green-text"}">${spo2 ?? "--"}</td>
        <td>${steps !== null ? Math.round(steps).toLocaleString("en-US") : "--"}</td>
        <td>
          <span class="pill ${status.level === "danger" ? "pill-alert" : status.level === "ok" ? "pill-ok" : ""}">
            ${status.text}
          </span>
        </td>
        <td>${status.note}</td>
      </tr>
    `;
  }).join("");
}

function getRowStatus(bpm, spo2, fall, sos) {
  if (sos) return { level: "danger", text: "SOS", note: "Cần kiểm tra" };
  if (fall) return { level: "danger", text: "Té ngã", note: "Cần kiểm tra" };
  if (bpm !== null && (bpm < 60 || bpm > 100)) {
    return { level: "warning", text: "Nhịp tim cao", note: "Đo lại khi nghỉ ngơi" };
  }
  if (spo2 !== null && spo2 < 95) {
    return { level: "warning", text: "SpO2 thấp", note: "Nên đo lại" };
  }
  return { level: "ok", text: "Bình thường", note: "--" };
}

function getRowTime(row) {
  return Number(row.createdAt || row.updatedAt || row.timestamp || 0);
}

function formatRowTime(row) {
  const timestamp = getRowTime(row);

  if (!timestamp) return "--:--";

  return new Date(timestamp).toLocaleString("vi-VN", {
    day: "2-digit",
    month: "2-digit",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false
  });
}
