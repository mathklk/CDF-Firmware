/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#include "my_stm32wl3x_hal.h"
#include "checksum.h"
#include "banner.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef union {
    struct {
        int16_t I;
        int16_t Q;
    } iq;
    struct {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
    } b;
    uint32_t w;
} IQ;

#define IQ_BUFFER_SIZE 3500
typedef struct {
    IQ buf0[IQ_BUFFER_SIZE];
    IQ buf1[IQ_BUFFER_SIZE];
} IqBuffers;

typedef enum {
    idle,
    print
} State;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define UART_RX_BUFFER_SIZE 16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define BEGIN_UART_RX HAL_UART_Receive_IT(&huart1, uartRxBuffer, 1);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

IWDG_HandleTypeDef hiwdg;

RNG_HandleTypeDef hrng;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
__attribute__((aligned(4))) IqBuffers buffers;
//__attribute__((aligned(4))) IQ databuffer0[IQ_BUFFER_SIZE];
//__attribute__((aligned(4))) IQ databuffer1[IQ_BUFFER_SIZE];
// IQ* databuffers[] = {databuffer0, databuffer1};

uint8_t uartRxBuffer[UART_RX_BUFFER_SIZE];
char command = 0;

State state = idle;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_RNG_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Makes UART work with printf
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

void putIq(IQ iq) {
  putchar(iq.b.b0);
  putchar(iq.b.b1);
  putchar(iq.b.b2);
  putchar(iq.b.b3);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */
  // Initialize with ordered, debuggable values
  for (uint32_t i = 0; i < IQ_BUFFER_SIZE; ++i) {
    buffers.buf0[i].b.b0 = (uint8_t)(i);
    buffers.buf0[i].b.b1 = (uint8_t)(i);
    buffers.buf0[i].b.b2 = (uint8_t)(i);
    buffers.buf0[i].b.b3 = (uint8_t)(i);
    buffers.buf1[i].b.b0 = (uint8_t)(i);
    buffers.buf1[i].b.b1 = (uint8_t)(i);
    buffers.buf1[i].b.b2 = (uint8_t)(i);
    buffers.buf1[i].b.b3 = (uint8_t)(i);
  }
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_IWDG_Init();
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */
  printf("\r\n\r\n");
  printf(BANNER);
  printf("\r\n@ %s - %s\r\n", __DATE__, __TIME__);
  BEGIN_UART_RX;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (state == print) {
      uint32_t const nBytes = sizeof(buffers);
      if (nBytes > UINT16_MAX) {
        printf("FRAME TOO LARGE TO REPRESENT AS UINT16\r\n");
      }

      uint32_t const checksum = crc32((uint8_t*)buffers.buf0, sizeof(buffers));

      // Start of Header
      putchar(0x01);
      // Frame Length
      putchar((nBytes >> 8) & 0xFF);
      putchar( nBytes       & 0xFF);
      // Checksum
      putchar((checksum >> 24) & 0xFF);
      putchar((checksum >> 16) & 0xFF);
      putchar((checksum >>  8) & 0xFF);
      putchar( checksum        & 0xFF);

      for (uint32_t i = 0; i < IQ_BUFFER_SIZE; ++i) {
        putIq(buffers.buf0[i]);
        HAL_IWDG_Refresh(&hiwdg);
      }
      for (uint32_t i = 0; i < IQ_BUFFER_SIZE; ++i) {
        putIq(buffers.buf1[i]);
        HAL_IWDG_Refresh(&hiwdg);
      }
      state = idle;
    }

    // Command Interpreter
    if (command != 0) {
      if (command == 'u') {
        printf("%" PRIX32 "-%" PRIX32 "\r\n", HAL_GET_UID64_M(), HAL_GET_UID64_L());
      }
      else if (command == 'd') {
        printf("%s - %s\r\n", __DATE__, __TIME__);
      }
      else if (command == 'r') {
        printf("Resetting...\r\n");
        NVIC_SystemReset();
      }
      else if (command == 'i') {
        state = idle;
        printf("state=idle\r\n");
      }
      else if (command == 'p') {
        state = print;
      }
      else if (command == 'f') {
        for (uint32_t i = 0; i < IQ_BUFFER_SIZE; ++i) {
          HAL_RNG_GenerateRandomNumber(&hrng, &buffers.buf0[i].w);
          HAL_RNG_GenerateRandomNumber(&hrng, &buffers.buf1[i].w);
        }
        printf("Buffers filled randomly %lx\r\n", buffers.buf0[0].w);
      }

      command = 0;
    }

    // Reset Watchdog
    HAL_IWDG_Refresh(&hiwdg);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource and SYSCLKDivider
  */
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_RC64MPLL_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAIT_STATES_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLK_DIV2;
  PeriphClkInitStruct.KRMRateMultiplier = 2;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

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
  huart1.Init.BaudRate = 2000000;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_8;
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  BEGIN_UART_RX;
  command = uartRxBuffer[0];


  //  char sanitizedBuffer[UART_RX_BUFFER_SIZE + 1];
  //  for (size_t i = 0; i < UART_RX_BUFFER_SIZE; i++) {
  //    char c = uartRxBuffer[i];
  //    if (c >= 32 && c <= 126) { // Printable ASCII range
  //      sanitizedBuffer[i] = c;
  //    } else {
  //      sanitizedBuffer[i] = '#';
  //    }
  //  }
  //  sanitizedBuffer[UART_RX_BUFFER_SIZE] = '\0'; // Null-terminate the string
  //
  //    printf("ECHO ");
  //    printf(sanitizedBuffer);
  //    printf("\r\n");
  //
  //  for (size_t i = 0; i < UART_RX_BUFFER_SIZE; i++) {
  //    uartRxBuffer[i] = 0; // Clear the buffer after processing
  //  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    printf("ERROR\r\n");
    __disable_irq();
    while (1)
    {
    }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
