#define I2C_EEPROM

#ifdef I2C_EEPROM

#include <extEEPROM.h>  // "extEEPROM" via Arduino IDE Library Manager, see https://github.com/PaoloP74/extEEPROM v3.3.5

// One 24LC256 EEPROMs on the bus
extEEPROM i2cEEPROM(kbits_256, 1,
                    64);  // device size, number of devices, page size

/* *****************************************************************************
 * I2C MEMORY FUNCTIONS
 * Later on, use without lib, like Eugen:
 * https://github.com/KONNEKTING/KonnektingFirmware/blob/master/Multi-Interface/IR_Transmitter_1.0/mi.h#L102
 */

byte readMemory(int index) { return i2cEEPROM.read(index); }

void writeMemory(int index, byte val) { i2cEEPROM.write(index, val); }

void updateMemory(int index, byte val) { i2cEEPROM.update(index, val); }

void commitMemory() {
    // nothing to do for I2C EEPROM
}

void setupMemory() {
    // uint8_t status = i2cEEPROM.begin(i2cEEPROM.twiClock400kHz); // 400kHz Fast
    // Mode
    byte status = i2cEEPROM.begin(i2cEEPROM.twiClock100kHz);  // 100kHz Normal Mode
    if (status) {
        Debug.println(F("extEEPROM.begin() failed, status = %i"), status);
        while (1)
            ;
    }

    // Debug.println(F("extEEPROM clear first 2k ..."));
    // for(int i=0;i<2048;i++) {
    //     writeMemory(i,0xff);
    //     Debug.print(F("."));
    //     if (i%32==1) {
    //         Debug.println(F(""));
    //     }
    // }
    // Debug.println(F("extEEPROM clear first 2k ... *done*"));
}

#else

// Include EEPROM-like API for FlashStorage
// let's have 2k of emulated EEPROM
#define EEPROM_EMULATION_SIZE 2048

// see: https://github.com/cmaglie/FlashStorage >0.7.0
#include <FlashAsEEPROM.h>

/* *****************************************************************************
 * SAMD Flash MEMORY FUNCTIONS
 */

void setupMemory() {
    // nothing to do when using SAMD Flash "EEPROM"
}

byte readMemory(int index) { return EEPROM.read(index); }

void writeMemory(int index, byte val) { EEPROM.write(index, val); }

void updateMemory(int index, byte val) { EEPROM.write(index, val); }

void commitMemory() { EEPROM.commit(); }

#endif

// ------------------------

#include <KonnektingDevice.h>
#include <SPI.h>
#include <SerialFlash.h>  // https://github.com/PaulStoffregen/SerialFlash

#define SPIFLASH_CSPIN 9

void formatFlash() {
    // We start by formatting the flash...
    uint8_t id[5];
    SerialFlash.readID(id);
    SerialFlash.eraseAll();

    while (!SerialFlash.ready()) {
        Debug.println(F("SerialFlash Format in progress ..."));
        delay(100);
    }
    Debug.println(F("SerialFlash Format *DONE*"));
}

void setupFlash() {
    if (!SerialFlash.begin(SPIFLASH_CSPIN)) {
        Debug.println(F("SerialFlash.begin() failed: Unable to access SPI Flash chip"));
        while (1) {
        };
    }
    Debug.println(F("SPI Flash initialized"));
    formatFlash();
}

SerialFlashFile _currFile;

void listFiles() {
    // list files
    SerialFlash.opendir();
    while (1) {
        char filen[64];
        uint32_t filesize;

        Debug.println("listFiles(): Folder:");
        if (SerialFlash.readdir(filen, sizeof(filen), filesize)) {
            Debug.println(F("\t'%s' -> %ld bytes"), filen, filesize);
        } else {
            Debug.println(F("listFiles(): end with folder"));
            break;  // no more files
        }
    }
    // --------------------
}

bool dataOpenWrite(byte type, byte id, unsigned long size) {
    char filename[10];

    if (type == DATA_TYPE_ID_UPDATE) {
        sprintf(filename, "UPDATE.BIN");
    } else {
        sprintf(filename, "FI_%03d_%03d", type, id);
    }

    Debug.println(F("dataOpenWrite(): filename=%s size=%d"), filename, size);

    if (SerialFlash.exists(filename)) {

        // get current file size and compare with new file size
        _currFile = SerialFlash.open(filename);
        unsigned long currsize = _currFile.size();
        _currFile.close();
        if (size != currsize) {
            Debug.println(F("dataOpenWrite(): can't overwrite file with size=%ld with different size=%ld"), currsize, size);
            return false;
        }
        
        Debug.println(F("dataOpenWrite(): removing existing filename=%s"), filename);
        SerialFlash.remove(filename);
    }
    if (SerialFlash.createErasable(filename, size)) {
        //if (SerialFlash.create(filename, size)) {
        _currFile = SerialFlash.open(filename);

        Debug.println(F("dataOpenWrite(): created file with size %d"), size);
        if (_currFile) {
            listFiles();
            return true;
        } else {
            Debug.println(F("dataOpenWrite(): error creating file on flash"));
            return false;
        }
    }
}

bool dataWrite(byte data[], int length) {
    Debug.println(F("dataWrite() length=%d"), length);
    if (!_currFile) {
        return false;
    } 
    _currFile.write(data, length);
    return true;
}

// ------------------------

unsigned long dataOpenRead(byte type, byte id) {
    listFiles();

    char filename[10];

    if (type == DATA_TYPE_ID_UPDATE) {
        return -1;
    } else {
        sprintf(filename, "FI_%03d_%03d", type, id);
    }

    Debug.println(F("dataOpen(): filename=%s"), filename);

    if (SerialFlash.exists(filename)) {
        _currFile = SerialFlash.open(filename);
        Debug.println(F("dataOpen(): filename=%s opened"), filename);
        return _currFile.size();
    } else {
        Debug.println(F("dataOpen(): file '%s' does not exist"), filename);
        return -1;
    }
}

bool dataRead(byte data[], int length) {
    if (_currFile.available() > 0) {
        _currFile.read(data, length);
    } else {
        Debug.println(F("dataRead(): nothing available"));
        return false;
    }
    return true;
}

bool dataClose() {
    if (_currFile != NULL) {
        _currFile.close();
        return true;
    } else {
        return false;
    }
}
