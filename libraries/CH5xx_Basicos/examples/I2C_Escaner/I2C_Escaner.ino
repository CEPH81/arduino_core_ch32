/*
 * I2C_Escaner - busca dispositivos I2C y muestra su direccion por Serial.
 * CH58x/CH59x: I2C por hardware en SDA PB12 / SCL PB13 (Wire.setPins(PB20, PB21) para el remapeo).
 * CH573: no tiene I2C por hardware; se usa I2C por software en las mismas patas
 *        (o las que elija con Wire.setPins(sda, scl)).
 * Ponga resistencias de 4,7 kOhm de SDA y SCL a 3,3 V.
 */
#include <Wire.h>

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
}

void loop()
{
  uint8_t n = 0;
  Serial.println("Buscando dispositivos I2C...");
  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    uint8_t r = Wire.endTransmission();
    if (r == 0) {
      Serial.print("  encontrado en 0x");
      if (dir < 16) Serial.print('0');
      Serial.println(dir, HEX);
      n++;
    } else if (r == 5) {
      Serial.println("  tiempo agotado: revise las resistencias de pull-up");
      break;
    }
  }
  Serial.print(n);
  Serial.println(" dispositivo(s)\n");
  delay(3000);
}
