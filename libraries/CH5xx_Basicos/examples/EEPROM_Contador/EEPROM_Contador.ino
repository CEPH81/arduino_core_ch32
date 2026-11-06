/*
 * EEPROM_Contador - cuenta cuantas veces arranco la placa (se guarda en la Data-Flash).
 * EEPROM.begin() lee 1024 bytes a RAM; write() cambia la copia; commit() la graba.
 */
#include <EEPROM.h>

void setup()
{
  Serial.begin(115200);
  delay(1000);
  EEPROM.begin();
  uint32_t arranques;
  EEPROM.get(0, arranques);
  if (arranques == 0xFFFFFFFF) arranques = 0;   /* Data-Flash borrada */
  arranques++;
  EEPROM.put(0, arranques);
  bool ok = EEPROM.commit();
  Serial.print("Arranques: ");
  Serial.print(arranques);
  Serial.println(ok ? "  (guardado)" : "  (error al grabar)");
}

void loop() {}
