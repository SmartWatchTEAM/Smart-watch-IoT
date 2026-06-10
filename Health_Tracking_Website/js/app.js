import { firebaseConfig, DEVICE_ID } from "./config/firebaseConfig.js";
import { renderSidebar } from "./core/sidebar.js";
import {
  updateHeaderByTab,
  setOnlineStatus,
  initNotificationCenter,
  updateNotificationCenter
} from "./core/header.js";
import { router } from "./core/router.js";

const state = {
  currentTab: "overview",
  deviceId: DEVICE_ID || "watch_001",
  latestData: {},
  historyRows: [],
  dailySummary: {},
  lastUpdateTime: null,
  isOnline: false,
  firebaseReady: false,
  firebaseError: ""
};

function $(id) {
  return document.getElementById(id);
}

function getTodayKey() {
  const d = new Date();
  const yyyy = d.getFullYear();
  const mm = String(d.getMonth() + 1).padStart(2, "0");
  const dd = String(d.getDate()).padStart(2, "0");
  return `${yyyy}-${mm}-${dd}`;
}

function isFirebaseConfigPlaceholder() {
  const values = [
    firebaseConfig?.apiKey,
    firebaseConfig?.messagingSenderId,
    firebaseConfig?.appId
  ];

  return values.some((value) => {
    const text = String(value || "");
    return !text || text.includes("DAN_") || text.includes("CUA_BAN");
  });
}

function updateSidebarDevice() {
  const name = $("sidebarDeviceName");
  const time = $("sidebarTime");
  const date = $("sidebarDate");
  const dot = document.querySelector(".sidebar-device .status-dot");
  const statusText = document.querySelector(".sidebar-device-status span:last-child");
  const sync = document.querySelector(".sidebar-device-sync");

  if (name) name.textContent = state.deviceId;

  const now = new Date();
  if (time) time.textContent = now.toLocaleTimeString("vi-VN", { hour12: false });
  if (date) date.textContent = now.toLocaleDateString("vi-VN");

  if (statusText) {
    if (state.firebaseError) statusText.textContent = "Firebase Config Error";
    else statusText.textContent = state.isOnline ? "Device Online" : "Waiting Data";
  }

  if (sync) {
    if (state.firebaseError) sync.textContent = state.firebaseError;
    else sync.textContent = state.isOnline ? "Firebase realtime" : "Chưa có dữ liệu";
  }

  if (dot) {
    dot.style.background = state.isOnline ? "var(--green)" : "var(--orange)";
    dot.style.boxShadow = state.isOnline ? "0 0 14px var(--green)" : "0 0 14px var(--orange)";
  }
}

function renderCurrentPage() {
  updateHeaderByTab(state.currentTab);
  renderSidebar(state.currentTab, navigateTo);
  router(state);
  updateSidebarDevice();
  updateNotificationCenter(state);
}

function navigateTo(tabId) {
  state.currentTab = tabId || "overview";
  renderCurrentPage();
}

function showFirebaseConfigWarning() {
  const page = $("pageContent");
  if (!page) return;

  page.className = "page-content overview-page";
  page.innerHTML = `
    <div class="grid-12">
      <div class="card span-12">
        <h2 class="section-title">Dashboard đã chạy, nhưng Firebase chưa cấu hình đúng</h2>
        <p class="section-desc">
          File <b>js/config/firebaseConfig.js</b> vẫn đang dùng giá trị mẫu như
          <b>DAN_API_KEY_CUA_BAN</b>, <b>DAN_MESSAGING_SENDER_ID</b>, <b>DAN_APP_ID</b>.
          Vì vậy web chưa thể kết nối Firebase để lấy dữ liệu.
        </p>
        <div class="insight-list" style="margin-top:16px;">
          <div class="insight-item">
            <div class="insight-title">Cần sửa</div>
            <div class="insight-desc">Vào Firebase Project Settings → Your apps → Web app → SDK setup and configuration → copy firebaseConfig thật.</div>
          </div>
          <div class="insight-item">
            <div class="insight-title">File cần sửa</div>
            <div class="insight-desc">Health_Tracking_Website/js/config/firebaseConfig.js</div>
          </div>
        </div>
      </div>

      <div class="card span-3"><div class="card-head"><div class="label">Heart Rate</div></div><div class="value-row"><span class="value heart">--</span><span class="unit">BPM</span></div><div class="pill">MAX30102</div></div>
      <div class="card span-3"><div class="card-head"><div class="label">SpO2</div></div><div class="value-row"><span class="value cyan-text">--</span><span class="unit">%</span></div><div class="pill">Blood Oxygen</div></div>
      <div class="card span-3"><div class="card-head"><div class="label">Fall Detection</div></div><div class="value-row"><span class="value orange-text">--</span></div><div class="pill">Waiting Data</div></div>
      <div class="card span-3"><div class="card-head"><div class="label">SOS Alert</div></div><div class="value-row"><span class="value red-text">--</span></div><div class="pill">Waiting Data</div></div>
    </div>
  `;
}

async function initFirebaseListeners() {
  if (isFirebaseConfigPlaceholder()) {
    state.firebaseError = "Thiếu Firebase config thật";
    state.isOnline = false;
    setOnlineStatus("Config Error");
    updateSidebarDevice();
    showFirebaseConfigWarning();
    return;
  }

  try {
    const {
      initFirebaseRealtime,
      listenTodayHistory,
      listenDailySummary
    } = await import("./services/firebaseService.js");

    const todayKey = getTodayKey();

    initFirebaseRealtime((data) => {
      state.latestData = data || {};
      state.isOnline = Boolean(data);
      state.lastUpdateTime = data ? Date.now() : null;
      setOnlineStatus(data ? "Online" : "Waiting Data");
      renderCurrentPage();
    });

    listenTodayHistory(todayKey, (rows) => {
      state.historyRows = Array.isArray(rows) ? rows : [];
      renderCurrentPage();
    });

    listenDailySummary(todayKey, (daily) => {
      state.dailySummary = daily || {};
      renderCurrentPage();
    });

    state.firebaseReady = true;
    setOnlineStatus("Connecting...");
  } catch (error) {
    console.error("Firebase init error:", error);
    state.firebaseError = "Firebase JS Error";
    state.isOnline = false;
    setOnlineStatus("Firebase Error");
    updateSidebarDevice();
    showFirebaseConfigWarning();
  }
}

function init() {
  initNotificationCenter(state);
  renderSidebar(state.currentTab, navigateTo);
  updateHeaderByTab(state.currentTab);
  setOnlineStatus("Connecting...");
  router(state);
  updateSidebarDevice();

  setInterval(updateSidebarDevice, 1000);
  initFirebaseListeners();
}

document.addEventListener("DOMContentLoaded", init);
