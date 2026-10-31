import { Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VendingService, GhostEmotion } from '../../services/vending.service';
import { VoiceWorkflowService } from '../../services/voice-workflow.service';
import { GhostAvatarComponent } from '../ghost-avatar/ghost-avatar.component';

@Component({
  selector: 'app-vending-control',
  standalone: true,
  imports: [CommonModule, FormsModule, GhostAvatarComponent],
  templateUrl: './vending-control.component.html',
  styleUrls: ['./vending-control.component.css']
})
export class VendingControlComponent {
  vending = inject(VendingService);
  voice = inject(VoiceWorkflowService);
  customUid = '4A:8F:12:9C';

  // Config inputs temporales para el modal de ElevenLabs y n8n
  tempApiKey = this.voice.elevenLabsApiKey();
  tempVoiceId = this.voice.elevenLabsVoiceId();
  tempN8nUrl = this.voice.n8nWebhookUrl();

  // Voces predefinidas populares en ElevenLabs
  voicePresets = [
    { id: '21m00Tcm4TlvDq8ikWAM', name: 'Rachel (Espectral / Serena)' },
    { id: 'pNInz6obpgDQGcFmaJgB', name: 'Adam (Profundo / Épico)' },
    { id: 'ErXwobaYiN019PkySvjV', name: 'Antoni (Enérgico)' },
    { id: 'AZnzlk1XvdvUeBnXmlld', name: 'Domi (Dinámica)' }
  ];

  onToggleVoice() {
    this.voice.toggleListening();
  }

  onQuickVoiceOrder(phrase: string) {
    this.voice.processVoiceCommand(phrase);
  }

  onOpenConfig() {
    this.tempApiKey = this.voice.elevenLabsApiKey();
    this.tempVoiceId = this.voice.elevenLabsVoiceId();
    this.tempN8nUrl = this.voice.n8nWebhookUrl();
    this.voice.showConfigModal.set(true);
  }

  onCloseConfig() {
    this.voice.showConfigModal.set(false);
  }

  onSaveConfig() {
    this.voice.saveConfig(this.tempApiKey, this.tempVoiceId, this.tempN8nUrl);
  }

  onSelectPrevious() {
    this.vending.selectPreviousProduct();
  }

  onSelectNext() {
    this.vending.selectNextProduct();
  }

  onSelectProduct(index: number) {
    this.vending.selectDirectProduct(index);
    const prod = this.vending.catalogo()[index];
    this.voice.triggerN8nWorkflow({
      event: 'PRODUCT_SELECTED_MANUAL',
      product: prod,
      channel: `A${index + 1}`
    });
  }

  onPressOk() {
    this.vending.pressOkButton();
  }

  onCancel() {
    this.vending.cancelPayment();
    this.voice.triggerN8nWorkflow({
      event: 'PAYMENT_CANCELLED_MANUAL'
    });
  }

  onTapCard() {
    const prod = this.vending.productoActual();
    this.vending.processCardTap(this.customUid);

    // 1. Notificar a n8n del pago confirmado por NFC
    this.voice.triggerN8nWorkflow({
      event: 'NFC_PAYMENT_CONFIRMED',
      card_uid: this.customUid,
      product: prod,
      amount: prod.precio,
      channel: `A${this.vending.opcionSeleccionada() + 1}`,
      status: 'DISPENSING'
    });

    // 2. Hablar con la voz de ElevenLabs confirmando el pago y el giro de Motor B
    this.voice.speakWithElevenLabs(
      `¡Pago confirmado con tarjeta NFC! Despachando tu ${prod.nombre}. Motor B activado en la Pico.`
    );
  }

  onSimulateDrop() {
    this.vending.simulateIRDrop();
    const prod = this.vending.productoActual();
    this.voice.triggerN8nWorkflow({
      event: 'DISPENSE_SUCCESS_IR_DETECTED',
      product: prod,
      ir_status: 'ITEM_DROPPED'
    });
  }

  onSimulateTimeout() {
    this.vending.simulateIRTimeout();
    this.voice.triggerN8nWorkflow({
      event: 'DISPENSE_TIMEOUT_ALERT',
      error: 'IR_SENSOR_NOT_TRIGGERED'
    });
  }

  onToggleDirection() {
    this.vending.toggleServoDirection();
  }

  onManualServoStart() {
    this.vending.manualServoStart();
    this.voice.triggerN8nWorkflow({
      event: 'MANUAL_MOTOR_B_STARTED'
    });
  }

  onManualServoStop() {
    this.vending.manualServoStop();
  }

  onSetEmotion(emotion: GhostEmotion) {
    this.vending.setGhostEmotion(emotion);
  }

  onConnectUsb() {
    this.vending.connectSerial();
  }

  onDisconnectUsb() {
    this.vending.disconnectSerial();
  }

  onTestElevenLabsVoice() {
    this.voice.speakWithElevenLabs(
      '¡Buuu! 👻 Soy IAFantasma impulsado por ElevenLabs. Tu orden de voz y el flujo n8n están sincronizados.'
    );
  }

  onTestN8nWebhook() {
    this.voice.triggerN8nWorkflow({
      event: 'TEST_CONNECTION',
      message: 'Prueba de enlace exitosa entre IAFantasma y n8n'
    });
  }
}
