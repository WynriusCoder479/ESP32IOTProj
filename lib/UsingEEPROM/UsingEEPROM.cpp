#include "UsingEEPROM.h"

#define EEPROM_SIZE 512

void WriteEPPROM(String data, int address) {
  for(int i = 0; i < data.length(); i++){
    EEPROM.write(address + i, data[i]);
  }
  EEPROM.write(address + data.length(),0);
}

String ReadEEPROM(int address){
    String data;
    int i;
    byte x;

    i = address;
    do
    {
        x = EEPROM.read(i);
        i++;
        if(x != 0){
            data += char(x);
        }
    } while (x != 0);
    return data;
}

void ClearEEPROM(){
    Serial.println("start....");

    if(!EEPROM.begin(EEPROM_SIZE)){
        Serial.println("Failed to initialise EEPROM");
        delay(1000000);
    }
    Serial.println("Bytes read from flash");

    for(int i = 0; i<EEPROM_SIZE; i++){
        EEPROM.write(i,0);
        Serial.println();
        Serial.println("Clear in memory");
    }
}

