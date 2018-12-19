#define I2C_EEPROM


#ifdef I2C_EEPROM

#include <extEEPROM.h>    // "extEEPROM" via Arduino IDE Library Manager, see https://github.com/PaoloP74/extEEPROM v3.3.5


//One 24LC256 EEPROMs on the bus
extEEPROM i2cEEPROM(kbits_256, 1, 64); //device size, number of devices, page size

/* *****************************************************************************
 * I2C MEMORY FUNCTIONS
 * Later on, use without lib, like Eugen: https://github.com/KONNEKTING/KonnektingFirmware/blob/master/Multi-Interface/IR_Transmitter_1.0/mi.h#L102
 */

byte readMemory(int index) {
    return i2cEEPROM.read(index);
}

void writeMemory(int index, byte val) {
    i2cEEPROM.write(index, val);
}

void updateMemory(int index, byte val) {
    i2cEEPROM.update(index, val);
}

void commitMemory() {
    // nothing to do for I2C EEPROM
}

void setupMemory() {
    //uint8_t status = i2cEEPROM.begin(i2cEEPROM.twiClock400kHz); // 400kHz Fast Mode
    byte status = i2cEEPROM.begin(i2cEEPROM.twiClock100kHz); // 100kHz Normal Mode
    if (status) {
        Debug.println(F("extEEPROM.begin() failed, status = %i"), status);
        while (1);
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

byte readMemory(int index) {
    return EEPROM.read(index);
}

void writeMemory(int index, byte val) {
    EEPROM.write(index, val);
}

void updateMemory(int index, byte val) {
    EEPROM.write(index, val);
}

void commitMemory() {
    EEPROM.commit();
}

#endif

// ------------------------

#include <KonnektingDevice.h>
#include <SerialFlash.h> // https://github.com/PaulStoffregen/SerialFlash
#include <CRC32.h> // https://github.com/bakercp/CRC32
#include <SPI.h>

#define SPIFLASH_CSPIN 9

void formatFlash() {
    //We start by formatting the flash...
    uint8_t id[5];
    SerialFlash.readID(id);
    SerialFlash.eraseAll();

    //Flash LED at 1Hz while formatting
    while (!SerialFlash.ready()) {
        Debug.println(F("SerialFlash Format in progress ..."));
        delay(500);
    }

    Debug.println(F("SerialFlash finishing ..."));
    Debug.println(F("SerialFlash *DONE*"));
}

void setupFlash() {
    if (!SerialFlash.begin(SPIFLASH_CSPIN)) {
        Debug.println(F("SerialFlash.begin() failed: Unable to access SPI Flash chip"));
        while (1);
    }
    Debug.println(F("SPI Flash initialized"));
    formatFlash();
}


SerialFlashFile _currFile;
CRC32 crc;

bool dataWritePrepare(DataWritePrepare dwp) {
    unsigned long size = dwp.size;
    char filename[10];
    
    if (dwp.type == DATA_TYPE_ID_UPDATE) {
        sprintf(filename, "UPDATE.BIN");
    } else {
        sprintf(filename, "FI_%03d_%03d", dwp.type, dwp.id);
    }

    Debug.println(F("dataWritePrepare(): filename=%s size=%d"), filename, dwp.size);

    if (SerialFlash.exists(filename)) {
        SerialFlash.remove(filename);
    }
    if (SerialFlash.createErasable(filename, dwp.size)) {
        _currFile = SerialFlash.open(filename);
        Debug.println(F("created file with size %d"), dwp.size);
        if (_currFile) {
            return true;
        } else {
            Debug.println(F("error creating file on flash"));
            return false;
        }

    }

}

bool dataWrite(DataWrite dw){
    Debug.println(F("dataWrite() coun=%d"), dw.count);
    _currFile.write(dw.data, dw.count);
    // update crc calculation
    for (int i = 0; i < dw.count; i++) {
        crc.update(dw.data[i]);
    }
    return true;
}

bool dataWriteFinish(unsigned long crc32) {
    
    unsigned long thisChecksum = crc.finalize();
    Debug.println(F("dataWriteFinish() this=%d other=%d"), thisChecksum, crc32);
    if (thisChecksum == crc32) {
        Debug.println(F("dataWriteFinish() *done*"));
        _currFile.close();
        return true;
    } else {
        Debug.println(F("dataWriteFinish() *failed* erasing ..."));
        _currFile.erase();
        return false;
    }
}