import {
  valueOrDash,
  numberOrNull,
  clamp,
  formatUptime,
  formatUptimeFromMs,
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
  finger: `<svg viewBox="0 0 24 24" fill="none"><path d="M12 3a5 5 0 0 0-5 5v5" stroke="currentColor" stroke-linecap="round"/><path d="M12 6a2 2 0 0 0-2 2v8" stroke="currentColor" stroke-linecap="round"/><path d="M14 9v8M17 11v5" stroke="currentColor" stroke-linecap="round"/><path d="M7 17c1 3 3 4 6 4 4 0 6-2.5 6-6" stroke="currentColor" stroke-linecap="round"/></svg>`
};

export function renderOverviewTab(state) {
  const hasLatest = !!state.latestData;
  const d = state.latestData || {};
  const page = document.getElementById("pageContent");

  const bpm = numberOrNull(d.bpm ?? d.heartRate);
  const spo2 = numberOrNull(d.spo2);
  const fall = isTrue(d.fall);
  const sos = isTrue(d.sos);
  const finger = isTrue(d.finger);
  const quality = hasLatest ? calcSignalQuality(d) : null;

  const daily = state.dailySummary || {};
  const steps = numberOrNull(d.steps ?? daily.steps);
  const stepGoal = numberOrNull(d.stepGoal ?? daily.stepGoal) ?? 10000;
  const distanceKm = numberOrNull(d.distanceKm ?? daily.distanceKm);
  const calories = numberOrNull(d.calories ?? daily.calories);
  const cadence = numberOrNull(d.cadenceSPM ?? daily.cadenceSPM);
  const rawGoalPercent = numberOrNull(d.stepGoalPercent ?? daily.stepGoalPercent);
  const stepGoalPercent = rawGoalPercent !== null
    ? clamp(rawGoalPercent, 0, 100)
    : (steps !== null && stepGoal ? clamp((steps / stepGoal) * 100, 0, 100) : 0);

  const historyRows = Array.isArray(state.historyRows) ? state.historyRows : [];
  const bpmValues = historyRows.map(row => numberOrNull(row.bpm ?? row.heartRate)).filter(v => v !== null && v > 0);
  const spo2Values = historyRows.map(row => numberOrNull(row.spo2)).filter(v => v !== null && v > 0);
  const avgBpm = bpmValues.length ? Math.round(bpmValues.reduce((a, b) => a + b, 0) / bpmValues.length) : null;
  const minSpo2 = spo2Values.length ? Math.round(Math.min(...spo2Values)) : null;

  page.className = "page-content overview-page";

  page.innerHTML = `
    <div class="grid-12">
      <div class="card span-3 overview-kpi-card">
        <div class="card-head">
          <div class="icon red">${icons.heart}</div>
          <div class="label">Heart Rate</div>
        </div>
        <div class="value-row">
          <span class="value heart">${bpm ?? "--"}</span>
          <span class="unit">BPM</span>
          <span class="pill kpi-status">${getBpmStatus(bpm)}</span>
        </div>
        <div class="pill">MAX30102</div>
        <canvas id="bpmMiniChart" class="kpi-mini-chart"></canvas>
      </div>

      <div class="card span-3 overview-kpi-card">
        <div class="card-head">
          <div class="icon cyan">${icons.drop}</div>
          <div class="label">SpO2</div>
        </div>
        <div class="value-row">
          <span class="value cyan-text">${spo2 ?? "--"}</span>
          <span class="unit">%</span>
          <span class="pill kpi-status">${getSpo2Status(spo2)}</span>
        </div>
        <div class="pill">Blood Oxygen</div>
        <canvas id="spo2MiniChart" class="kpi-mini-chart"></canvas>
      </div>

      <div class="card span-3 overview-kpi-card">
        <div class="card-head">
          <div class="icon orange">${icons.fall}</div>
          <div class="label">Fall Detection</div>
        </div>
        <div class="value-row">
          <span class="value ${fall ? "red-text" : "orange-text"}">${hasLatest ? (fall ? "ALERT" : "OK") : "--"}</span>
        </div>
        <div class="pill ${fall ? "pill-alert" : "pill-ok"}">${hasLatest ? (fall ? "Fall detected" : "No fall detected") : "Waiting data"}</div>
      </div>

      <div class="card span-3 overview-kpi-card">
        <div class="card-head">
          <div class="icon red">${icons.siren}</div>
          <div class="label">SOS Alert</div>
        </div>
        <div class="value-row">
          <span class="value red-text">${hasLatest ? (sos ? "ON" : "OFF") : "--"}</span>
        </div>
        <div class="pill ${sos ? "pill-alert" : "pill-ok"}">${hasLatest ? (sos ? "SOS active" : "Normal") : "Waiting data"}</div>
      </div>

      <div class="card span-9 ppg-card">
        <div class="ppg-title-row">
          <div class="ppg-title-left">
            <div class="icon red">${icons.heart}</div>
            <div class="label">Heart Rate Waveform <span>(PPG Signal)</span></div>
          </div>
          <div class="ppg-legend">
            <span class="legend-dot legend-ir"></span> IR (Infrared)
            <span class="legend-dot legend-red"></span> Red
          </div>
        </div>
        <canvas id="ppgCanvas"></canvas>
      </div>

      <div class="span-3 right-health-stack">
        <div class="card signal-quality-card">
          <div class="label">Signal Quality</div>
          <div class="quality-ring ${quality === null ? "quality-empty" : ""}" style="--p:${quality ?? 0}">
            <div>
              <span>${quality === null ? "Waiting" : (quality >= 75 ? "Good" : quality >= 45 ? "Fair" : "Weak")}</span>
              <strong>${quality === null ? "--%" : `${quality}%`}</strong>
            </div>
          </div>
        </div>

        <div class="card finger-detected-card">
          <div class="label">Finger Detected</div>
          <div class="finger-display">
            <span class="${finger ? "green-text" : "red-text"}">${hasLatest ? (finger ? "YES" : "NO") : "--"}</span>
            <div class="finger-icon ${finger ? "green-text" : "red-text"}">${icons.finger}</div>
          </div>
        </div>
      </div>

      <div class="card span-4 activity-card-v2">
        <div class="activity-title">Activity Summary <span>(Today)</span></div>
        <div class="activity-grid-v2">
          <div>
            <div class="activity-icon green-text">${icons.shoe}</div>
            <strong class="green-text">${steps !== null ? Math.round(steps).toLocaleString("en-US") : "--"}</strong>
            <small>Steps</small>
          </div>
          <div>
            <div class="activity-icon cyan-text">${icons.route}</div>
            <strong class="cyan-text">${distanceKm !== null ? distanceKm.toFixed(2) : "--"}</strong>
            <small>km</small>
          </div>
          <div>
            <div class="activity-icon orange-text">${icons.flame}</div>
            <strong class="orange-text">${calories !== null ? Math.round(calories) : "--"}</strong>
            <small>kcal</small>
          </div>
          <div>
            <div class="activity-icon purple-text">${icons.run}</div>
            <strong class="purple-text">${cadence !== null ? Math.round(cadence) : "--"}</strong>
            <small>SPM</small>
          </div>
        </div>
        <div class="goal-row">
          <span>Goal: ${Math.round(stepGoal).toLocaleString("en-US")}</span>
          <b>${steps !== null ? Math.round(stepGoalPercent) : "--"}%</b>
        </div>
        <div class="bar"><span style="width:${steps !== null ? stepGoalPercent : 0}%"></span></div>
      </div>

      <div class="card span-4 overview-chart-card">
        <div class="chart-head-v2">
          <div class="label">Heart Rate <span>(Today)</span></div>
          <b class="heart">Avg ${avgBpm ?? "--"} BPM</b>
        </div>
        <canvas id="heartHistoryChart"></canvas>
      </div>

      <div class="card span-4 overview-chart-card">
        <div class="chart-head-v2">
          <div class="label">SpO2 <span>(Today)</span></div>
          <b class="cyan-text">Min ${minSpo2 ?? "--"}%</b>
        </div>
        <canvas id="spo2HistoryChart"></canvas>
      </div>

      <div class="card span-12 system">
        <div class="system-grid">
          <div class="system-item">
            <div class="system-icon">${icons.chip}</div>
            <div>
              <div class="system-label">Firmware</div>
              <div class="system-value">${hasLatest ? (d.firmware ?? "v1.2.3") : "--"}</div>
              <div class="system-sub">${hasLatest ? "Latest" : "Waiting"}</div>
            </div>
          </div>
          <div class="system-item">
            <div class="system-icon">${icons.clock}</div>
            <div>
              <div class="system-label">Uptime</div>
              <div class="system-value">${d.uptimeMs ? formatUptimeFromMs(d.uptimeMs) : formatUptime(state.firstOnlineTime)}</div>
              <div class="system-sub">${hasLatest ? "Running" : "Waiting"}</div>
            </div>
          </div>
          <div class="system-item">
            <div class="system-icon">${icons.signal}</div>
            <div>
              <div class="system-label">Signal Strength</div>
              <div class="system-value">${d.rssi ? `${d.rssi} dBm` : "-- dBm"}</div>
              <div class="system-sub">${getRssiStatus(numberOrNull(d.rssi))}</div>
            </div>
          </div>
          <div class="system-item">
            <div class="system-icon">${icons.refresh}</div>
            <div>
              <div class="system-label">Data Sync</div>
              <div class="system-value">Realtime</div>
              <div class="system-sub">Firebase</div>
            </div>
          </div>
          <div class="system-item">
            <div class="system-icon">${icons.refresh}</div>
            <div>
              <div class="system-label">Last Update</div>
              <div class="system-value">${getLastUpdateText(state.lastUpdateTime)}</div>
              <div class="system-sub">Realtime</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  `;

  bpm !== null ? drawMiniChart("bpmMiniChart", bpmValues.length ? bpmValues.slice(-24) : [bpm], "#ff4d6d") : drawEmptyMiniChart("bpmMiniChart");
  spo2 !== null ? drawMiniChart("spo2MiniChart", spo2Values.length ? spo2Values.slice(-24) : [spo2], "#38bdf8") : drawEmptyMiniChart("spo2MiniChart");
  drawPPGWaveform("ppgCanvas", d);
  drawAreaChart("heartHistoryChart", bpmValues, 0, 150, "#ff4d6d");
  drawAreaChart("spo2HistoryChart", spo2Values, 80, 100, "#38bdf8");
}

function getBpmStatus(bpm) {
  if (!bpm) return "--";
  if (bpm < 50) return "Low";
  if (bpm > 120) return "High";
  return "Normal";
}

function getSpo2Status(spo2) {
  if (!spo2) return "--";
  if (spo2 < 92) return "Low";
  return "Normal";
}

function getRssiStatus(rssi) {
  if (rssi === null) return "--";
  if (rssi >= -65) return "Good";
  if (rssi >= -80) return "Fair";
  return "Weak";
}

function calcSignalQuality(d) {
  const explicit = numberOrNull(d.signalQuality);
  if (explicit !== null) return clamp(Math.round(explicit), 0, 100);
  if (isTrue(d.signalOK)) return 92;
  if (isTrue(d.finger) && numberOrNull(d.bpm) && numberOrNull(d.spo2)) return 92;
  if (isTrue(d.finger)) return 65;
  return 28;
}

function getCanvas(id, minWidth, minHeight) {
  const canvas = document.getElementById(id);
  if (!canvas) return null;

  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();

  canvas.width = Math.max(minWidth, rect.width || minWidth) * dpr;
  canvas.height = Math.max(minHeight, rect.height || minHeight) * dpr;

  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

  return { canvas, ctx, w: canvas.width / dpr, h: canvas.height / dpr };
}

function drawEmptyMiniChart(canvasId) {
  const c = getCanvas(canvasId, 130, 48);
  if (!c) return;
  const { ctx, w, h } = c;
  ctx.clearRect(0, 0, w, h);
  ctx.strokeStyle = "rgba(148, 163, 184, 0.14)";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, h / 2);
  ctx.lineTo(w, h / 2);
  ctx.stroke();
}

function drawMiniChart(canvasId, values, color) {
  const c = getCanvas(canvasId, 130, 48);
  if (!c) return;
  const { ctx, w, h } = c;
  ctx.clearRect(0, 0, w, h);

  const data = values.length >= 2 ? values : [values[0] || 0, values[0] || 0];
  const min = Math.min(...data);
  const max = Math.max(...data);

  ctx.strokeStyle = color;
  ctx.lineWidth = 3;
  ctx.lineCap = "round";
  ctx.lineJoin = "round";
  ctx.beginPath();
  data.forEach((v, i) => {
    const x = (i / Math.max(1, data.length - 1)) * w;
    const y = max === min ? h / 2 : h - ((v - min) / Math.max(1, max - min)) * (h - 10) - 5;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

function drawAreaChart(canvasId, values, minY, maxY, color) {
  const c = getCanvas(canvasId, 360, 170);
  if (!c) return;
  const { ctx, w, h } = c;
  const pad = { l: 34, r: 12, t: 12, b: 28 };
  const cw = w - pad.l - pad.r;
  const ch = h - pad.t - pad.b;

  ctx.clearRect(0, 0, w, h);
  drawSmallGrid(ctx, pad, cw, ch);

  const data = values.slice(-80);
  if (data.length < 2) {
    drawNoDataText(ctx, w, h);
    drawTimeLabels(ctx, pad, cw, h);
    return;
  }

  const gradient = ctx.createLinearGradient(0, pad.t, 0, pad.t + ch);
  gradient.addColorStop(0, `${color}55`);
  gradient.addColorStop(1, `${color}05`);

  ctx.beginPath();
  data.forEach((v, i) => {
    const x = pad.l + (i / (data.length - 1)) * cw;
    const y = pad.t + (1 - (v - minY) / (maxY - minY)) * ch;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.lineTo(pad.l + cw, pad.t + ch);
  ctx.lineTo(pad.l, pad.t + ch);
  ctx.closePath();
  ctx.fillStyle = gradient;
  ctx.fill();

  ctx.beginPath();
  data.forEach((v, i) => {
    const x = pad.l + (i / (data.length - 1)) * cw;
    const y = pad.t + (1 - (v - minY) / (maxY - minY)) * ch;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.strokeStyle = color;
  ctx.lineWidth = 2.4;
  ctx.stroke();
  drawTimeLabels(ctx, pad, cw, h);
}

function drawSmallGrid(ctx, pad, cw, ch) {
  ctx.strokeStyle = "rgba(148, 163, 184, 0.10)";
  ctx.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const y = pad.t + (i / 4) * ch;
    ctx.beginPath();
    ctx.moveTo(pad.l, y);
    ctx.lineTo(pad.l + cw, y);
    ctx.stroke();
  }
  for (let i = 0; i <= 6; i++) {
    const x = pad.l + (i / 6) * cw;
    ctx.beginPath();
    ctx.moveTo(x, pad.t);
    ctx.lineTo(x, pad.t + ch);
    ctx.stroke();
  }
}

function drawTimeLabels(ctx, pad, cw, h) {
  ctx.fillStyle = "#94a3b8";
  ctx.font = "11px Segoe UI, Arial";
  ["00:00", "06:00", "12:00", "18:00", "23:59"].forEach((label, i) => {
    ctx.fillText(label, pad.l + (i / 4) * cw - 14, h - 6);
  });
}

function drawNoDataText(ctx, w, h) {
  ctx.fillStyle = "rgba(148, 163, 184, 0.62)";
  ctx.font = "13px Segoe UI, Arial";
  ctx.textAlign = "center";
  ctx.fillText("Waiting data", w / 2, h / 2);
  ctx.textAlign = "left";
}

function drawPPGWaveform(canvasId, d) {
  const c = getCanvas(canvasId, 780, 240);
  if (!c) return;
  const { ctx, w, h } = c;
  const pad = { l: 74, r: 18, t: 18, b: 38 };
  const cw = w - pad.l - pad.r;
  const ch = h - pad.t - pad.b;

  ctx.clearRect(0, 0, w, h);
  drawPPGGrid(ctx, pad, cw, ch);

  const ir = Array.isArray(d.ppgIr) ? d.ppgIr.slice(-180) : null;
  const red = Array.isArray(d.ppgRed) ? d.ppgRed.slice(-180) : null;

  if (!ir || !red || ir.length < 8 || red.length < 8) {
    drawNoDataText(ctx, w, h);
    return;
  }

  plotPPG(ctx, ir, "#ff4d6d", pad, cw, ch);
  plotPPG(ctx, red, "#22c55e", pad, cw, ch);
}

function drawPPGGrid(ctx, pad, cw, ch) {
  ctx.save();
  ctx.strokeStyle = "rgba(100, 165, 220, 0.12)";
  ctx.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const y = pad.t + (i / 4) * ch;
    ctx.beginPath();
    ctx.moveTo(pad.l, y);
    ctx.lineTo(pad.l + cw, y);
    ctx.stroke();
  }
  for (let i = 0; i <= 20; i++) {
    const x = pad.l + (i / 20) * cw;
    ctx.beginPath();
    ctx.moveTo(x, pad.t);
    ctx.lineTo(x, pad.t + ch);
    ctx.stroke();
  }
  ctx.strokeStyle = "rgba(148, 163, 184, 0.26)";
  ctx.beginPath();
  ctx.moveTo(pad.l, pad.t + ch);
  ctx.lineTo(pad.l + cw, pad.t + ch);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(pad.l, pad.t);
  ctx.lineTo(pad.l, pad.t + ch);
  ctx.stroke();

  ctx.fillStyle = "#94a3b8";
  ctx.font = "12px Segoe UI, Arial";
  [-40000, -20000, 0, 20000, 40000].forEach((v) => {
    const y = pad.t + (1 - (v + 40000) / 80000) * ch + 4;
    ctx.fillText(v === 0 ? "0" : `${v / 1000}k`, 18, y);
  });
  for (let i = 0; i <= 10; i += 2) {
    const x = pad.l + (i / 20) * cw;
    ctx.fillText(String(i * 2), x - 4, pad.t + ch + 24);
  }
  ctx.save();
  ctx.translate(18, pad.t + ch / 2 + 42);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText("Signal (a.u.)", 0, 0);
  ctx.restore();
  ctx.fillText("Time (s)", pad.l + cw / 2 - 28, pad.t + ch + 24);
  ctx.restore();
}

function plotPPG(ctx, data, color, pad, cw, ch) {
  if (!data || data.length < 2) return;
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 2.4;
  ctx.lineCap = "round";
  ctx.lineJoin = "round";
  ctx.shadowColor = color;
  ctx.shadowBlur = 7;
  ctx.beginPath();
  data.forEach((v, i) => {
    const x = pad.l + (i / (data.length - 1)) * cw;
    const y = pad.t + (1 - (v + 40000) / 80000) * ch;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();
  ctx.restore();
}
