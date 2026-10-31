import { Component, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { VendingControlComponent } from './components/vending-control/vending-control.component';
import { ChatPanelComponent } from './components/chat-panel/chat-panel.component';
import { ParticlesComponent } from './components/particles/particles.component';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, VendingControlComponent, ChatPanelComponent, ParticlesComponent],
  template: `
    <div class="app-shell">
      <!-- Partículas de fondo -->
      <app-particles></app-particles>

      <!-- Líneas decorativas de fondo -->
      <div class="bg-grid"></div>
      <div class="bg-glow-top"></div>
      <div class="bg-glow-bottom"></div>

      <!-- Barra de navegación principal -->
      <nav class="main-navbar">
        <div class="brand">
          <span class="brand-ghost">👻</span>
          <div class="brand-text">
            <span class="brand-title">IAFANTASMA</span>
            <span class="brand-tag">EXPENDEDORA WEB3 & ELEVENLABS</span>
          </div>
        </div>

        <div class="nav-tabs">
          <button class="tab-btn" 
                  [class.active]="activeTab() === 'vending'" 
                  (click)="activeTab.set('vending')">
            🎮 Control Máquina & Fantasma
          </button>
          <button class="tab-btn" 
                  [class.active]="activeTab() === 'chat'" 
                  (click)="activeTab.set('chat')">
            💬 Chat Espectral
          </button>
        </div>

        <div class="system-badge">
          <span class="online-indicator"></span>
          <span>RP2040 / ESP32 & n8n</span>
        </div>
      </nav>

      <!-- Contenido principal -->
      <main class="app-main">
        <div *ngIf="activeTab() === 'vending'" class="tab-content">
          <app-vending-control></app-vending-control>
        </div>
        <div *ngIf="activeTab() === 'chat'" class="tab-content chat-view">
          <app-chat-panel></app-chat-panel>
        </div>
      </main>
    </div>
  `,
  styles: [`
    .app-shell {
      position: relative;
      width: 100vw;
      height: 100vh;
      display: flex;
      flex-direction: column;
      overflow: hidden;
      background: #08080f;
      font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
    }

    /* Grid de líneas sutil */
    .bg-grid {
      position: fixed;
      inset: 0;
      background-image:
        linear-gradient(rgba(255,255,255,0.02) 1px, transparent 1px),
        linear-gradient(90deg, rgba(255,255,255,0.02) 1px, transparent 1px);
      background-size: 40px 40px;
      pointer-events: none;
      z-index: 0;
    }

    /* Resplandores de fondo */
    .bg-glow-top {
      position: fixed;
      top: -120px;
      left: 50%;
      transform: translateX(-50%);
      width: 700px;
      height: 350px;
      background: radial-gradient(ellipse, rgba(0, 242, 254, 0.08) 0%, transparent 70%);
      pointer-events: none;
      z-index: 0;
    }

    .bg-glow-bottom {
      position: fixed;
      bottom: -100px;
      right: 10%;
      width: 500px;
      height: 350px;
      background: radial-gradient(ellipse, rgba(255, 0, 127, 0.06) 0%, transparent 70%);
      pointer-events: none;
      z-index: 0;
    }

    /* Barra Superior */
    .main-navbar {
      position: relative;
      z-index: 10;
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px 24px;
      background: rgba(12, 12, 20, 0.85);
      backdrop-filter: blur(12px);
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
      flex-shrink: 0;
    }

    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .brand-ghost {
      font-size: 1.8rem;
      filter: drop-shadow(0 0 8px rgba(0, 242, 254, 0.6));
    }

    .brand-text {
      display: flex;
      flex-direction: column;
    }

    .brand-title {
      font-weight: 800;
      font-size: 1.1rem;
      letter-spacing: 1.5px;
      background: linear-gradient(90deg, #00f2fe, #4facfe);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    .brand-tag {
      font-size: 0.65rem;
      font-weight: 700;
      letter-spacing: 1px;
      color: #ff007f;
    }

    .nav-tabs {
      display: flex;
      gap: 8px;
      background: rgba(0, 0, 0, 0.4);
      padding: 4px;
      border-radius: 12px;
      border: 1px solid rgba(255, 255, 255, 0.08);
    }

    .tab-btn {
      background: transparent;
      border: none;
      color: #9c9cb8;
      padding: 8px 18px;
      border-radius: 8px;
      font-size: 0.86rem;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.2s;
    }

    .tab-btn:hover {
      color: #fff;
      background: rgba(255, 255, 255, 0.05);
    }

    .tab-btn.active {
      background: linear-gradient(135deg, rgba(0, 242, 254, 0.2), rgba(79, 172, 254, 0.2));
      color: #00f2fe;
      border: 1px solid rgba(0, 242, 254, 0.4);
      box-shadow: 0 0 12px rgba(0, 242, 254, 0.25);
    }

    .system-badge {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 0.78rem;
      color: #a0a0c0;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid rgba(255, 255, 255, 0.1);
      padding: 5px 12px;
      border-radius: 20px;
    }

    .online-indicator {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #00ff88;
      box-shadow: 0 0 8px #00ff88;
    }

    /* Contenido principal */
    .app-main {
      position: relative;
      z-index: 1;
      flex: 1;
      height: calc(100vh - 60px);
      overflow-y: auto;
    }

    .tab-content {
      width: 100%;
      height: 100%;
    }

    .chat-view {
      display: flex;
      justify-content: center;
      align-items: center;
    }
  `]
})
export class AppComponent {
  activeTab = signal<'vending' | 'chat'>('vending');
}
