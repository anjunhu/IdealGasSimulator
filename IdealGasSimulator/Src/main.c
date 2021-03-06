/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "arm_math.h"
#include "stm32l4s5i_iot01_gyro.h"
#include "stm32l4s5i_iot01_hsensor.h"
#include "stm32l4s5i_iot01_nfctag.h"
#include "stm32l4s5i_iot01_psensor.h"
#include "stm32l4s5i_iot01_tsensor.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <physics.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define ITM_Port32(n) (*((volatile unsigned long *) (0xE0000000+4*n)))
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAX_12 4095
#define LOOKUP_LEN 180
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac1;
DMA_HandleTypeDef hdma_dac1_ch1;

I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

osThreadId measureHandle;
osThreadId transmitHandle;
osThreadId listenHandle;
osTimerId WorkSoundEffectTimerHandle;
osTimerId TempSoundEffectTimerHandle;
osMutexId dataMutexHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_DAC1_Init(void);
void StartMeasureTask(void const * argument);
void StartTransmitTask(void const * argument);
void StartListenTask(void const * argument);
void CallbackWorkSoundEffectTimer(void const * argument);
void CallbackTempSoundEffectTimer(void const * argument);

/* USER CODE BEGIN PFP */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t buffer[150];
float gyro[3];
float pressure;
float temperature;
float work;
uint8_t note = 0;
int8_t ringtone = -1;
uint8_t chordUpdate = 0;
uint16_t sin_lookup_chord[180]; // lcm(45, 36, 30)
uint16_t sin_lookup_C6[180]; // 1046.5 Hz, sample rate = 48kHz
uint16_t sin_lookup_E6[180]; // 1318.51 Hz
uint16_t sin_lookup_G6[180]; // 1567.98 Hz
struct KalmanState ksTemp = { // temperature filter
		.q = 0.01,	// 1% process noise
		.r = 0.5,	// precision in degree celcius
		.x = 25,	// typical room temp
		.p = 0.1,
		.k = 0.0
};
struct KalmanState ksWork = { // work filter
		.q = 0.01,	// 1% process noise
		.r = 75,	// precision in mdps
		.x = 0,		// typical work (no motion)
		.p = 0.01,
		.k = 0.0
};
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */
	// initialize notes for alerts
	for (int i = 0; i < LOOKUP_LEN; i++) {
		sin_lookup_C6[i] = (MAX_12 * 0.6 * (arm_sin_f32(6.283 * i / 45) + 1));
	}
	for (int i = 0; i < LOOKUP_LEN; i++) {
		sin_lookup_E6[i] = (MAX_12 * 0.6 * (arm_sin_f32(6.283 * i / 36) + 1));
	}
	for (int i = 0; i < LOOKUP_LEN; i++) {
		sin_lookup_G6[i] = (MAX_12 * 0.6 * (arm_sin_f32(6.283 * i / 30) + 1));
	}
	memset(sin_lookup_chord, 0, sizeof(sin_lookup_chord));
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

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
	MX_I2C2_Init();
	MX_USART1_UART_Init();
	MX_TIM2_Init();
	MX_DAC1_Init();
	/* USER CODE BEGIN 2 */
	// Initialize individual sensors
	BSP_GYRO_Init();
	BSP_TSENSOR_Init();
	BSP_PSENSOR_Init();
	// Initialize Sound Effect Player
	HAL_TIM_Base_Start(&htim2);

	/* USER CODE END 2 */

	/* Create the mutex(es) */
	/* definition and creation of dataMutex */
	osMutexDef(dataMutex);
	dataMutexHandle = osMutexCreate(osMutex(dataMutex));

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	/* USER CODE END RTOS_SEMAPHORES */

	/* Create the timer(s) */
	/* definition and creation of WorkSoundEffectTimer */
	osTimerDef(WorkSoundEffectTimer, CallbackWorkSoundEffectTimer);
	WorkSoundEffectTimerHandle = osTimerCreate(osTimer(WorkSoundEffectTimer), osTimerPeriodic, NULL);

	/* definition and creation of TempSoundEffectTimer */
	osTimerDef(TempSoundEffectTimer, CallbackTempSoundEffectTimer);
	TempSoundEffectTimerHandle = osTimerCreate(osTimer(TempSoundEffectTimer), osTimerPeriodic, NULL);

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* definition and creation of measure */
	osThreadDef(measure, StartMeasureTask, osPriorityNormal, 0, 256);
	measureHandle = osThreadCreate(osThread(measure), NULL);

	/* definition and creation of transmit */
	osThreadDef(transmit, StartTransmitTask, osPriorityIdle, 0, 128);
	transmitHandle = osThreadCreate(osThread(transmit), NULL);

	/* definition and creation of listen */
	osThreadDef(listen, StartListenTask, osPriorityIdle, 0, 128);
	listenHandle = osThreadCreate(osThread(listen), NULL);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

	/* Start scheduler */
	osKernelStart();

	/* We should never get here as control is now taken by the scheduler */
	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 60;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C2;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief DAC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_DAC1_Init(void)
{

	/* USER CODE BEGIN DAC1_Init 0 */

	/* USER CODE END DAC1_Init 0 */

	DAC_ChannelConfTypeDef sConfig = {0};

	/* USER CODE BEGIN DAC1_Init 1 */

	/* USER CODE END DAC1_Init 1 */
	/** DAC Initialization
	 */
	hdac1.Instance = DAC1;
	if (HAL_DAC_Init(&hdac1) != HAL_OK)
	{
		Error_Handler();
	}
	/** DAC channel OUT1 config
	 */
	sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
	sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
	sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_DISABLE;
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
	sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
	if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN DAC1_Init 2 */

	/* USER CODE END DAC1_Init 2 */

}

/**
 * @brief I2C2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C2_Init(void)
{

	/* USER CODE BEGIN I2C2_Init 0 */

	/* USER CODE END I2C2_Init 0 */

	/* USER CODE BEGIN I2C2_Init 1 */

	/* USER CODE END I2C2_Init 1 */
	hi2c2.Instance = I2C2;
	hi2c2.Init.Timing = 0x307075B1;
	hi2c2.Init.OwnAddress1 = 0;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c2) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN I2C2_Init 2 */

	/* USER CODE END I2C2_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 2500;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

	/* DMA controller clock enable */
	__HAL_RCC_DMAMUX1_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : RED_LED_Pin */
	GPIO_InitStruct.Pin = RED_LED_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(RED_LED_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : USER_BUTTON_Pin */
	GPIO_InitStruct.Pin = USER_BUTTON_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : GREEN_LED_Pin */
	GPIO_InitStruct.Pin = GREEN_LED_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GREEN_LED_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartMeasureTask */
/**
 * @brief  Function implementing the measure thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartMeasureTask */
void StartMeasureTask(void const * argument)
{
	/* USER CODE BEGIN 5 */
	/* Infinite loop */
	for (;;) {
		// sampling rate = 100Hz
		osDelay(10);

		// ensure the buffer is safe to write
		osMutexWait(dataMutexHandle, osWaitForever);

		memset(buffer, 0, strlen(buffer));

		// take measurements and pass them through Kalman filters
		BSP_GYRO_GetXYZ(gyro); // unit:mdps
		pressure = BSP_PSENSOR_ReadPressure(); // unit:mbar
		temperature = kalmanFilterA(&ksTemp, BSP_TSENSOR_ReadTemp()); // unit:celcius
		work = kalmanFilterA(&ksWork, gyro2work (gyro)); //unit:mW

		// to be transmitted to UART
		sprintf((char*) buffer,
				"AngAc=[%d, %d, %d]\nP=%d\nT=%d\nW=%d\n",
				(int) (gyro[0]), (int) (gyro[1]), (int) (gyro[2]),
				(int) pressure, (int) temperature, (int) work);

		// non-negative value for two kinds of ringtone alerts
		if (work>10) {
			ringtone = 0;
		} else if (kalmanFilterA(&ksTemp, BSP_TSENSOR_ReadTemp()) - temperature > 0.04){
			ringtone = 1;

		}
		else {
			ringtone = -1;
		}

		osSignalSet(transmitHandle, 0x0001);
		osMutexRelease(dataMutexHandle);

	}
	/* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTransmitTask */
/**
 * @brief Function implementing the transmit thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTransmitTask */
void StartTransmitTask(void const * argument)
{
	/* USER CODE BEGIN StartTransmitTask */
	/* Infinite loop */
	for (;;) {
		osDelay(10);
		// ensure the buffer has new data and is safe to transmit
		osSignalWait(0x0001, osWaitForever);
		osMutexWait(dataMutexHandle, osWaitForever);

		HAL_UART_Transmit(&huart1, buffer, strlen(buffer), 5);

		osMutexRelease(dataMutexHandle);

	}
	/* USER CODE END StartTransmitTask */
}

/* USER CODE BEGIN Header_StartListenTask */
/**
 * @brief Function implementing the listen thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartListenTask */
void StartListenTask(void const * argument)
{
	/* USER CODE BEGIN StartListenTask */
	/* Infinite loop */
	for (;;) {
		osDelay(500);
		//osSignalWait(0x0002, osWaitForever);
		osMutexWait(dataMutexHandle, osWaitForever);
		HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, sin_lookup_chord, 180,
				DAC_ALIGN_12B_R);

		// use software timers to play ring tone through DAC speaker
		// LEDs helps in case user does not have access to a speaker
		if (ringtone == 0) {
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
			osTimerStart(WorkSoundEffectTimerHandle, 250);
		}
		else if (ringtone == 1){
			HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
			osTimerStart(TempSoundEffectTimerHandle, 250);
		}
		else{
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
		}

		osMutexRelease(dataMutexHandle);
	}
	/* USER CODE END StartListenTask */
}

/* CallbackWorkSoundEffectTimer function */
void CallbackWorkSoundEffectTimer(void const * argument)
{
	/* USER CODE BEGIN CallbackWorkSoundEffectTimer */
	switch (note) {
	case 0:
		memcpy(sin_lookup_chord, sin_lookup_C6, sizeof(sin_lookup_chord));
		note += 1;
		break;
	case 1:
		memcpy(sin_lookup_chord, sin_lookup_E6, sizeof(sin_lookup_chord));
		note += 1;
		break;
	case 2:
		memcpy(sin_lookup_chord, sin_lookup_G6, sizeof(sin_lookup_chord));
		note += 1;
		break;
	default:
		note = 0;
		memset(sin_lookup_chord, 0, sizeof(sin_lookup_chord));
		HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
		osTimerStop(WorkSoundEffectTimerHandle);

	}

	/* USER CODE END CallbackWorkSoundEffectTimer */
}

/* CallbackTempSoundEffectTimer function */
void CallbackTempSoundEffectTimer(void const * argument)
{
	/* USER CODE BEGIN CallbackTempSoundEffectTimer */
	switch (note) {
	case 0:
		memcpy(sin_lookup_chord, sin_lookup_E6, sizeof(sin_lookup_chord));
		note += 1;
		break;
	case 1:
		memcpy(sin_lookup_chord, sin_lookup_G6, sizeof(sin_lookup_chord));
		note += 1;
		break;
	case 2:
		memcpy(sin_lookup_chord, sin_lookup_C6, sizeof(sin_lookup_chord));
		note += 1;
		break;
	default:
		note = 0;
		memset(sin_lookup_chord, 0, sizeof(sin_lookup_chord));
		HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
		osTimerStop(TempSoundEffectTimerHandle);

	}

	/* USER CODE END CallbackTempSoundEffectTimer */
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
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
