#ifndef NVM_H
#define NVM_H

typedef struct
{
    int offset;
    int size;
    int writeOnce;
    int secondCopy;
} NvM_Block;


extern const NvM_Block BlockA;
extern const NvM_Block BlockB;
extern const NvM_Block BlockC;

extern unsigned char NvM_DataA[4];
extern unsigned char NvM_DataB[10];
extern unsigned char NvM_DataC[20];

int  NvM_WriteBlock(const NvM_Block *block, unsigned char data[]);
int  NvM_ReadBlock(const NvM_Block *block, unsigned char data[]);
void NvM_ReadAll(void);
int  NvM_EraseAll(void);

#endif
