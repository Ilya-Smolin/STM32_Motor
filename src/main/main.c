#include <stdlib.h>
#include "stm32f4xx.h"
#include "stdint.h"

volatile uint8_t STATE = 0;
volatile uint16_t DAC_data = 0;
volatile uint8_t current_byte = 0;

#define BUFF_SIZE  0x8
#define BUF_MASK  (BUFF_SIZE - 1)

uint8_t buffer [BUFF_SIZE];
uint8_t idxIN = 0, idxOUT = 0;

void addElement (uint8_t data)
{
    buffer[idxIN++] = data;
    idxIN &= BUF_MASK;
}

uint8_t getElement (void)
{
    uint8_t data = buffer[idxOUT++];
    idxOUT &= BUF_MASK;
    return data;
}

void send_UART (uint8_t message)
{
	while (!USART_GetFlagStatus(USART2,USART_FLAG_TXE)) {}
		USART_SendData(USART2, message);
}

void clockInit (void)
{
	RCC_DeInit();
	// Set system clock to 168 MHz
	FLASH_SetLatency(FLASH_ACR_LATENCY_5WS); // Flash latency 5
	RCC_HCLKConfig(RCC_SYSCLK_Div1); // HCLK = SYSCLK (168 MHz)
	RCC_PCLK1Config(RCC_HCLK_Div4); // PCLK1 = HCLK/4 (42 MHz)
	RCC_PCLK2Config(RCC_HCLK_Div2); // PCLK2 = HCLK/2 (84 MHz)
	RCC_HSEConfig(RCC_HSE_ON); // Enable HSE crystal (8 MHz)

	//if(RCC_WaitForHSEStartUp() == ERROR) return; // Wait for crystal stabilize
	RCC_PLLConfig(RCC_PLLSource_HSE, 4, 168, 2, 4); // PLL source is HSE
								// PLLM = 12, PLLN = 504 --> FVCO = 8 * 252 / 6 = 336 MHz
								// PLLP = 2 --> FPLL = 336 / 2 = 168 MHz
								// PLLQ = 7 --> FUSB = 336 / 8 = 48 MHz
	RCC_PLLCmd(ENABLE); // Enable the PLL
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // Wait for PLL activation
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); // Set PLL as system clock
	while(RCC_GetSYSCLKSource() != 0x08); // Wait for PLL to be set as system clock
	SystemCoreClockUpdate();
}

void forward ()
{
	GPIO_SetBits (GPIOA, GPIO_Pin_5);
	GPIO_ResetBits (GPIOA, GPIO_Pin_6);
}

void reverse ()
{
	GPIO_SetBits (GPIOA, GPIO_Pin_5 | GPIO_Pin_6);
}

void stop ()
{
	GPIO_ResetBits (GPIOA, GPIO_Pin_5 | GPIO_Pin_6);
}

void DACInit (void)
{
	GPIO_InitTypeDef DACGPIOinit;
	DAC_InitTypeDef DACinit;
	
	 /* GPIOA clock enable (to be used with DAC) */
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	/* DAC Periph clock enable */
	RCC_APB1PeriphClockCmd (RCC_APB1Periph_DAC, ENABLE);
	
	DACGPIOinit.GPIO_Pin = GPIO_Pin_4;
	DACGPIOinit.GPIO_Mode = GPIO_Mode_AN;
	DACGPIOinit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init (GPIOA, &DACGPIOinit);
	
	DACinit.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	DACinit.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DACinit.DAC_Trigger = DAC_Trigger_None;
	DACinit.DAC_WaveGeneration = DAC_WaveGeneration_None;
	
	/*  Function used to set the DAC configuration to the default reset state *****/
	DAC_DeInit();
	
	DAC_Init (DAC_Channel_1, &DACinit);
	DAC_Cmd (DAC_Channel_1, ENABLE);
}

void GPIO_and_UART_init (void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // change port
	
	GPIO_InitTypeDef InitStructure;
	InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
	InitStructure.GPIO_Mode = GPIO_Mode_AF;
	InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	InitStructure.GPIO_OType = GPIO_OType_PP;
	InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOA, &InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART1);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // change UART
	
	USART_DeInit(USART2);
	USART_InitTypeDef initUSART1;
	initUSART1.USART_BaudRate = 9600;
	initUSART1.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	initUSART1.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	initUSART1.USART_Parity = USART_Parity_No;
	initUSART1.USART_StopBits = USART_StopBits_1;
	initUSART1.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &initUSART1);
	USART_Cmd (USART2, ENABLE);
	
	//Pins
	InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	InitStructure.GPIO_OType = GPIO_OType_PP;
	InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_5;
	GPIO_Init(GPIOA, &InitStructure);
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void USART2_IRQHandler ()
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		addElement(USART_ReceiveData(USART2));
	}
}

void send_message (uint8_t state, uint8_t dac)
{
	send_UART (0x65);			
	send_UART (state);
	send_UART (dac);
	send_UART (0x66);
}


int main(void)
{
	__disable_irq();
	clockInit();
	DACInit();
	GPIO_and_UART_init();
	__enable_irq();
	
	
	while (1)
	{
		send_message (STATE, ((uint8_t) ((DAC_data*100)/0xFFF)));
		
		DAC_SetChannel1Data (DAC_Align_12b_R, DAC_data);
		
		if (idxIN != idxOUT)
		{			
			current_byte = getElement();
		
			if (current_byte == 0xFE)			//пришло кодовое слово на STATE
			{	
				STATE = buffer[(idxOUT - 2) & BUF_MASK];
				switch (STATE)
				{
					case 0:
					{
						stop();
						break;
					}
					case 1:
					{	
						forward();
						break;
					}
					case 2:
					{	
						reverse();
						break;
					}
				}	
			}
			
			if (current_byte == 0xFD && STATE != 0) //пришло кодовое слово на изменение скорости и у нас двигательный режим
				DAC_data = (int) ((buffer[(idxOUT - 2) & BUF_MASK] * 0xFFF)/252);
			
			if (current_byte == 0x68 && buffer[(idxOUT - 2) & BUF_MASK] == 0xFF) //пришло сообщение на reset
				NVIC_SystemReset();
		}
	}
}










#ifdef USE_FULL_ASSERT

// эта функция вызывается, если assert_param обнаружил ошибку
void assert_failed(uint8_t * file, uint32_t line)
{ 
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	 
	(void)file;
	(void)line;

	__disable_irq();
	while(1)
	{
			// это ассемблерная инструкция "отладчик, стой тут"
			// если вы попали сюда, значит вы ошиблись в параметрах вызова функции из SPL. 
			// Смотрите в call stack, чтобы найти ее
			__BKPT(0xAB);
	}
}

#endif
