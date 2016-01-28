#include<cyfitter.h>
#include"CyDmac.h"
#include"`$INSTANCE_NAME`.h"
#include"`$INSTANCE_NAME`_Count7.h"

void `$INSTANCE_NAME`_Start(uint8 *buffer,uint16 length,DMA_Init DMA_In,DMA_Init DMA_Out)
{
	if(length>4095) length=4095;
	
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F0_REG=0;
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F0_REG=0;
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F0_REG=0;
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F0_REG=0; //ensure F0 is full
	
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F1_REG;
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F1_REG;
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F1_REG;
	*(reg8*)`$INSTANCE_NAME`_dp_u0__F1_REG; //ensure F1 is empty
	
	if(DMA_In)
	{
		uint8 in_ch=DMA_In(1,1,HI16(CYDEV_SRAM_BASE),HI16(CYDEV_PERIPH_BASE));
	    uint8 in_td=CyDmaTdAllocate();
		CyDmaTdSetConfiguration(in_td,length,in_td,TD_INC_SRC_ADR);
		CyDmaTdSetAddress(in_td,LO16((uint32)buffer),LO16(`$INSTANCE_NAME`_dp_u0__F0_REG));
		CyDmaChSetInitialTd(in_ch,in_td);
		CyDmaChPriority(in_ch,6); //ensure priority for DMA_In channel is higher
		CyDmaChEnable(in_ch,1);
	}
	
	if(DMA_Out)
	{
		uint8 out_ch=DMA_Out(1,1,HI16(CYDEV_PERIPH_BASE),HI16(CYDEV_SRAM_BASE));
	    uint8 out_td=CyDmaTdAllocate();
		CyDmaTdSetConfiguration(out_td,length,out_td,TD_INC_DST_ADR);
		CyDmaTdSetAddress(out_td,LO16(`$INSTANCE_NAME`_dp_u0__F1_REG),LO16((uint32)buffer));
		CyDmaChSetInitialTd(out_ch,out_td);
		CyDmaChPriority(out_ch,7);
		CyDmaChEnable(out_ch,1);
	}
	
	`$INSTANCE_NAME`_Count7_Start();
}