// Killer Stardust - a Cursed Arcade game.
// Steer the comet, eat the stardust, grow your tail; the invaders fall in waves, faster each time. Touch one, or your
// own tail, and it's over. Arrows / WASD (or swipe on a phone); Space or R to play again; P to pause.
(function () {
  "use strict";
  var canvas = document.getElementById("gameCanvas");
  var ctx = canvas.getContext("2d");
  var scoreEl = document.getElementById("score");

  var CELL = 22;
  var COLS, ROWS;
  // the Cursed colours: corporate pink and blue, and an acid green for the invaders
  var COLOURS = { tail: "#f088e9", head: "#ffffff", dust: "#34a9e3", invader: "#39ff6a", text: "#f088e9" };

  // the space behind it all: images/space-bg.jpg when it's there, else only the starfield drawn here
  var bg = new Image();
  var bgReady = false;
  bg.onload = function () { bgReady = true; };
  bg.src = "images/space-bg.jpg";
  var stars = [];

  var player, pellets, invaders, score, length, cuts, wave, waveEvery, lastWave, stepEvery, lastStep, state, nextDir;
  var started = false;                    // the splash shows until the first key or tap
  var splash = document.getElementById("splash");
  var best = parseInt(localStorage.getItem("killerStardustBest") || "0", 10);

  function resize() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
    COLS = Math.floor(canvas.width / CELL);
    ROWS = Math.floor(canvas.height / CELL);
    stars = [];
    var n = Math.floor(canvas.width * canvas.height / 2500);
    for (var i = 0; i < n; i++) {
      stars.push({ x: Math.random() * canvas.width, y: Math.random() * canvas.height,
                   r: Math.random() * 1.4 + 0.2, t: Math.random() * Math.PI * 2, s: 0.5 + Math.random() * 2 });
    }
  }

  function freeCell() {
    return { x: Math.floor(Math.random() * COLS), y: Math.floor(Math.random() * ROWS) };
  }

  function reset() {
    player = { x: Math.floor(COLS / 2), y: Math.floor(ROWS / 2), dir: { x: 1, y: 0 }, tail: [] };
    player.tail.push({ x: player.x, y: player.y });     // the comet's head shows from the start
    nextDir = player.dir;
    pellets = [];
    for (var i = 0; i < 40; i++) pellets.push(freeCell());
    invaders = [];
    score = 0;
    length = 3;                            // how long the tail may grow (each stardust adds one)
    cuts = [];                             // where invaders just cut the tail (a flash)
    wave = 1;
    waveEvery = 6000;
    stepEvery = 110;                       // ms per move: about 9 cells a second
    lastWave = lastStep = performance.now();
    spawnWave();
    state = "playing";
    showScore();
  }

  function spawnWave() {
    for (var i = 0; i < 4 + wave * 2; i++) {
      invaders.push({ x: Math.floor(Math.random() * COLS), y: -Math.random() * 8,
                      v: 0.05 + Math.random() * 0.05 + wave * 0.01 });
    }
  }

  function showScore() {
    scoreEl.textContent = "Score " + score + "   Wave " + wave + "   Best " + best;
  }

  function step() {
    player.dir = nextDir;
    player.x = (player.x + player.dir.x + COLS) % COLS;
    player.y = (player.y + player.dir.y + ROWS) % ROWS;
    for (var i = 1; i < player.tail.length; i++) {
      if (player.tail[i].x === player.x && player.tail[i].y === player.y) return gameOver();
    }
    player.tail.unshift({ x: player.x, y: player.y });
    while (player.tail.length > length) player.tail.pop();
    for (var j = 0; j < pellets.length; j++) {
      if (pellets[j].x === player.x && pellets[j].y === player.y) {
        score++;
        length++;
        pellets[j] = freeCell();           // stardust keeps coming
        stepEvery = Math.max(55, stepEvery - 1);
        showScore();
      }
    }
  }

  function update(now) {
    if (!started || state !== "playing") return;
    if (now - lastWave > waveEvery) {
      lastWave = now;
      wave++;
      spawnWave();
      waveEvery = Math.max(2000, waveEvery - 400);
      showScore();
    }
    while (now - lastStep > stepEvery && state === "playing") {
      lastStep += stepEvery;
      step();
    }
    for (var i = 0; i < invaders.length && state === "playing"; i++) {
      var inv = invaders[i];
      inv.y += inv.v;
      if (inv.y >= ROWS) { inv.y = -1; inv.x = Math.floor(Math.random() * COLS); }
      if (Math.abs(inv.x - player.x) < 1 && Math.abs(inv.y - player.y) < 1) { gameOver(); break; }
      // an invader through the tail cuts it there: everything behind is lost, and the invader goes back up
      for (var t = 1; t < player.tail.length; t++) {
        var seg = player.tail[t];
        if (Math.abs(inv.x - seg.x) < 1 && Math.abs(inv.y - seg.y) < 1) {
          cuts.push({ x: seg.x, y: seg.y, until: now + 400 });
          player.tail.length = t;
          length = Math.max(3, t);
          inv.y = -1;
          inv.x = Math.floor(Math.random() * COLS);
          break;
        }
      }
    }
  }

  function gameOver() {
    state = "over";
    if (score > best && location.hash !== "#shot") { best = score; localStorage.setItem("killerStardustBest", String(best)); }
    showScore();
  }

  function glow(colour, blur) {
    ctx.shadowColor = colour;
    ctx.shadowBlur = blur;
    ctx.fillStyle = colour;
  }

  function drawBackground(now) {
    if (bgReady) {
      var k = Math.max(canvas.width / bg.width, canvas.height / bg.height);
      ctx.drawImage(bg, (canvas.width - bg.width * k) / 2, (canvas.height - bg.height * k) / 2, bg.width * k, bg.height * k);
      ctx.fillStyle = "rgba(0, 10, 25, 0.35)";      // a little darker, so the glow reads
      ctx.fillRect(0, 0, canvas.width, canvas.height);
    } else {
      ctx.fillStyle = "#020611";
      ctx.fillRect(0, 0, canvas.width, canvas.height);
    }
    ctx.shadowBlur = 0;
    ctx.fillStyle = "#ffffff";
    for (var i = 0; i < stars.length; i++) {
      var s = stars[i];
      ctx.globalAlpha = 0.35 + 0.65 * Math.abs(Math.sin(s.t + now / 1000 * s.s));
      ctx.fillRect(s.x, s.y, s.r, s.r);
    }
    ctx.globalAlpha = 1;
  }

  function draw(now) {
    drawBackground(now);
    if (!started) return;                   // only the stars behind the splash
    var pulse = 14 + 6 * Math.sin(now / 200);

    glow(COLOURS.dust, 12);                  // the stardust
    for (var i = 0; i < pellets.length; i++) {
      ctx.beginPath();
      ctx.arc((pellets[i].x + 0.5) * CELL, (pellets[i].y + 0.5) * CELL, CELL / 5, 0, Math.PI * 2);
      ctx.fill();
    }

    for (var t = player.tail.length - 1; t >= 0; t--) {     // the comet and its tail, fading
      var seg = player.tail[t];
      ctx.globalAlpha = Math.max(0.15, 1 - t / (player.tail.length + 2));
      glow(t === 0 ? COLOURS.head : COLOURS.tail, t === 0 ? 24 : pulse);
      ctx.fillRect(seg.x * CELL + 2, seg.y * CELL + 2, CELL - 4, CELL - 4);
    }
    ctx.globalAlpha = 1;

    cuts = cuts.filter(function (c) { return c.until > now; });   // where the tail was just cut: a flash
    glow("#ffffff", 30);
    for (var c = 0; c < cuts.length; c++) {
      var r = CELL * (1.6 - (cuts[c].until - now) / 400);
      ctx.beginPath();
      ctx.arc((cuts[c].x + 0.5) * CELL, (cuts[c].y + 0.5) * CELL, r, 0, Math.PI * 2);
      ctx.strokeStyle = COLOURS.invader;
      ctx.lineWidth = 3;
      ctx.stroke();
    }

    glow(COLOURS.invader, 16);               // the invaders
    for (var j = 0; j < invaders.length; j++) {
      var x = invaders[j].x * CELL, y = invaders[j].y * CELL;
      ctx.fillRect(x + 3, y + 5, CELL - 6, CELL - 10);
      ctx.fillRect(x + 1, y + 9, 3, 6);
      ctx.fillRect(x + CELL - 4, y + 9, 3, 6);
    }
    ctx.shadowBlur = 0;

    if (state !== "playing") {
      ctx.fillStyle = "rgba(2, 6, 17, 0.6)";
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.textAlign = "center";
      glow(COLOURS.text, 24);
      ctx.font = "bold " + Math.min(72, canvas.width / 10) + "px 'Arial Black', Arial, sans-serif";
      ctx.fillText(state === "over" ? "GAME OVER" : "PAUSED", canvas.width / 2, canvas.height / 2 - 20);
      ctx.shadowBlur = 0;
      ctx.fillStyle = "#ffffff";
      ctx.font = "20px Arial, sans-serif";
      ctx.fillText(state === "over" ? "Score " + score + "  ·  Space, R or tap to play again" : "P to carry on",
                   canvas.width / 2, canvas.height / 2 + 24);
    }
  }

  function loop(now) {
    update(now);
    draw(now);
    requestAnimationFrame(loop);
  }

  function turn(x, y) {
    if (state !== "playing") return;
    if (player.dir.x === -x && player.dir.y === -y) return;   // no reversing into yourself
    nextDir = { x: x, y: y };
  }

  function start() {
    started = true;
    splash.classList.add("gone");
    document.body.classList.add("playing");
    reset();
  }
  splash.addEventListener("click", start);

  document.addEventListener("keydown", function (e) {
    if (!started) { start(); e.preventDefault(); return; }
    var k = e.key.toLowerCase();
    if (k === "arrowup" || k === "w") turn(0, -1);
    else if (k === "arrowdown" || k === "s") turn(0, 1);
    else if (k === "arrowleft" || k === "a") turn(-1, 0);
    else if (k === "arrowright" || k === "d") turn(1, 0);
    else if ((k === " " || k === "r") && state === "over") reset();
    else if (k === "p" && state !== "over") {
      state = state === "paused" ? "playing" : "paused";
      lastStep = lastWave = performance.now();
    } else return;
    e.preventDefault();
  });

  // swipe on a phone; tap to play again
  var touch = null;
  canvas.addEventListener("touchstart", function (e) {
    touch = { x: e.touches[0].clientX, y: e.touches[0].clientY };
    if (!started) { start(); touch = null; return; }
    if (state === "over") reset();
  }, { passive: true });
  canvas.addEventListener("touchend", function (e) {
    if (!touch) return;
    var dx = e.changedTouches[0].clientX - touch.x, dy = e.changedTouches[0].clientY - touch.y;
    if (Math.max(Math.abs(dx), Math.abs(dy)) > 24) {
      if (Math.abs(dx) > Math.abs(dy)) turn(dx > 0 ? 1 : -1, 0); else turn(0, dy > 0 ? 1 : -1);
    }
    touch = null;
  }, { passive: true });

  window.addEventListener("resize", function () { if (location.hash !== "#shot") { resize(); reset(); } });
  resize();
  reset();
  var shot = location.hash === "#shot";
  if (shot) {                              // for README screenshots: a few hundred moves played at once, then drawn
    splash.style.display = "none";
    start();
    var t = performance.now(), turns = [[0, -1], [1, 0], [0, 1], [1, 0]];
    for (var i = 0; i < 260 && state === "playing"; i++) {
      if (i % 9 === 0) { var d = turns[(i / 9) % 4]; turn(d[0], d[1]); }
      t += stepEvery;
      update(t);
      if (i % 3 === 0) { score++; length++; }            // as if eating as it goes, for a long tail
    }
    state = "playing";
    showScore();
    draw(t);
    return;
  }
  requestAnimationFrame(loop);
})();
