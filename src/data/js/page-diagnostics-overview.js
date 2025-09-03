(() => {
  function e() {
    return {
      loaded: !1,
      error: null,
      loadData() {
        this.loaded = !0;
      },
    };
  }
  document.addEventListener("alpine:init", () => {
    Alpine.store("diagnosticsOverview", e());
  });
})();
