import {
  Component, OnInit, OnDestroy, ElementRef, ViewChild, AfterViewInit
} from '@angular/core';

@Component({
  selector: 'app-particles',
  standalone: true,
  template: `
    <canvas #canvas class="particles-canvas"></canvas>
  `,
  styles: [`
    .particles-canvas {
      position: fixed;
      inset: 0;
      width: 100%;
      height: 100%;
      pointer-events: none;
      z-index: 0;
      opacity: 0.6;
    }
  `]
})
export class ParticlesComponent implements AfterViewInit, OnDestroy {
  @ViewChild('canvas') canvasRef!: ElementRef<HTMLCanvasElement>;

  private ctx!: CanvasRenderingContext2D;
  private particles: Particle[] = [];
  private animFrame!: number;
  private resizeObs!: ResizeObserver;

  ngAfterViewInit(): void {
    const canvas = this.canvasRef.nativeElement;
    this.ctx = canvas.getContext('2d')!;
    this.resize();
    this.initParticles();
    this.animate();

    this.resizeObs = new ResizeObserver(() => this.resize());
    this.resizeObs.observe(document.body);
  }

  ngOnDestroy(): void {
    cancelAnimationFrame(this.animFrame);
    this.resizeObs?.disconnect();
  }

  private resize(): void {
    const canvas = this.canvasRef.nativeElement;
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
  }

  private initParticles(): void {
    const count = Math.floor(window.innerWidth / 18);
    for (let i = 0; i < count; i++) {
      this.particles.push(new Particle(window.innerWidth, window.innerHeight));
    }
  }

  private animate(): void {
    const canvas = this.canvasRef.nativeElement;
    this.ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (const p of this.particles) {
      p.update();
      if (p.y < -20) {
        p.reset(canvas.width, canvas.height);
      }
      p.draw(this.ctx);
    }

    this.animFrame = requestAnimationFrame(() => this.animate());
  }
}

class Particle {
  x!: number;
  y!: number;
  size!: number;
  speedY!: number;
  speedX!: number;
  opacity!: number;
  color!: string;
  rotation!: number;
  rotSpeed!: number;

  constructor(private maxW: number, private maxH: number) {
    this.reset(maxW, maxH, true);
  }

  reset(w: number, h: number, init = false): void {
    this.x = Math.random() * w;
    this.y = init ? Math.random() * h : h + 20;
    this.size = 1 + Math.random() * 3;
    this.speedY = -(0.3 + Math.random() * 0.8);
    this.speedX = (Math.random() - 0.5) * 0.4;
    this.opacity = 0.15 + Math.random() * 0.4;
    this.rotation = Math.random() * 360;
    this.rotSpeed = (Math.random() - 0.5) * 2;
    // Alterna entre blanco y rojo
    const isRed = Math.random() > 0.6;
    this.color = isRed ? `rgba(224,32,32,${this.opacity})` : `rgba(245,245,245,${this.opacity})`;
  }

  update(): void {
    this.y += this.speedY;
    this.x += this.speedX;
    this.rotation += this.rotSpeed;
  }

  draw(ctx: CanvasRenderingContext2D): void {
    ctx.save();
    ctx.translate(this.x, this.y);
    ctx.rotate((this.rotation * Math.PI) / 180);

    // Dibuja un diamante o círculo aleatorio
    if (this.size > 2) {
      ctx.beginPath();
      ctx.moveTo(0, -this.size);
      ctx.lineTo(this.size, 0);
      ctx.lineTo(0, this.size);
      ctx.lineTo(-this.size, 0);
      ctx.closePath();
    } else {
      ctx.beginPath();
      ctx.arc(0, 0, this.size, 0, Math.PI * 2);
    }

    ctx.fillStyle = this.color;
    ctx.shadowBlur = 6;
    ctx.shadowColor = this.color;
    ctx.fill();
    ctx.restore();
  }
}
