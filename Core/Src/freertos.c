/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mytask.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defauleTask */
osThreadId_t defauleTaskHandle;
const osThreadAttr_t defauleTask_attributes = {
  .name = "defauleTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LED2Task */
osThreadId_t LED2TaskHandle;
const osThreadAttr_t LED2Task_attributes = {
  .name = "LED2Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LED1Task */
osThreadId_t LED1TaskHandle;
const osThreadAttr_t LED1Task_attributes = {
  .name = "LED1Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UART1_recv_Task */
osThreadId_t UART1_recv_TaskHandle;
const osThreadAttr_t UART1_recv_Task_attributes = {
  .name = "UART1_recv_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LCDDisplayTask */
osThreadId_t LCDDisplayTaskHandle;
const osThreadAttr_t LCDDisplayTask_attributes = {
  .name = "LCDDisplayTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for uart1_printf_gsem */
osSemaphoreId_t uart1_printf_gsemHandle;
const osSemaphoreAttr_t uart1_printf_gsem_attributes = {
  .name = "uart1_printf_gsem"
};
/* Definitions for uart1_rxok_gsem */
osSemaphoreId_t uart1_rxok_gsemHandle;
const osSemaphoreAttr_t uart1_rxok_gsem_attributes = {
  .name = "uart1_rxok_gsem"
};
/* Definitions for LCD_refresh_gsem */
osSemaphoreId_t LCD_refresh_gsemHandle;
const osSemaphoreAttr_t LCD_refresh_gsem_attributes = {
  .name = "LCD_refresh_gsem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartdefauleTask(void *argument);
extern void StartLED2TaskFunction(void *argument);
extern void StartLED1TaskFunction(void *argument);
extern void StartUART1_recv_TaskFunction(void *argument);
extern void StartLCDDisplayTaskFunction(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of uart1_printf_gsem */
  uart1_printf_gsemHandle = osSemaphoreNew(1, 1, &uart1_printf_gsem_attributes);

  /* creation of uart1_rxok_gsem */
  uart1_rxok_gsemHandle = osSemaphoreNew(1, 1, &uart1_rxok_gsem_attributes);

  /* creation of LCD_refresh_gsem */
  LCD_refresh_gsemHandle = osSemaphoreNew(1, 1, &LCD_refresh_gsem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defauleTask */
  defauleTaskHandle = osThreadNew(StartdefauleTask, NULL, &defauleTask_attributes);

  /* creation of LED2Task */
  LED2TaskHandle = osThreadNew(StartLED2TaskFunction, NULL, &LED2Task_attributes);

  /* creation of LED1Task */
  LED1TaskHandle = osThreadNew(StartLED1TaskFunction, NULL, &LED1Task_attributes);

  /* creation of UART1_recv_Task */
  UART1_recv_TaskHandle = osThreadNew(StartUART1_recv_TaskFunction, NULL, &UART1_recv_Task_attributes);

  /* creation of LCDDisplayTask */
  LCDDisplayTaskHandle = osThreadNew(StartLCDDisplayTaskFunction, NULL, &LCDDisplayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartdefauleTask */
/**
  * @brief  Function implementing the defauleTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartdefauleTask */
__weak void StartdefauleTask(void *argument)
{
  /* USER CODE BEGIN StartdefauleTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartdefauleTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

