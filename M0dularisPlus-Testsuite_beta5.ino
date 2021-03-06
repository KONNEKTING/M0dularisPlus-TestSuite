/* *****************************************************************************
   Includes
 */
#include <SerialFlash.h> // http://librarymanager/All#SerialFlash @ 0.5.0
//#include <SerialFlash.h> // http://librarymanager/All#xxx @ 1.2.3
#include "KonnektingDevice.h"
#include "Memory.h"
//#include <Arduino.h>

/* *****************************************************************************
   Defines
 */

#define KNX_SERIAL Serial1
#define PROG_LED 19 // A1
#define PROG_BTN 15 // A5

#define MANUFACTURER_ID 0xDEAD
#define DEVICE_ID 0xFF
#define REVISION 0x00

KnxComObject KnxDevice::_comObjectsList[] = {
    /* Index 0 - Test Receive Object */ KnxComObject(KNX_DPT_1_001, 0x2a),
    /* Index 1 - Test Send Object */ KnxComObject(KNX_DPT_1_001, 0x34),
};
const byte KnxDevice::_numberOfComObjects =
    sizeof(_comObjectsList) / sizeof(KnxComObject); // do not change this code

byte KonnektingDevice::_paramSizeList[] = {
    /* Index 0 - startupDelay */ PARAM_UINT8,
    /* Index 1 - blink1Delay */ PARAM_UINT8,
    /* Index 2 - blink2Delay */ PARAM_UINT8,
};
const int KonnektingDevice::_numberOfParams =
    sizeof(_paramSizeList); // do not change this code


// void writeComObj(byte comObjId, word ga) {
//     byte gaHi = (ga >> 8) & 0xff;
//     byte gaLo = (ga >> 0) & 0xff;
//     byte settings = 0x80;
//     writeMemory(EEPROM_COMOBJECTTABLE_START + (comObjId * 3) + 0, gaHi);
//     writeMemory(EEPROM_COMOBJECTTABLE_START + (comObjId * 3) + 1, gaLo);
//     writeMemory(EEPROM_COMOBJECTTABLE_START + (comObjId * 3) + 2, settings);
// }


/* *****************************************************************************
   S E T U P
 */
void setup() {

  SerialUSB.begin(115200);
  pinMode(PROG_LED, OUTPUT);
  digitalWrite(PROG_LED, HIGH);
  long start = millis();
  while (!SerialUSB) {
  //while (!SerialUSB && (millis() - start < 2000)) {
    // wait for serial connection at most 2sec
  }
  digitalWrite(PROG_LED, LOW);
  /*
     make debug serial port known to debug class
     Means: KONNEKTING will use this serial port for console debugging
   */
  Debug.setPrintStream(&SerialUSB);

  Debug.println(F("Test-Suite:  M0dularis+ v1.0 @ beta5 build date=%s time=%s"),
                F(__DATE__), F(__TIME__));

  /*
     Set memory functions
   */
  Debug.println(F("Setup Memory ..."));
  setupMemory();
  setupFlash();
  Debug.println(F("--> DONE"));

  /*
   * Fake parametrization
   */
  
      // Debug.println(F("Fake Memory Data..."));
      // word individualAddress = P_ADDR(1,1,1);
      // byte iaHi = (individualAddress >> 8) & 0xff;
      // byte iaLo = (individualAddress >> 0) & 0xff;
      // byte deviceFlags = 0xFE;
      // writeMemory(EEPROM_DEVICE_FLAGS, deviceFlags);
      // writeMemory(EEPROM_INDIVIDUALADDRESS_HI, iaHi);
      // writeMemory(EEPROM_INDIVIDUALADDRESS_LO, iaLo);
      // // writeComObj(0, G_ADDR(15,7,253));
      // // writeComObj(1, G_ADDR(15,7,254));
      // Debug.println(F("--> DONE"));
  

  Debug.println(F("Setup Memory Fctptr ..."));
    // ptr for memory access
    Konnekting.setMemoryReadFunc(&readMemory);
    Konnekting.setMemoryWriteFunc(&writeMemory);
    Konnekting.setMemoryUpdateFunc(&updateMemory);
    Konnekting.setMemoryCommitFunc(&commitMemory);

    // ptr for writing/reading data to/from device
    Konnekting.setDataOpenWriteFunc(&dataOpenWrite);
    Konnekting.setDataOpenReadFunc(&dataOpenRead);
    Konnekting.setDataWriteFunc(&dataWrite);
    Konnekting.setDataReadFunc(&dataRead);
    Konnekting.setDataRemoveFunc(&dataRemove);
    Konnekting.setDataCloseFunc(&dataClose);
    
  Debug.println(F("--> DONE"));

  // M0dularis+ uses "Serial1" !!!
  Konnekting.init(KNX_SERIAL, PROG_BTN, PROG_LED, MANUFACTURER_ID, DEVICE_ID,
                  REVISION);

  Debug.println(F("--> setup() DONE"));
}

long last = 0;
bool state = false;

/* *****************************************************************************
   L O O P
 */
void loop() {

  Knx.task();
  // it's not ready, unless it's programmed via Suite
  if (Konnekting.isReadyForApplication()) {

    // Blink Prog-LED SLOW for testing purpose
    if (millis() - last > 1000) {

      // send state every second to bus
      Knx.write(1, (bool) state);
      //Debug.println(F("Sending %i on 15/7/254 from 1.1.1"), state);
      //Debug.println(F("blink_0 %i "), state);

      if (state) {
        digitalWrite(PROG_LED, HIGH);
        state = false;
      } else {
        digitalWrite(PROG_LED, LOW);
        state = true;
      }
      last = millis();
    }
  } else {
    // Blink Prog-LED FAST for testing purpose
    if (Konnekting.isProgState() && (millis() - last > 200)) {

//      Debug.println(F("blink_1 %i "), state);

      if (state) {
        digitalWrite(PROG_LED, HIGH);
        state = false;
      } else {
        digitalWrite(PROG_LED, LOW);
        state = true;
      }
      last = millis();
    } else {

      // Blink Prog-LED SLOW for testing purpose
      if (millis() - last > 1000) {

        //Debug.println(F("blink_2 %i "), state);

        if (state) {
          digitalWrite(PROG_LED, HIGH);
          state = false;
        } else {
          digitalWrite(PROG_LED, LOW);
          state = true;
        }
        last = millis();
      }
    }
  }
}

// Callback function to handle com objects updates
void knxEvents(byte index) { Debug.println(F("knxEvents: i=%i"), index); };
