// Killer Stardust's sound effects, made with the Web Audio API (no files): a chime for eating stardust, a zap when an
// invader cuts the tail, a falling tone at game over. The browser only allows sound after a key or tap, so the audio
// starts on the first one.
var Sfx = (function () {
  "use strict";
  var ctx = null, out = null;

  function wake() {
    if (!ctx) {
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return;
      ctx = new AC();
      out = ctx.createGain();
      out.gain.value = 0.35;
      out.connect(ctx.destination);
    }
    if (ctx.state === "suspended") ctx.resume();
  }
  ["keydown", "pointerdown", "touchstart"].forEach(function (e) { document.addEventListener(e, wake, true); });

  // one note: a waveform gliding from f0 to f1, fading out over len seconds
  function tone(type, f0, f1, at, len, vol) {
    var o = ctx.createOscillator(), g = ctx.createGain();
    o.type = type;
    o.frequency.setValueAtTime(f0, at);
    o.frequency.exponentialRampToValueAtTime(f1, at + len);
    g.gain.setValueAtTime(vol, at);
    g.gain.exponentialRampToValueAtTime(0.001, at + len);
    o.connect(g); g.connect(out);
    o.start(at); o.stop(at + len + 0.02);
  }

  function noise(at, len, vol) {
    var n = Math.floor(ctx.sampleRate * len), buf = ctx.createBuffer(1, n, ctx.sampleRate), d = buf.getChannelData(0);
    for (var i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * (1 - i / n);
    var s = ctx.createBufferSource(), g = ctx.createGain();
    s.buffer = buf; g.gain.value = vol;
    s.connect(g); g.connect(out);
    s.start(at);
  }

  var lastEat = 0;
  return {
    eat: function () {                    // a quick rising two-note chime, a little higher on a run
      if (!ctx) return;
      var t = ctx.currentTime, up = t - lastEat < 0.5 ? 1.12 : 1;
      lastEat = t;
      tone("triangle", 988 * up, 988 * up, t, 0.07, 0.5);
      tone("square", 1480 * up, 1760 * up, t + 0.055, 0.12, 0.12);
      tone("sine", 1480 * up, 1480 * up, t + 0.055, 0.14, 0.4);
    },
    cut: function () {                    // a falling zap with some grit
      if (!ctx) return;
      var t = ctx.currentTime;
      tone("square", 900, 120, t, 0.22, 0.25);
      noise(t, 0.15, 0.2);
    },
    over: function () {                   // a long falling tone
      if (!ctx) return;
      var t = ctx.currentTime;
      tone("square", 440, 110, t, 0.9, 0.25);
      tone("triangle", 220, 55, t, 0.9, 0.4);
    }
  };
})();
