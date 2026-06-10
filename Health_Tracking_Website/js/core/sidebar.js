const navItems = [
  { id: "overview", icon: "⌂", label: "Tổng quát" },
  { id: "stats", icon: "▥", label: "Thống kê" },
  { id: "device", icon: "◉", label: "Thiết bị" },
  { id: "alerts", icon: "⚠", label: "Cảnh báo" },
  { id: "settings", icon: "⚙", label: "Cài đặt" },
];

export function renderSidebar(activeTab, onNavigate) {
  const sidebar = document.getElementById("sidebar");
  if (!sidebar) return;

  const nav = sidebar.querySelector(".sidebar-nav");
  if (!nav) return;

  nav.innerHTML = navItems.map(item => `
    <button class="sidebar-link ${item.id === activeTab ? "active" : ""}" data-tab="${item.id}" type="button">
      <span class="sidebar-link-icon">${item.icon}</span>
      <span>${item.label}</span>
    </button>
  `).join("");

  nav.querySelectorAll(".sidebar-link").forEach(button => {
    button.addEventListener("click", () => {
      onNavigate(button.dataset.tab);
    });
  });
}
