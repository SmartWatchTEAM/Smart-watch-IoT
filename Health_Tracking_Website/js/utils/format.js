export function valueOrDash(value) {
  if (value === undefined || value === null || value === "") return "--";
  return value;
}

export function numberOrNull(value) {
  if (value === undefined || value === null || value === "") return null;
  const n = Number(value);
  return Number.isFinite(n) ? n : null;
}

export function clamp(value, min, max) {
  return Math.min(Math.max(value, min), max);
}

export function anglePercent(angle) {
  const n = Number(angle);
  if (Number.isNaN(n)) return 50;
  return clamp(((n + 180) / 360) * 100, 0, 100);
}

export function formatUptimeFromMs(ms) {
  const n = Number(ms || 0);
  if (!n) return "--";

  const totalSec = Math.floor(n / 1000);
  const d = Math.floor(totalSec / 86400);
  const h = Math.floor((totalSec % 86400) / 3600);
  const m = Math.floor((totalSec % 3600) / 60);

  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m`;
}

export function formatUptime(startTime) {
  if (!startTime) return "--";

  const diff = Math.floor((Date.now() - startTime) / 1000);
  const d = Math.floor(diff / 86400);
  const h = Math.floor((diff % 86400) / 3600);
  const m = Math.floor((diff % 3600) / 60);

  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m`;
}

export function boolText(value, trueText, falseText) {
  return isTrue(value) ? trueText : falseText;
}

export function isTrue(value) {
  return value === true
    || value === 1
    || value === "1"
    || value === "true"
    || value === "TRUE"
    || value === "ON"
    || value === "YES"
    || value === "OK";
}

export function getTodayKey() {
  const date = new Date();
  const yyyy = date.getFullYear();
  const mm = String(date.getMonth() + 1).padStart(2, "0");
  const dd = String(date.getDate()).padStart(2, "0");
  return `${yyyy}-${mm}-${dd}`;
}

export function getLastUpdateText(timestampMs) {
  if (!timestampMs) return "--";

  const seconds = Math.max(0, Math.floor((Date.now() - timestampMs) / 1000));
  if (seconds < 5) return "Vừa xong";
  if (seconds < 60) return `${seconds}s ago`;

  const minutes = Math.floor(seconds / 60);
  if (minutes < 60) return `${minutes}m ago`;

  const hours = Math.floor(minutes / 60);
  return `${hours}h ago`;
}
