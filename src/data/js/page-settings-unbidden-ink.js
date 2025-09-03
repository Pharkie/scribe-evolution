(() => {
  async function d() {
    try {
      let n = await fetch("/api/config");
      if (!n.ok)
        throw new Error(`Config API returned ${n.status}: ${n.statusText}`);
      return await n.json();
    } catch (n) {
      throw (console.error("API: Failed to load configuration:", n), n);
    }
  }
  async function c(n) {
    try {
      let e = await fetch("/api/config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(n),
      });
      if (!e.ok) {
        let t = await e.text();
        throw (
          console.error("API: Server error response:", t),
          new Error(`Server error: ${e.status} - ${t}`)
        );
      }
      return "Configuration saved";
    } catch (e) {
      throw (console.error("API: Failed to save configuration:", e), e);
    }
  }
  function u() {
    return {
      loaded: !1,
      saving: !1,
      error: null,
      initialized: !1,
      config: {},
      originalValues: {},
      passwordModified: !1,
      validation: { errors: {} },
      async loadConfiguration() {
        var n, e, t, o, s, i, a;
        if (!this.initialized) {
          ((this.initialized = !0), (this.loaded = !1), (this.error = null));
          try {
            let r = await d();
            ((this.config.unbiddenInk = {
              enabled: ((n = r.unbiddenInk) == null ? void 0 : n.enabled) || !1,
              startHour:
                ((e = r.unbiddenInk) == null ? void 0 : e.startHour) || 8,
              endHour: ((t = r.unbiddenInk) == null ? void 0 : t.endHour) || 22,
              frequencyMinutes:
                ((o = r.unbiddenInk) == null ? void 0 : o.frequencyMinutes) ||
                120,
              prompt: ((s = r.unbiddenInk) == null ? void 0 : s.prompt) || "",
              chatgptApiToken:
                ((i = r.unbiddenInk) == null ? void 0 : i.chatgptApiToken) ||
                "",
              promptPresets:
                ((a = r.unbiddenInk) == null ? void 0 : a.promptPresets) || {},
            }),
              (this.originalValues = {
                enabled: this.config.unbiddenInk.enabled,
                startHour: this.config.unbiddenInk.startHour,
                endHour: this.config.unbiddenInk.endHour,
                frequencyMinutes: this.config.unbiddenInk.frequencyMinutes,
                prompt: this.config.unbiddenInk.prompt,
                chatgptApiToken: this.config.unbiddenInk.chatgptApiToken,
              }),
              (this.loaded = !0));
          } catch (r) {
            (console.error("Failed to load configuration:", r),
              (this.error = r.message));
          }
        }
      },
      async saveConfiguration() {
        ((this.saving = !0), (this.error = null));
        try {
          let n = {
            unbiddenInk: {
              enabled: this.config.unbiddenInk.enabled,
              startHour: this.config.unbiddenInk.startHour,
              endHour: this.config.unbiddenInk.endHour,
              frequencyMinutes: this.config.unbiddenInk.frequencyMinutes,
              prompt: this.config.unbiddenInk.prompt,
            },
          };
          (this.passwordModified &&
            (n.unbiddenInk.chatgptApiToken =
              this.config.unbiddenInk.chatgptApiToken),
            await c(n),
            Object.assign(this.originalValues, {
              enabled: this.config.unbiddenInk.enabled,
              startHour: this.config.unbiddenInk.startHour,
              endHour: this.config.unbiddenInk.endHour,
              frequencyMinutes: this.config.unbiddenInk.frequencyMinutes,
              prompt: this.config.unbiddenInk.prompt,
            }),
            this.passwordModified &&
              ((this.originalValues.chatgptApiToken =
                this.config.unbiddenInk.chatgptApiToken),
              (this.passwordModified = !1)),
            (window.location.href = "/settings/?saved=unbiddenInk"));
        } catch (n) {
          (console.error("Failed to save configuration:", n),
            (this.error = n.message),
            (this.saving = !1));
        }
      },
      cancelConfiguration() {
        window.location.href = "/settings/";
      },
      get canSave() {
        let n = this.config.unbiddenInk,
          e = this.originalValues,
          t = Object.keys(this.validation.errors).length > 0;
        return (
          (n.enabled !== e.enabled ||
            n.startHour !== e.startHour ||
            n.endHour !== e.endHour ||
            n.frequencyMinutes !== e.frequencyMinutes ||
            n.prompt !== e.prompt ||
            this.passwordModified) &&
          !t
        );
      },
      clearChatgptTokenFieldOnFocus() {
        this.config.unbiddenInk.chatgptApiToken &&
          ((this.config.unbiddenInk.chatgptApiToken = ""),
          (this.passwordModified = !0));
      },
      trackChatgptTokenChange(n) {
        let e = n !== this.originalValues.chatgptApiToken;
        ((this.passwordModified = e), this.validateChatgptToken(n));
      },
      validateChatgptToken(n) {
        this.config.unbiddenInk.enabled && (!n || n.trim() === "")
          ? (this.validation.errors["unbiddenInk.chatgptApiToken"] =
              "API Token cannot be blank when Unbidden Ink is enabled")
          : this.validation.errors["unbiddenInk.chatgptApiToken"] &&
            delete this.validation.errors["unbiddenInk.chatgptApiToken"];
      },
      validateAll() {
        this.validateChatgptToken(this.config.unbiddenInk.chatgptApiToken);
      },
      get timeRangeDisplay() {
        let n = this.config.unbiddenInk.startHour || 0,
          e = this.config.unbiddenInk.endHour || 24;
        return n === 0 && (e === 0 || e === 24)
          ? "All Day"
          : `${this.formatHour(n)} - ${this.formatHour(e)}`;
      },
      get timeRangeStyle() {
        let n = this.config.unbiddenInk.startHour || 0,
          e = this.config.unbiddenInk.endHour || 24;
        if (n === 0 && (e === 0 || e === 24))
          return { left: "0%", width: "100%" };
        let t = (n / 24) * 100,
          o = (e / 24) * 100;
        return { left: `${Math.min(t, o)}%`, width: `${Math.abs(o - t)}%` };
      },
      get startHourSafe() {
        return this.config.unbiddenInk.startHour || 8;
      },
      set startHourSafe(n) {
        this.config.unbiddenInk.startHour = Math.max(
          0,
          Math.min(24, parseInt(n)),
        );
      },
      get endHourSafe() {
        return this.config.unbiddenInk.endHour || 22;
      },
      set endHourSafe(n) {
        this.config.unbiddenInk.endHour = Math.max(
          0,
          Math.min(24, parseInt(n)),
        );
      },
      handleStartHourChange(n) {
        let e = parseInt(n.target.value),
          t = this.config.unbiddenInk.endHour;
        if (e >= t) {
          n.target.value = this.config.unbiddenInk.startHour;
          return;
        }
        this.config.unbiddenInk.startHour = e;
      },
      handleEndHourChange(n) {
        let e = parseInt(n.target.value),
          t = this.config.unbiddenInk.startHour;
        if (e <= t) {
          n.target.value = this.config.unbiddenInk.endHour;
          return;
        }
        this.config.unbiddenInk.endHour = e;
      },
      formatHour(n) {
        return n == null
          ? "--:--"
          : n === 0
            ? "00:00"
            : n === 24
              ? "24:00"
              : n.toString().padStart(2, "0") + ":00";
      },
      formatHour12(n) {
        return n == null
          ? "--"
          : n === 0 || n === 24
            ? "12 am"
            : n === 12
              ? "12 pm"
              : n < 12
                ? `${n} am`
                : `${n - 12} pm`;
      },
      get frequencyOptions() {
        return [15, 30, 60, 120, 240, 360, 480];
      },
      get frequencyLabels() {
        return this.frequencyOptions.map((n) =>
          n < 60 ? `${n}min` : `${n / 60}hr`,
        );
      },
      get frequencySliderValue() {
        let n = this.frequencyOptions,
          e = this.config.unbiddenInk.frequencyMinutes || 120,
          t = n.indexOf(e);
        return t >= 0 ? t : 3;
      },
      set frequencySliderValue(n) {
        this.config.unbiddenInk.frequencyMinutes =
          this.frequencyOptions[n] || 120;
      },
      get frequencyDisplay() {
        let n = this.config.unbiddenInk.frequencyMinutes || 120,
          e = Math.floor(n / 60),
          t = n % 60,
          o = this.config.unbiddenInk.startHour || 0,
          s = this.config.unbiddenInk.endHour || 24,
          i = "";
        if (o === 0 && (s === 0 || s === 24)) i = "all day long";
        else {
          let a = this.formatHour12(o),
            r = this.formatHour12(s);
          i = `from ${a} to ${r}`;
        }
        return e > 0 && t > 0
          ? `Every ${e}h ${t}m ${i}`
          : e > 0
            ? `Every ${e}h ${i}`
            : `Every ${t}m ${i}`;
      },
      setQuickPrompt(n) {
        let e = this.config.unbiddenInk.promptPresets || {};
        e[n] && (this.config.unbiddenInk.prompt = e[n]);
      },
      isPromptActive(n) {
        let e = this.config.unbiddenInk.promptPresets || {};
        return (
          (this.config.unbiddenInk.prompt || "").trim() === (e[n] || "").trim()
        );
      },
      showErrorMessage(n) {
        this.error = n;
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    let n = u();
    (Alpine.store("settingsUnbiddenInk", n),
      console.log(
        "\u2705 Unbidden Ink Settings Store registered with ES6 modules",
      ));
  });
})();
