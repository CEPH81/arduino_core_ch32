/*
 * Tono - melodia corta con tone() en PA4 (zumbador pasivo con resistencia de 100 ohm).
 */
const uint8_t ZUMBADOR = PA4;
const unsigned int notas[] = {262, 294, 330, 349, 392, 440, 494, 523};

void setup() {}

void loop()
{
  for (uint8_t i = 0; i < 8; i++) {
    tone(ZUMBADOR, notas[i], 200);
    delay(250);
  }
  noTone(ZUMBADOR);
  delay(2000);
}
