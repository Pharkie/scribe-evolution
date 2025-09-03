(() => {
  var A = Object.defineProperty,
    P = Object.defineProperties;
  var m = Object.getOwnPropertyDescriptors;
  var f = Object.getOwnPropertySymbols;
  var I = Object.prototype.hasOwnProperty,
    $ = Object.prototype.propertyIsEnumerable;
  var g = (o, t, e) =>
      t in o
        ? A(o, t, { enumerable: !0, configurable: !0, writable: !0, value: e })
        : (o[t] = e),
    p = (o, t) => {
      for (var e in t || (t = {})) I.call(t, e) && g(o, e, t[e]);
      if (f) for (var e of f(t)) $.call(t, e) && g(o, e, t[e]);
      return o;
    },
    h = (o, t) => P(o, m(t));
  async function w() {
    try {
      console.log("API: Loading configuration from server...");
      let o = await fetch("/api/config");
      if (!o.ok)
        throw new Error(`Config API returned ${o.status}: ${o.statusText}`);
      let t = await o.json();
      return (console.log("API: Configuration loaded successfully"), t);
    } catch (o) {
      throw (console.error("API: Failed to load configuration:", o), o);
    }
  }
  function y() {
    return {
      loaded: !1,
      error: null,
      config: {},
      async loadData() {
        ((this.loaded = !1), (this.error = null));
        try {
          let o = await w(),
            t = [],
            e = new Set(),
            l = (n, c = "", a = "root") => {
              for (let [i, r] of Object.entries(n)) {
                let u = c ? `${c}.${i}` : i;
                if (
                  r &&
                  typeof r == "object" &&
                  !Array.isArray(r) &&
                  r.constructor === Object
                ) {
                  let d = c === "" ? i : a;
                  (e.add(d), l(r, u, d));
                } else
                  (a !== "root" && e.add(a),
                    t.push({
                      key: u,
                      value: String(r),
                      type: typeof r,
                      section: a,
                    }));
              }
            };
          l(o);
          let s = {};
          (t.forEach((n) => {
            (s[n.section] || (s[n.section] = []), s[n.section].push(n));
          }),
            (this.config = h(p({}, o), {
              entries: t,
              entriesBySection: s,
              statistics: { totalEntries: t.length, configSections: e.size },
            })),
            (this.loaded = !0));
        } catch (o) {
          ((this.error = `Failed to load configuration data: ${o.message}`),
            (this.loaded = !0));
        }
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    Alpine.store("diagnosticsRuntimeConfig", y());
  });
})();
