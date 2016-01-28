#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H

#include <project.h>

typedef uint8 (*DMA_Init)(uint8,uint8,uint16,uint16);
void `$INSTANCE_NAME`_Start(volatile uint16 *buffer_rise,volatile uint16 *buffer_fall,uint16 length,DMA_Init DMA_Rise,DMA_Init DMA_Fall);
extern reg16 *`$INSTANCE_NAME`_DMA_Rise_BytesLeft;
extern reg16 *`$INSTANCE_NAME`_DMA_Fall_BytesLeft;

#endif
