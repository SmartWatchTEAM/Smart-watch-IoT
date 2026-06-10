const tabMeta = {
  overview: {
    title: "Smart Health Watch",
    subtitle: "Firebase Realtime IoT Dashboard",
  },
  stats: {
    title: "Thống kê sức khỏe",
    subtitle: "Phân tích dữ liệu trung bình theo ngày và theo tháng từ Firebase",
  },
  device: {
    title: "Quản lý thiết bị",
    subtitle: "Thông tin cảm biến, kết nối, firmware và trạng thái hệ thống",
  },
  alerts: {
    title: "Cảnh báo sức khỏe",
    subtitle: "Theo dõi Fall Detection, SOS Alert và các bất thường quan trọng",
  },
  settings: {
    title: "Cài đặt hệ thống",
    subtitle: "Tùy chỉnh thiết bị, dữ liệu và giao diện dashboard",
  },
};

let notificationPanel = null;
let notificationBadge = null;
let notificationButton = null;
let notificationOpen = false;

export function updateHeaderByTab(tabName) {
  const meta = tabMeta[tabName] || tabMeta.overview;

  const title = document.getElementById("pageTitle");
  const subtitle = document.getElementById("pageSubtitle");

  if (title) title.textContent = meta.title;
  if (subtitle) subtitle.textContent = meta.subtitle;
}

export function setOnlineStatus(text) {
  const onlineStatus = document.getElementById("onlineStatus");

  if (onlineStatus) {
    onlineStatus.textContent = text;
  }
}

export function initNotificationCenter(state) {
  notificationButton = document.querySelector(".notification-btn");

  if (!notificationButton) return;

  notificationButton.classList.add("notification-btn-ready");
  notificationButton.setAttribute("aria-expanded", "false");
  notificationButton.setAttribute("title", "Xem thông báo hệ thống");

  notificationBadge = document.createElement("span");
  notificationBadge.className = "notification-badge hidden";
  notificationButton.appendChild(notificationBadge);

  notificationPanel = document.createElement("div");
  notificationPanel.className = "notification-panel hidden";
  notificationPanel.innerHTML = `
    <div class="notification-panel-header">
      <div>
        <div class="notification-panel-title">Thông báo hệ thống</div>
        <div class="notification-panel-subtitle">Theo dõi trạng thái thiết bị realtime</div>
      </div>
      <button class="notification-close" type="button" aria-label="Đóng thông báo">×</button>
    </div>

    <div class="notification-list" id="notificationList"></div>
  `;

  const headerActions = document.querySelector(".header-actions");

  if (headerActions) {
    headerActions.style.position = "relative";
    headerActions.appendChild(notificationPanel);
  }

  notificationButton.addEventListener("click", (event) => {
    event.stopPropagation();
    toggleNotificationPanel();
  });

  notificationPanel.querySelector(".notification-close").addEventListener("click", (event) => {
    event.stopPropagation();
    closeNotificationPanel();
  });

  document.addEventListener("click", (event) => {
    if (!notificationPanel || !notificationButton) return;

    const clickedInsidePanel = notificationPanel.contains(event.target);
    const clickedButton = notificationButton.contains(event.target);

    if (!clickedInsidePanel && !clickedButton) {
      closeNotificationPanel();
    }
  });

  updateNotificationCenter(state);
}

export function updateNotificationCenter(state) {
  if (!notificationPanel) return;

  const notifications = buildNotifications(state);
  const list = notificationPanel.querySelector("#notificationList");

  list.innerHTML = notifications.map(renderNotificationItem).join("");

  const importantCount = notifications.filter(item => item.level === "danger" || item.level === "warning").length;

  if (notificationBadge) {
    if (importantCount > 0) {
      notificationBadge.classList.remove("hidden");
      notificationBadge.textContent = importantCount > 9 ? "9+" : importantCount;
    } else {
      notificationBadge.classList.add("hidden");
      notificationBadge.textContent = "";
    }
  }

  const notificationTitle = notificationPanel.querySelector(".notification-panel-title");

  if (notificationTitle) {
    notificationTitle.textContent = importantCount > 0
      ? `Thông báo hệ thống (${importantCount})`
      : "Thông báo hệ thống";
  }
}

function buildNotifications(state) {
  const data = state?.latestData;

  if (!data) {
    return [
      {
        level: "warning",
        title: "Chưa có dữ liệu thiết bị",
        desc: "Firebase chưa có dữ liệu mới từ ESP32 hoặc node latest đang trống.",
        time: "Realtime"
      },
      {
        level: "info",
        title: "Gợi ý kiểm tra",
        desc: "Kiểm tra WiFi ESP32, Firebase Database path và nguồn cấp cảm biến.",
        time: "Hệ thống"
      }
    ];
  }

  const notifications = [];

  if (isTrue(data.sos)) {
    notifications.push({
      level: "danger",
      title: "SOS đang bật",
      desc: "Thiết bị đang gửi trạng thái khẩn cấp. Cần kiểm tra người dùng ngay.",
      time: getLastUpdateText(state)
    });
  } else {
    notifications.push({
      level: "success",
      title: "SOS bình thường",
      desc: "Không ghi nhận tín hiệu SOS khẩn cấp.",
      time: getLastUpdateText(state)
    });
  }

  if (isTrue(data.fall)) {
    notifications.push({
      level: "danger",
      title: "Phát hiện té ngã",
      desc: "Fall Detection đang ở trạng thái cảnh báo. Cần kiểm tra lại thiết bị và người dùng.",
      time: getLastUpdateText(state)
    });
  } else {
    notifications.push({
      level: "success",
      title: "Không phát hiện té ngã",
      desc: "Fall Detection đang ở trạng thái an toàn.",
      time: getLastUpdateText(state)
    });
  }

  const bpm = toNumber(data.bpm);
  if (bpm === null || bpm <= 0) {
    notifications.push({
      level: "warning",
      title: "Chưa có nhịp tim",
      desc: "MAX30102 chưa trả về BPM hợp lệ hoặc người dùng chưa đặt tay đúng vị trí.",
      time: getLastUpdateText(state)
    });
  } else if (bpm < 60 || bpm > 100) {
    notifications.push({
      level: "warning",
      title: "Nhịp tim cần theo dõi",
      desc: `BPM hiện tại là ${bpm}. Giá trị tham khảo bình thường thường nằm khoảng 60–100 BPM.`,
      time: getLastUpdateText(state)
    });
  } else {
    notifications.push({
      level: "success",
      title: "Nhịp tim ổn định",
      desc: `BPM hiện tại là ${bpm}.`,
      time: getLastUpdateText(state)
    });
  }

  const spo2 = toNumber(data.spo2);
  if (spo2 === null || spo2 <= 0) {
    notifications.push({
      level: "warning",
      title: "Chưa có SpO2",
      desc: "Cảm biến chưa trả về SpO2 hợp lệ hoặc tín hiệu đo chưa ổn định.",
      time: getLastUpdateText(state)
    });
  } else if (spo2 < 90) {
    notifications.push({
      level: "danger",
      title: "SpO2 thấp nghiêm trọng",
      desc: `SpO2 hiện tại là ${spo2}%. Cần kiểm tra người dùng và cảm biến ngay.`,
      time: getLastUpdateText(state)
    });
  } else if (spo2 < 95) {
    notifications.push({
      level: "warning",
      title: "SpO2 cần theo dõi",
      desc: `SpO2 hiện tại là ${spo2}%. Nên đo lại để xác nhận.`,
      time: getLastUpdateText(state)
    });
  } else {
    notifications.push({
      level: "success",
      title: "SpO2 tốt",
      desc: `SpO2 hiện tại là ${spo2}%.`,
      time: getLastUpdateText(state)
    });
  }

  const battery = toNumber(data.battery);
  if (battery !== null) {
    if (battery <= 10) {
      notifications.push({
        level: "danger",
        title: "Pin rất thấp",
        desc: `Dung lượng pin còn ${battery}%. Cần sạc ngay.`,
        time: getLastUpdateText(state)
      });
    } else if (battery <= 20) {
      notifications.push({
        level: "warning",
        title: "Pin yếu",
        desc: `Dung lượng pin còn ${battery}%. Nên sạc thiết bị.`,
        time: getLastUpdateText(state)
      });
    } else {
      notifications.push({
        level: "success",
        title: "Pin ổn định",
        desc: `Dung lượng pin hiện tại ${battery}%.`,
        time: getLastUpdateText(state)
      });
    }
  }

  if (!isTrue(data.wifi)) {
    notifications.push({
      level: "warning",
      title: "WiFi chưa ổn định",
      desc: "Thiết bị báo WiFi OFF hoặc mất kết nối.",
      time: getLastUpdateText(state)
    });
  }

  if (!isTrue(data.finger)) {
    notifications.push({
      level: "info",
      title: "Chưa đặt tay lên cảm biến",
      desc: "MAX30102 có thể chưa nhận được ngón tay hoặc tín hiệu đo chưa đủ tốt.",
      time: getLastUpdateText(state)
    });
  }

  if (!isTrue(data.timeSynced)) {
    notifications.push({
      level: "info",
      title: "NTP chưa đồng bộ",
      desc: "Thời gian thiết bị chưa được đồng bộ từ internet.",
      time: getLastUpdateText(state)
    });
  }

  const lastUpdateMs = state?.lastUpdateTime;
  if (lastUpdateMs) {
    const ageSeconds = Math.floor((Date.now() - lastUpdateMs) / 1000);

    if (ageSeconds > 60) {
      notifications.unshift({
        level: "warning",
        title: "Dữ liệu đã cũ",
        desc: `Không nhận bản cập nhật mới trong khoảng ${ageSeconds} giây.`,
        time: "Kết nối"
      });
    }
  }

  return notifications.slice(0, 10);
}

function renderNotificationItem(item) {
  const iconMap = {
    danger: "!",
    warning: "!",
    success: "✓",
    info: "i"
  };

  return `
    <div class="notification-item notification-${item.level}">
      <div class="notification-item-icon">${iconMap[item.level] || "i"}</div>

      <div class="notification-item-body">
        <div class="notification-item-top">
          <div class="notification-item-title">${item.title}</div>
          <div class="notification-item-time">${item.time || ""}</div>
        </div>

        <div class="notification-item-desc">${item.desc}</div>
      </div>
    </div>
  `;
}

function toggleNotificationPanel() {
  if (!notificationPanel || !notificationButton) return;

  notificationOpen = !notificationOpen;

  notificationPanel.classList.toggle("hidden", !notificationOpen);
  notificationButton.setAttribute("aria-expanded", notificationOpen ? "true" : "false");
}

function closeNotificationPanel() {
  if (!notificationPanel || !notificationButton) return;

  notificationOpen = false;
  notificationPanel.classList.add("hidden");
  notificationButton.setAttribute("aria-expanded", "false");
}

function getLastUpdateText(state) {
  if (!state?.lastUpdateTime) return "Chưa cập nhật";

  const seconds = Math.floor((Date.now() - state.lastUpdateTime) / 1000);

  if (seconds < 5) return "Vừa xong";
  if (seconds < 60) return `${seconds}s trước`;

  const minutes = Math.floor(seconds / 60);
  return `${minutes} phút trước`;
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

function toNumber(value) {
  if (value === undefined || value === null || value === "") return null;

  const number = Number(value);

  if (Number.isNaN(number)) return null;

  return number;
}
