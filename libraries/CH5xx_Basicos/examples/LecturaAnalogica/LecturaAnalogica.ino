// LecturaAnalogica - A0 (PA4) en milivoltios, la alimentacion y la temperatura del chip.
void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.print("A0 = ");
  Serial.print(analogRead(A0));
  Serial.print(" (");
  Serial.print(analogReadMilliVolts(A0));
  Serial.print(" mV)   VCC = ");
  Serial.print(analogReadVccMilliVolts());
  Serial.print(" mV   temperatura = ");
  Serial.print(temperatureRead(), 1);
  Serial.println(" C");
  delay(1000);
}
