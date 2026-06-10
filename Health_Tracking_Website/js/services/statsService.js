import {
  ref,
  get
} from "https://www.gstatic.com/firebasejs/11.10.0/firebase-database.js";

import { db } from "./firebaseService.js";

const DEVICE_ID = "watch_001";

export async function getStatsByDay(dateValue) {
  const date = dateValue || getTodayString();
  const rows = await readDayRows(date);

  return buildStatsResult({
    mode: "day",
    date,
    rows
  });
}

export async function getStatsByMonth(monthValue) {
  const month = monthValue || getTodayString().slice(0, 7);
  const rows = await readMonthRows(month);

  return buildStatsResult({
    mode: "month",
    month,
    rows,
    groupByDay: true
  });
}

async function readDayRows(date) {
  const path = `devices/${DEVICE_ID}/history/${date}`;
  const snapshot = await get(ref(db, path));

  if (!snapshot.exists()) {
    return [];
  }

  return objectToRows(snapshot.val());
}

async function readMonthRows(month) {
  const snapshot = await get(ref(db, `devices/${DEVICE_ID}/history`));

  if (!snapshot.exists()) {
    return [];
  }

  const monthData = snapshot.val();
  const rows = [];

  Object.keys(monthData).forEach((dateKey) => {
    if (!dateKey.startsWith(month)) return;

    const dayRows = objectToRows(monthData[dateKey]).map((row) => ({
      ...row,
      dateKey
    }));

    rows.push(...dayRows);
  });

  return sortRowsByTime(rows);
}

function objectToRows(value) {
  if (!value || typeof value !== "object") return [];

  return Object.keys(value)
    .map((key) => ({
      id: key,
      ...value[key]
    }))
    .filter((row) => row && typeof row === "object");
}

function buildStatsResult({ mode, date = null, month = null, rows, groupByDay = false }) {
  const cleanRows = sortRowsByTime(rows || []);
  const totalRecords = cleanRows.length;

  if (totalRecords === 0) {
    return {
      success: true,
      source: "firebase",
      mode,
      device_id: DEVICE_ID,
      date,
      month,

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
      steps_series: []
    };
  }

  if (groupByDay) {
    return buildMonthGroupedStats({
      mode,
      month,
      rows: cleanRows
    });
  }

  return {
    success: true,
    source: "firebase",
    mode,
    device_id: DEVICE_ID,
    date,
    month,

    avg_bpm: average(cleanRows.map(row => row.bpm)),
    avg_spo2: average(cleanRows.map(row => row.spo2)),
    avg_acc: average(cleanRows.map(row => row.acc)),
    avg_battery: average(cleanRows.map(row => row.battery)),
    total_fall: countTrue(cleanRows.map(row => row.fall)),
    total_sos: countTrue(cleanRows.map(row => row.sos)),
    total_records: totalRecords,

    labels: cleanRows.map(row => formatTimeLabel(row)),
    bpm_series: cleanRows.map(row => valueOrNull(row.bpm)),
    spo2_series: cleanRows.map(row => valueOrNull(row.spo2)),
    battery_series: cleanRows.map(row => valueOrNull(row.battery)),
    steps_series: cleanRows.map(row => valueOrNull(row.steps))
  };
}

function buildMonthGroupedStats({ mode, month, rows }) {
  const groups = {};

  rows.forEach((row) => {
    const key = row.dateKey || getDateFromTimestamp(row.createdAt) || "unknown";

    if (!groups[key]) {
      groups[key] = [];
    }

    groups[key].push(row);
  });

  const labels = Object.keys(groups).sort();

  const dayStats = labels.map((dateKey) => {
    const dayRows = groups[dateKey];

    return {
      label: formatDateLabel(dateKey),
      avg_bpm: average(dayRows.map(row => row.bpm)),
      avg_spo2: average(dayRows.map(row => row.spo2)),
      avg_battery: average(dayRows.map(row => row.battery)),
      steps: maxValid(dayRows.map(row => row.steps)),
      total_fall: countTrue(dayRows.map(row => row.fall)),
      total_sos: countTrue(dayRows.map(row => row.sos)),
      total_records: dayRows.length
    };
  });

  return {
    success: true,
    source: "firebase",
    mode,
    device_id: DEVICE_ID,
    date: null,
    month,

    avg_bpm: average(rows.map(row => row.bpm)),
    avg_spo2: average(rows.map(row => row.spo2)),
    avg_acc: average(rows.map(row => row.acc)),
    avg_battery: average(rows.map(row => row.battery)),
    total_fall: countTrue(rows.map(row => row.fall)),
    total_sos: countTrue(rows.map(row => row.sos)),
    total_records: rows.length,

    labels: dayStats.map(row => row.label),
    bpm_series: dayStats.map(row => row.avg_bpm),
    spo2_series: dayStats.map(row => row.avg_spo2),
    battery_series: dayStats.map(row => row.avg_battery),
    steps_series: dayStats.map(row => row.steps)
  };
}

function sortRowsByTime(rows) {
  return [...rows].sort((a, b) => {
    const timeA = Number(a.createdAt || a.updatedAt || a.timestamp || 0);
    const timeB = Number(b.createdAt || b.updatedAt || b.timestamp || 0);
    return timeA - timeB;
  });
}

function average(values) {
  const valid = values
    .map(value => Number(value))
    .filter(value => !Number.isNaN(value) && value > 0);

  if (!valid.length) return null;

  const result = valid.reduce((sum, value) => sum + value, 0) / valid.length;
  return Number(result.toFixed(1));
}

function maxValid(values) {
  const valid = values
    .map(value => Number(value))
    .filter(value => !Number.isNaN(value) && value >= 0);

  if (!valid.length) return null;

  return Math.max(...valid);
}

function countTrue(values) {
  return values.filter(isTrue).length;
}

function isTrue(value) {
  return value === true
    || value === 1
    || value === "1"
    || value === "true"
    || value === "TRUE"
    || value === "ON"
    || value === "YES"
    || value === "OK";
}

function valueOrNull(value) {
  const number = Number(value);
  return Number.isNaN(number) ? null : number;
}

function formatTimeLabel(row) {
  const timestamp = Number(row.createdAt || row.updatedAt || row.timestamp || 0);

  if (!timestamp) {
    return "--:--";
  }

  const date = new Date(timestamp);

  return date.toLocaleTimeString("vi-VN", {
    hour: "2-digit",
    minute: "2-digit"
  });
}

function formatDateLabel(dateKey) {
  const parts = String(dateKey).split("-");

  if (parts.length !== 3) return dateKey;

  return `${parts[2]}/${parts[1]}`;
}

function getDateFromTimestamp(timestamp) {
  const number = Number(timestamp);

  if (!number) return null;

  const date = new Date(number);
  const yyyy = date.getFullYear();
  const mm = String(date.getMonth() + 1).padStart(2, "0");
  const dd = String(date.getDate()).padStart(2, "0");

  return `${yyyy}-${mm}-${dd}`;
}

function getTodayString() {
  const date = new Date();
  const yyyy = date.getFullYear();
  const mm = String(date.getMonth() + 1).padStart(2, "0");
  const dd = String(date.getDate()).padStart(2, "0");

  return `${yyyy}-${mm}-${dd}`;
}