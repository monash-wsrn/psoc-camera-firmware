#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H

#include<cytypes.h>

typedef uint8 (*DMA_Init)(uint8,uint8,uint16,uint16);
void `$INSTANCE_NAME`_Start(uint8 *buffer,uint16 length,DMA_Init DMA_In,DMA_Init DMA_Out);

#endif
