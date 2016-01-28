#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H

#include <project.h>

typedef uint8 (*DMA_Init)(uint8,uint8,uint16,uint16);
void `$INSTANCE_NAME`_Start(volatile uint8 *buffer,uint16 length,DMA_Init DMA);
extern reg16 *`$INSTANCE_NAME`_DMA_BytesLeft;

#endif
