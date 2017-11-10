#include "OV7670_2.h"
#include "ticks.h"
#include "tm_stm32f4_ili9341.h"
volatile uint16_t frame_buffer[CAMERA_ROWS * CAMERA_COLUMNS];

/*
HSYNC / HREF: PA4
VSYNC: PB7
PCLK: PA6
XCLK: PA8
SIOD: PB11
SIOC: PB10
D0: PC6
D1: PC7
D2: PE0
D3: PE1
D4: PE4
D5: PB6
D6: PE5
D7: PE6
*/

/**
  * @brief  Configures the MCO Module
  * @param  None
  * @retval None
  */
void OV7670_MCO_Configuration(void){
  // GPIO PC9
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_MCO);
  
  RCC_PLLI2SCmd(DISABLE);
  
  RCC_PLLI2SConfig(48, 2); // 192..432, 2..7, ie range of 27.429 Mhz to 192 MHz
  /* PLLI2SSCLK = 240 / 5 = 48Mhz */
	/* MCO2 Output Freq = 48 / 2 = 24Mhz*/
  RCC_MCO2Config(RCC_MCO2Source_PLLI2SCLK, RCC_MCO2Div_2); // 24 MHz with default PLL fComp
  
  RCC_PLLI2SCmd(ENABLE);
  
  while(RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY) == RESET);
}

/*
  * @brief  Configures the DCMI to interface with the OV7670 camera module.
  * @param  None
  * @retval None
  */
void OV7670_DCMI_Configuration(void)
{
    
    /*
      DCMI D0    = PC6
            DCMI D1    = PC7
            DCMI D2    = PE0
            DCMI D3    = PE1
      DCMI D4    = PE4
            DCMI D5    = PB6
            DCMI D6    = PE5
            DCMI D7    = PE6
      DCMI HSYNC = PA4
            DCMI VSYNC = PB7
            DCMI XCLK  = PC9
            DCMI PXCLK = PA6
            I2C_SCL    = PB10
            I2C_SDA    = PB11
    */
  DCMI_InitTypeDef DCMI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  DMA_InitTypeDef  DMA_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  // Enable DCMI GPIOs clocks
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE | 
                         RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOA, ENABLE); 

  // Enable DCMI clock
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);

  // Connect DCMI pins to AF13
  
  // PCLK
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_DCMI);
  
  // D0-D7
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_DCMI);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_DCMI);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource0, GPIO_AF_DCMI);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_DCMI);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource4, GPIO_AF_DCMI);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_DCMI);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_DCMI);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource6, GPIO_AF_DCMI);
  // VSYNC
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_DCMI);
  // HSYNC
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_DCMI);
  
  // DCMI GPIO configuration
  // D0 D1(PC6/7)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;  
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // D2..D4(PE0/1/4) D6/D7(PE5/6)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 
	                              | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  // D5(PB6), VSYNC(PB7)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // PCLK(PA6) HSYNC(PA4)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // DCMI configuration
  DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_Continuous;
  DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
  DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
  DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_High;
  DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_High;
  DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
  DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
  DCMI_Init(&DCMI_InitStructure);
  DCMI_Cmd(ENABLE);
  // Configures the DMA2 to transfer Data from DCMI to the LCD
  // Enable DMA2 clock
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);  
  
  DMA_DeInit(DMA2_Stream1);  
  DMA_InitStructure.DMA_Channel = DMA_Channel_1;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&DCMI->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)frame_buffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = CAMERA_ROWS * CAMERA_COLUMNS / 2; 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream1, &DMA_InitStructure);
  DMA_Cmd(DMA2_Stream1, ENABLE);

  // DMA interrupt
  DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);
  NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  DMA_Cmd(DMA2_Stream1, ENABLE); 
}



void Cam_Init(void)
{
  	GPIO_InitTypeDef GPIO_InitStructure;
  	DCMI_InitTypeDef DCMI_InitStructure;
  	DMA_InitTypeDef  DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
  	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);//DCMI 
  	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);//DMA2
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | 
                           RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE, ENABLE);//ʹ��DCMI��GPIOʱ��
  
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_MCO);
  
  RCC_PLLI2SCmd(DISABLE);
  
  RCC_PLLI2SConfig(48, 2); // 192..432, 2..7, ie range of 27.429 Mhz to 192 MHz
  /* PLLI2SSCLK = 240 / 5 = 48Mhz */
	/* MCO2 Output Freq = 48 / 2 = 24Mhz*/
  RCC_MCO2Config(RCC_MCO2Source_PLLI2SCLK, RCC_MCO2Div_2); // 24 MHz with default PLL fComp
  
  RCC_PLLI2SCmd(ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;//PE5:PWRDOWN
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ; 
    GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOE, GPIO_Pin_5);//power on

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_DCMI);//DCMI_HSYNC 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_DCMI);//DCMI_PIXCLK
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_DCMI);//DCMI_D5 			  
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_DCMI);//DCMI_VSYNC 
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_DCMI);//DCMI_D6 
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource6, GPIO_AF_DCMI);//DCMI_D7 			  
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_DCMI);//DCMI_D0 
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_DCMI);//DCMI_D1 			  
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource0, GPIO_AF_DCMI);//DCMI_D2 
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_DCMI);//DCMI_D3 
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource4, GPIO_AF_DCMI);//DCMI_D4 

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ; 
    GPIO_Init(GPIOC, &GPIO_InitStructure); 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6; 
    GPIO_Init(GPIOE, &GPIO_InitStructure);     
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 ; 
    GPIO_Init(GPIOB, &GPIO_InitStructure);   
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);		 


	// DCMI config
	DCMI_DeInit();
  DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_Continuous; 
  DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b; 
  DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
	DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
	DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low; 
  DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_High;	        
  DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;                                                              
  DCMI_Init(&DCMI_InitStructure);
	DCMI_Cmd(ENABLE);

	// DMA config
  DMA_DeInit(DMA2_Stream1);  
	DMA_InitStructure.DMA_Channel = DMA_Channel_1;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&DCMI->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)frame_buffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = CAMERA_ROWS*CAMERA_COLUMNS*2/4; 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream1, &DMA_InitStructure);
	DMA_Cmd(DMA2_Stream1, ENABLE);

	// DMA interrupt
	DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);		

  
}

u8 OV7670_Init(void)
{
  	u8 i;
	SCCB_Init();
    OV7670_MCO_Configuration();
	OV_Reset();
	Delayms(5);
  	for(i=0;i<OV7670_REG_NUM;i++)
  	{
    	if(OV_WriteReg(OV7670_reg[i][0],OV7670_reg[i][1]))return 1;
  	}
    OV7670_DCMI_Configuration();
    // Cam_Init();
	return OV_ReadID(); 
}
/*
void Cam_Start(void)
{
  	LCD_WriteReg(0x03,0x1018);
  	LCD_Cursor(0,319);  
  	DMA_Cmd(DMA2_Stream1, ENABLE); 
  	DCMI_Cmd(ENABLE); 
  	DCMI_CaptureCmd(ENABLE); 
  	LCD_REG=0x22;
}
*/
								  
void OV7670_HW(u16 hstart,u16 vstart,u16 hstop,u16 vstop)
{
	u8 v;		
	OV_WriteReg(0x17,(hstart>>3)&0xff);//HSTART
	OV_WriteReg(0x18,(hstop>>3)&0xff);//HSTOP
	OV_ReadReg(0x32,&v);
	v=(v&0xc0)|((hstop&0x7)<<3)|(hstart&0x7);
	OV_WriteReg(0x32,v);//HREF
	
	OV_WriteReg(0x19,(vstart>>2)&0xff);//VSTART ��ʼ��8λ
	OV_WriteReg(0x1a,(vstop>>2)&0xff);//VSTOP	������8λ
	OV_ReadReg(0x03,&v);
	v=(v&0xf0)|((vstop&0x3)<<2)|(vstart&0x3);	
	OV_WriteReg(0x03,v);//VREF																 
	OV_WriteReg(0x11,0x00);
}								
/////////////////////////////////////
void SCCB_Init(void)
{
  	GPIO_InitTypeDef  GPIO_InitStructure;
  	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;  //SCCB_SIC:PB10
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;			  //SCCB_SID:PB11
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void SCCB_SID_OUT(void)//����SCCB_SIDΪ���
{
  	GPIO_InitTypeDef  GPIO_InitStructure;

  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;               
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;			
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
void SCCB_SID_IN(void)//����SCCB_SIDΪ����
{
  	GPIO_InitTypeDef  GPIO_InitStructure;

  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;               
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;			
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void SCCB_Start(void)
{
    SCCB_SID_H();     //??????
    Delay(50);
    SCCB_SIC_H();	   //???????????????
    Delay(50);
    SCCB_SID_L();
    Delay(50);
    SCCB_SIC_L();	 //????????,???????
    Delay(50);
}

void SCCB_Stop(void)
{
    SCCB_SID_L();
    Delay(50);
    SCCB_SIC_H();	
    Delay(50);  
    SCCB_SID_H();	
    Delay(50);   
}

void noAck(void)
{	
	SCCB_SID_H();	
	Delay(50);	
	SCCB_SIC_H();	
	Delay(50);	
	SCCB_SIC_L();	
	Delay(50);	
	SCCB_SID_L();	
	Delay(50);
}

u8 SCCB_Write(u8 m_data)
{
	u8 j,tem;

	for(j=0;j<8;j++) //??8?????
	{
		if((m_data<<j)&0x80)SCCB_SID_H();
		else SCCB_SID_L();
		Delay(50);
		SCCB_SIC_H();	
		Delay(50);
		SCCB_SIC_L();	
		Delay(50);
	}
	Delay(10);
	SCCB_DATA_IN;
	Delay(50);
	SCCB_SIC_H();	
	Delay(10);
	if(SCCB_SID_STATE)tem=0;//SDA=1????
	else tem=1;//SDA=0????,??1
	SCCB_SIC_L();	
	Delay(50);	
    SCCB_DATA_OUT;
	return tem;    
}

u8 SCCB_Read(void)
{
	u8 read,j;
	read=0x00;
	
	SCCB_DATA_IN;
	Delay(50);
	for(j=8;j>0;j--) //??8?????
	{		     
		Delay(50);
		SCCB_SIC_H();
		Delay(50);
		read=read<<1;
		if(SCCB_SID_STATE)read=read+1; 
		SCCB_SIC_L();
		Delay(50);
	}	
    SCCB_DATA_OUT;
	return read;
}

//дOV7670�Ĵ���
u8 OV_WriteReg(u8 regID, u8 regDat)
{
	SCCB_Start();//??SCCB ????????
	if(SCCB_Write(0x42)==0)//???
	{
		SCCB_Stop();//??SCCB ????????
		return 1;//????
	}
	Delay(10);
  	if(SCCB_Write(regID)==0)//???ID
	{
		SCCB_Stop();//??SCCB ????????
		return 2;//????
	}
	Delay(10);
  	if(SCCB_Write(regDat)==0)//???????
	{
		SCCB_Stop();//??SCCB ????????
		return 3;//????
	}
  	SCCB_Stop();//??SCCB ????????	
  	return 0;//????
}

//��OV7660�Ĵ���
u8 OV_ReadReg(u8 regID, u8 *regDat)
{
	//????????????
	SCCB_Start();
	if(SCCB_Write(0x42)==0)//???
	{
		SCCB_Stop();//??SCCB ????????
		return 1;//????
	}
	Delay(10);
  	if(SCCB_Write(regID)==0)//???ID
	{
		SCCB_Stop();//??SCCB ????????
		return 2;//????
	}
	SCCB_Stop();//??SCCB ????????	
	Delay(10);	
	//????????,???
	SCCB_Start();
	if(SCCB_Write(0x43)==0)//???
	{
		SCCB_Stop();//??SCCB ????????
		return 3;//????
	}
	Delay(10);
  	*regDat=SCCB_Read();//??????
  	noAck();//??NACK??
  	SCCB_Stop();//??SCCB ????????
  	return 0;//????
}

void OV_Reset(void)
{
	OV_WriteReg(0x12,0x80);
}

u8 OV_ReadID(void)
{
	u8 temp;
	OV_ReadReg(0x1c,&temp);
  	return temp;
}
