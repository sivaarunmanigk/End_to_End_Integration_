#include "sdk_project_config.h"
#include "NvM.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 128u

static void UART_Print(const char *message){
    if (LPUART_DRV_SendData(INST_LPUART_1, (uint8_t *)message,(uint32_t)strlen(message)) == STATUS_SUCCESS){
        while (LPUART_DRV_GetTransmitStatus(INST_LPUART_1, NULL) == STATUS_BUSY){
        }
    }
}

static void UART_PrintBlock(const char *name, const unsigned char data[], int size){
    char text[BUFFER_SIZE];
    int i;

    sprintf(text, "\r\n%s (%d bytes):\r\n", name, size);
    UART_Print(text);

    for (i = 0; i < size; i++){
        sprintf(text, "%02X ", data[i]);
        UART_Print(text);

        if ((i + 1) % 10 == 0)
            UART_Print("\r\n");
    }
    UART_Print("\r\n");
}

static void PrintAllBlocks(void){
    UART_PrintBlock("Block A", NvM_DataA, sizeof(NvM_DataA));
    UART_PrintBlock("Block B", NvM_DataB, sizeof(NvM_DataB));
    UART_PrintBlock("Block C", NvM_DataC, sizeof(NvM_DataC));
}

static void UART_ReadLine(char *buffer, uint32_t maxLength){
    uint32_t length = 0;
    uint8_t ch;
    char echo[2] = {0, 0};

    while (1){
        if (LPUART_DRV_ReceiveData(INST_LPUART_1, &ch, 1) != STATUS_SUCCESS){
            OSIF_TimeDelay(1);
            continue;
        }
        while (LPUART_DRV_GetReceiveStatus(INST_LPUART_1, NULL) == STATUS_BUSY){

        }

        if (ch == '\n'){

        }
        else if (ch == '\r'){
            buffer[length] = '\0';
            UART_Print("\r\n");
            return;
        }
        else if ((ch == '\b') || (ch == 127)){
            if (length > 0){
                length--;
                UART_Print("\b \b");
            }
        }
        else if (length < maxLength - 1){
            buffer[length++] = (char)ch;
            echo[0] = (char)ch;
            UART_Print(echo);
        }
    }
}

static int SelectBlock(const NvM_Block **block, unsigned char **ram){
    char input[BUFFER_SIZE];

    UART_Print("\r\nSelect block: 1=A, 2=B, 3=C: ");
    UART_ReadLine(input, sizeof(input));

    if (input[0] == '1'){
        *block = &BlockA;
        *ram = NvM_DataA;
    }
    else if (input[0] == '2'){
        *block = &BlockB;
        *ram = NvM_DataB;
    }
    else if (input[0] == '3'){
        *block = &BlockC;
        *ram = NvM_DataC;
    }
    else{
        return 0;
    }
    return 1;
}

static void ReadBlockMenu(void){
    const NvM_Block *block;
    unsigned char *ram;

    if (!SelectBlock(&block, &ram))
        return;

    if (NvM_ReadBlock(block, ram) == 0){
        UART_PrintBlock("Block data", ram, block->size);
    }
}

static void WriteBlockMenu(void){
    const NvM_Block *block;
    unsigned char *ram;
    char input[BUFFER_SIZE];
    char text[BUFFER_SIZE];
    char *pos;
    int i;

    if (!SelectBlock(&block, &ram))
        return;

    sprintf(text, "Enter %d hexadecimal bytes separated by spaces:\r\n",block->size);
    UART_Print(text);
    UART_ReadLine(input, sizeof(input));

    pos = input;
    for (i = 0; i < block->size; i++){
        char *next;
        ram[i] = (unsigned char)strtoul(pos, &next, 16);

        if (next == pos)
            return;
        pos = next;
    }

    if (NvM_WriteBlock(block, ram) == -3){
        UART_Print("Block A is write-locked.\r\n");
        return;
    }

    if (NvM_ReadBlock(block, ram) == 0){
        UART_PrintBlock("Verified data", ram, block->size);
    }
}

static void ReadAllMenu(void){
    NvM_ReadAll();
    PrintAllBlocks();
}

static void EraseAllMenu(void){
    NvM_EraseAll();
    NvM_ReadAll();
    PrintAllBlocks();
}

int main(void){
    char command[BUFFER_SIZE];
    CLOCK_DRV_Init(&clockMan1_InitConfig0);
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);
    LPUART_DRV_Init(INST_LPUART_1, &lpUartState0, &lpuart_0_InitConfig0);
    OSIF_TimeDelay(100);
    I2C_MasterInit(&lpi2c0_instance, &i2c_pal_lpi2c0_MasterConfig0);
    NvM_ReadAll();

    while (1){
        UART_Print("\r\nNvM MANAGER \r\n");
        UART_Print("1. Write Block\r\n");
        UART_Print("2. Read Block\r\n");
        UART_Print("3. Read All\r\n");
        UART_Print("4. Erase All\r\n");
        UART_Print("Enter choice  ");
        UART_ReadLine(command, sizeof(command));

        switch (command[0]){
            case '1': WriteBlockMenu(); break;
            case '2': ReadBlockMenu();  break;
            case '3': ReadAllMenu();    break;
            case '4': EraseAllMenu();   break;
        }
    }
}
