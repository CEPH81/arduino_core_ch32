// Blink - LED de la placa (PA8 en las de WeAct, se enciende en bajo).
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, LED_ON);
  delay(200);
  digitalWrite(LED_BUILTIN, LED_OFF);
  delay(800);
}
