#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H

#include <cyfitter.h>

#define `$INSTANCE_NAME`_SetThresholds(u,v) {\
	*(reg8*)`$INSTANCE_NAME`_dp__D0_REG=u;\
	*(reg8*)`$INSTANCE_NAME`_dp__D1_REG=v;}

#define `$INSTANCE_NAME`_Start() {*(reg8*)`$INSTANCE_NAME`_dp__DP_AUX_CTL_REG|=1;}

#endif
