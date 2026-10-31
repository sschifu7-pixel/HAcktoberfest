import {
  Component, Input, OnChanges, SimpleChanges,
  ChangeDetectionStrategy, OnInit, OnDestroy
} from '@angular/core';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-ghost-avatar',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './ghost-avatar.component.html',
  styleUrls: ['./ghost-avatar.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class GhostAvatarComponent implements OnInit, OnChanges, OnDestroy {
  @Input() state: 'idle' | 'thinking' | 'talking' | 'happy' | 'surprised' = 'idle';

  eyeOffsetX = 0;
  eyeOffsetY = 0;
  isBlinking = false;
  mouthPath = 'M 55 108 Q 80 122 105 108';
  private blinkInterval: any;
  private eyeInterval: any;
  private mouthInterval: any;
  private particleInterval: any;

  sparkles: { x: number; y: number; size: number; delay: number; id: number }[] = [];
  private sparkleId = 0;

  ngOnInit(): void {
    this.startBlinking();
    this.startEyeMovement();
    this.generateSparkles();
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes['state']) {
      this.updateMouth();
      this.updateEyesForState();
    }
  }

  ngOnDestroy(): void {
    clearInterval(this.blinkInterval);
    clearInterval(this.eyeInterval);
    clearInterval(this.mouthInterval);
    clearInterval(this.particleInterval);
  }

  private startBlinking(): void {
    const scheduleBlink = () => {
      const delay = 2500 + Math.random() * 3000;
      this.blinkInterval = setTimeout(() => {
        this.isBlinking = true;
        setTimeout(() => {
          this.isBlinking = false;
          scheduleBlink();
        }, 200);
      }, delay);
    };
    scheduleBlink();
  }

  private startEyeMovement(): void {
    this.eyeInterval = setInterval(() => {
      if (this.state === 'thinking') {
        this.eyeOffsetX = -4;
        this.eyeOffsetY = -3;
      } else if (this.state === 'idle') {
        this.eyeOffsetX = (Math.random() - 0.5) * 6;
        this.eyeOffsetY = (Math.random() - 0.5) * 4;
      } else {
        this.eyeOffsetX = 0;
        this.eyeOffsetY = 0;
      }
    }, 1800);
  }

  private updateMouth(): void {
    clearInterval(this.mouthInterval);
    switch (this.state) {
      case 'talking':
        let toggle = false;
        this.mouthInterval = setInterval(() => {
          toggle = !toggle;
          this.mouthPath = toggle
            ? 'M 55 108 Q 80 130 105 108'
            : 'M 55 108 Q 80 118 105 108';
        }, 220);
        break;
      case 'happy':
        // Gran sonrisa feliz
        this.mouthPath = 'M 48 108 Q 80 142 112 108';
        break;
      case 'surprised':
        // Boca en "O" grande
        this.mouthPath = 'M 68 106 Q 80 132 92 106';
        break;
      case 'thinking':
        // Boca torcida / dubitativa
        this.mouthPath = 'M 58 112 Q 72 108 82 114 Q 92 120 105 110';
        break;
      default:
        // Idle: sonrisa suave neutral
        this.mouthPath = 'M 55 108 Q 80 122 105 108';
    }
  }

  private updateEyesForState(): void {
    if (this.state === 'surprised') {
      this.eyeOffsetX = 0;
      this.eyeOffsetY = -5;
    } else if (this.state === 'happy') {
      this.eyeOffsetX = 0;
      this.eyeOffsetY = 2;
    }
  }

  private generateSparkles(): void {
    this.particleInterval = setInterval(() => {
      const spark = {
        x: 20 + Math.random() * 120,
        y: 20 + Math.random() * 140,
        size: 2 + Math.random() * 4,
        delay: Math.random() * 0.5,
        id: this.sparkleId++
      };
      this.sparkles.push(spark);
      setTimeout(() => {
        this.sparkles = this.sparkles.filter(s => s.id !== spark.id);
      }, 1500);
    }, 600);
  }

  /** Retorna el id de gradiente SVG correcto para el cuerpo del fantasma */
  getBodyGradient(): string {
    switch (this.state) {
      case 'happy':      return 'url(#bodyGradHappy)';
      case 'thinking':   return 'url(#bodyGradThinking)';
      case 'talking':    return 'url(#bodyGradTalking)';
      case 'surprised':  return 'url(#bodyGradSurprised)';
      default:           return 'url(#bodyGrad)';
    }
  }

  /** Retorna el color del glow interior del cuerpo */
  getInnerGlowColor(): string {
    switch (this.state) {
      case 'happy':      return 'rgba(255, 230, 60, 0.18)';
      case 'thinking':   return 'rgba(60, 130, 255, 0.18)';
      case 'talking':    return 'rgba(224, 32, 32, 0.15)';
      case 'surprised':  return 'rgba(255, 153, 0, 0.22)';
      default:           return 'rgba(224, 32, 32, 0.10)';
    }
  }

  /** Retorna el color de la boca según emoción */
  getMouthColor(): string {
    switch (this.state) {
      case 'happy':      return '#e6a800';
      case 'thinking':   return '#4a90d9';
      case 'surprised':  return '#ff9900';
      default:           return '#cc1515';
    }
  }

  getEyeScale(): number {
    if (this.isBlinking) return 0.05;
    if (this.state === 'surprised') return 1.3;
    return 1;
  }

  trackById(_i: number, s: { id: number }): number {
    return s.id;
  }
}
