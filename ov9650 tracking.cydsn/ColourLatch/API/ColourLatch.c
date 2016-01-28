#include "`$INSTANCE_NAME`.h"

reg16 *`$INSTANCE_NAME`_DMA_BytesLeft;

void `$INSTANCE_NAME`_Start(volatile uint8 *buffer,uint16 length,DMA_Init DMA)
{
	if(length>4095) length=4095;
	
	uint8 ch=DMA(1,1,HI16(CYDEV_PERIPH_BASE),HI16(CYDEV_SRAM_BASE));
    uint8 td=CyDmaTdAllocate();
	CyDmaTdSetConfiguration(td,length,td,TD_INC_DST_ADR);
	CyDmaTdSetAddress(td,LO16(`$INSTANCE_NAME`_dp__F0_REG),LO16((uint32)buffer));
	CyDmaChSetInitialTd(ch,td);
	
	*(reg8*)`$INSTANCE_NAME`_dp__F0_REG;
	*(reg8*)`$INSTANCE_NAME`_dp__F0_REG;
	*(reg8*)`$INSTANCE_NAME`_dp__F0_REG;
	*(reg8*)`$INSTANCE_NAME`_dp__F0_REG; //ensure F0 is empty
	
	`$INSTANCE_NAME`_DMA_BytesLeft=(reg16 *)CY_DMA_TDMEM_STRUCT_PTR[ch].TD0;
	
	CyDmaChEnable(ch,1);
}