(() => {
  var p = Object.defineProperty,
    f = Object.defineProperties;
  var y = Object.getOwnPropertyDescriptors;
  var n = Object.getOwnPropertySymbols;
  var k = Object.prototype.hasOwnProperty,
    b = Object.prototype.propertyIsEnumerable;
  var s = (e, t, r) =>
      t in e
        ? p(e, t, { enumerable: !0, configurable: !0, writable: !0, value: r })
        : (e[t] = r),
    a = (e, t) => {
      for (var r in t || (t = {})) k.call(t, r) && s(e, r, t[r]);
      if (n) for (var r of n(t)) b.call(t, r) && s(e, r, t[r]);
      return e;
    },
    i = (e, t) => f(e, y(t));
  async function c() {
    try {
      console.log("API: Loading NVS dump from server...");
      let e = await fetch("/api/nvs-dump");
      if (!e.ok)
        throw new Error(`NVS dump API returned ${e.status}: ${e.statusText}`);
      let t = await e.json();
      return (console.log("API: NVS dump loaded successfully"), t);
    } catch (e) {
      throw (console.error("API: Failed to load NVS dump:", e), e);
    }
  }
  function l() {
    return {
      loaded: !1,
      error: null,
      nvs: {},
      async loadData() {
        var e;
        ((this.loaded = !1), (this.error = null));
        try {
          let t = await c(),
            r = [],
            d = t.keys || {};
          for (let [g, o] of Object.entries(d))
            o.exists &&
              r.push({
                key: g,
                value: o.value,
                type: o.type,
                size: o.length || 0,
                namespace: t.namespace || "scribe-app",
              });
          let u = [
            {
              name: t.namespace || "scribe-app",
              description: "Main application configuration namespace",
              entryCount: r.length,
              size: "~2KB",
              lastModified: t.timestamp || "Unknown",
            },
          ];
          ((this.nvs = i(a({}, t), {
            entries: r,
            namespaces: u,
            statistics: {
              totalEntries:
                ((e = t.summary) == null ? void 0 : e.totalKeys) || r.length,
              usedSpace: "~2KB",
              freeSpace: "~6KB",
            },
          })),
            (this.loaded = !0));
        } catch (t) {
          ((this.error = `Failed to load NVS data: ${t.message}`),
            (this.loaded = !0));
        }
      },
      getTypeClass(e) {
        return (
          {
            string: "bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200",
            int: "bg-blue-100 text-blue-800 dark:bg-blue-900 dark:text-blue-200",
            bool: "bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200",
            u8: "bg-blue-100 text-blue-800 dark:bg-blue-900 dark:text-blue-200",
            u16: "bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200",
            u32: "bg-yellow-100 text-yellow-800 dark:bg-yellow-900 dark:text-yellow-200",
            i8: "bg-purple-100 text-purple-800 dark:bg-purple-900 dark:text-purple-200",
            i16: "bg-pink-100 text-pink-800 dark:bg-pink-900 dark:text-pink-200",
            i32: "bg-indigo-100 text-indigo-800 dark:bg-indigo-900 dark:text-indigo-200",
            str: "bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200",
            blob: "bg-gray-100 text-gray-800 dark:bg-gray-900 dark:text-gray-200",
          }[e] ||
          "bg-gray-100 text-gray-800 dark:bg-gray-900 dark:text-gray-200"
        );
      },
      getDisplayValue(e, t) {
        return e == null
          ? "null"
          : t === "blob"
            ? `[${e.length || 0} bytes]`
            : typeof e == "string" && e.length > 100
              ? e.substring(0, 100) + "..."
              : String(e);
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    Alpine.store("diagnosticsNvs", l());
  });
})();
