#include "`$INSTANCE_NAME`.h"
#include <project.h>
void `$INSTANCE_NAME`_SetThresholds(uint8 ru,uint8 rv,uint8 gu,uint8 gv,uint8 bu,uint8 bv,uint8 mu,uint8 mv)
{
	`$INSTANCE_NAME`_Red_SetThresholds(ru,rv);
	`$INSTANCE_NAME`_Green_SetThresholds(gu,gv);
	`$INSTANCE_NAME`_Blue_SetThresholds(bu,bv);
	`$INSTANCE_NAME`_Magenta_SetThresholds(mu,mv);
}

void `$INSTANCE_NAME`_WriteReg(const uint8 reg,const uint8 value)
{
	#if `$ff_i2c`
		while(`$INSTANCE_NAME`_I2C_MasterSendStart(0x30,0)!=`$INSTANCE_NAME`_I2C_MSTR_NO_ERROR);
	    `$INSTANCE_NAME`_I2C_MasterWriteByte(reg);
	    `$INSTANCE_NAME`_I2C_MasterWriteByte(value);
	    `$INSTANCE_NAME`_I2C_MasterSendStop();
	#else
		while(`$INSTANCE_NAME`_I2C_UDB_MasterSendStart(0x30,0)!=`$INSTANCE_NAME`_I2C_UDB_MSTR_NO_ERROR);
	    `$INSTANCE_NAME`_I2C_UDB_MasterWriteByte(reg);
	    `$INSTANCE_NAME`_I2C_UDB_MasterWriteByte(value);
	    `$INSTANCE_NAME`_I2C_UDB_MasterSendStop();
	#endif
}

void `$INSTANCE_NAME`_WriteRegs(const uint8 regs[][2])
{
	uint16 i;
	for(i=0;regs[i][0]!=0xff;i++) `$INSTANCE_NAME`_WriteReg(regs[i][0],regs[i][1]);
}

typedef struct blob
{
    uint32 M00; //0th moment (number of pixels in blob)
    uint32 M10,M01; //1st moments (sum of x/y values of every pixel)
	#if `$blob_second_moments`
	    uint32 M20,M11,M02; //2nd moments (sum of x^2/x*y/y^2 values of every pixel)
	#endif
	uint16 C[4]; //number of pixels of each colour
} blob;

static blob blobs[`$max_blobs`];

//DMA arrays should be in bitband region (so that CPU/DMA access different memory banks)
__attribute__((section(".bitband")))
static volatile uint16 run_start[`$run_buffer_size`]; //buffer for rising edge locations
__attribute__((section(".bitband")))
static volatile uint16 run_end[`$run_buffer_size`]; //buffer for falling edge locations
__attribute__((section(".bitband")))
static volatile uint8 run_colours[`$run_buffer_size`]; //buffer for run colours

static volatile uint16 row_index[`$max_rows`]={[0 ... `$max_rows`-1]=0}; //index into buffers for every row
static volatile uint16 rows_ready=0;
static uint16 current_row=0;
static volatile int16 last_row=-1;

static point *points;
static volatile int16 *n_points;
static volatile uint32 *timestamp;
static volatile uint32 *time;

#if `$isr_in_ram`
	__attribute__((section(".ram")))
#endif
static void row_isr()
{
	uint16 rise=*`$INSTANCE_NAME`_Counter_DualCapture_DMA_Rise_BytesLeft; //get position of DMA in buffer
	uint16 fall=*`$INSTANCE_NAME`_Counter_DualCapture_DMA_Fall_BytesLeft;
	uint16 colours=*`$INSTANCE_NAME`_ColourLatch_DMA_BytesLeft;
	
	if(rise!=fall||rise/2!=colours) //very bad! this should only happen if isr runs longer than horizontal blanking
	{
		//TODO something useful (error recovery)
		CyDelay(1000);
		CySoftwareReset();
		CyHalt(0xc6);
	}
	
	row_index[current_row++]=(`$run_buffer_size`-rise/2)%`$run_buffer_size`;
	current_row%=`$max_rows`;
	
	if(++rows_ready>=`$max_rows`) rows_ready-=`$max_rows`;
}

#if `$isr_in_ram`
	__attribute__((section(".ram")))
#endif
static void frame_isr()
{
	if(*n_points<0) *timestamp=*time;
	last_row=current_row;
	`$INSTANCE_NAME`_process_row_SetPending();
}

static void process_frame()
{
	uint16 i;
	if(*n_points<0)
	{
		uint16 m=0;
		for(i=0;i<`$max_blobs`;i++) if(blobs[i].M00)
		{
			uint32 n=blobs[i].M00;
			blobs[i].M00=0;
			
			//calculate centroid of blob with 1/2 x 1/2 pixel resolution
			uint32 x=blobs[i].M10*2/n;
			uint32 y=blobs[i].M01*2/n;
			
			uint16 *C=blobs[i].C;
			uint16 *max=C;
			for(;C!=blobs[i].C+4;C++) if(*C>*max) max=C;
			uint8 colour=max-blobs[i].C;
			
			points[m].x=x;
			points[m].y=y;
			points[m].size=n;
			points[m].colour=colour;
			m++;
			if(m==-*n_points) break;
		}
		for(;i<`$max_blobs`;i++) blobs[i].M00=0;
		*n_points=m;
	}
	else for(i=0;i<`$max_blobs`;i++) blobs[i].M00=0;
}

static void blob_add(blob *dest,const blob *src) //merges two blobs together
{
    dest->M00+=src->M00;
    dest->M10+=src->M10;
    dest->M01+=src->M01;
	#if `$blob_second_moments`
		dest->M20+=src->M20;
	    dest->M11+=src->M11;
	    dest->M02+=src->M02;
	#endif
	uint8 i;
	for(i=0;i<4;i++) dest->C[i]+=src->C[i];
}

static uint8 next_blob() //finds next available blob index
{
	static uint8 next=0;
	uint8 old=next++;
	next%=`$max_blobs`;
	while(blobs[next].M00&&next!=old) next++,next%=`$max_blobs`;
	return next;
}

#if `$isr_in_ram`
	__attribute__((section(".ram")))
#endif
static void process_runs()
{
	static uint16 A=0; //current row index
	static uint16 y=0; //current row being processed
	
	if(rows_ready==0&&A==last_row)
	{
		last_row=-1;
		y=0;
		process_frame();
	}
	
	while(rows_ready>0)
	{
		if(A==last_row)
		{
			last_row=-1;
			y=0;
			process_frame();
		}
		
		static uint8 labels[`$run_buffer_size`]; //component label for each run
		
		uint16 i_max=row_index[A]; //end of current row
		static uint16 j_max=0; //end of previous row/start of current row
		static uint16 j_min=0; //start of previous row
		#if `$merge_blobs_across_row`
			static uint16 k_min=0; //start of two rows ago
			uint16 k=k_min;
		#endif
		
		uint16 j=j_min;
		uint16 i=j_max;
		
		for(;i!=i_max;i++,i%=`$run_buffer_size`) //for every run in current row
		{
			uint16 start=run_start[i];
			uint16 end=run_end[i];
			uint16 colour=run_colours[i];
			
			blob c; //calculate blob parameters for this run
	        c.M00=2*(end-start);
	        c.M10=c.M00*(start+end-1);
	        c.M01=y*c.M00;
			#if `$blob_second_moments`
		        c.M20=c.M00*(4*(start*start+start*end+end*end)-6*(start+end)+2)/3;
		        c.M11=y*c.M10
		        c.M02=y*c.M01;
			#endif
			
			uint8 z;
			for(z=0;z<4;z++)
				if(colour&(1<<z)) c.C[z]=c.M00;
				else c.C[z]=0;
			
			if(y==0)
			{
				blobs[labels[i]=next_blob()]=c; //create new blob
				continue;
			}
			
			for(;j!=j_max;j++,j%=`$run_buffer_size`) if(start<=run_end[j]) break; //find first run in previous row that overlaps with this run
			if(j!=j_max&&end>=run_start[j]) //if there is some overlap
			{
				blob *b=blobs+(labels[i]=labels[j]); //copy label to current run
	            blob_add(b,&c); //add blob information
				#if `$merge_blobs`
					if(end>run_end[j])
					{
						j++,j%=`$run_buffer_size`;
						for(;end>=run_start[j]&&j!=j_max;j++,j%=`$run_buffer_size`) //find all runs in previous row that overlap with this run
						{
							if(labels[i]!=labels[j])
							{
								blob *b2=blobs+labels[j]; //blob to merge
								blob_add(blobs+labels[i],b2);
								b2->M00=0; //clear old blob
								
								uint8 l=labels[j];
								#if `$merge_blobs_across_row`
									uint16 jj=k;
								#else
									uint16 jj=j;
								#endif
								for(;jj!=i;jj++,jj%=`$run_buffer_size`) if(labels[jj]==l) labels[jj]=labels[i]; //relabel all runs with old label
							}
							if(end<=run_end[j]) break; //ensure overlapping run is processed in next iteration of current row
						}
					}
				#endif
			}
			else blobs[labels[i]=next_blob()]=c; //create new blob
			
			#if `$merge_blobs_across_row`
				if(y==1) continue; //prevent merging from previous frame
				for(;k!=j_min;k++,k%=`$run_buffer_size`) if(start<=run_end[k]) break;
				for(;end>=run_start[k]&&k!=j_min;k++,k%=`$run_buffer_size`)
				{
					if(labels[i]!=labels[k])
					{	
						blob *b2=blobs+labels[k]; //blob to merge
						blob_add(blobs+labels[i],b2);
						b2->M00=0; //clear old blob
						
						uint8 l=labels[k];
						uint16 kk=k;
						for(;kk!=i;kk++,kk%=`$run_buffer_size`) if(labels[kk]==l) labels[kk]=labels[i]; //relabel all runs with old label
					}
					if(end<=run_end[k]) break; //ensure overlapping run is processed in next iteration of current row
				}
			#endif
		}
		
		//increment row indices
		#if `$merge_blobs_across_row`
			k_min=j_min;
		#endif
		j_min=j_max;
		j_max=i_max;
		A++,A%=`$max_rows`;
		y++;
				
		uint8 x=CyEnterCriticalSection();
		rows_ready--; //safely decrement shared variable
		CyExitCriticalSection(x);
	}
}

void `$INSTANCE_NAME`_Init(const uint8 settings[][2])
{
	`$INSTANCE_NAME`_SIOD_SetDriveMode(`$INSTANCE_NAME`_SIOD_DM_RES_UP);
	`$INSTANCE_NAME`_SIOC_SetDriveMode(`$INSTANCE_NAME`_SIOC_DM_RES_UP);
	#if `$ff_i2c`
		`$INSTANCE_NAME`_I2C_Start();
	#else
		`$INSTANCE_NAME`_I2C_UDB_Start();
	#endif
	`$INSTANCE_NAME`_WriteReg(0x12,0x80); //reset OV9650
	CyDelay(1);
	`$INSTANCE_NAME`_WriteRegs(settings);
}

void `$INSTANCE_NAME`_Start(point *points_,volatile int16 *n_points_,volatile uint32 *timestamp_,volatile uint32 *time_)
{
	points=points_;
	n_points=n_points_;
	timestamp=timestamp_;
	time=time_;
	
	`$INSTANCE_NAME`_Red_Start();
	`$INSTANCE_NAME`_Green_Start();
	`$INSTANCE_NAME`_Blue_Start();
	`$INSTANCE_NAME`_Magenta_Start();
	
	while(`$INSTANCE_NAME`_VSYNC_Read());
	while(!`$INSTANCE_NAME`_VSYNC_Read()); //sync to rising edge of vsync
	
	`$INSTANCE_NAME`_ColourLatch_Start(run_colours,sizeof run_colours,`$INSTANCE_NAME`_DMA_Colours_DmaInitialize);
	`$INSTANCE_NAME`_Counter_DualCapture_Start(run_start,run_end,sizeof run_start,`$INSTANCE_NAME`_DMA_Rise_DmaInitialize,`$INSTANCE_NAME`_DMA_Fall_DmaInitialize);
	
	`$INSTANCE_NAME`_end_row_ClearPending();
	`$INSTANCE_NAME`_end_row_StartEx(row_isr);
	`$INSTANCE_NAME`_end_row_SetPriority(`$interrupt_priority_high`);
	`$INSTANCE_NAME`_process_row_ClearPending();
	`$INSTANCE_NAME`_process_row_StartEx(process_runs);
	`$INSTANCE_NAME`_process_row_SetPriority(`$interrupt_priority_low`);
	`$INSTANCE_NAME`_frame_ClearPending();
	`$INSTANCE_NAME`_frame_StartEx(frame_isr);
	`$INSTANCE_NAME`_frame_SetPriority(`$interrupt_priority_high`);
}