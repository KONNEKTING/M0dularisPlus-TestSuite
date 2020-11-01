// uncomment to use I2C EEPROM instead of SAMD internal flash with EEPROM emulation
//#define I2C_EEPROM

// --------------------------------------------------------------------------------

#ifdef I2C_EEPROM // use I2C EEPROM

    #include <extEEPROM.h>  // "extEEPROM" via Arduino IDE Library Manager, see https://github.com/PaoloP74/extEEPROM v3.3.5

    // One 24LC256 EEPROMs on the bus
    extEEPROM i2cEEPROM(kbits_256, 1, 64);  // device size, number of devices, page size

    /* *****************************************************************************
    * I2C MEMORY FUNCTIONS
    * Later on, use without lib, like Eugen:
    * https://github.com/KONNEKTING/KonnektingFirmware/blob/master/Multi-Interface/IR_Transmitter_1.0/mi.h#L102
    */
    byte readMemory(int index) { return i2cEEPROM.read(index); }
    void writeMemory(int index, byte val) { i2cEEPROM.write(index, val); }
    void updateMemory(int index, byte val) { i2cEEPROM.update(index, val); }
    void commitMemory() { } // nothing to do for I2C EEPROM
    void setupMemory() {
        
        // uint8_t status = i2cEEPROM.begin(i2cEEPROM.twiClock400kHz); // 400kHz Fast Mode
        byte status = i2cEEPROM.begin(i2cEEPROM.twiClock100kHz);  // 100kHz Normal Mode

        if (status) {
            Debug.println(F("extEEPROM.begin() failed, status = %i"), status);
            while (1); // block any further processing
        }

//         Debug.println(F("extEEPROM clear first 2k ..."));
//         for(int i=0;i<2048;i++) {
//             writeMemory(i,0xff);
//             Debug.print(F("."));
//             if (i%32==1) {
//                 Debug.println(F(""));
//             }
//         }
//         Debug.println(F("extEEPROM clear first 2k ... *done*"));
    }


#else // use SAMD internal flash storage with EEPROM emulation

    // let's have 2k of emulated EEPROM
    #define EEPROM_EMULATION_SIZE 4096

    // see: https://github.com/cmaglie/FlashStorage >0.7.0
    #include <FlashAsEEPROM.h>

    void setupMemory() { } // nothing to do when using SAMD Flash "EEPROM" 
    byte readMemory(int index) { return EEPROM.read(index); }
    void writeMemory(int index, byte val) { EEPROM.write(index, val); }
    void updateMemory(int index, byte val) { EEPROM.write(index, val); }
    void commitMemory() { 
      Debug.println(F("Doing commit ... %i"), EEPROM.length());
      EEPROM.commit(); 
      }

#endif

// ==========================================================
//   S P I   F L A S H   for storing files
// ==========================================================

#include <KonnektingDevice.h>
#include <SPI.h>
#include <SerialFlash.h>  // https://github.com/PaulStoffregen/SerialFlash

#define SPIFLASH_CSPIN 9

// reference to current selected file
SerialFlashFile _currFile;

/**
 * Formats the flash memory, must in code just before setupMemory(), otherwise function will not be found/known
 */
void formatFlash() {
    
    uint8_t id[5];
    SerialFlash.readID(id);
    SerialFlash.eraseAll();

    while (!SerialFlash.ready()) {
        Debug.println(F("SerialFlash Format in progress ..."));
        delay(100);
    }
    Debug.println(F("SerialFlash Format *DONE*"));
}

/**
 * setup the flash memory
 */
void setupFlash() {
    if (!SerialFlash.begin(SPIFLASH_CSPIN)) {
        Debug.println(F("SerialFlash.begin() failed: Unable to access SPI Flash chip"));
        while (1) {
        };
    }
    Debug.println(F("SPI Flash initialized"));
    //formatFlash(); // --> use KONNEKTING unload-feature to clear spi flash
}

/**
 * For debugging purpose: List all files in SPI Flash
 */
void listFiles() {

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
    
}

/**
 * Open file for writing
 * @param type
 * @param id
 * @param size size of file 
 */
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
        // _currFile = SerialFlash.open(filename);
        // unsigned long currsize = _currFile.size();
        // _currFile.close();
        // if (size != currsize) {
        //     Debug.println(F("dataOpenWrite(): can't overwrite file with size=%ld with different size=%ld"), currsize, size);
        //     return false;
        // }

        Debug.println(F("dataOpenWrite(): removing existing filename=%s"), filename);
        SerialFlash.remove(filename);
    }
    //if (SerialFlash.createErasable(filename, size)) {
        if (SerialFlash.create(filename, size)) {
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

/**
 * write data to opened file
 * @param data byte[] to write
 * @param length number of bytes to write. Number must not be greater that array size
 * @return false if file is available for write; true if data has been written
 */
bool dataWrite(byte data[], int length) {
    Debug.println(F("dataWrite() length=%d"), length);
    if (!_currFile) {
        return false;
    } 
    _currFile.write(data, length);
    return true;
}



/**
 * open file for reading
 * @param type
 * @param id
 * @return file size
 */
unsigned long dataOpenRead(byte type, byte id) {

    // just for debugging
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

/**
 * read data from opened file
 * @param data byte[] to read data into
 * @param length number of bytes to read
 * 
 */
bool dataRead(byte data[], int length) {
    if (_currFile.available() > 0) {
        _currFile.read(data, length);
    } else {
        Debug.println(F("dataRead(): nothing available"));
        return false;
    }
    return true;
}

/**
 * Close current opened file
 */
bool dataClose() {
    if (_currFile != NULL) {
        _currFile.close();
        return true;
    } else {
        return false;
    }
}

/**
 * Removes a file from storage
 * @param type 
 * @param id
 * @return true if succeeded, false if remove is not allowed or does not exist
 */
bool dataRemove(byte type, byte id) {
    char filename[10];

    if (type == DATA_TYPE_ID_UPDATE) {
        Debug.println(F("dataRemove(): not allowed to remove update: type=%i id=%i"), type, id);
        return false;
    }
    
    sprintf(filename, "FI_%03d_%03d", type, id);
    
    //Debug.println(F("dataRemove(): filename=%s"), filename);

    if (SerialFlash.exists(filename)) {

        //Debug.println(F("dataRemove(): removing existing filename=%s"), filename);
        SerialFlash.remove(filename);
        return true;
    } else {
        //Debug.println(F("dataRemove(): file does not exist filename=%s"), filename);
        return false;
    }
}
