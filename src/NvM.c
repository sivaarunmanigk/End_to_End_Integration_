#include "sdk_project_config.h"
#include "NvM.h"
#include <string.h>

#define EEPROM_ADDRESS   0x50
#define EEPROM_SIZE      512
#define PAGE_SIZE        8
#define MAX_COPY_SIZE    32

#define LOCK_ADDRESS     5
#define LOCK_VALUE       0xA5

const NvM_Block BlockA = {0,  4, 1, -1};
const NvM_Block BlockB = {8, 10, 0, 24};
const NvM_Block BlockC = {40, 20, 0, -1};

unsigned char NvM_DataA[4];
unsigned char NvM_DataB[10];
unsigned char NvM_DataC[20];

static unsigned char CRC_Calculate(const unsigned char data[], int size){
    unsigned char crc = 0;
    int i, bit;

    for (i = 0; i < size; i++){
        crc ^= data[i];

        for (bit = 0; bit < 8; bit++){
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

static void SelectEepromHalf(int address){
    I2C_MasterSetSlaveAddress(&lpi2c0_instance,EEPROM_ADDRESS | ((address >> 8) & 1), false);
}

static int EepromWrite(int address, const unsigned char data[], int size){
    uint8_t txBuffer[PAGE_SIZE + 1];
    int done = 0;

    while (done < size){
        int current = address + done;
        int count = PAGE_SIZE - (current % PAGE_SIZE);

        if (count > size - done)
            count = size - done;

        txBuffer[0] = (uint8_t)current;
        memcpy(&txBuffer[1], &data[done], count);

        SelectEepromHalf(current);
        if (I2C_MasterSendDataBlocking(&lpi2c0_instance, txBuffer, count + 1,true, 100) != STATUS_SUCCESS)
            return -1;

        OSIF_TimeDelay(5);
        done += count;
    }
    return 0;
}

static int EepromRead(int address, unsigned char data[], int size){
    uint8_t wordAddress = (uint8_t)address;

    SelectEepromHalf(address);

    if (I2C_MasterSendDataBlocking(&lpi2c0_instance, &wordAddress, 1,false, 100) != STATUS_SUCCESS)
        return -1;

    if (I2C_MasterReceiveDataBlocking(&lpi2c0_instance, data, size,true, 100) != STATUS_SUCCESS)
        return -1;

    return 0;
}

static int WriteCopy(int address, const unsigned char data[], int size){
    unsigned char buffer[MAX_COPY_SIZE];

    memcpy(buffer, data, size);
    buffer[size] = CRC_Calculate(data, size);

    return EepromWrite(address, buffer, size + 1);
}

static int ReadCopy(int address, unsigned char data[], int size){
    unsigned char buffer[MAX_COPY_SIZE];

    if (EepromRead(address, buffer, size + 1) != 0)
        return -1;

    memcpy(data, buffer, size);

    if (CRC_Calculate(buffer, size) != buffer[size])
        return -1;

    return 0;
}

int NvM_WriteBlock(const NvM_Block *block, unsigned char data[]){
    unsigned char lock = 0;
    if (block->writeOnce == 1){
        EepromRead(LOCK_ADDRESS, &lock, 1);

        if (lock == LOCK_VALUE)
            return -3;
    }

    if (WriteCopy(block->offset, data, block->size) != 0)
        return -1;

    if (block->secondCopy >= 0){
        if (WriteCopy(block->secondCopy, data, block->size) != 0)
            return -1;
    }

    if (block->writeOnce == 1){
        lock = LOCK_VALUE;
        EepromWrite(LOCK_ADDRESS, &lock, 1);
    }
    return 0;
}

int NvM_ReadBlock(const NvM_Block *block, unsigned char data[]){
    if (ReadCopy(block->offset, data, block->size) == 0)
        return 0;

    if (block->secondCopy >= 0)
        return ReadCopy(block->secondCopy, data, block->size);

    return -1;
}

void NvM_ReadAll(void){
    if (NvM_ReadBlock(&BlockA, NvM_DataA) != 0)
        memset(NvM_DataA, 0, sizeof(NvM_DataA));

    if (NvM_ReadBlock(&BlockB, NvM_DataB) != 0)
        memset(NvM_DataB, 0, sizeof(NvM_DataB));

    if (NvM_ReadBlock(&BlockC, NvM_DataC) != 0)
        memset(NvM_DataC, 0, sizeof(NvM_DataC));
}

int NvM_EraseAll(void){
    unsigned char page[PAGE_SIZE];
    int address;

    memset(page, 0xFF, sizeof(page));

    for (address = 0; address < EEPROM_SIZE; address += PAGE_SIZE){
        if (EepromWrite(address, page, PAGE_SIZE) != 0)
            return -1;
    }
    return 0;
}
