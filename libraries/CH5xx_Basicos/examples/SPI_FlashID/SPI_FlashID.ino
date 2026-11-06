/*
 * SPI_FlashID - lee el ID JEDEC de una memoria flash SPI (W25Qxx) por SPI0.
 * Conexion: SCK PA13, MOSI PA14, MISO PA15, CS PA12 (3,3 V).
 * Un W25Q32 responde EF 40 16; un W25Q128, EF 40 18.
 */
#include <SPI.h>

const uint8_t CS = PA12;

void setup()
{
  Serial.begin(115200);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin();
}

void loop()
{
  uint8_t id[3];
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS, LOW);
  SPI.transfer(0x9F);            /* JEDEC ID */
  SPI.transfer(NULL, id, 3);
  digitalWrite(CS, HIGH);
  SPI.endTransaction();

  Serial.print("ID JEDEC: ");
  for (uint8_t i = 0; i < 3; i++) {
    if (id[i] < 16) Serial.print('0');
    Serial.print(id[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
  delay(2000);
}
