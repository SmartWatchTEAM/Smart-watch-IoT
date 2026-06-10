import { getStatsByDay, getStatsByMonth } from "../services/statsService.js";

let chartInstance = null;
let currentStats = null;
let currentMode = "day";
let currentFilterValue = "";

export function renderStatsTab() {
  const page = document.getElementById("pageContent");
  page.className = "page-content stats-page";

  page.innerHTML = `
    <div class="grid-12">
      <div class="card span-12">
        <div class="filter-row">
          <div>
            <h2 class="section-title">Bộ lọc thống kê</h2>
            <p class="section-desc">Chọn kiểu thống kê theo ngày hoặc theo tháng. Dữ liệu lấy trực tiếp từ Firebase.</p>
          </div>

          <select class="select" id="statsMode">
            <option value="day">Theo ngày</option>
            <option value="month">Theo tháng</option>
          </select>

          <input class="input-date" id="dateInput" type="date" />
          <input class="input-month hidden" id="monthInput" type="month" />

          <button class="btn" id="loadStats" type="button">Xem thống kê</button>
          <button class="btn btn-success" id="exportStats" type="button">Xuất PDF</button>
        </div>
      </div>

      <div class="card span-12 hidden" id="statsMessageCard">
        <div id="statsMessage" class="section-desc"></div>
      </div>

      <div class="card span-3 kpi-card">
        <div class="kpi-label">BPM trung bình</div>
        <div class="kpi-value red-text" id="avgBpm">--</div>
        <div class="kpi-note">Dữ liệu từ Firebase</div>
      </div>

      <div class="card span-3 kpi-card">
        <div class="kpi-label">SpO2 trung bình</div>
        <div class="kpi-value cyan-text" id="avgSpo2">--</div>
        <div class="kpi-note">Dữ liệu từ Firebase</div>
      </div>

      <div class="card span-3 kpi-card">
        <div class="kpi-label">Fall Alert</div>
        <div class="kpi-value orange-text" id="fallCount">--</div>
        <div class="kpi-note">Tổng số lần cảnh báo</div>
      </div>

      <div class="card span-3 kpi-card">
        <div class="kpi-label">SOS Alert</div>
        <div class="kpi-value red-text" id="sosCount">--</div>
        <div class="kpi-note">Tổng số lần khẩn cấp</div>
      </div>

      <div class="card span-6 chart-card">
        <h2 class="section-title">Biểu đồ BPM & SpO2</h2>
        <p class="section-desc">Hiển thị dữ liệu lịch sử theo thời gian.</p>
        <canvas id="statsChart"></canvas>
      </div>

      <div class="card span-6">
        <h2 class="section-title">Phân tích nhanh</h2>
        <p class="section-desc">Tổng hợp trạng thái sức khỏe từ dữ liệu Firebase.</p>

        <div class="insight-list" id="insightList">
          <div class="insight-item">
            <div class="insight-title cyan-text">Đang chờ dữ liệu</div>
            <div class="insight-desc">Chọn ngày hoặc tháng rồi bấm Xem thống kê.</div>
          </div>
        </div>
      </div>
    </div>
  `;

  bindStatsEvents();
  loadStats();
}

function bindStatsEvents() {
  const mode = document.getElementById("statsMode");
  const dateInput = document.getElementById("dateInput");
  const monthInput = document.getElementById("monthInput");

  const today = new Date().toISOString().slice(0, 10);
  dateInput.value = today;
  monthInput.value = today.slice(0, 7);

  mode.addEventListener("change", () => {
    const isMonth = mode.value === "month";

    dateInput.classList.toggle("hidden", isMonth);
    monthInput.classList.toggle("hidden", !isMonth);
  });

  document.getElementById("loadStats").addEventListener("click", loadStats);
  document.getElementById("exportStats").addEventListener("click", exportStatsToPDF);
}

async function loadStats() {
  currentMode = document.getElementById("statsMode").value;

  clearMessage();
  setLoadingState(true);

  try {
    let result;

    if (currentMode === "day") {
      currentFilterValue = document.getElementById("dateInput").value;
      result = await getStatsByDay(currentFilterValue);
    } else {
      currentFilterValue = document.getElementById("monthInput").value;
      result = await getStatsByMonth(currentFilterValue);
    }

    currentStats = normalizeStats(result);
    renderStatsResult(currentStats);

    if (!hasStatsData(currentStats)) {
      showMessage(
        `Không có dữ liệu cho ${currentMode === "day" ? "ngày" : "tháng"} ${currentFilterValue}.`,
        "warning"
      );
    }
  } catch (error) {
    console.error("Không lấy được dữ liệu thống kê từ Firebase:", error);

    currentStats = getEmptyStats();
    renderStatsResult(currentStats);

    showMessage(
      "Không đọc được dữ liệu Firebase. Hãy kiểm tra kết nối internet, Firebase Config hoặc Database Rules.",
      "error"
    );
  } finally {
    setLoadingState(false);
  }
}

function normalizeStats(result) {
  const data = result || {};

  return {
    success: data.success ?? false,
    source: data.source ?? "firebase",
    mode: data.mode ?? currentMode,
    device_id: data.device_id ?? "watch_001",
    date: data.date ?? null,
    month: data.month ?? null,

    avg_bpm: emptyToNull(data.avg_bpm),
    avg_spo2: emptyToNull(data.avg_spo2),
    avg_acc: emptyToNull(data.avg_acc),
    avg_battery: emptyToNull(data.avg_battery),

    total_fall: numberOrZero(data.total_fall),
    total_sos: numberOrZero(data.total_sos),
    total_records: numberOrZero(data.total_records),

    labels: Array.isArray(data.labels) ? data.labels : [],
    bpm_series: Array.isArray(data.bpm_series) ? data.bpm_series : [],
    spo2_series: Array.isArray(data.spo2_series) ? data.spo2_series : [],
    battery_series: Array.isArray(data.battery_series) ? data.battery_series : [],
    steps_series: Array.isArray(data.steps_series) ? data.steps_series : [],
  };
}

function renderStatsResult(result) {
  const hasData = hasStatsData(result);

  document.getElementById("avgBpm").textContent = hasData ? displayValue(result.avg_bpm) : "--";
  document.getElementById("avgSpo2").textContent = hasData ? displayValue(result.avg_spo2) : "--";
  document.getElementById("fallCount").textContent = hasData ? displayValue(result.total_fall) : "--";
  document.getElementById("sosCount").textContent = hasData ? displayValue(result.total_sos) : "--";

  renderChart(result);
  renderInsights(result);
}

function renderChart(result) {
  const ctx = document.getElementById("statsChart");
  const hasData = hasStatsData(result);

  if (!ctx) return;

  if (chartInstance) {
    chartInstance.destroy();
  }

  const labels = hasData ? result.labels : [];
  const bpmData = hasData ? result.bpm_series : [];
  const spo2Data = hasData ? result.spo2_series : [];
  const batteryData = hasData ? result.battery_series : [];

  chartInstance = new Chart(ctx, {
    type: "line",
    data: {
      labels,
      datasets: [
        {
          label: "BPM",
          data: bpmData,
          yAxisID: "yBpm",
          borderColor: "#38bdf8",
          backgroundColor: "rgba(56, 189, 248, 0.16)",
          pointBackgroundColor: "#38bdf8",
          pointBorderColor: "#38bdf8",
          borderWidth: 2.5,
          tension: 0.35,
          pointRadius: 4,
          pointHoverRadius: 6,
          fill: false,
          spanGaps: true,
        },
        {
          label: "SpO2",
          data: spo2Data,
          yAxisID: "ySpo2",
          borderColor: "#ff4d6d",
          backgroundColor: "rgba(255, 77, 109, 0.16)",
          pointBackgroundColor: "#ff4d6d",
          pointBorderColor: "#ff4d6d",
          borderWidth: 2.5,
          tension: 0.35,
          pointRadius: 4,
          pointHoverRadius: 6,
          fill: false,
          spanGaps: true,
        },
        {
          label: "Battery",
          data: batteryData,
          yAxisID: "ySpo2",
          borderColor: "#f59e0b",
          backgroundColor: "rgba(245, 158, 11, 0.16)",
          pointBackgroundColor: "#f59e0b",
          pointBorderColor: "#f59e0b",
          borderWidth: 2,
          tension: 0.35,
          hidden: true,
          fill: false,
          spanGaps: true,
        }
      ]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: {
        mode: "index",
        intersect: false,
      },
      plugins: {
        legend: {
          labels: {
            color: "#cbd5e1",
            usePointStyle: false,
            boxWidth: 46,
            boxHeight: 10,
            font: {
              size: 13,
              weight: "700"
            }
          }
        },
        tooltip: {
          enabled: true,
          backgroundColor: "rgba(15, 23, 42, 0.96)",
          titleColor: "#f8fafc",
          bodyColor: "#cbd5e1",
          borderColor: "rgba(148, 163, 184, 0.22)",
          borderWidth: 1,
          padding: 12,
          callbacks: {
            label(context) {
              const label = context.dataset.label || "";
              const value = context.parsed.y;

              if (value === null || value === undefined) return `${label}: --`;
              if (label === "BPM") return `${label}: ${value} BPM`;
              if (label === "SpO2") return `${label}: ${value}%`;
              if (label === "Battery") return `${label}: ${value}%`;

              return `${label}: ${value}`;
            }
          }
        }
      },
      scales: {
        x: {
          ticks: {
            color: "#94a3b8",
            maxRotation: 0,
            autoSkip: true,
            maxTicksLimit: 8,
          },
          grid: {
            color: "rgba(148,163,184,0.12)"
          }
        },
        yBpm: {
          type: "linear",
          position: "left",
          min: 40,
          max: 160,
          ticks: {
            color: "#94a3b8"
          },
          grid: {
            color: "rgba(148,163,184,0.12)"
          },
          title: {
            display: true,
            text: "BPM",
            color: "#94a3b8"
          }
        },
        ySpo2: {
          type: "linear",
          position: "right",
          min: 80,
          max: 100,
          ticks: {
            color: "#94a3b8",
            callback(value) {
              return `${value}%`;
            }
          },
          grid: {
            drawOnChartArea: false
          },
          title: {
            display: true,
            text: "SpO2 (%)",
            color: "#94a3b8"
          }
        }
      }
    }
  });
}

function renderInsights(result) {
  const insightList = document.getElementById("insightList");

  if (!hasStatsData(result)) {
    insightList.innerHTML = `
      <div class="insight-item">
        <div class="insight-title orange-text">Không có dữ liệu</div>
        <div class="insight-desc">Ngày/tháng bạn chọn chưa có bản ghi nào trong Firebase history.</div>
      </div>

      <div class="insight-item">
        <div class="insight-title cyan-text">Gợi ý kiểm tra</div>
        <div class="insight-desc">Hãy chọn ngày khác hoặc để ESP32 gửi dữ liệu lịch sử lên Firebase.</div>
      </div>
    `;
    return;
  }

  const avgBpm = Number(result.avg_bpm);
  const avgSpo2 = Number(result.avg_spo2);
  const totalFall = Number(result.total_fall || 0);
  const totalSos = Number(result.total_sos || 0);

  insightList.innerHTML = `
    <div class="insight-item">
      <div class="insight-title ${avgBpm >= 60 && avgBpm <= 100 ? "green-text" : "orange-text"}">
        ${avgBpm >= 60 && avgBpm <= 100 ? "Nhịp tim ổn định" : "Nhịp tim cần theo dõi"}
      </div>
      <div class="insight-desc">BPM trung bình hiện tại là ${displayValue(result.avg_bpm)} BPM.</div>
    </div>

    <div class="insight-item">
      <div class="insight-title ${avgSpo2 >= 95 ? "cyan-text" : "orange-text"}">
        ${avgSpo2 >= 95 ? "SpO2 tốt" : "SpO2 cần theo dõi"}
      </div>
      <div class="insight-desc">SpO2 trung bình hiện tại là ${displayValue(result.avg_spo2)}%.</div>
    </div>

    <div class="insight-item">
      <div class="insight-title ${totalFall > 0 ? "orange-text" : "green-text"}">
        ${totalFall > 0 ? "Có cảnh báo té ngã" : "Không có cảnh báo té ngã"}
      </div>
      <div class="insight-desc">Tổng số lần Fall Alert: ${totalFall}.</div>
    </div>

    <div class="insight-item">
      <div class="insight-title ${totalSos > 0 ? "red-text" : "green-text"}">
        ${totalSos > 0 ? "Có cảnh báo SOS" : "SOS an toàn"}
      </div>
      <div class="insight-desc">Tổng số lần SOS Alert: ${totalSos}.</div>
    </div>
  `;
}

async function exportStatsToPDF() {
  if (!currentStats || !hasStatsData(currentStats)) {
    alert("Không có dữ liệu để xuất PDF cho thời gian bạn chọn.");
    return;
  }

  if (!window.html2canvas || !window.jspdf) {
    alert("Thiếu thư viện xuất PDF. Hãy thêm html2canvas và jsPDF vào index.html.");
    return;
  }

  const chartCanvas = document.getElementById("statsChart");
  const chartImage = chartCanvas ? chartCanvas.toDataURL("image/png", 1.0) : "";

  const report = document.createElement("div");

  report.style.position = "fixed";
  report.style.left = "-99999px";
  report.style.top = "0";
  report.style.width = "900px";
  report.style.background = "#ffffff";
  report.style.color = "#0f172a";
  report.style.fontFamily = "Arial, sans-serif";
  report.style.padding = "42px";
  report.style.boxSizing = "border-box";

  report.innerHTML = `
    <div style="border:1px solid #e2e8f0;border-radius:24px;overflow:hidden;box-shadow:0 20px 60px rgba(15,23,42,0.12);">
      <div style="padding:34px 38px;background:linear-gradient(135deg,#020617,#0f172a 55%,#075985);color:white;">
        <div style="font-size:13px;letter-spacing:2px;text-transform:uppercase;color:#7dd3fc;font-weight:700;">
          Smart Health Watch
        </div>

        <div style="font-size:34px;font-weight:900;margin-top:8px;">
          Báo cáo thống kê sức khỏe
        </div>

        <div style="margin-top:10px;color:#cbd5e1;font-size:15px;">
          Phân tích dữ liệu trung bình theo ${currentMode === "day" ? "ngày" : "tháng"} từ Firebase Realtime Database
        </div>
      </div>

      <div style="padding:30px 38px;">
        <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:14px;margin-bottom:26px;">
          ${reportInfoBox("Kiểu thống kê", currentMode === "day" ? "Theo ngày" : "Theo tháng")}
          ${reportInfoBox("Thời gian", currentFilterValue || "--")}
          ${reportInfoBox("Ngày xuất", new Date().toLocaleDateString("vi-VN"))}
        </div>

        <div style="display:grid;grid-template-columns:repeat(4,1fr);gap:14px;margin-bottom:30px;">
          ${reportKpi("BPM trung bình", currentStats.avg_bpm ?? "--", "BPM", "#ff4d6d")}
          ${reportKpi("SpO2 trung bình", currentStats.avg_spo2 ?? "--", "%", "#0ea5e9")}
          ${reportKpi("Fall Alert", currentStats.total_fall ?? "--", "lần", "#f59e0b")}
          ${reportKpi("SOS Alert", currentStats.total_sos ?? "--", "lần", "#ef4444")}
        </div>

        <div style="margin-bottom:28px;">
          <div style="font-size:22px;font-weight:900;margin-bottom:12px;">
            Biểu đồ BPM và SpO2
          </div>

          <div style="border:1px solid #e2e8f0;border-radius:20px;padding:18px;background:#f8fafc;">
            ${chartImage ? `<img src="${chartImage}" style="width:100%;display:block;" />` : ""}
          </div>
        </div>

        <div>
          <div style="font-size:22px;font-weight:900;margin-bottom:12px;">
            Bảng dữ liệu chi tiết
          </div>

          <table style="width:100%;border-collapse:collapse;overflow:hidden;border-radius:14px;font-size:14px;">
            <thead>
              <tr style="background:#0f172a;color:white;">
                <th style="padding:12px;text-align:left;">Thời gian</th>
                <th style="padding:12px;text-align:left;">BPM</th>
                <th style="padding:12px;text-align:left;">SpO2</th>
                <th style="padding:12px;text-align:left;">Số bước</th>
              </tr>
            </thead>

            <tbody>
              ${buildReportRows()}
            </tbody>
          </table>
        </div>

        <div style="margin-top:28px;padding-top:18px;border-top:1px solid #e2e8f0;color:#64748b;font-size:13px;display:flex;justify-content:space-between;">
          <span>Generated by Smart Health Watch Dashboard</span>
          <span>Device ID: watch_001</span>
        </div>
      </div>
    </div>
  `;

  document.body.appendChild(report);

  const canvas = await html2canvas(report, {
    scale: 2,
    useCORS: true,
    backgroundColor: "#ffffff"
  });

  document.body.removeChild(report);

  const imgData = canvas.toDataURL("image/png");

  const { jsPDF } = window.jspdf;
  const pdf = new jsPDF("p", "mm", "a4");

  const pageWidth = 210;
  const pageHeight = 297;
  const margin = 10;

  const imgWidth = pageWidth - margin * 2;
  const imgHeight = canvas.height * imgWidth / canvas.width;

  let heightLeft = imgHeight;
  let position = margin;

  pdf.addImage(imgData, "PNG", margin, position, imgWidth, imgHeight);
  heightLeft -= pageHeight - margin * 2;

  while (heightLeft > 0) {
    position = heightLeft - imgHeight + margin;
    pdf.addPage();
    pdf.addImage(imgData, "PNG", margin, position, imgWidth, imgHeight);
    heightLeft -= pageHeight - margin * 2;
  }

  const safeFilter = String(currentFilterValue || "report")
    .replaceAll("/", "-")
    .replaceAll(":", "-")
    .replaceAll(" ", "-");

  pdf.save(`smart-health-watch-${currentMode}-${safeFilter}.pdf`);
}

function reportInfoBox(label, value) {
  return `
    <div style="background:#f8fafc;border:1px solid #e2e8f0;border-radius:18px;padding:16px;">
      <div style="font-size:12px;color:#64748b;font-weight:700;">${label}</div>
      <div style="font-size:18px;font-weight:900;margin-top:6px;">${value}</div>
    </div>
  `;
}

function reportKpi(label, value, unit, color) {
  return `
    <div style="border:1px solid #e2e8f0;border-radius:18px;padding:16px;background:#ffffff;">
      <div style="font-size:12px;color:#64748b;font-weight:800;">${label}</div>
      <div style="font-size:34px;font-weight:900;margin-top:8px;color:${color};">${value}</div>
      <div style="font-size:13px;color:#64748b;margin-top:4px;">${unit}</div>
    </div>
  `;
}

function buildReportRows() {
  const labels = currentStats.labels ?? [];
  const bpm = currentStats.bpm_series ?? [];
  const spo2 = currentStats.spo2_series ?? [];
  const steps = currentStats.steps_series ?? [];

  return labels.map((label, index) => `
    <tr style="background:${index % 2 === 0 ? "#ffffff" : "#f8fafc"};">
      <td style="padding:11px 12px;border:1px solid #e2e8f0;">${label}</td>
      <td style="padding:11px 12px;border:1px solid #e2e8f0;">${bpm[index] ?? ""}</td>
      <td style="padding:11px 12px;border:1px solid #e2e8f0;">${spo2[index] ?? ""}</td>
      <td style="padding:11px 12px;border:1px solid #e2e8f0;">${formatSteps(steps[index])}</td>
    </tr>
  `).join("");
}

function formatSteps(value) {
  if (value === undefined || value === null || value === "") return "";

  const number = Number(value);

  if (Number.isNaN(number)) return "";

  return Math.round(number).toLocaleString("vi-VN");
}

function showMessage(message, type = "info") {
  const card = document.getElementById("statsMessageCard");
  const text = document.getElementById("statsMessage");

  if (!card || !text) return;

  card.classList.remove("hidden");
  text.textContent = message;

  if (type === "error") {
    text.style.color = "#fca5a5";
  } else if (type === "warning") {
    text.style.color = "#facc15";
  } else {
    text.style.color = "#cbd5e1";
  }
}

function clearMessage() {
  const card = document.getElementById("statsMessageCard");
  const text = document.getElementById("statsMessage");

  if (!card || !text) return;

  card.classList.add("hidden");
  text.textContent = "";
}

function setLoadingState(isLoading) {
  const button = document.getElementById("loadStats");

  if (!button) return;

  button.disabled = isLoading;
  button.textContent = isLoading ? "Đang tải..." : "Xem thống kê";
}

function hasStatsData(stats) {
  if (!stats) return false;
  if (Number(stats.total_records || 0) > 0) return true;
  if (Array.isArray(stats.labels) && stats.labels.length > 0) return true;
  return false;
}

function getEmptyStats() {
  return {
    avg_bpm: null,
    avg_spo2: null,
    avg_acc: null,
    avg_battery: null,
    total_fall: 0,
    total_sos: 0,
    total_records: 0,
    labels: [],
    bpm_series: [],
    spo2_series: [],
    battery_series: [],
    steps_series: [],
  };
}

function emptyToNull(value) {
  if (value === undefined || value === null || value === "") return null;

  const number = Number(value);

  if (Number.isNaN(number)) return null;

  return number;
}

function numberOrZero(value) {
  if (value === undefined || value === null || value === "") return 0;

  const number = Number(value);

  if (Number.isNaN(number)) return 0;

  return number;
}

function displayValue(value) {
  if (value === undefined || value === null || value === "") return "--";
  return value;
}