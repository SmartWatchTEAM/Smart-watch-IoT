import { initializeApp } from "https://www.gstatic.com/firebasejs/11.10.0/firebase-app.js";
import {
  getDatabase,
  ref,
  onValue,
  query,
  limitToLast
} from "https://www.gstatic.com/firebasejs/11.10.0/firebase-database.js";

import { firebaseConfig, DEVICE_ID } from "../config/firebaseConfig.js";

export const app = initializeApp(firebaseConfig);
export const db = getDatabase(app);

const deviceId = DEVICE_ID || "watch_001";
const basePath = `devices/${deviceId}`;

export function initFirebaseRealtime(callback) {
  const latestRef = ref(db, `${basePath}/latest`);

  return onValue(
    latestRef,
    (snapshot) => callback(snapshot.exists() ? snapshot.val() : null),
    (error) => {
      console.error("Firebase realtime error:", error);
      callback(null);
    }
  );
}

export function listenTodayHistory(dateKey, callback, rowLimit = 240) {
  const historyQuery = query(ref(db, `${basePath}/history/${dateKey}`), limitToLast(rowLimit));

  return onValue(
    historyQuery,
    (snapshot) => {
      if (!snapshot.exists()) {
        callback([]);
        return;
      }

      const rows = Object.entries(snapshot.val()).map(([id, value]) => ({
        id,
        ...(value || {})
      }));

      rows.sort((a, b) => Number(a.createdAt || a.updatedAt || a.timestamp || 0) - Number(b.createdAt || b.updatedAt || b.timestamp || 0));
      callback(rows);
    },
    (error) => {
      console.error("Firebase history error:", error);
      callback([]);
    }
  );
}

export function listenDailySummary(dateKey, callback) {
  const dailyRef = ref(db, `${basePath}/daily/${dateKey}/summary`);

  return onValue(
    dailyRef,
    (snapshot) => callback(snapshot.exists() ? snapshot.val() : {}),
    (error) => {
      console.error("Firebase daily summary error:", error);
      callback({});
    }
  );
}
