/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "../../CAN_ID.h"
#include "nodeConf.h"
#include "can.h"
#include "can2.h"
#include "serial.h"
#include "ts_lib.h"
#include "thermistor.h"
#include "nodeMiscHelpers.h"
#include "psb0cal.h"

// RTOS Task functions + helpers
#include "Can_Processor.h"

//MCP3909
#include "mcp3909.h"

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

WWDG_HandleTypeDef hwwdg;

osThreadId ApplicationHandle;
osThreadId Can_ProcessorHandle;
osThreadId RTHandle;
osThreadId TMTHandle;
osThreadId HouseKeepingHandle;
osThreadId PPTPollHandle;
osMessageQId mainCanTxQHandle;
osMessageQId mainCanRxQHandle;
osMessageQId can2TxQHandle;
osMessageQId Can2RxQHandle;
osTimerId WWDGTmrHandle;
osTimerId HBTmrHandle;
osMutexId swMtxHandle;
osSemaphoreId mcp3909_DRHandle;
osSemaphoreId mcp3909_RXHandle;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
MCP3909HandleTypeDef hmcp1;
uint8_t mcpRxBuf[REG_LEN * REGS_NUM];
uint8_t mcpTxBuf[REG_LEN * REGS_NUM + CTRL_LEN];

uint8_t init_Done = 0;

// SW_Sentinel will fail the CC firmware check and result in node addition failure!
//const uint32_t firmwareString = 0x00000001;			// Firmware Version string
//const uint8_t selfNodeID = bps_nodeID;					// The nodeID of this node
//uint32_t selfStatusWord = INIT;							// Initialize
//#define NODE_CONFIGURED
//
//#ifndef NODE_CONFIGURED
//#error "NODE NOT CONFIGURED. GO CONFIGURE IT IN NODECONF.H!"
//#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI2_Init(void);
static void MX_WWDG_Init(void);
static void MX_CAN2_Init(void);
void doApplication(void const * argument);
void doProcessCan(void const * argument);
void doRT(void const * argument);
void doTMT(void const * argument);
void doHouseKeeping(void const * argument);
void doPPTPoll(void const * argument);
void TmrKickDog(void const * argument);
void TmrSendHB(void const * argument);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

// Data Ready pin triggered callback (PA1)
void HAL_GPIO_EXTI_Callback(uint16_t pinNum);
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
void EM_Init();
void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan);
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan);
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */
//#define DISABLE_RT
#define DISABLE_TMT
//#define DISABLE_CAN
	#define DISABLE_SERIAL_OUT
	selfStatusWord = INIT;
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_CAN1_Init();
  MX_SPI2_Init();
  MX_WWDG_Init();
  MX_CAN2_Init();

  /* USER CODE BEGIN 2 */
	__HAL_GPIO_EXTI_CLEAR_IT(DR1_Pin);
	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	__HAL_GPIO_EXTI_CLEAR_IT(DR1_Pin);

	init_Done = 1;

	Serial2_begin();
	Serial2_writeBuf("Booting... \n");

	bxCan_begin(&hcan1, &mainCanRxQHandle, &mainCanTxQHandle);
	bxCan_addMaskedFilterStd(p2pOffset,0xFF0,0);

	bxCan2_begin(&hcan2, &Can2RxQHandle, &can2TxQHandle);
	bxCan2_addMaskedFilterStd(0,0,0);
	bxCan2_addMaskedFilterExt(0,0,0);

	#ifndef DISABLE_TMT
		Temp_begin(&hadc1);
	#endif

	#ifndef DISABLE_RT
		EM_Init();
		HAL_WWDG_Refresh(&hwwdg);
	#endif
  /* USER CODE END 2 */

  /* Create the mutex(es) */
  /* definition and creation of swMtx */
  osMutexDef(swMtx);
  swMtxHandle = osMutexCreate(osMutex(swMtx));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of mcp3909_DR */
  osSemaphoreDef(mcp3909_DR);
  mcp3909_DRHandle = osSemaphoreCreate(osSemaphore(mcp3909_DR), 1);

  /* definition and creation of mcp3909_RX */
  osSemaphoreDef(mcp3909_RX);
  mcp3909_RXHandle = osSemaphoreCreate(osSemaphore(mcp3909_RX), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of WWDGTmr */
  osTimerDef(WWDGTmr, TmrKickDog);
  WWDGTmrHandle = osTimerCreate(osTimer(WWDGTmr), osTimerPeriodic, NULL);

  /* definition and creation of HBTmr */
  osTimerDef(HBTmr, TmrSendHB);
  HBTmrHandle = osTimerCreate(osTimer(HBTmr), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  osTimerStart(WWDGTmrHandle, WD_Interval);
  osTimerStart(HBTmrHandle, HB_Interval);
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of Application */
  osThreadDef(Application, doApplication, osPriorityLow, 0, 512);
  ApplicationHandle = osThreadCreate(osThread(Application), NULL);

  /* definition and creation of Can_Processor */
  osThreadDef(Can_Processor, doProcessCan, osPriorityAboveNormal, 0, 512);
  Can_ProcessorHandle = osThreadCreate(osThread(Can_Processor), NULL);

  /* definition and creation of RT */
  osThreadDef(RT, doRT, osPriorityRealtime, 0, 512);
  RTHandle = osThreadCreate(osThread(RT), NULL);

  /* definition and creation of TMT */
  osThreadDef(TMT, doTMT, osPriorityHigh, 0, 512);
  TMTHandle = osThreadCreate(osThread(TMT), NULL);

  /* definition and creation of HouseKeeping */
  osThreadDef(HouseKeeping, doHouseKeeping, osPriorityBelowNormal, 0, 512);
  HouseKeepingHandle = osThreadCreate(osThread(HouseKeeping), NULL);

  /* definition and creation of PPTPoll */
  osThreadDef(PPTPoll, doPPTPoll, osPriorityNormal, 0, 512);
  PPTPollHandle = osThreadCreate(osThread(PPTPoll), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the queue(s) */
  /* definition and creation of mainCanTxQ */
  osMessageQDef(mainCanTxQ, 64, Can_frame_t);
  mainCanTxQHandle = osMessageCreate(osMessageQ(mainCanTxQ), NULL);

  /* definition and creation of mainCanRxQ */
  osMessageQDef(mainCanRxQ, 32, Can_frame_t);
  mainCanRxQHandle = osMessageCreate(osMessageQ(mainCanRxQ), NULL);

  /* definition and creation of can2TxQ */
  osMessageQDef(can2TxQ, 32, Can_frame_t);
  can2TxQHandle = osMessageCreate(osMessageQ(can2TxQ), NULL);

  /* definition and creation of Can2RxQ */
  osMessageQDef(Can2RxQ, 32, Can_frame_t);
  Can2RxQHandle = osMessageCreate(osMessageQ(Can2RxQ), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */


  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* CAN1 init function */
static void MX_CAN1_Init(void)
{

  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 5;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SJW = CAN_SJW_1TQ;
  hcan1.Init.BS1 = CAN_BS1_13TQ;
  hcan1.Init.BS2 = CAN_BS2_2TQ;
  hcan1.Init.TTCM = DISABLE;
  hcan1.Init.ABOM = ENABLE;
  hcan1.Init.AWUM = DISABLE;
  hcan1.Init.NART = DISABLE;
  hcan1.Init.RFLM = DISABLE;
  hcan1.Init.TXFP = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* CAN2 init function */
static void MX_CAN2_Init(void)
{

  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 10;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SJW = CAN_SJW_1TQ;
  hcan2.Init.BS1 = CAN_BS1_11TQ;
  hcan2.Init.BS2 = CAN_BS2_4TQ;
  hcan2.Init.TTCM = DISABLE;
  hcan2.Init.ABOM = ENABLE;
  hcan2.Init.AWUM = DISABLE;
  hcan2.Init.NART = DISABLE;
  hcan2.Init.RFLM = DISABLE;
  hcan2.Init.TXFP = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* SPI2 init function */
static void MX_SPI2_Init(void)
{

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 230400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* WWDG init function */
static void MX_WWDG_Init(void)
{

  hwwdg.Instance = WWDG;
  hwwdg.Init.Prescaler = WWDG_PRESCALER_8;
  hwwdg.Init.Window = 127;
  hwwdg.Init.Counter = 127;
  hwwdg.Init.EWIMode = WWDG_EWI_DISABLE;
  if (HAL_WWDG_Init(&hwwdg) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MCP1_CS_GPIO_Port, MCP1_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|EN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, EN1_Pin|S2_Pin|S1_Pin|S3_Pin
                          |S0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MCP1_CS_Pin */
  GPIO_InitStruct.Pin = MCP1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(MCP1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EN1_Pin */
  GPIO_InitStruct.Pin = EN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(EN1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : S2_Pin S1_Pin S3_Pin S0_Pin */
  GPIO_InitStruct.Pin = S2_Pin|S1_Pin|S3_Pin|S0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : DR1_Pin */
  GPIO_InitStruct.Pin = DR1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DR1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EN2_Pin */
  GPIO_InitStruct.Pin = EN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(EN2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t pinNum){
	if(init_Done){
		if(pinNum == DR1_Pin){
	//		HAL_NVIC_DisableIRQ(EXTI1_IRQn);
	//		HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	//		HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
			mcp3909_readAllChannels(&hmcp1,hmcp1.pRxBuf);
			xSemaphoreGiveFromISR(mcp3909_DRHandle, NULL);
		}
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){
	// Check which SPI issued interrupt
	if(hspi == (hmcp1.hspi)){
		HAL_GPIO_WritePin(MCP1_CS_GPIO_Port,MCP1_CS_Pin, GPIO_PIN_SET);
		xSemaphoreGiveFromISR(mcp3909_RXHandle, NULL);
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
	// Check which SPI issued interrupt
	if(hspi == (hmcp1.hspi)){
		HAL_GPIO_WritePin(MCP1_CS_GPIO_Port,MCP1_CS_Pin, GPIO_PIN_SET);
	}
}

void EM_Init(){
	hmcp1.phase[0] = 0;
	hmcp1.phase[1] = 0;
	hmcp1.phase[2] = 0;

	// TODO: Shutdown channels 2-5 for BPS
	for(uint8_t i= 0; i < MAX_CHANNEL_NUM; i++){
		hmcp1.channel[i].PGA = PGA_1;
		hmcp1.channel[i].boost = BOOST_ON;
		hmcp1.channel[i].dither = DITHER_ON;
		hmcp1.channel[i].reset = RESET_OFF;
		hmcp1.channel[i].shutdown = SHUTDOWN_OFF;
		hmcp1.channel[i].resolution = RES_24;
	}

	// Amplify current sense channels to improve dynamic resolution
	hmcp1.channel[1].PGA = PGA_2;
	hmcp1.channel[3].PGA = PGA_4;
	hmcp1.channel[5].PGA = PGA_4;

	hmcp1.extCLK = 0;
	hmcp1.extVREF = 0;
	hmcp1.hspi = &hspi2;
	hmcp1.osr = OSR_256;
	hmcp1.prescale = PRESCALE_1;
	hmcp1.readType = READ_TYPE;

	hmcp1.pRxBuf = mcpRxBuf;
	hmcp1.pTxBuf = mcpTxBuf;

	// HAL_NVIC_SetPriority(EXTI1_IRQn, 6, 0); // set DR pin interrupt priority
	// HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
	mcp3909_init(&hmcp1);
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan){
	if (hcan == &hcan1){
		CAN1_TxCpltCallback(hcan);
	}else if (hcan == &hcan2){
		CAN2_TxCpltCallback(hcan);
	}
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan){
	if (hcan == &hcan1){
		CAN1_RxCpltCallback(hcan);
	}else if (hcan == &hcan2){
		CAN2_RxCpltCallback(hcan);
	}
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan){
	if (hcan == &hcan1){
		CAN1_ErrorCallback(hcan);
	}else if (hcan == &hcan2){
		CAN2_ErrorCallback(hcan);
	}
}
/* USER CODE END 4 */

/* doApplication function */
void doApplication(void const * argument)
{

  /* USER CODE BEGIN 5 */
	vTaskSuspend(NULL);
	for(;;){
		osDelay(100000);
	}
  /* USER CODE END 5 */
}

/* doProcessCan function */
void doProcessCan(void const * argument)
{
  /* USER CODE BEGIN doProcessCan */
	for(;;){
		// Wrapper function for the CAN Processing Logic
		// Handles all CAN Protocol Suite based responses and tasks
#ifndef DISABLE_CAN
		Can_Processor();
#else
		osDelay(10000);
#endif
	}
  /* USER CODE END doProcessCan */
}

/* doRT function */
void doRT(void const * argument)
{
  /* USER CODE BEGIN doRT */
#ifndef DISABLE_RT

#ifndef DISABLE_CAN
	static Can_frame_t newFrame;
	newFrame.isExt = 0;
	newFrame.isRemote = 0;
	newFrame.dlc = 8;

	static Can_frame_t dcFrame;
	dcFrame.dlc = 0;
	dcFrame.id = 0x701;
	dcFrame.isExt = 0;
	dcFrame.isRemote = 0;
	uint8_t dcSent = 0;
#else
	osDelay(10);
#endif
#ifndef DISABLE_SERIAL_OUT
	static uint8_t intBuf[10];
#endif
	static uint32_t previousWaitTime;

	for(;;){
#ifndef DISABLE_CAN
		if((selfStatusWord & 0x07) == ACTIVE){
#endif
			previousWaitTime = osKernelSysTick();
			mcp3909_wakeup(&hmcp1);
			xSemaphoreTake(mcp3909_DRHandle, portMAX_DELAY);
			xSemaphoreTake(mcp3909_RXHandle, portMAX_DELAY);
			mcp3909_parseChannelData(&hmcp1);

			while(!mcp3909_verify(&hmcp1)){
				if(!dcSent) bxCan_sendFrame(&dcFrame);
				dcSent = 1;
				HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 1);
				EM_Init();
				osDelay(100);
				HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 0);
			}
			dcSent = 0;

#ifndef DISABLE_CAN
			int32_t temp;

			newFrame.id = pptAPwr;
			temp = psb0ch0Map((hmcp1.registers[0]));
			*(int32_t*)(&(newFrame.Data[0])) = __REV(temp);
			temp= psb0ch1Map((hmcp1.registers[1]));
			*(int32_t*)(&(newFrame.Data[4])) = __REV(temp);
			bxCan_sendFrame(&newFrame);

			newFrame.id = pptBPwr;
			temp = psb0ch2Map((hmcp1.registers[2]));
			*(int32_t*)(&(newFrame.Data[0])) = __REV(temp);
			temp= psb0ch3Map((hmcp1.registers[3]));
			*(int32_t*)(&(newFrame.Data[4])) = __REV(temp);
			bxCan_sendFrame(&newFrame);

			newFrame.id = pptCPwr;
			temp = psb0ch4Map((hmcp1.registers[4]));
			*(int32_t*)(&(newFrame.Data[0])) = __REV(temp);
			temp= psb0ch5Map((hmcp1.registers[5]));
			*(int32_t*)(&(newFrame.Data[4])) = __REV(temp);
			bxCan_sendFrame(&newFrame);
#endif

			// XXX: Energy metering algorithm

			mcp3909_sleep(&hmcp1);
			HAL_WWDG_Refresh(&hwwdg);
			osDelayUntil(&previousWaitTime, RT_Interval);
#ifndef DISABLE_CAN
		}else{
			osDelay(1);
		}
#endif
	}
#else
	for(;;){
		osDelay(1000);
	}
#endif
  /* USER CODE END doRT */
}

/* doTMT function */
void doTMT(void const * argument)
{
  /* USER CODE BEGIN doTMT */
#ifndef DISABLE_TMT

#ifndef DISABLE_CAN
	static Can_frame_t newFrame;
	newFrame.dlc = 8;
	newFrame.isRemote = 0;
	newFrame.isExt = 0;
#else
	osDelay(10);
#endif
	for(;;){
#ifndef DISABLE_CAN
		if((selfStatusWord & 0x07) == ACTIVE){
#endif
			int32_t microCelcius;
			for(int i=0; 2*i<TEMP_CHANNELS; i++){
				for(int j=0; j<2; j++){
					microCelcius = getMicroCelcius(2*i+j);
					resetReading(2*i+j);

//					if(microCelcius >= OVER_TEMPERATURE || microCelcius <= UNDER_TEMPERATURE) assert_bps_fault(tempOffset+i*2+j, microCelcius);
#ifndef DISABLE_CAN
					*(int32_t*)(&(newFrame.Data[j*4])) = __REV(microCelcius);
#endif
				}
#ifndef DISABLE_CAN
				newFrame.id = adcTempOffset + i;
				bxCan_sendFrame(&newFrame);
#endif
			}
			osDelay(TMT_Interval);
#ifndef DISABLE_CAN
		}else{
			osDelay(1);
		}
#endif
	}
#else
	for(;;){
		osDelay(1000);
	}
#endif
  /* USER CODE END doTMT */
}

/* doHouseKeeping function */
void doHouseKeeping(void const * argument)
{
  /* USER CODE BEGIN doHouseKeeping */
	static int bamboozle;
	bamboozle = 0;
	static int bamboozle2;
	bamboozle2 = 0;

	for(;;){
		if(hcan1.State == HAL_CAN_STATE_READY || hcan1.State == HAL_CAN_STATE_BUSY_TX || \
		hcan1.State == HAL_CAN_STATE_TIMEOUT || hcan1.State == HAL_CAN_STATE_ERROR){
			bamboozle++;
		}else{
			bamboozle = 0;
		}
		if(bamboozle > 8){
			HAL_CAN_Receive_IT(&hcan1, 0);
		}
		if(bamboozle > 12){
			NVIC_SystemReset();
		}

		if(hcan2.State == HAL_CAN_STATE_READY || hcan2.State == HAL_CAN_STATE_BUSY_TX || \
		hcan2.State == HAL_CAN_STATE_TIMEOUT || hcan2.State == HAL_CAN_STATE_ERROR){
			bamboozle2++;
		}else{
			bamboozle2 = 0;
		}
		if(bamboozle2 > 8){
			HAL_CAN_Receive_IT(&hcan2, 0);
		}
		if(bamboozle2 > 12){
			NVIC_SystemReset();
		}

		osDelay(17);    //nice prime number
	}
  /* USER CODE END doHouseKeeping */
}

/* doPPTPoll function */
void doPPTPoll(void const * argument)
{
  /* USER CODE BEGIN doPPTPoll */
	static Can_frame_t newFrame;

	for(;;){
		xQueueReceive(Can2RxQHandle, &newFrame, portMAX_DELAY);
		bxCan_sendFrame(&newFrame);
	}
  /* USER CODE END doPPTPoll */
}

/* TmrKickDog function */
void TmrKickDog(void const * argument)
{
  /* USER CODE BEGIN TmrKickDog */
	taskENTER_CRITICAL();
	HAL_WWDG_Refresh(&hwwdg);
	taskEXIT_CRITICAL();
  /* USER CODE END TmrKickDog */
}

/* TmrSendHB function */
void TmrSendHB(void const * argument)
{
  /* USER CODE BEGIN TmrSendHB */
	// CHECKED
	static Can_frame_t newFrame;

	// newFrame.isExt = 0;
	// newFrame.isRemote = 0;
	// ^ is initialized as 0

	if(getSelfState() == ACTIVE){
		// Assemble new heartbeat frame
		newFrame.id = selfNodeID + swOffset;
		newFrame.dlc = CAN_HB_DLC;
		for(int i=0; i<4; i++){
			newFrame.Data[3-i] = (selfStatusWord >> (8*i)) & 0xff;			// Convert uint32_t -> uint8_t
		}
		bxCan_sendFrame(&newFrame);
		#ifdef DEBUG
			static uint8_t hbmsg[] = "Heartbeat issued\n";
			Serial2_writeBytes(hbmsg, sizeof(hbmsg)-1);
		#endif
	}
	else if (getSelfState() == INIT){
		// Assemble new addition request (firmware version) frame
		newFrame.id = selfNodeID + fwOffset;
		newFrame.dlc = CAN_FW_DLC;
		for(int i=0; i<4; i++){
			newFrame.Data[3-i] = (firmwareString >> (8*i)) & 0xff;			// Convert uint32_t -> uint8_t
		}
		bxCan_sendFrame(&newFrame);
		#ifdef DEBUG
			static uint8_t hbmsg[] = "Init handshake issued\n";
			Serial2_writeBytes(hbmsg, sizeof(hbmsg)-1);
		#endif
	}
	// No heartbeats sent in other states
  /* USER CODE END TmrSendHB */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
/* USER CODE BEGIN Callback 0 */

/* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
/* USER CODE BEGIN Callback 1 */

/* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */

/**
  * @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
