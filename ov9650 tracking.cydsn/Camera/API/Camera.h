#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H

#include<cytypes.h>

void `$INSTANCE_NAME`_WriteReg(const uint8 reg,const uint8 value);
void `$INSTANCE_NAME`_WriteRegs(const uint8 regs[][2]);
void `$INSTANCE_NAME`_SetThresholds(uint8 ru,uint8 rv,uint8 gu,uint8 gv,uint8 bu,uint8 bv,uint8 mu,uint8 mv);

typedef struct point
{
	uint16 x,y,size,colour;
} point;

void `$INSTANCE_NAME`_Init(const uint8 settings[][2]);
void `$INSTANCE_NAME`_Start(point *points_,volatile int16 *n_points_,volatile uint32 *timestamp_,volatile uint32 *time_);

#endif
