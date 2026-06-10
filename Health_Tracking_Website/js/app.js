import { DEVICE_ID } from "./config/firebaseConfig.js";
import {
  initFirebaseRealtime,
  listenTodayHistory,
  listenDailySummary
} from "./services/firebaseService.js";
import { renderSidebar } from "./core/sidebar.js";
import { router } from "./core/router.js";
import {
  updateHeaderByTab,
  setOnlineStatus,
  initNotificationCenter,
  updateNotificationCenter
} from "./core/header.js";
import { getTodayKey, getLastUpdateText } from "./utils/format.js";

const state = {
  currentTab: "overview",
  latestData: null,
  historyRows: [],
  dailySummary: {},
  firstOnlineTime: null,
  lastUpdateTime: null,
};

function boot() {
  renderSidebar(state.currentTab, navigateTo);
  updateHeaderByTab(state.currentTab);
  initNotificationCenter(state);
  renderCurrentPage();
  bindRealtimeData();
  startHeaderClock();
  refreshRelativeTime();
}

function navigateTo(tabName) {
  state.currentTab = tabName || "overview";
  renderSidebar(state.currentTab, navigateTo);
  updateHeaderByTab(state.currentTab);
  renderCurrentPage();
}

function renderCurrentPage() {
  router(state);
  updateNotificationCenter(state);
}

function bindRealtimeData() {
  const todayKey = getTodayKey();

  initFirebaseRealtime((data) => {
    state.latestData = data;
    state.lastUpdateTime = data ? Date.now() : null;

    if (data && !state.firstOnlineTime) {
      state.firstOnlineTime = Date.now();
    }

    setOnlineStatus(data ? `Online • ${new Date().toLocaleTimeString("vi-VN", { hour12: false })}` : "Waiting data...");
    renderCurrentPage();
  });

  listenTodayHistory(todayKey, (rows) => {
    state.historyRows = rows;
    renderCurrentPage();
  });

  listenDailySummary(todayKey, (daily) => {
    state.dailySummary = daily || {};
    renderCurrentPage();
  });
}

function startHeaderClock() {
  const tick = () => {
    const now = new Date();
    const time = now.toLocaleTimeString("vi-VN", { hour12: false });
    const date = now.toLocaleDateString("vi-VN");

    const sideTime = document.getElementById("sidebarTime");
    const sideDate = document.getElementById("sidebarDate");
    const sideName = document.getElementById("sidebarDeviceName");

    if (sideTime) sideTime.textContent = time;
    if (sideDate) sideDate.textContent = date;
    if (sideName) sideName.textContent = DEVICE_ID;
  };

  tick();
  setInterval(tick, 1000);
}

function refreshRelativeTime() {
  setInterval(() => {
    if (state.latestData && state.lastUpdateTime) {
      const onlineText = `Online • ${getLastUpdateText(state.lastUpdateTime)}`;
      setOnlineStatus(onlineText);
    }

    if (state.currentTab === "overview") {
      renderCurrentPage();
    } else {
      updateNotificationCenter(state);
    }
  }, 5000);
}

document.addEventListener("DOMContentLoaded", boot);
