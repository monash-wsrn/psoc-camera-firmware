#include <project.h>
#include "wireless.h"

void overclock(uint8 freq) //overclock to freq*1MHz
{
    if(freq>48) CyFlash_SetWaitCycles(freq);
    CyPLL_OUT_SetPQ(freq,3,7);
    if(freq<=48) CyFlash_SetWaitCycles(freq);
    CyDelayFreq(freq*1000000);
}

extern const uint8 ov9650_settings[][2];
extern const uint8 ov9650_format_regs[];
extern const uint8 ov9650_sxga_settings[];
extern const uint8 ov9650_vga_settings[];
extern const uint8 ov9650_qvga_settings[];
extern const uint8 ov9650_cif_settings[];
extern const uint8 ov9650_qcif_settings[];

int main()
{
	LED1_Write(0);
	CyDelay(10);
	LED1_Write(1);
	//overclock(40);
	reload_StartEx(CySoftwareReset);
	reset_StartEx(CySoftwareReset);
	
	CapSense_Start();
	CyDelay(100);
	reset_ClearPending();
	reload_ClearPending();
    
	CyGlobalIntEnable;
	
	Camera_Init(ov9650_settings);
	uint16 i;
	for(i=0;ov9650_format_regs[i]!=0xff;i++) Camera_WriteReg(ov9650_format_regs[i],ov9650_vga_settings[i]);
	
	Wireless_Start();
	Camera_WriteReg(0x11,0x00);
	
	Camera_SetThresholds(0x79,0x9a,0xb1,0x5a,0xc6,0x70,0xa3,0x82);
	
	for(;;);
}
