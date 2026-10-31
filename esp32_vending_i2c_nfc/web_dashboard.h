#pragma once
#include <Arduino.h>

const char PAGE_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>IAFantasma - Expensedora Web3</title>
  <style>
    :root {
      --bg: #08080f;
      --card-bg: rgba(18, 18, 28, 0.85);
      --card-border: rgba(255, 255, 255, 0.08);
      --primary: #00f2fe;
      --success: #00ffaa;
      --warning: #ffb700;
      --danger: #ff3366;
      --text: #f0f0f5;
      --text-muted: #9aa0b4;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      overflow-x: hidden;
    }
    /* Grid de fondo decorativa */
    body::before {
      content: "";
      position: fixed;
      inset: 0;
      background-image: linear-gradient(rgba(255,255,255,0.02) 1px, transparent 1px),
                        linear-gradient(90deg, rgba(255,255,255,0.02) 1px, transparent 1px);
      background-size: 36px 36px;
      pointer-events: none;
      z-index: 0;
    }
    /* Navbar */
    .navbar {
      position: relative;
      z-index: 10;
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 12px 24px;
      background: rgba(12, 12, 20, 0.9);
      backdrop-filter: blur(10px);
      border-bottom: 1px solid var(--card-border);
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .brand-icon { font-size: 1.8rem; filter: drop-shadow(0 0 10px rgba(0,242,254,0.5)); }
    .brand-title { font-weight: 800; font-size: 1.15rem; letter-spacing: 1px; color: #fff; }
    .brand-sub { font-size: 0.7rem; color: var(--primary); font-weight: 700; letter-spacing: 1.5px; }
    .nav-status {
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .badge-ip {
      display: flex;
      align-items: center;
      gap: 6px;
      background: rgba(0, 255, 170, 0.1);
      border: 1px solid rgba(0, 255, 170, 0.3);
      color: var(--success);
      padding: 4px 10px;
      border-radius: 20px;
      font-size: 0.8rem;
      font-weight: 600;
    }
    .badge-ip .dot {
      width: 8px; height: 8px; border-radius: 50%; background: var(--success);
      box-shadow: 0 0 8px var(--success);
      animation: pulse 1.5s infinite alternate;
    }
    @keyframes pulse { from { opacity: 0.4; } to { opacity: 1; } }

    /* Top banner */
    .top-banner {
      position: relative;
      z-index: 5;
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px 24px;
      background: var(--card-bg);
      border-bottom: 1px solid var(--card-border);
      font-size: 0.85rem;
    }
    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 4px 12px;
      border-radius: 20px;
      font-weight: 700;
      letter-spacing: 0.5px;
      background: rgba(0, 242, 254, 0.1);
      color: var(--primary);
      border: 1px solid rgba(0, 242, 254, 0.3);
    }
    .status-pill.ESPERANDO_PAGO {
      background: rgba(255, 183, 0, 0.15);
      color: var(--warning);
      border-color: rgba(255, 183, 0, 0.4);
    }
    .status-pill.PAGO_EXITOSO {
      background: rgba(0, 255, 170, 0.15);
      color: var(--success);
      border-color: rgba(0, 255, 170, 0.4);
    }
    .active-product {
      color: var(--text-muted);
    }
    .active-product strong {
      color: #fff;
    }

    /* Layout principal */
    .dashboard-grid {
      position: relative;
      z-index: 5;
      display: grid;
      grid-template-columns: 290px 1fr 340px;
      gap: 20px;
      padding: 20px 24px;
      max-width: 1450px;
      margin: 0 auto;
      width: 100%;
      flex: 1;
    }
    @media (max-width: 1100px) {
      .dashboard-grid { grid-template-columns: 1fr; }
    }

    /* Cards */
    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 20px;
      backdrop-filter: blur(12px);
      box-shadow: 0 10px 30px rgba(0,0,0,0.4);
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid var(--card-border);
      padding-bottom: 10px;
    }
    .card-title {
      font-size: 1rem;
      font-weight: 700;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .tag-badge {
      font-size: 0.7rem;
      background: rgba(255,255,255,0.08);
      padding: 3px 8px;
      border-radius: 6px;
      color: var(--text-muted);
    }

    /* Columna 1: Fantasma */
    .speech-bubble {
      background: rgba(255,255,255,0.04);
      border: 1px solid rgba(0, 242, 254, 0.2);
      border-radius: 12px;
      padding: 12px 14px;
      font-size: 0.85rem;
      line-height: 1.4;
      color: #e2e8f0;
      min-height: 58px;
      position: relative;
    }
    .ghost-avatar-box {
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px 0;
      position: relative;
    }
    .ghost-svg-wrapper {
      width: 160px;
      height: 180px;
      filter: drop-shadow(0 0 25px rgba(255,255,255,0.25));
      animation: ghost-float 3s ease-in-out infinite alternate;
    }
    @keyframes ghost-float {
      0% { transform: translateY(0px); }
      100% { transform: translateY(-10px); }
    }

    /* Joystick virtual */
    .joystick-panel {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
      padding-top: 10px;
      border-top: 1px solid var(--card-border);
    }
    .joy-btn {
      background: rgba(255,255,255,0.06);
      border: 1px solid rgba(255,255,255,0.15);
      color: #fff;
      padding: 10px 18px;
      border-radius: 10px;
      font-size: 0.85rem;
      font-weight: 700;
      cursor: pointer;
      transition: all 0.15s ease;
      width: 100%;
    }
    .joy-btn:hover {
      background: rgba(0, 242, 254, 0.2);
      border-color: var(--primary);
      transform: translateY(-1px);
    }
    .joy-btn.ok {
      background: linear-gradient(135deg, rgba(0,242,254,0.3), rgba(0,255,170,0.3));
      border-color: var(--primary);
    }
    .joy-btn.cancel {
      background: rgba(255, 51, 102, 0.2);
      border-color: var(--danger);
      color: #ff99aa;
    }

    /* Columna 2: Catálogo */
    .products-list {
      display: flex;
      flex-direction: column;
      gap: 12px;
    }
    .product-row {
      display: flex;
      align-items: center;
      gap: 16px;
      padding: 14px 18px;
      border-radius: 12px;
      background: rgba(255,255,255,0.03);
      border: 1px solid var(--card-border);
      cursor: pointer;
      transition: all 0.2s ease;
    }
    .product-row:hover {
      background: rgba(255,255,255,0.06);
      border-color: rgba(0, 242, 254, 0.4);
    }
    .product-row.selected {
      background: rgba(0, 242, 254, 0.12);
      border-color: var(--primary);
      box-shadow: 0 0 20px rgba(0, 242, 254, 0.2);
    }
    .prod-badge {
      font-size: 0.65rem;
      font-weight: 800;
      padding: 3px 6px;
      border-radius: 4px;
      background: rgba(0, 242, 254, 0.2);
      color: var(--primary);
    }
    .prod-icon { font-size: 2rem; }
    .prod-info { flex: 1; }
    .prod-name { font-size: 1.05rem; font-weight: 700; margin-bottom: 2px; }
    .prod-desc { font-size: 0.78rem; color: var(--text-muted); }
    .prod-meta { display: flex; gap: 12px; margin-top: 6px; align-items: center; }
    .prod-price { font-weight: 800; color: var(--warning); font-size: 0.95rem; }
    .prod-stock { font-size: 0.75rem; color: var(--text-muted); }
    .btn-choose {
      padding: 8px 16px;
      border-radius: 8px;
      border: 1px solid var(--primary);
      background: transparent;
      color: var(--primary);
      font-weight: 700;
      font-size: 0.8rem;
      cursor: pointer;
      transition: all 0.15s ease;
    }
    .product-row.selected .btn-choose {
      background: var(--primary);
      color: #000;
    }

    /* Caja NFC */
    .nfc-box {
      margin-top: 8px;
      padding: 16px;
      border-radius: 12px;
      background: rgba(0, 242, 254, 0.04);
      border: 1px dashed rgba(0, 242, 254, 0.3);
      display: flex;
      flex-direction: column;
      gap: 12px;
    }
    .nfc-box.waiting {
      border-color: var(--warning);
      background: rgba(255, 183, 0, 0.06);
    }
    .nfc-row {
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .nfc-card-icon {
      font-size: 2rem;
      background: rgba(255,255,255,0.05);
      padding: 10px;
      border-radius: 10px;
    }
    .nfc-input {
      background: rgba(0,0,0,0.5);
      border: 1px solid var(--card-border);
      color: #fff;
      padding: 6px 10px;
      border-radius: 6px;
      font-family: monospace;
      font-size: 0.95rem;
      width: 140px;
    }
    .btn-nfc-tap {
      background: linear-gradient(135deg, #00f2fe, #4facfe);
      color: #000;
      font-weight: 800;
      font-size: 0.95rem;
      padding: 14px;
      border-radius: 10px;
      border: none;
      cursor: pointer;
      box-shadow: 0 6px 20px rgba(0,242,254,0.4);
      transition: all 0.2s ease;
    }
    .btn-nfc-tap:hover {
      transform: translateY(-2px);
      box-shadow: 0 10px 25px rgba(0,242,254,0.6);
    }

    /* Columna 3: Telemetría y Consola */
    .hw-widget {
      padding: 14px;
      border-radius: 12px;
      background: rgba(255,255,255,0.03);
      border: 1px solid var(--card-border);
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .hw-top {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .hw-title { font-weight: 700; font-size: 0.9rem; }
    .hw-sub { font-size: 0.7rem; color: var(--text-muted); }
    .hw-status-badge {
      font-size: 0.7rem;
      font-weight: 700;
      padding: 2px 8px;
      border-radius: 10px;
      background: rgba(255,255,255,0.08);
    }
    .hw-status-badge.running {
      background: rgba(0, 255, 170, 0.2);
      color: var(--success);
    }
    .hw-btns {
      display: flex;
      gap: 8px;
      margin-top: 4px;
    }
    .btn-subhw {
      flex: 1;
      padding: 6px;
      border-radius: 6px;
      border: 1px solid rgba(255,255,255,0.1);
      background: rgba(255,255,255,0.05);
      color: #fff;
      font-size: 0.75rem;
      cursor: pointer;
    }
    .btn-subhw:hover { background: rgba(255,255,255,0.12); }

    /* Consola UART */
    .console-box {
      display: flex;
      flex-direction: column;
      gap: 6px;
      flex: 1;
    }
    .console-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.75rem;
      color: var(--text-muted);
    }
    .console-logs {
      background: #040408;
      border: 1px solid var(--card-border);
      border-radius: 8px;
      padding: 10px;
      height: 180px;
      overflow-y: auto;
      font-family: monospace;
      font-size: 0.75rem;
      display: flex;
      flex-direction: column;
      gap: 4px;
    }
    .log-row { word-break: break-all; }
    .log-row.rx { color: #00f2fe; }
    .log-row.tx { color: #00ffaa; }
    .log-row.sys { color: #ffb700; }
  </style>
</head>
<body>

  <!-- Navbar -->
  <nav class="navbar">
    <div class="brand">
      <span class="brand-icon">👻</span>
      <div>
        <div class="brand-title">IAFANTASMA</div>
        <div class="brand-sub">EXPENSEDORA WEB3</div>
      </div>
    </div>
    <div class="nav-status">
      <div class="badge-ip">
        <span class="dot"></span>
        <span>ESP32 AP (192.168.4.1)</span>
      </div>
    </div>
  </nav>

  <!-- Banner de Estado -->
  <div class="top-banner">
    <div>
      <span id="state-pill" class="status-pill MENU_SELECCION">ESTADO: MENU_SELECCION</span>
    </div>
    <div class="active-product">
      PRODUCTO ACTIVO: <strong id="active-prod-name">[1] Soda Web3</strong>
    </div>
  </div>

  <!-- Dashboard 3 Columnas -->
  <div class="dashboard-grid">

    <!-- COLUMNA 1: Fantasma y Joystick -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">👻 Avatar Espectral</span>
        <span class="tag-badge" id="ghost-emotion-tag">IDLE</span>
      </div>

      <div class="speech-bubble" id="ghost-speech">
        ¡Bienvenido! Selecciona un producto del catálogo con el joystick o en pantalla.
      </div>

      <!-- Barra de Asistente de Voz ElevenLabs -->
      <div style="background:rgba(18,18,30,0.85); border:1px solid rgba(0,242,254,0.3); border-radius:10px; padding:10px; margin:10px 0; display:flex; flex-direction:column; gap:6px;">
        <div style="display:flex; justify-content:space-between; align-items:center;">
          <button id="btn-voice" onclick="toggleVoiceOrder()" style="background:linear-gradient(135deg,rgba(0,242,254,0.2),rgba(121,40,202,0.3)); border:1px solid #00f2fe; color:#fff; border-radius:18px; padding:5px 12px; font-weight:700; cursor:pointer; font-size:0.78rem; display:flex; align-items:center; gap:6px;">
            <span id="voice-icon">🎙️</span>
            <span id="voice-btn-text">Pedir por Voz</span>
          </button>
          <span style="font-size:0.68rem; color:#00f2fe; background:rgba(0,242,254,0.1); border:1px solid rgba(0,242,254,0.25); padding:2px 6px; border-radius:4px;">ElevenLabs AI</span>
        </div>
        <div id="voice-transcript" style="font-size:0.76rem; font-style:italic; color:#e0f2fe; min-height:16px;">
          "Di: Quiero una Soda, Dame un Snack, o Crypto Energy"
        </div>
        <div style="display:flex; gap:4px; flex-wrap:wrap; margin-top:2px;">
          <button onclick="processVoicePhrase('Quiero una Soda Web3')" style="background:rgba(255,255,255,0.06); border:1px solid rgba(255,255,255,0.1); border-radius:10px; color:#aaa; font-size:0.68rem; padding:2px 6px; cursor:pointer;">🥤 Soda</button>
          <button onclick="processVoicePhrase('Dame un Cyber Snack')" style="background:rgba(255,255,255,0.06); border:1px solid rgba(255,255,255,0.1); border-radius:10px; color:#aaa; font-size:0.68rem; padding:2px 6px; cursor:pointer;">🍫 Snack</button>
          <button onclick="processVoicePhrase('Comprar Crypto Energy')" style="background:rgba(255,255,255,0.06); border:1px solid rgba(255,255,255,0.1); border-radius:10px; color:#aaa; font-size:0.68rem; padding:2px 6px; cursor:pointer;">⚡ Energy</button>
        </div>
      </div>


      <div class="ghost-avatar-box">
        <svg class="ghost-svg-wrapper" viewBox="0 0 160 200" xmlns="http://www.w3.org/2000/svg">
          <defs>
            <radialGradient id="bodyGrad" cx="50%" cy="35%" r="60%">
              <stop offset="0%" stop-color="#ffffff"/>
              <stop offset="60%" stop-color="#e8e8e8"/>
              <stop offset="100%" stop-color="#c0c0c0"/>
            </radialGradient>
            <filter id="glow">
              <feGaussianBlur stdDeviation="3" result="b"/>
              <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
            </filter>
          </defs>
          <!-- Cuerpo Fantasma -->
          <ellipse cx="80" cy="192" rx="38" ry="7" fill="rgba(0,0,0,0.3)"/>
          <path id="ghost-body" d="M 20 90 Q 18 50 80 20 Q 142 50 140 90 L 140 145 Q 80 165 20 145 Z" fill="url(#bodyGrad)" filter="url(#glow)"/>
          <path d="M 20 140 Q 10 162 24 175 Q 34 188 44 172 Q 52 158 58 172 Q 66 186 75 170 Q 82 155 88 170 Q 95 185 105 170 Q 114 155 120 172 Q 128 188 138 175 Q 150 162 140 140 Z" fill="url(#bodyGrad)"/>
          
          <!-- Ojos dinámicos -->
          <g id="ghost-eyes">
            <ellipse cx="58" cy="82" rx="12" ry="12" fill="#cc1515"/>
            <ellipse cx="55" cy="80" rx="4" ry="4" fill="#000"/>
            <ellipse cx="61" cy="78" rx="2" ry="2" fill="#fff"/>
            <ellipse cx="102" cy="82" rx="12" ry="12" fill="#cc1515"/>
            <ellipse cx="99" cy="80" rx="4" ry="4" fill="#00"/>
            <ellipse cx="105" cy="78" rx="2" ry="2" fill="#fff"/>
          </g>
          <!-- Boca -->
          <path id="ghost-mouth" d="M 68 114 Q 80 126 92 114" fill="none" stroke="#cc1515" stroke-width="3.5" stroke-linecap="round"/>
        </svg>
      </div>

      <!-- Joystick Pico / Controles -->
      <div class="joystick-panel">
        <button class="joy-btn" onclick="sendCmd('ARRIBA')">▲ ARRIBA</button>
        <button id="btn-joy-ok" class="joy-btn ok" onclick="sendCmd('OK')">OK (Pagar)</button>
        <button class="joy-btn" onclick="sendCmd('ABAJO')">▼ ABAJO</button>
      </div>
    </div>

    <!-- COLUMNA 2: Catálogo y NFC -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">📦 Catálogo de Productos</span>
        <span class="tag-badge">TFT 320x240 ILI9341</span>
      </div>

      <!-- Indicador de Flujo n8n -->
      <div style="display:flex; justify-content:space-between; align-items:center; background:linear-gradient(90deg, rgba(255,109,0,0.12), rgba(255,0,127,0.08)); border:1px solid rgba(255,109,0,0.3); border-radius:8px; padding:6px 12px; margin-bottom:12px; font-size:0.78rem;">
        <div>⚡ <strong>Flujo n8n:</strong> <span id="n8n-status-text" style="color:#ff9e40;">Listo</span></div>
        <button onclick="testN8n()" style="background:rgba(255,109,0,0.2); border:1px solid rgba(255,109,0,0.4); color:#ffb74d; border-radius:4px; padding:2px 8px; font-size:0.7rem; cursor:pointer;">Probar Webhook</button>
      </div>


      <div class="products-list">
        <!-- Opcion 1 -->
        <div class="product-row selected" id="prod-0" onclick="selectProduct(0)">
          <div class="prod-badge">OPCIÓN 1</div>
          <div class="prod-icon">🥤</div>
          <div class="prod-info">
            <div class="prod-name">Soda Web3</div>
            <div class="prod-desc">Refresco gasificado con menta cuántica</div>
            <div class="prod-meta">
              <span class="prod-price">1.50 USDC</span>
              <span class="prod-stock">Stock: 5</span>
            </div>
          </div>
          <button class="btn-choose">Elegir</button>
        </div>

        <!-- Opcion 2 -->
        <div class="product-row" id="prod-1" onclick="selectProduct(1)">
          <div class="prod-badge">OPCIÓN 2</div>
          <div class="prod-icon">🍫</div>
          <div class="prod-info">
            <div class="prod-name">Cyber Snack</div>
            <div class="prod-desc">Barra con chocolate espectral</div>
            <div class="prod-meta">
              <span class="prod-price">0.80 USDC</span>
              <span class="prod-stock">Stock: 6</span>
            </div>
          </div>
          <button class="btn-choose">Elegir</button>
        </div>

        <!-- Opcion 3 -->
        <div class="product-row" id="prod-2" onclick="selectProduct(2)">
          <div class="prod-badge">OPCIÓN 3</div>
          <div class="prod-icon">⚡</div>
          <div class="prod-info">
            <div class="prod-name">Crypto Energy</div>
            <div class="prod-desc">Bebida de alto voltaje para hackathones</div>
            <div class="prod-meta">
              <span class="prod-price">2.20 USDC</span>
              <span class="prod-stock">Stock: 4</span>
            </div>
          </div>
          <button class="btn-choose">Elegir</button>
        </div>
      </div>

      <!-- Módulo NFC PN532 -->
      <div class="nfc-box" id="nfc-box">
        <div class="nfc-row">
          <div class="nfc-card-icon">💳</div>
          <div>
            <div style="font-weight:700;font-size:0.9rem;">Lector PN532 I2C (SDA 22 / SCL 21)</div>
            <div style="font-size:0.75rem;color:var(--text-muted);margin-top:3px;">
              Tangem / MIFARE ISO14443A &bull; UID:
              <input id="uid-input" class="nfc-input" value="4A:8F:12:9C" />
            </div>
          </div>
        </div>
        <button class="btn-nfc-tap" onclick="tapCard()">
          ⚡ ACERCAR TARJETA / TAG NFC
        </button>
      </div>
    </div>

    <!-- COLUMNA 3: Telemetría de Hardware -->
    <div class="card">
      <div class="card-header">
        <span class="card-title">⚙️ Telemetría de Hardware</span>
        <span class="tag-badge">ESP32 &amp; PICO RP2040</span>
      </div>

      <!-- Servomotor -->
      <div class="hw-widget">
        <div class="hw-top">
          <div>
            <div class="hw-title">🔄 Servomotor DS04-NFC</div>
            <div class="hw-sub">Pico GP15 (PWM 50Hz)</div>
          </div>
          <span id="servo-badge" class="hw-status-badge">DETENIDO</span>
        </div>
        <div style="font-size:0.8rem;color:var(--text-muted);">
          Duty Cycle: <strong>4915</strong> &bull; Sentido: <strong>NORMAL (6000)</strong>
        </div>
        <div class="hw-btns">
          <button class="btn-subhw" onclick="sendCmd('DESPACHAR')">▶ Despacho Manual</button>
          <button class="btn-subhw" onclick="sendCmd('STOP_SERVO')">⏹ Parar Motor</button>
        </div>
      </div>

      <!-- Sensor IR -->
      <div class="hw-widget">
        <div class="hw-top">
          <div>
            <div class="hw-title">🔴 Sensor Infrarrojo IR</div>
            <div class="hw-sub">Pico GP14 (Caída de Producto)</div>
          </div>
          <span id="ir-badge" class="hw-status-badge" style="color:var(--primary);">HAZ DESPEJADO (1)</span>
        </div>
        <div class="hw-btns">
          <button class="btn-subhw" onclick="simulateDrop(true)">✅ Simular Caída</button>
          <button class="btn-subhw" onclick="simulateDrop(false)">⚠️ Simular Timeout</button>
        </div>
      </div>

      <!-- Consola Serie UART -->
      <div class="console-box">
        <div class="console-header">
          <span>📟 Registro UART (115200 bps)</span>
          <a href="javascript:void(0)" onclick="clearLogs()" style="color:var(--primary);text-decoration:none;">Limpiar</a>
        </div>
        <div class="console-logs" id="console-logs">
          <div class="log-row sys">[SYS] Conectando a ESP32 vía WebSocket...</div>
        </div>
      </div>
    </div>

  </div>

  <script>
    const CATALOG = [
      { name: "Soda Web3", price: "1.50 USDC" },
      { name: "Cyber Snack", price: "0.80 USDC" },
      { name: "Crypto Energy", price: "2.20 USDC" }
    ];
    let selectedIdx = 0;
    let currentState = "MENU_SELECCION";
    let ws;

    let eventSource = null;

    function initConnection() {
      // 1. Iniciar Server-Sent Events (SSE) en /events
      if (window.EventSource) {
        eventSource = new EventSource("/events");
        eventSource.onopen = () => {
          addLog("sys", "¡Conexión en tiempo real con ESP32 activa!");
        };
        eventSource.onmessage = (e) => {
          if (e.data) handleServerMessage(e.data.trim());
        };
        eventSource.onerror = () => {
          addLog("sys", "Reconectando con ESP32...");
        };
      }

      // 2. Poll inicial de estado
      pollStatus();
      setInterval(pollStatus, 1500);
    }

    function pollStatus() {
      fetch("/status")
        .then(res => res.json())
        .then(data => {
          if (data) {
            currentState = data.state || currentState;
            selectedIdx = (data.selected !== undefined) ? data.selected : selectedIdx;
            updateUI();
          }
        })
        .catch(() => {});
    }

    function sendCmd(cmd) {
      addLog("tx", cmd);
      fetch("/api/cmd?c=" + encodeURIComponent(cmd), { method: "POST" })
        .then(res => res.json())
        .then(data => {
          if (data && data.state) {
            currentState = data.state;
            if (data.selected !== undefined) selectedIdx = data.selected;
            updateUI();
          }
        })
        .catch(err => {
          addLog("sys", "Error al enviar: " + err);
        });
    }

    function selectProduct(idx) {
      if (idx !== selectedIdx) {
        selectedIdx = idx;
        updateUI();
        sendCmd("SELECT:" + idx);
      }
    }

    function tapCard() {
      const uid = document.getElementById("uid-input").value.trim() || "4A:8F:12:9C";
      sendCmd("NFC_TAP:" + uid);
    }

    function simulateDrop(success) {
      if (success) {
        sendCmd("ITEM_DROPPED");
        document.getElementById("ir-badge").textContent = "CAÍDA CONFIRMADA (0)";
        document.getElementById("ir-badge").style.color = "var(--success)";
        setTimeout(() => {
          document.getElementById("ir-badge").textContent = "HAZ DESPEJADO (1)";
          document.getElementById("ir-badge").style.color = "var(--primary)";
        }, 2000);
      } else {
        sendCmd("ERROR_DROP");
      }
    }

    function handleServerMessage(msg) {
      addLog("rx", msg);

      if (msg.startsWith("STATE:")) {
        const parts = msg.split(":");
        currentState = parts[1];
        if (parts.length > 2) selectedIdx = parseInt(parts[2]);
        updateUI();
      } else if (msg.includes("DESPACHAR")) {
        triggerDispenseUI();
      } else if (msg.includes("Rx Pico: ARRIBA") || msg.includes("ARRIBA")) {
        selectedIdx = (selectedIdx - 1 + 3) % 3;
        updateUI();
      } else if (msg.includes("Rx Pico: ABAJO") || msg.includes("ABAJO")) {
        selectedIdx = (selectedIdx + 1) % 3;
        updateUI();
      } else if (msg.includes("Rx Pico: OK") || msg === "OK") {
        if (currentState === "MENU_SELECCION") {
          currentState = "ESPERANDO_PAGO";
        } else {
          currentState = "MENU_SELECCION";
        }
        updateUI();
      }
    }

    function updateUI() {
      for (let i = 0; i < 3; i++) {
        const row = document.getElementById("prod-" + i);
        if (row) {
          if (i === selectedIdx) {
            row.classList.add("selected");
            row.querySelector(".btn-choose").textContent = "Seleccionado";
          } else {
            row.classList.remove("selected");
            row.querySelector(".btn-choose").textContent = "Elegir";
          }
        }
      }

      const prod = CATALOG[selectedIdx];
      document.getElementById("active-prod-name").textContent = "[" + (selectedIdx + 1) + "] " + prod.name;

      const pill = document.getElementById("state-pill");
      pill.className = "status-pill " + currentState;
      pill.textContent = "ESTADO: " + currentState;

      const btnOk = document.getElementById("btn-joy-ok");
      if (currentState === "ESPERANDO_PAGO") {
        btnOk.textContent = "CANCELAR";
        btnOk.className = "joy-btn cancel";
        document.getElementById("nfc-box").classList.add("waiting");
        setGhost("thinking", "Acerque su tarjeta o tag NFC para pagar " + prod.name + " (" + prod.price + ")");
      } else if (currentState === "PAGO_EXITOSO") {
        btnOk.textContent = "OK (Pagar)";
        btnOk.className = "joy-btn ok";
        document.getElementById("nfc-box").classList.remove("waiting");
        setGhost("happy", "¡Pago Aprobado! Despachando " + prod.name + "...");
      } else {
        btnOk.textContent = "OK (Pagar)";
        btnOk.className = "joy-btn ok";
        document.getElementById("nfc-box").classList.remove("waiting");
        setGhost("idle", "Seleccionaste: " + prod.name + " (" + prod.price + "). Presiona OK para pagar.");
      }
    }

    function setGhost(emotion, text) {
      document.getElementById("ghost-emotion-tag").textContent = emotion.toUpperCase();
      document.getElementById("ghost-speech").textContent = text;
      const eyes = document.getElementById("ghost-eyes");
      const mouth = document.getElementById("ghost-mouth");

      if (emotion === "thinking") {
        eyes.innerHTML = `
          <ellipse cx="58" cy="83" rx="12" ry="10" fill="#00f2fe"/>
          <ellipse cx="55" cy="80" rx="4" ry="4" fill="#001a33"/>
          <ellipse cx="102" cy="83" rx="12" ry="6" fill="#00f2fe"/>
          <ellipse cx="99" cy="82" rx="4" ry="4" fill="#001a33"/>
          <text x="120" y="55" font-size="20" font-weight="bold" fill="#00f2fe">?</text>
        `;
        mouth.setAttribute("d", "M 70 116 Q 80 114 90 117");
        mouth.setAttribute("stroke", "#00f2fe");
      } else if (emotion === "happy") {
        eyes.innerHTML = `
          <path d="M 46 85 Q 58 72 70 85" fill="none" stroke="#ffb700" stroke-width="4" stroke-linecap="round"/>
          <path d="M 90 85 Q 102 72 114 85" fill="none" stroke="#ffb700" stroke-width="4" stroke-linecap="round"/>
          <text x="42" y="72" font-size="12" fill="#ffd54f">★</text>
          <text x="112" y="72" font-size="12" fill="#ffd54f">★</text>
        `;
        mouth.setAttribute("d", "M 66 110 Q 80 130 94 110");
        mouth.setAttribute("stroke", "#ff3366");
      } else {
        eyes.innerHTML = `
          <ellipse cx="58" cy="82" rx="12" ry="12" fill="#cc1515"/>
          <ellipse cx="55" cy="80" rx="4" ry="4" fill="#000"/>
          <ellipse cx="61" cy="78" rx="2" ry="2" fill="#fff"/>
          <ellipse cx="102" cy="82" rx="12" ry="12" fill="#cc1515"/>
          <ellipse cx="99" cy="80" rx="4" ry="4" fill="#000"/>
          <ellipse cx="105" cy="78" rx="2" ry="2" fill="#fff"/>
        `;
        mouth.setAttribute("d", "M 68 114 Q 80 126 92 114");
        mouth.setAttribute("stroke", "#cc1515");
      }
    }

    function triggerDispenseUI() {
      const badge = document.getElementById("servo-badge");
      badge.textContent = "DESPACHANDO";
      badge.classList.add("running");
      setTimeout(() => {
        badge.textContent = "DETENIDO";
        badge.classList.remove("running");
      }, 3500);
    }

    function addLog(type, text) {
      const logs = document.getElementById("console-logs");
      const d = document.createElement("div");
      d.className = "log-row " + type;
      const t = new Date().toLocaleTimeString();
      d.textContent = "[" + t + "] " + type.toUpperCase() + ": " + text;
      logs.appendChild(d);
      logs.scrollTop = logs.scrollHeight;
    }

    function clearLogs() {
      document.getElementById("console-logs").innerHTML = "";
    }

    // ===== CONTROL DE VOZ ELEVENLABS Y FLUJO n8n =====
    let elevenLabsApiKey = localStorage.getItem("elevenlabs_api_key") || "";
    let elevenLabsVoiceId = localStorage.getItem("elevenlabs_voice_id") || "21m00Tcm4TlvDq8ikWAM";
    let n8nWebhookUrl = localStorage.getItem("n8n_webhook_url") || "https://mak0ia0.app.n8n.cloud/webhook/vending-order";

    let recognition = null;
    let isListening = false;

    function initVoice() {
      const SpeechRec = window.SpeechRecognition || window.webkitSpeechRecognition;
      if (SpeechRec) {
        recognition = new SpeechRec();
        recognition.lang = "es-MX";
        recognition.continuous = false;
        recognition.interimResults = true;

        recognition.onstart = () => {
          isListening = true;
          document.getElementById("voice-icon").textContent = "🔴";
          document.getElementById("voice-btn-text").textContent = "Escuchando...";
          document.getElementById("voice-transcript").textContent = "Escuchando tu voz...";
          addLog("sys", "🎙️ Micrófono activado: Di lo que deseas pedir.");
        };

        recognition.onresult = (e) => {
          let final = "";
          for (let i = e.resultIndex; i < e.results.length; ++i) {
            if (e.results[i].isFinal) final += e.results[i][0].transcript;
          }
          if (final) {
            document.getElementById("voice-transcript").textContent = '"' + final + '"';
            processVoicePhrase(final);
          }
        };

        recognition.onerror = (e) => {
          isListening = false;
          document.getElementById("voice-icon").textContent = "🎙️";
          document.getElementById("voice-btn-text").textContent = "Pedir por Voz";
          document.getElementById("voice-transcript").textContent = "Error mic: " + e.error;
        };

        recognition.onend = () => {
          isListening = false;
          document.getElementById("voice-icon").textContent = "🎙️";
          document.getElementById("voice-btn-text").textContent = "Pedir por Voz";
        };
      }
    }

    function toggleVoiceOrder() {
      if (!recognition) initVoice();
      if (!recognition) {
        alert("Reconocimiento de voz no disponible en este navegador. Usa Google Chrome o Microsoft Edge.");
        return;
      }
      if (isListening) {
        recognition.stop();
      } else {
        try {
          recognition.start();
        } catch(e) {
          recognition.stop();
          setTimeout(() => recognition.start(), 200);
        }
      }
    }

    function processVoicePhrase(phrase) {
      const t = phrase.toLowerCase().trim();
      addLog("rx", 'Voz detectada: "' + phrase + '"');
      let selected = -1;
      let reply = "";

      if (t.includes("soda") || t.includes("refresco") || t.includes("lata") || t.includes("uno") || t.includes("1")) {
        selected = 0;
      } else if (t.includes("snack") || t.includes("chocolate") || t.includes("barra") || t.includes("dos") || t.includes("2")) {
        selected = 1;
      } else if (t.includes("energy") || t.includes("energía") || t.includes("crypto") || t.includes("tres") || t.includes("3")) {
        selected = 2;
      }

      if (selected >= 0) {
        selectProduct(selected);
        sendCmd("OK");
        const prod = CATALOG[selected];
        reply = "¡Excelente elección! Has pedido " + prod.name + " (" + prod.price + "). Por favor acerca tu tarjeta NFC.";
        triggerN8n("VOICE_ORDER_RECEIVED", { phrase: phrase, product: prod, action: "WAIT_NFC" });
      } else if (t.includes("cancelar") || t.includes("atras") || t.includes("atrás")) {
        sendCmd("CANCEL");
        reply = "Operación cancelada. Elige otro producto cuando gustes.";
        triggerN8n("ORDER_CANCELLED_BY_VOICE", { phrase: phrase });
      } else if (t.includes("pagar") || t.includes("ok") || t.includes("comprar")) {
        sendCmd("OK");
        reply = "Esperando tu tarjeta NFC para completar el pago.";
      } else {
        reply = "Escuché: " + phrase + ". Prueba diciendo 'Quiero una Soda', 'Dame un Snack', o 'Crypto Energy'.";
      }

      speakElevenLabs(reply);
    }

    async function speakElevenLabs(text) {
      setGhost("happy", text);

      if (!elevenLabsApiKey) {
        if ('speechSynthesis' in window) {
          window.speechSynthesis.cancel();
          const u = new SpeechSynthesisUtterance(text);
          u.lang = 'es-ES';
          u.pitch = 0.85;
          window.speechSynthesis.speak(u);
        }
        return;
      }

      try {
        addLog("tx", "ElevenLabs TTS -> Generando voz espectral...");
        const res = await fetch("https://api.elevenlabs.io/v1/text-to-speech/" + elevenLabsVoiceId, {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            "xi-api-key": elevenLabsApiKey
          },
          body: JSON.stringify({
            text: text,
            model_id: "eleven_multilingual_v2",
            voice_settings: { stability: 0.45, similarity_boost: 0.85 }
          })
        });

        if (res.ok) {
          const blob = await res.blob();
          const audio = new Audio(URL.createObjectURL(blob));
          audio.play();
          addLog("rx", "ElevenLabs TTS -> Reproduciendo voz.");
        } else {
          throw new Error("HTTP " + res.status);
        }
      } catch(e) {
        console.warn("Fallo ElevenLabs API, usando fallback nativo:", e);
        if ('speechSynthesis' in window) {
          const u = new SpeechSynthesisUtterance(text);
          u.lang = 'es-ES';
          u.pitch = 0.85;
          window.speechSynthesis.speak(u);
        }
      }
    }

    async function triggerN8n(event, data) {
      const statusEl = document.getElementById("n8n-status-text");
      if (statusEl) statusEl.textContent = "Enviando a n8n...";
      addLog("tx", "n8n Webhook [" + event + "] -> " + n8nWebhookUrl);

      try {
        await fetch(n8nWebhookUrl, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            event: event,
            timestamp: new Date().toISOString(),
            device: "ESP32_WEB_DASHBOARD",
            channel: "Canal A" + (selectedIdx + 1),
            ...data
          })
        });
        if (statusEl) statusEl.textContent = "Enviado OK (200)";
        addLog("rx", "n8n Webhook -> Flujo procesado con éxito.");
      } catch(e) {
        if (statusEl) statusEl.textContent = "Sin conexión n8n";
        addLog("sys", "n8n Webhook aviso: No se pudo conectar con " + n8nWebhookUrl);
      }
    }

    function testN8n() {
      triggerN8n("TEST_CONNECTION", { message: "Prueba desde Web Dashboard ESP32" });
    }

    window.onload = function() {
      initConnection();
      initVoice();
    };

  </script>
</body>
</html>
)rawliteral";
