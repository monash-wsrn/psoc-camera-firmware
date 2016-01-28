#include "`$INSTANCE_NAME`.h"

reg16 *`$INSTANCE_NAME`_DMA_Rise_BytesLeft;
reg16 *`$INSTANCE_NAME`_DMA_Fall_BytesLeft;

void `$INSTANCE_NAME`_Start(volatile uint16 *buffer_rise,volatile uint16 *buffer_fall,uint16 length,DMA_Init DMA_Rise,DMA_Init DMA_Fall)
{
	if(length>4095) length=4095;
	
	uint8 rise_ch=DMA_Rise(2,1,HI16(CYDEV_PERIPH_BASE),HI16(CYDEV_SRAM_BASE));
    uint8 td=CyDmaTdAllocate();
	CyDmaTdSetConfiguration(td,length,td,TD_INC_DST_ADR);
	CyDmaTdSetAddress(td,LO16(`$INSTANCE_NAME`_dp_u0__16BIT_F0_REG),LO16((uint32)buffer_rise));
	CyDmaChSetInitialTd(rise_ch,td);
	
	uint8 fall_ch=DMA_Fall(2,1,HI16(CYDEV_PERIPH_BASE),HI16(CYDEV_SRAM_BASE));
    td=CyDmaTdAllocate();
	CyDmaTdSetConfiguration(td,length,td,TD_INC_DST_ADR);
	CyDmaTdSetAddress(td,LO16(`$INSTANCE_NAME`_dp_u0__16BIT_F1_REG),LO16((uint32)buffer_fall));
	CyDmaChSetInitialTd(fall_ch,td);
	
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F0_REG;
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F0_REG;
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F0_REG;
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F0_REG; //ensure F0 is empty
	
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F1_REG;
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F1_REG;
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F1_REG;
	*(reg16*)`$INSTANCE_NAME`_dp_u0__16BIT_F1_REG; //ensure F1 is empty
	
	`$INSTANCE_NAME`_DMA_Rise_BytesLeft=(reg16 *)CY_DMA_TDMEM_STRUCT_PTR[rise_ch].TD0;
	`$INSTANCE_NAME`_DMA_Fall_BytesLeft=(reg16 *)CY_DMA_TDMEM_STRUCT_PTR[fall_ch].TD0;
	
	CyDmaChEnable(rise_ch,1);
	CyDmaChEnable(fall_ch,1);
}