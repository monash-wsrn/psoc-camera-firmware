extern "C"
{
	#include <project.h>
	#include "wireless.h"
	#include "version.h"
	volatile uint32 time;
	extern void overclock(uint8);
}
#include "RF24.h"

RF24 nrf(0,0);

__attribute__((section(".ram")))
static void SysTick_isr()
{
	time++;
}

static point points[256];
static volatile int16 n_points=-(int16)(sizeof(points)/sizeof(point));
static volatile uint32 timestamp;

static uint8 sqrt16(uint32 n) //integer sqrt for uint16
{  
    uint32 c=0x80;
    uint32 g=0x80;
    for(;;)
	{
        if(g*g>n) g^=c;  
        c>>=1;
        if(!c) return g;  
        g|=c;
    }
}

volatile uint8 persist __attribute__((section(".noinit")));
bool unicast_address_set=false;
__attribute__((section(".ram")))
static void nrf_isr()
{
	uint8 pipe;
	while(nrf.available(&pipe))
	{
		uint8 n=nrf.getDynamicPayloadSize();
		
		uint8 packet[32];
		nrf.read(packet,n);
		if(!n) return;
	
		if(pipe==1) //multicast packet
		{
			LED4_Write(0);
			switch(packet[0])
			{
				case 0xb0: //Neighbour discovery request (only if no unicast address set)
					if(unicast_address_set) break;
				case 0xb1: //Neighbour discovery request
					{
						uint32 delay=((uint8*)CYDEV_FLSHID_CUST_TABLES_BASE)[packet[1]];
						CyDelayUs(50*delay); //delay based on byte in serial number
						//TODO is it better to have some PRNG seeded with the serial number to set the delay? it's easy to set one up in hardware (could even use a true RNG using 2 analogue blocks -- probably overkill)
					}
					memcpy(packet+1,(uint8*)CYDEV_FLSHID_CUST_TABLES_BASE,7); //reply with serial number
					nrf.stopListening();
					nrf.startWrite(packet,8,true);
					CyDelayUs(200);
					nrf.startListening();
					break;
				case 0xb2: //Set unicast address
					if(!memcmp(packet+1,(uint8*)CYDEV_FLSHID_CUST_TABLES_BASE,7)) //if serial number matches
						unicast_address_set=true,nrf.openReadingPipe(2,packet+8);
					break;
				case 0xb3: //Forget unicast address
					unicast_address_set=false,nrf.closeReadingPipe(2);
					break;
			}
			LED4_Write(1);
			continue;
		}
		
		LED3_Write(0);
		switch(packet[0])
		{
			case 0x00: //Dummy packet to get pending ACKs
				break;
			case 0x01: //Serial number and device type
				memcpy(packet+1,(uint8*)CYDEV_FLSHID_CUST_TABLES_BASE,7); //send 7 bytes of ID (location of die, wafer number, lot number, week/year of manufacturer)
				packet[8]=1; //device type: 1 means Glomerida camera
				nrf.writeAckPayload(2,packet,9);
				break;
			case 0x02: //Firmware version
				{
					char version[]=GIT_VERSION;
					uint32 len=sizeof(version)-1;
					if(len>30) len=30;
					memcpy(packet+1,version,len);
					nrf.writeAckPayload(2,packet,len+1);
				}
				break;
			case 0x03: //JTAG ID
				*(uint32*)(packet+1)=*(reg32*)CYDEV_PANTHER_DEVICE_ID;
				nrf.writeAckPayload(2,packet,5);
				break;
			
			case 0x90: //Camera blobs
				{
					//TODO optional flush command to move on to next frame even if we haven't finished sending this one
					struct packed_point
					{
						uint16 x:11;
						uint16 y:11;
						uint8 colour:2;
						uint8 size:8;
					}__attribute__((packed));
					
					static int16 points_sent=0;
					*(uint32*)(packet+1)=timestamp; //identifies frame and gives approximate frame timing
					uint32 i;
					for(i=0;i<6&&points_sent<n_points;i++,points_sent++)
					{
						point p=points[points_sent];
						packed_point pp;
						pp.x=p.x;
						pp.y=p.y;
						pp.colour=p.colour;
						pp.size=sqrt16(p.size);
						((packed_point*)(packet+5))[i]=pp;
					}
					if(points_sent>=n_points)
					{
						points_sent=0;
						n_points=-(int16)(sizeof(points)/sizeof(point));
					}
					nrf.writeAckPayload(2,packet,5+i*sizeof(packed_point));
				}
				break;
			case 0x91: //Write camera register
				Camera_WriteReg(packet[1],packet[2]);
				break;
			case 0x92: //Read camera register
				//TODO implement Camera_ReadReg function in Camera.c
				//packet[1]=Camera_ReadReg(packet[1]);
				nrf.writeAckPayload(2,packet,2);
				break;
			case 0x93: //Set thresholds
				Camera_SetThresholds(packet[1],packet[2],packet[3],packet[4],packet[5],packet[6],packet[7],packet[8]);
				break;
			case 0x94: //Overclock 
				overclock(packet[1]);
				break;
				
			case 0xb3: //Forget unicast address
				unicast_address_set=false,nrf.closeReadingPipe(2);
				break;
			case 0xff: //Reset to bootloader
				persist=2; //TODO implement wireless bootloader
			case 0xfe: //Reset
				CySoftwareReset();
		}
		LED3_Write(1);
	}
}

void Wireless_Start()
{
	Camera_Start(points,&n_points,&timestamp,&time);
	
	CyIntSetSysVector(SysTick_IRQn+16,SysTick_isr);
	SysTick_Config(BCLK__BUS_CLK__HZ/1000); //1ms SysTick
	CyIntSetPriority(SysTick_IRQn+16,0);
	
	uint8 multicast_address[]={0xff,0x21,0xc8};
	
	nrf.begin();
	nrf.setAddressWidth(3);
	nrf.setRetries(1,15);
	nrf.setChannel(0);
	nrf.enableAckPayload();
	nrf.enableDynamicPayloads();
	nrf.enableDynamicAck();
	nrf.setDataRate(RF24_2MBPS);
	nrf.setCRCLength(RF24_CRC_16);
	nrf.maskIRQ(true,true,false);
	
	nrf.openReadingPipe(1,multicast_address);
	nrf.setAutoAck(1,false); //do not ACK on multicast pipe
	multicast_address[0]=0x00; //address to reply to multicast packets
	nrf.openWritingPipe(multicast_address);
	
	{ //TODO this is for testing, remove (or maybe store last unicast address in EEPROM)
		uint8 rx_address=0x19; //unicast
		unicast_address_set=true,nrf.openReadingPipe(2,&rx_address);
	}
	
	nrf.startListening();
		
	nrf_packet_ClearPending();
	nrf_packet_StartEx(nrf_isr);
}

void Wireless_Sleep()
{
	nrf.powerDown();
}