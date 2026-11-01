// Interrupcion_Boton - el boton BOOT (PB22, a GND) cambia el LED por interrupcion.
volatile bool pulsado = false;

void alPulsar() {
  pulsado = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(KEY_BUILTIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KEY_BUILTIN), alPulsar, FALLING);
}

void loop() {
  if (pulsado) {
    pulsado = false;
    digitalToggle(LED_BUILTIN);
    Serial.println("pulsado");
    delay(50);
  }
}
