// PWM_Fade - sube y baja el brillo de un LED en PA12 (PWM4).
// Patas con PWM: PA12, PA13, PB0, PB4, PB6, PB7, PB14, PB23.
const int pata = PA12;

void setup() {
  analogWriteFrequency(1000);
}

void loop() {
  for (int i = 0; i <= 255; i++) { analogWrite(pata, i); delay(4); }
  for (int i = 255; i >= 0; i--) { analogWrite(pata, i); delay(4); }
}
