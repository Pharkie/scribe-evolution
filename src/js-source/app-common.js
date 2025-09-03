/**
 * Common application utilities and functions
 * Shared across all pages
 *
 * Exposes a minimal global API via window for pages that are not wired
 * to a specific Alpine store (legacy compatibility):
 *  - window.showMessage(message, type = 'info')
 */

(function () {
  // Create (or reuse) a toast root container
  function getToastRoot() {
    let root = document.getElementById("toast-root");
    if (!root) {
      root = document.createElement("div");
      root.id = "toast-root";
      root.className = "fixed z-[9999] top-4 right-4 flex flex-col gap-2";
      document.body.appendChild(root);
    }
    return root;
  }

  function classForType(type) {
    switch ((type || "info").toLowerCase()) {
      case "success":
        return "bg-green-600 text-white border border-green-500";
      case "error":
        return "bg-red-600 text-white border border-red-500";
      case "warn":
      case "warning":
        return "bg-amber-500 text-black border border-amber-400";
      default:
        return "bg-blue-600 text-white border border-blue-500";
    }
  }

  // Show a lightweight toast message. Uses Tailwind classes already present on pages.
  function showMessage(message, type = "info", opts = {}) {
    try {
      const root = getToastRoot();
      const toast = document.createElement("div");
      toast.setAttribute("role", "status");
      toast.className = `pointer-events-auto px-4 py-3 rounded-xl shadow-lg ${classForType(type)}`;
      toast.textContent = String(message ?? "");

      // Optional close button
      const close = document.createElement("button");
      close.type = "button";
      close.setAttribute("aria-label", "Dismiss");
      close.className = "ml-3 inline-flex items-center justify-center text-current/80 hover:text-current focus:outline-none";
      close.innerHTML = "&times;";

      const row = document.createElement("div");
      row.className = "flex items-start";
      const msg = document.createElement("div");
      msg.className = "flex-1";
      msg.textContent = String(message ?? "");
      row.appendChild(msg);
      row.appendChild(close);

      toast.textContent = ""; // clear and append structured nodes
      toast.appendChild(row);

      root.appendChild(toast);

      const duration = Number(opts.duration ?? 3000);
      let hideTimer = setTimeout(remove, duration);

      function remove() {
        if (!toast.isConnected) return;
        toast.style.transition = "opacity 200ms ease-out, transform 200ms ease-out";
        toast.style.opacity = "0";
        toast.style.transform = "translateY(-4px)";
        setTimeout(() => toast.remove(), 220);
      }

      close.addEventListener("click", () => {
        clearTimeout(hideTimer);
        remove();
      });

      // Pause on hover
      toast.addEventListener("mouseenter", () => clearTimeout(hideTimer));
      toast.addEventListener("mouseleave", () => {
        hideTimer = setTimeout(remove, 1200);
      });
    } catch (e) {
      // Absolute fallback
      try {
        alert(String(message ?? ""));
      } catch {}
    }
  }

  // Expose as globals for simple access from templates and stores
  if (!("showMessage" in window)) window.showMessage = showMessage;
})();
