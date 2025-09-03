(() => {
  async function n() {
    try {
      console.log("API: Loading memos from server...");
      let e = await fetch("/api/memos");
      if (!e.ok)
        throw new Error(`Memos API returned ${e.status}: ${e.statusText}`);
      let t = await e.json();
      return (console.log("API: Memos loaded successfully"), t);
    } catch (e) {
      throw (console.error("API: Failed to load memos:", e), e);
    }
  }
  async function s(e) {
    try {
      console.log("API: Sending memos to server...");
      let t = await fetch("/api/memos", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(e),
      });
      if (!t.ok) {
        let o = await t.text();
        throw (
          console.error("API: Server error response:", o),
          new Error(`Server error: ${t.status} - ${o}`)
        );
      }
      let r = await t.text();
      return (console.log("API: Server response:", r), r);
    } catch (t) {
      throw (console.error("API: Failed to save memos:", t), t);
    }
  }
  function a() {
    return {
      loaded: !1,
      saving: !1,
      error: null,
      initialized: !1,
      hasUnsavedChanges: !1,
      memos: {},
      async loadConfiguration() {
        if (!this.initialized) {
          ((this.initialized = !0), (this.loaded = !1), (this.error = null));
          try {
            let e = await n();
            ((this.memos = {
              memo1: (e == null ? void 0 : e.memo1) || "",
              memo2: (e == null ? void 0 : e.memo2) || "",
              memo3: (e == null ? void 0 : e.memo3) || "",
              memo4: (e == null ? void 0 : e.memo4) || "",
            }),
              (this.hasUnsavedChanges = !1),
              (this.loaded = !0));
          } catch (e) {
            (console.error("Error loading memos:", e),
              (this.error = `Failed to load memo settings: ${e.message}`),
              this.showErrorMessage(this.error));
          }
        }
      },
      async saveMemos() {
        if (this.hasUnsavedChanges) {
          ((this.saving = !0), (this.error = null));
          try {
            let e = {
              memo1: this.memos.memo1,
              memo2: this.memos.memo2,
              memo3: this.memos.memo3,
              memo4: this.memos.memo4,
            };
            (await s(e),
              (this.hasUnsavedChanges = !1),
              (window.location.href = "/settings/?saved=memos"));
          } catch (e) {
            (console.error("Error saving memos:", e),
              (this.error = `Failed to save memo settings: ${e.message}`),
              this.showErrorMessage(this.error),
              (this.saving = !1));
          }
        }
      },
      get memo1CharacterCount() {
        var e;
        return ((e = this.memos.memo1) == null ? void 0 : e.length) || 0;
      },
      get memo1CharacterText() {
        let e = this.memo1CharacterCount,
          t = 500;
        if (e > t) {
          let r = e - t;
          return `${e}/${t} (${r} over)`;
        }
        return `${e}/${t}`;
      },
      get memo1CharacterClass() {
        let e = this.memo1CharacterCount,
          t = 500,
          r = e / t;
        return e > t
          ? "text-red-600 dark:text-red-400 font-semibold"
          : r > 0.8
            ? "text-yellow-600 dark:text-yellow-400"
            : "text-gray-500 dark:text-gray-400";
      },
      get memo2CharacterCount() {
        var e;
        return ((e = this.memos.memo2) == null ? void 0 : e.length) || 0;
      },
      get memo2CharacterText() {
        let e = this.memo2CharacterCount,
          t = 500;
        if (e > t) {
          let r = e - t;
          return `${e}/${t} (${r} over)`;
        }
        return `${e}/${t}`;
      },
      get memo2CharacterClass() {
        let e = this.memo2CharacterCount,
          t = 500,
          r = e / t;
        return e > t
          ? "text-red-600 dark:text-red-400 font-semibold"
          : r > 0.8
            ? "text-yellow-600 dark:text-yellow-400"
            : "text-gray-500 dark:text-gray-400";
      },
      get memo3CharacterCount() {
        var e;
        return ((e = this.memos.memo3) == null ? void 0 : e.length) || 0;
      },
      get memo3CharacterText() {
        let e = this.memo3CharacterCount,
          t = 500;
        if (e > t) {
          let r = e - t;
          return `${e}/${t} (${r} over)`;
        }
        return `${e}/${t}`;
      },
      get memo3CharacterClass() {
        let e = this.memo3CharacterCount,
          t = 500,
          r = e / t;
        return e > t
          ? "text-red-600 dark:text-red-400 font-semibold"
          : r > 0.8
            ? "text-yellow-600 dark:text-yellow-400"
            : "text-gray-500 dark:text-gray-400";
      },
      get memo4CharacterCount() {
        var e;
        return ((e = this.memos.memo4) == null ? void 0 : e.length) || 0;
      },
      get memo4CharacterText() {
        let e = this.memo4CharacterCount,
          t = 500;
        if (e > t) {
          let r = e - t;
          return `${e}/${t} (${r} over)`;
        }
        return `${e}/${t}`;
      },
      get memo4CharacterClass() {
        let e = this.memo4CharacterCount,
          t = 500,
          r = e / t;
        return e > t
          ? "text-red-600 dark:text-red-400 font-semibold"
          : r > 0.8
            ? "text-yellow-600 dark:text-yellow-400"
            : "text-gray-500 dark:text-gray-400";
      },
      get hasCharacterLimitExceeded() {
        return (
          this.memo1CharacterCount > 500 ||
          this.memo2CharacterCount > 500 ||
          this.memo3CharacterCount > 500 ||
          this.memo4CharacterCount > 500
        );
      },
      get canSave() {
        return !this.loaded || this.saving || this.error
          ? !1
          : this.hasUnsavedChanges && !this.hasCharacterLimitExceeded;
      },
      cancelConfiguration() {
        window.location.href = "/settings/";
      },
      showErrorMessage(e, t = 5e3) {
        console.error("Error:", e);
        let r = document.createElement("div");
        ((r.className =
          "fixed top-4 right-4 bg-red-500 text-white p-4 rounded-lg shadow-lg z-50"),
          (r.textContent = e),
          document.body.appendChild(r),
          setTimeout(() => {
            r.parentNode && r.parentNode.removeChild(r);
          }, t));
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    let e = a();
    (Alpine.store("settingsMemos", e),
      console.log("\u2705 Memos Settings Store registered with ES6 modules"));
  });
})();
