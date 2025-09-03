(() => {
  function i() {
    return {
      method: "GET",
      path: "/unknown",
      showContent: !1,
      init() {
        (this.extractErrorDetails(),
          setTimeout(() => {
            this.showContent = !0;
          }, 100),
          this.startFloatingAnimation());
      },
      extractErrorDetails() {
        let e = document.querySelector("[data-error-method]"),
          t = document.querySelector("[data-error-path]");
        if (e) {
          let r = e.textContent || e.getAttribute("data-error-method");
          r && r !== "{{METHOD}}" && (this.method = r);
        }
        if (t) {
          let r = t.textContent || t.getAttribute("data-error-path");
          r && r !== "{{URI}}" && (this.path = r);
        }
        let o = new URLSearchParams(window.location.search);
        (o.get("method") && (this.method = o.get("method")),
          o.get("path") && (this.path = o.get("path")));
      },
      get errorMessage() {
        return this.path.includes("api")
          ? "That API endpoint seems to have gotten lost in the paper feed."
          : this.path.includes("css") || this.path.includes("js")
            ? "That resource got caught in the ribbon cartridge."
            : this.path.includes("image") || this.path.includes("img")
              ? "That image fell behind the thermal head."
              : "Looks like that page got tangled up in the rollers.";
      },
      get suggestions() {
        let e = ["Try the main printer interface", "Check system diagnostics"];
        return (
          this.path.includes("settings")
            ? e.unshift("Go to settings configuration")
            : this.path.includes("api")
              ? e.unshift("Review API documentation")
              : this.path.includes("diagnostics") &&
                e.unshift("Access system diagnostics"),
          e
        );
      },
      get errorIconClass() {
        return "";
      },
      goHome() {
        window.location.href = "/";
      },
      goToSettings() {
        window.location.href = "/settings/";
      },
      goToDiagnostics() {
        window.location.href = "/diagnostics.html";
      },
      goBack() {
        window.goBack();
      },
      tryAgain() {
        window.location.reload();
      },
      startFloatingAnimation() {
        if (
          window.matchMedia &&
          window.matchMedia("(prefers-reduced-motion: reduce)").matches
        )
          return;
        if (
          (setInterval(() => {
            let t = document.createElement("div");
            ((t.className = "floating-paper"),
              (t.style.cssText = `
                    position: fixed;
                    top: -50px;
                    left: ${Math.random() * 100}vw;
                    width: 20px;
                    height: 30px;
                    background: linear-gradient(45deg, #93c5fd, #60a5fa); /* blue-300 to blue-400 */
                    border: 1px solid rgba(59, 130, 246, 0.35); /* blue-500 outline for contrast */
                    border-radius: 2px;
                    opacity: 0.6; /* more visible */
                    z-index: 1;
                    pointer-events: none;
                    animation: float-down ${5 + Math.random() * 5}s linear infinite;
                `),
              document.body.appendChild(t),
              setTimeout(() => {
                t.parentNode && t.parentNode.removeChild(t);
              }, 1e4));
          }, 2e3),
          !document.getElementById("floating-animation-styles"))
        ) {
          let t = document.createElement("style");
          ((t.id = "floating-animation-styles"),
            (t.textContent = `
                    @keyframes float-down {
                        0% {
                            transform: translateY(-50px) rotate(0deg);
                            opacity: 0;
                        }
                        10% {
                            opacity: 0.3;
                        }
                        90% {
                            opacity: 0.3;
                        }
                        100% {
                            transform: translateY(100vh) rotate(360deg);
                            opacity: 0;
                        }
                    }
                    
                    .error-card {
                        animation: slide-up 0.6s ease-out;
                    }
                    
                    @keyframes slide-up {
                        from {
                            transform: translateY(30px);
                            opacity: 0;
                        }
                        to {
                            transform: translateY(0);
                            opacity: 1;
                        }
                    }
                `),
            document.head.appendChild(t));
        }
      },
      get timestamp() {
        return new Date().toISOString();
      },
      get errorReport() {
        return {
          timestamp: this.timestamp,
          method: this.method,
          path: this.path,
          userAgent: navigator.userAgent,
          referrer: document.referrer || "Direct access",
          url: window.location.href,
        };
      },
      async copyErrorDetails() {
        let e = `Scribe Evolution Printer 404 Error Report
Timestamp: ${this.errorReport.timestamp}
Method: ${this.errorReport.method}
Path: ${this.errorReport.path}
User Agent: ${this.errorReport.userAgent}
Referrer: ${this.errorReport.referrer}
Current URL: ${this.errorReport.url}`;
        try {
          if (navigator.clipboard && window.isSecureContext)
            await navigator.clipboard.writeText(e);
          else {
            let t = document.createElement("textarea");
            ((t.value = e),
              (t.style.position = "fixed"),
              (t.style.left = "-999999px"),
              (t.style.top = "-999999px"),
              document.body.appendChild(t),
              t.focus(),
              t.select(),
              document.execCommand("copy"),
              document.body.removeChild(t));
          }
          return (console.log("Error details copied to clipboard"), !0);
        } catch (t) {
          return (console.error("Failed to copy error details:", t), !1);
        }
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    if (window.errorStoreInstance) {
      console.log(
        "\u{1F5D1}\uFE0F 404: Store already exists, skipping alpine:init",
      );
      return;
    }
    let e = i();
    (Alpine.store("error", e),
      (window.errorStoreInstance = e),
      e.init(),
      console.log("\u2705 404 Error Store registered with ES6 modules"));
  });
})();
