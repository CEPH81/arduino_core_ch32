// SerialEco - Serial (UART1: TX PA9, RX PA8) a 115200 baudios: devuelve lo que recibe.
void setup() {
  Serial.begin(115200);
  Serial.printf("CH5xx a %lu MHz\r\n", (unsigned long)getCpuFrequencyMhz());
}

void loop() {
  while (Serial.available()) Serial.write(Serial.read());
}
