(() => {
  function a() {
    return {
      deviceSaved: !1,
      wifiSaved: !1,
      mqttSaved: !1,
      memosSaved: !1,
      buttonsSaved: !1,
      ledsSaved: !1,
      unbiddenInkSaved: !1,
      loaded: !1,
      error: null,
      loadData() {
        (this.checkSaveSuccess(), (this.loaded = !0));
      },
      checkSaveSuccess() {
        let e = new URLSearchParams(window.location.search).get("saved");
        e &&
          [
            "device",
            "wifi",
            "mqtt",
            "memos",
            "buttons",
            "leds",
            "unbiddenInk",
          ].includes(e) &&
          this.showSuccessFeedback(e);
      },
      showSuccessFeedback(t) {
        let e = window.location.pathname;
        window.history.replaceState({}, document.title, e);
        let s = t + "Saved";
        ((this[s] = !0),
          setTimeout(() => {
            this[s] = !1;
          }, 2e3));
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    let t = a();
    Alpine.store("settingsOverview", t);
  });
})();
