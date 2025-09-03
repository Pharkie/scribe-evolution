(() => {
  var p = Object.defineProperty;
  var h = Object.getOwnPropertySymbols;
  var q = Object.prototype.hasOwnProperty,
    T = Object.prototype.propertyIsEnumerable;
  var m = (t, e, s) =>
      e in t
        ? p(t, e, { enumerable: !0, configurable: !0, writable: !0, value: s })
        : (t[e] = s),
    o = (t, e) => {
      for (var s in e || (e = {})) q.call(e, s) && m(t, s, e[s]);
      if (h) for (var s of h(e)) T.call(e, s) && m(t, s, e[s]);
      return t;
    };
  async function g() {
    try {
      let t = await fetch("/api/config");
      if (!t.ok)
        throw new Error(`Config API returned ${t.status}: ${t.statusText}`);
      return await t.json();
    } catch (t) {
      throw (console.error("API: Failed to load configuration:", t), t);
    }
  }
  async function u(t) {
    try {
      let e = await fetch("/api/config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(t),
      });
      if (!e.ok) {
        let s = await e.text();
        throw (
          console.error("API: Server error response:", s),
          new Error(`Server error: ${e.status} - ${s}`)
        );
      }
      return "Configuration saved";
    } catch (e) {
      throw (console.error("API: Failed to save configuration:", e), e);
    }
  }
  function w() {
    return {
      showErrorMessage(t) {
        window.showMessage(t, "error");
      },
      loaded: !1,
      error: null,
      saving: !1,
      initialized: !1,
      originalConfig: null,
      mqttTesting: !1,
      mqttTestResult: null,
      mqttTestPassed: !1,
      mqttPasswordModified: !1,
      originalMaskedPassword: "",
      config: {},
      validation: { errors: {} },
      get canSave() {
        if (this.config.mqtt.enabled) {
          let t = this.validateConfiguration(),
            e = this.mqttTestPassed;
          return t && e && this.hasChanges();
        }
        return this.hasChanges();
      },
      get testButtonLabel() {
        return this.mqttTesting
          ? "Testing..."
          : this.mqttTestPassed
            ? "MQTT Connected"
            : this.mqttTestResult && !this.mqttTestResult.success
              ? "Connection Failed"
              : "Test Connection";
      },
      hasChanges() {
        return this.originalConfig
          ? this.config.mqtt.enabled !== this.originalConfig.mqtt.enabled ||
              this.config.mqtt.server !== this.originalConfig.mqtt.server ||
              this.config.mqtt.port !== this.originalConfig.mqtt.port ||
              this.config.mqtt.username !== this.originalConfig.mqtt.username ||
              this.mqttPasswordModified
          : !1;
      },
      validateServer(t) {
        this.config.mqtt.enabled && (!t || t.trim() === "")
          ? (this.validation.errors["mqtt.server"] =
              "MQTT server cannot be blank when enabled")
          : this.validation.errors["mqtt.server"] &&
            delete this.validation.errors["mqtt.server"];
      },
      validatePort(t) {
        if (this.config.mqtt.enabled) {
          let e = parseInt(t);
          isNaN(e) || e < 1 || e > 65535
            ? (this.validation.errors["mqtt.port"] =
                "Port must be between 1-65535")
            : this.validation.errors["mqtt.port"] &&
              delete this.validation.errors["mqtt.port"];
        } else
          this.validation.errors["mqtt.port"] &&
            delete this.validation.errors["mqtt.port"];
      },
      validateUsername(t) {
        this.config.mqtt.enabled && (!t || t.trim() === "")
          ? (this.validation.errors["mqtt.username"] =
              "Username cannot be blank when MQTT enabled")
          : this.validation.errors["mqtt.username"] &&
            delete this.validation.errors["mqtt.username"];
      },
      validatePassword(t) {
        this.config.mqtt.enabled && (!t || t.trim() === "")
          ? (this.validation.errors["mqtt.password"] =
              "Password cannot be blank when MQTT enabled")
          : this.validation.errors["mqtt.password"] &&
            delete this.validation.errors["mqtt.password"];
      },
      validateConfiguration() {
        let t = {};
        if (this.config.mqtt.enabled) {
          (!this.config.mqtt.server || this.config.mqtt.server.trim() === "") &&
            (t["mqtt.server"] = "MQTT server cannot be blank when enabled");
          let e = parseInt(this.config.mqtt.port);
          ((isNaN(e) || e < 1 || e > 65535) &&
            (t["mqtt.port"] = "Port must be between 1-65535"),
            (!this.config.mqtt.username ||
              this.config.mqtt.username.trim() === "") &&
              (t["mqtt.username"] =
                "Username cannot be blank when MQTT enabled"),
            (!this.config.mqtt.password ||
              this.config.mqtt.password.trim() === "") &&
              (t["mqtt.password"] =
                "Password cannot be blank when MQTT enabled"));
        }
        return ((this.validation.errors = t), Object.keys(t).length === 0);
      },
      clearPasswordFieldOnFocus() {
        this.config.mqtt.password &&
          ((this.config.mqtt.password = ""),
          (this.mqttPasswordModified = !0),
          this.resetMqttTestState());
      },
      trackMqttPasswordChange(t) {
        let e = t !== this.originalMaskedPassword;
        this.mqttPasswordModified = e;
      },
      async testMqttConnection() {
        ((this.mqttTesting = !0), (this.mqttTestResult = null));
        try {
          let t = {
            server: this.config.mqtt.server,
            port: this.config.mqtt.port,
            username: this.config.mqtt.username,
          };
          this.mqttPasswordModified &&
            this.config.mqtt.password &&
            (t.password = this.config.mqtt.password);
          let e = await fetch("/api/test-mqtt", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(t),
          });
          if (e.ok)
            ((this.mqttTestResult = {
              success: !0,
              message: "Successfully connected to MQTT broker",
            }),
              (this.mqttTestPassed = !0));
          else {
            try {
              let s = await e.json();
              this.mqttTestResult = {
                success: !1,
                message: s.error || "Connection test failed",
              };
            } catch (s) {
              this.mqttTestResult = {
                success: !1,
                message: "Connection test failed",
              };
            }
            this.mqttTestPassed = !1;
          }
        } catch (t) {
          (console.error("MQTT test error:", t),
            (this.mqttTestResult = {
              success: !1,
              message: "Network error during connection test",
            }),
            (this.mqttTestPassed = !1));
        } finally {
          this.mqttTesting = !1;
        }
      },
      resetMqttTestState() {
        ((this.mqttTestPassed = !1), (this.mqttTestResult = null));
      },
      async loadConfiguration() {
        var t, e, s, i, n, a, c, l, d, f;
        if (!this.initialized) {
          ((this.initialized = !0), (this.loaded = !1), (this.error = null));
          try {
            let r = await g();
            ((this.config.mqtt = {
              enabled:
                (e = (t = r.mqtt) == null ? void 0 : t.enabled) != null
                  ? e
                  : !1,
              server:
                (i = (s = r.mqtt) == null ? void 0 : s.server) != null ? i : "",
              port:
                (a = (n = r.mqtt) == null ? void 0 : n.port) != null ? a : 1883,
              username:
                (l = (c = r.mqtt) == null ? void 0 : c.username) != null
                  ? l
                  : "",
              password:
                (f = (d = r.mqtt) == null ? void 0 : d.password) != null
                  ? f
                  : "",
            }),
              (this.originalConfig = { mqtt: o({}, this.config.mqtt) }),
              (this.originalMaskedPassword = this.config.mqtt.password),
              (this.loaded = !0));
          } catch (r) {
            this.error = `Failed to load configuration: ${r.message}`;
          }
        }
      },
      async saveConfig() {
        if (this.canSave)
          try {
            this.saving = !0;
            let t = {};
            this.hasChanges() &&
              ((t.mqtt = {
                enabled: this.config.mqtt.enabled,
                server: this.config.mqtt.server,
                port: this.config.mqtt.port,
                username: this.config.mqtt.username,
              }),
              this.mqttPasswordModified &&
                this.config.mqtt.password &&
                (t.mqtt.password = this.config.mqtt.password));
            let e = await u(t);
            window.location.href = "/settings/?saved=mqtt";
            return;
          } catch (t) {
            (console.error("Error saving MQTT config:", t),
              this.showErrorMessage(
                `Failed to save MQTT settings: ${t.message}`,
              ),
              (this.saving = !1));
          }
      },
      cancelConfiguration() {
        window.location.href = "/settings/";
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    let t = w();
    (Alpine.store("settingsMqtt", t),
      Alpine.effect(() => {
        var e, s;
        ((s = (e = t.config) == null ? void 0 : e.mqtt) == null
          ? void 0
          : s.enabled) === !1 &&
          ((t.validation.errors = {}), t.resetMqttTestState());
      }),
      console.log("\u2705 MQTT Settings Store registered with ES6 modules"));
  });
})();
