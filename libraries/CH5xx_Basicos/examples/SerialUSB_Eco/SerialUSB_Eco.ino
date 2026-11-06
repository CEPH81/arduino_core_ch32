/*
 * SerialUSB_Eco - puerto serie por el USB de la placa (CDC), sin adaptador USB-UART.
 * En Windows 10/11 aparece como "Dispositivo serie USB (COMx)" sin instalar driver.
 * Abra el Monitor Serie en ese puerto: lo que escriba vuelve con eco.
 *
 * Con Herramientas -> USB support -> "Serial = USB CDC", Serial es SerialUSB y los
 * sketches de siempre (Serial.print...) salen por el USB.
 */
void setup()
{
  SerialUSB.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  static uint32_t t;
  while (SerialUSB.available()) {
    char c = SerialUSB.read();
    SerialUSB.print("eco: ");
    SerialUSB.println(c);
  }
  if (millis() - t > 1000) {
    t = millis();
    digitalToggle(LED_BUILTIN);
    if (SerialUSB) {          /* hay un Monitor Serie abierto (DTR) */
      SerialUSB.print("millis = ");
      SerialUSB.println(t);
    }
  }
}
