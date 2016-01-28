#ifndef IDENTIFY_H
#define IDENTIFY_H

#ifdef __cplusplus
	extern "C"
	{
#endif

#include <project.h>
typedef struct eBug
{
	uint16 id;
	uint16 x,y;
	uint16 angle;
	uint16 distance;
} eBug;
#define MAX_EBUGS 10
eBug eBugs[MAX_EBUGS];
uint8 identify(point *points,uint16 n);

#ifdef __cplusplus
	}
#endif

#endif
