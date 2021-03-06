/*
 * Copyright 2019-2022 MicroEJ Corp. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be found with this software.
 */

/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include "main.h"
#include "cmsis_os.h"
#include "microjvm_main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32l4r9i_discovery_psram.h"
#include "stm32l4r9i_discovery_ospi_nor.h"
#include "SEGGER_SYSVIEW.h"
#include "cpuload.h"
#include "framerate.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MICROJVM_STACK_SIZE (12 * 1024)
#define JAVA_TASK_PRIORITY      ( 11 ) /** Should be > tskIDLE_PRIORITY & < configTIMER_TASK_PRIORITY */
#define JAVA_TASK_STACK_SIZE     MICROJVM_STACK_SIZE/4

/* Private function prototypes -----------------------------------------------*/

static void GPIO_ConfigAN(void);
void SystemClock_Config(void);

/* Private functions ---------------------------------------------------------*/

static void xJavaTaskFunction(void * pvParameters)
{
	cpuload_init();
	microjvm_main();
	vTaskDelete( xTaskGetCurrentTaskHandle() );
}

static void xInfoTaskFunction(void * pvParameters)
{  
	uint32_t fps_average;
	uint32_t fps_peak;
	while(1)
	{
		osDelay(1000);
		printf("CPU LOAD = %d%\t", cpuload_get());
		framerate_get(&fps_average, & fps_peak);
		printf("FPS = avg: %d peak: %d\n", fps_average, fps_peak);
	}
}

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
	/* STM32L4xx HAL library initialization:
       - Configure the Flash prefetch
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
	 */
	HAL_Init();

	/* Configure the System clock to 120 MHz */
	SystemClock_Config();

	/* Configure GPIO's to AN to reduce power consumption */
	GPIO_ConfigAN();

	BSP_PSRAM_Init();
	BSP_OSPI_NOR_Init();
	BSP_OSPI_NOR_EnableMemoryMappedMode();

	printf("start BSP\n");

	/* Enable Trace */
	SEGGER_SYSVIEW_Conf();

	TaskHandle_t pvCreatedTask;
	xTaskCreate(xJavaTaskFunction, "MicroJvm", JAVA_TASK_STACK_SIZE, NULL, JAVA_TASK_PRIORITY, &pvCreatedTask);
	SEGGER_SYSVIEW_setMicroJVMTask((U32)pvCreatedTask);
	// xTaskCreate(xInfoTaskFunction, "InfoTask", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
	vTaskStartScheduler();

	/* We should never get here as control is now taken by the scheduler */
	for (;;);

}

/**
 * @brief  Pre Sleep Processing
 * @param  ulExpectedIdleTime: Expected time in idle state
 * @retval None
 */
void PreSleepProcessing(uint32_t * ulExpectedIdleTime)
{
	/* Called by the kernel before it places the MCU into a sleep mode because
  configPRE_SLEEP_PROCESSING() is #defined to PreSleepProcessing().

  NOTE:  Additional actions can be taken here to get the power consumption
  even lower.  For example, peripherals can be turned off here, and then back
  on again in the post sleep processing function.  For maximum power saving
  ensure all unused pins are in their lowest power state. */

	/*
    (*ulExpectedIdleTime) is set to 0 to indicate that PreSleepProcessing contains
    its own wait for interrupt or wait for event instruction and so the kernel vPortSuppressTicksAndSleep 
    function does not need to execute the wfi instruction  
	 */
	*ulExpectedIdleTime = 0;

	/*Enter to sleep Mode using the HAL function HAL_PWR_EnterSLEEPMode with WFI instruction*/
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

/**
 * @brief  Post Sleep Processing
 * @param  ulExpectedIdleTime: Not used
 * @retval None
 */
void PostSleepProcessing(uint32_t * ulExpectedIdleTime)
{
	/* Called by the kernel when the MCU exits a sleep mode because
  configPOST_SLEEP_PROCESSING is #defined to PostSleepProcessing(). */

	/* Avoid compiler warnings about the unused parameter. */
	(void) ulExpectedIdleTime;
}

/**
 * @brief  Configure all GPIO's to AN to reduce the power consumption
 * WARNING: it disable JTAG pins
 * @param  None
 * @retval None
 */
static void GPIO_ConfigAN(void)
{
#if configUSE_TICKLESS_IDLE == 1
	GPIO_InitTypeDef GPIO_InitStruct;

	/* Configure all GPIO as analog to reduce current consumption on non used IOs */
	/* Enable GPIOs clock */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();

	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

	/* Disable GPIOs clock */
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	__HAL_RCC_GPIOE_CLK_DISABLE();
	__HAL_RCC_GPIOF_CLK_DISABLE();
	__HAL_RCC_GPIOG_CLK_DISABLE();
	__HAL_RCC_GPIOH_CLK_DISABLE();
	__HAL_RCC_GPIOI_CLK_DISABLE();
#endif
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follows :
 *            System Clock source            = PLL (MSI)
 *            SYSCLK(Hz)                     = 120000000
 *            HCLK(Hz)                       = 120000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            MSI Frequency(Hz)              = 4000000
 *            PLL_M                          = 1
 *            PLL_N                          = 60
 *            PLL_Q                          = 2
 *            PLL_R                          = 2
 *            PLL_P                          = 7
 *            Flash Latency(WS)              = 5
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};

	/* Enable voltage range 1 boost mode for frequency above 80 Mhz */
	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
	__HAL_RCC_PWR_CLK_DISABLE();

	/* Enable MSI Oscillator and activate PLL with MSI as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 60;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLP = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		/* Initialization Error */
		while(1);
	}

	/* To avoid undershoot due to maximum frequency, select PLL as system clock source */
	/* with AHB prescaler divider 2 as first step */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
	{
		/* Initialization Error */
		while(1);
	}

	/* AHB prescaler divider at 1 as second step */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		/* Initialization Error */
		while(1);
	}
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{}
}
#endif
