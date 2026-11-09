// The theme starts with the first click, tap or key (browsers don't let a page play sound before that).
(function () {
  "use strict";
  var audio = document.getElementById("backgroundMusic");
  function start() {
    audio.volume = 0.6;
    audio.play().catch(function () {});
    ["click", "keydown", "touchstart"].forEach(function (ev) { document.removeEventListener(ev, start); });
  }
  ["click", "keydown", "touchstart"].forEach(function (ev) { document.addEventListener(ev, start); });
})();
