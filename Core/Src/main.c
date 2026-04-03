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
#include "banner.h"
#include "frame.h"
#include "BSP_CDF_V1.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define N_IQ_SAMPLES_PER_BUFFER 3500
//typedef struct {
//    IQ buf0[N_IQ_SAMPLES_PER_BUFFER];
//    IQ buf1[N_IQ_SAMPLES_PER_BUFFER];
//} IqBuffers;

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
  } bytes;
  uint32_t u32;
} IQ;

typedef union {
  IQ iq[N_IQ_SAMPLES_PER_BUFFER];
  uint8_t w[N_IQ_SAMPLES_PER_BUFFER * 4];
} Buffer;

typedef struct {
  Buffer buf0;
  Buffer buf1;
} Buffers;

typedef enum {
  IDLE,
  RECEIVING_IQ,
  ABORTING_AFTER_RECEIVE_IQ,
  RECEIVING_NORMAL,
  ABORTING_AFTER_RECEIVE_NORMAL_TIMEOUT,
  BEACON
} State;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define is ==
#define and &&
#define not !

#define rn "\r\n"

#define BEGIN_UART_RX HAL_UART_Receive_IT(&huart1, uartRxBuffer, 1);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

IWDG_HandleTypeDef hiwdg;

SMRSubGConfig MRSUBG_RadioInitStruct;
MRSubG_PcktBasicFields MRSUBG_PacketSettingsStruct;

RNG_HandleTypeDef hrng;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
__attribute__((aligned(4))) Buffers buffers;

#define UART_RX_BUFFER_SIZE 16
uint8_t uartRxBuffer[UART_RX_BUFFER_SIZE];

char command = 0;
bool extiTriggered = false;
uint32_t const beaconPeriodMs = 100;
uint32_t lastBeaconTime = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_RNG_Init(void);
static void MX_MRSUBG_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM16_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Makes UART work with printf
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

static inline uint16_t getUs(void) {
  return __HAL_TIM_GET_COUNTER(&htim2);
}
static inline bool usHavePassed(uint16_t const start, uint16_t const period) {
  return getUs() - start >= period;
}
static inline void delayUs(uint16_t delay) {
  uint16_t const start = getUs();
  while (not usHavePassed(start, delay)) {
    HAL_IWDG_Refresh(&hiwdg);
  }
}


static inline uint32_t getMs(void) {
  return HAL_GetTick();
}
static inline bool msHavePassed(uint32_t const start, uint32_t const period) {
  return getMs() - start >= period;
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
  // Initialize with ordered, easily debuggable values
  for (uint32_t i = 0; i < N_IQ_SAMPLES_PER_BUFFER; ++i) {
    buffers.buf0.iq[i].bytes.b0 = (uint8_t)(i);
    buffers.buf0.iq[i].bytes.b1 = (uint8_t)(i);
    buffers.buf0.iq[i].bytes.b2 = (uint8_t)(i);
    buffers.buf0.iq[i].bytes.b3 = (uint8_t)(i);
    buffers.buf1.iq[i].bytes.b0 = (uint8_t)(i);
    buffers.buf1.iq[i].bytes.b1 = (uint8_t)(i);
    buffers.buf1.iq[i].bytes.b2 = (uint8_t)(i);
    buffers.buf1.iq[i].bytes.b3 = (uint8_t)(i);
  }
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_IWDG_Init();
  MX_RNG_Init();
  MX_MRSUBG_Init();
  MX_TIM2_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_Base_Start(&htim16);
  bool const iAmMaster = HAL_GET_UID64_L() is 0x4FDFDA07;

  printf(rn rn);
  printf(BANNER rn);
  printf("@ %s - %s"rn, __DATE__, __TIME__);
  BEGIN_UART_RX;

  HAL_MRSUBG_SET_AGC_MEAS_TIME_BITS(10);
  HAL_MRSUBG_SET_AGC_FREEZE_ON_SYNC(false);
  HAL_MRSUBG_SET_AGC_FREEZE_ON_STEADY(false);
  BSP_SWITCH_RF_PATH_ANTENNAS();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  State state = IDLE;
  uint16_t pingTimestampMs = 0;
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // Command Interpreter
    if (command != 0) {
      if (command is 'a') {
        BSP_SWITCH_RF_PATH_ANTENNAS();
        printf("RF path set to antennas"rn);
      }
      else if (command is 'b') {
        BSP_SWITCH_RF_PATH_COMMON();
        printf("RF path set to common"rn);
      }
      else if (command is 'c') {
        if (state is IDLE) {
          state = BEACON;
          printf("Beacon mode enabled with T=%" PRId32 " ms"rn, beaconPeriodMs);
        } else if (state is BEACON) {
          state = IDLE;
          printf("Beacon mode disabled."rn);
        } else {
          printf("Can't switch beacon mode while in state %d"rn, state);
        }
      }
      else if (command is 'd') {
        printf("%s - %s"rn, __DATE__, __TIME__);
      }
      else if (command is 'g') {
        HAL_MRSUBG_SET_AGC_ENABLED(not HAL_MRSUBG_GET_AGC_ENABLED());
        printf("Automatic Gain Control=%d"rn, HAL_MRSUBG_GET_AGC_ENABLED());
      }
      else if (command is 'm') {
        int const msgLengthBits =
          MRSUBG_PacketSettingsStruct.PreambleLength +
          ( (MRSUBG_PacketSettingsStruct.SyncPresent == ENABLE) ? MRSUBG_PacketSettingsStruct.SyncLength : 0) +
          ( (MRSUBG_PacketSettingsStruct.FixVarLength == FIXED) ? 0 : (8*(1+MRSUBG_PacketSettingsStruct.LengthWidth)) ) +
          PAYLOAD_LENGTH +
          MRSUBG_PacketSettingsStruct.PostambleLength +
          crcBits(MRSUBG_PacketSettingsStruct.CrcMode)
        ;
        printf("Message structure: (%d bits total)"rn
          "        Preamble  : %d"rn
          "        Sync      : %d"rn
          "        Len       : %d"rn
          "        Payload   : %d"rn
          "        Postamble : %d"rn
          "        CRC       : %d"rn
          , msgLengthBits
          , MRSUBG_PacketSettingsStruct.PreambleLength
          , ( (MRSUBG_PacketSettingsStruct.SyncPresent == ENABLE) ? MRSUBG_PacketSettingsStruct.SyncLength : 0)
          , ( (MRSUBG_PacketSettingsStruct.FixVarLength == FIXED) ? 0 : (8*(1+MRSUBG_PacketSettingsStruct.LengthWidth)) )
          , PAYLOAD_LENGTH
          , MRSUBG_PacketSettingsStruct.PostambleLength
          , crcBits(MRSUBG_PacketSettingsStruct.CrcMode)
        );

        float const msgLengthMs = 1000.0f * (msgLengthBits) / MRSUBG_RadioInitStruct.lDatarate;
        printf("Msg duration = %f ms"rn, msgLengthMs);
      }
      else if (command is 'p') {
        transmitFrame(
          N_IQ_SAMPLES_PER_BUFFER * sizeof(IQ),
          buffers.buf0.w,
          N_IQ_SAMPLES_PER_BUFFER * sizeof(IQ),
          buffers.buf1.w,
          &hiwdg
        );
      }
      else if (command is 'r') {
        printf("Resetting..."rn);
        NVIC_SystemReset();
      }
      else if (command is 't') {
        if (state is IDLE) {
          printf("Transmitting..."rn);
          startTx();
          waitForTxDone();
          printf("Transmission finished"rn);
        } else {
          printf("Can't Transmit when not in idle"rn);
        }
      }
      else if (command is 'u') {
        printf("%" PRIX32 "-%" PRIX32 " %s"rn, HAL_GET_UID64_M(), HAL_GET_UID64_L(), iAmMaster ? "(MASTER)" : "");
      }
      else if (command is 'x') {
        if (iAmMaster) {
          if (state is IDLE) {
            printf("Starting routine"rn);

            // Positive Flanke. Löst bei slaves exti aus, der aufzeichnung startet
            HAL_GPIO_WriteMultipleStatePin(GPIOB, 0, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
            HAL_GPIO_WriteMultipleStatePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, 0);
            // Kalibrier-Nachricht senden
            BSP_SWITCH_RF_PATH_COMMON();
            LL_MRSubG_SetSecondarySync(true);
            LL_MRSubG_SetSecondarySyncWord(0x00000000);
            startTx();
            waitForTxDone();
            printf("Calibration sent"rn);
            // Ping senden
            BSP_SWITCH_RF_PATH_ANTENNAS();
            LL_MRSubG_SetSecondarySync(false);
            startTx();
            pingTimestampMs = getMs();
            waitForTxDone();
            delayUs(50);
            printf("Ping sent"rn);
            // Als master jetzt selbst auf normalen empfang schalten
            LL_MRSubG_SetSecondarySync(true);
            LL_MRSubG_SetSecondarySyncWord(0xEEEEEEEE);
            startNormalRx();
            state = RECEIVING_NORMAL;
            printf("Normal RX started on master after routine"rn);
          } else {
            printf("Can't start routine when not in idle"rn);
          }
        }
      }

      command = 0;
    } // End Command Interpreter

    if (extiTriggered) {
      extiTriggered = false;
      if (not iAmMaster) {
        printf("Slave started IQ RX on EXTI"rn);
        startIqRx();
        state = RECEIVING_IQ;
      } else {
        printf("EXTI on master!?!?!? ignored."rn);
      }
    }

    // Radio Handling

    // Fehlermeldung
    if (__HAL_MRSUBG_GET_RFSEQ_IRQ_STATUS() & MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_COMMAND_REJECTED_F) {
      __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_COMMAND_REJECTED_F);
      printf("COMMAND REJECTED"rn);
    }

    // Wenn beide Buffer beschrieben wurden, Empfang stoppen
    // TODO: Eventuell werden schon die ersten paar samples im ersten Buffer wieder überschrieben
    if (state is RECEIVING_IQ and HAL_MRSUBG_GET_NUMBER_OF_BUFFERS_USED() >= 2) {
      __HAL_MRSUBG_STROBE_CMD(CMD_SABORT);
      state = ABORTING_AFTER_RECEIVE_IQ;
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    }
    // Wenn gestoppt, Daten ausgeben und zurück zu IDLE Betrieb
    if (state is ABORTING_AFTER_RECEIVE_IQ and
      __HAL_MRSUBG_GET_RFSEQ_IRQ_STATUS() & MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_SABORT_DONE_F
    ) {
      __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_SABORT_DONE_F);

      uint8_t* pointerToOlderBuffer;
      uint8_t* pointerToNewerBuffer;
      int newerBuffer = not HAL_MRSUBG_GET_CURRENT_BUFFER(); // Negieren, weil Buffer bereits gewechselt wurde
      if (newerBuffer == 1) {
        pointerToOlderBuffer = buffers.buf0.w;
        pointerToNewerBuffer = buffers.buf1.w;
      } else {
        pointerToOlderBuffer = buffers.buf1.w;
        pointerToNewerBuffer = buffers.buf0.w;
      }
      transmitFrame(
        N_IQ_SAMPLES_PER_BUFFER * sizeof(IQ),
        pointerToOlderBuffer,
        N_IQ_SAMPLES_PER_BUFFER * sizeof(IQ),
        pointerToNewerBuffer,
        &hiwdg
      );
      state = IDLE;
    }
    if (state is RECEIVING_NORMAL and
      __HAL_MRSUBG_GET_RFSEQ_IRQ_STATUS() & MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_RX_OK_F
    ) {
      __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_RX_OK_F);
      uint16_t const rssiOnSync = LL_MRSubG_GetRssiLevelOnSync();
      uint32_t const roundTripTime = getMs()-pingTimestampMs;
      printf("Pong! after %"PRIu32" ms | rssi=%"PRIu16""rn, roundTripTime, rssiOnSync);
      // TODO: Wenn das packet daten hätte, dann hier auslesen
      state = IDLE;
    }
    uint16_t const pingTimeoutMs = 20;
    if (state is RECEIVING_NORMAL and msHavePassed(pingTimestampMs, pingTimeoutMs)) {
      __HAL_MRSUBG_STROBE_CMD(CMD_SABORT);
      state = ABORTING_AFTER_RECEIVE_NORMAL_TIMEOUT;
    }
    if (state is ABORTING_AFTER_RECEIVE_NORMAL_TIMEOUT and
      __HAL_MRSUBG_GET_RFSEQ_IRQ_STATUS() & MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_SABORT_DONE_F
    ) {
      __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_SABORT_DONE_F);
      printf("Timeout! Ping wasnt pong'd after %d ms"rn, pingTimeoutMs);
      state = IDLE;
    }

    if (state is BEACON and HAL_GetTick() - lastBeaconTime >= beaconPeriodMs) {
      startTx();
      waitForTxDone();
      lastBeaconTime = HAL_GetTick();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource and SYSCLKDivider
  */
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_RC64MPLL;
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
  * @brief MRSUBG Initialization Function
  * @param None
  * @retval None
  */
static void MX_MRSUBG_Init(void)
{

  /* USER CODE BEGIN MRSUBG_Init 0 */

  /* USER CODE END MRSUBG_Init 0 */

  /* USER CODE BEGIN MRSUBG_Init 1 */

  /* USER CODE END MRSUBG_Init 1 */

  /** Configures the radio parameters
  */
  MRSUBG_RadioInitStruct.lFrequencyBase = 433000000;
  MRSUBG_RadioInitStruct.xModulationSelect = MOD_OOK;
  MRSUBG_RadioInitStruct.lDatarate = 38400;
  MRSUBG_RadioInitStruct.lFreqDev = 20000;
  MRSUBG_RadioInitStruct.lBandwidth = 800000;
  MRSUBG_RadioInitStruct.dsssExp = 0;
  MRSUBG_RadioInitStruct.outputPower = 14;
  MRSUBG_RadioInitStruct.PADrvMode = PA_DRV_TX_HP;
  HAL_MRSubG_Init(&MRSUBG_RadioInitStruct);

  /** Configures the packet parameters
  */
  MRSUBG_PacketSettingsStruct.PreambleLength = 8;
  MRSUBG_PacketSettingsStruct.PostambleLength = 0;
  MRSUBG_PacketSettingsStruct.SyncLength = 31;
  MRSUBG_PacketSettingsStruct.SyncWord = 0x88888888;
  MRSUBG_PacketSettingsStruct.FixVarLength = FIXED;
  MRSUBG_PacketSettingsStruct.PreambleSequence = PRE_SEQ_0101;
  MRSUBG_PacketSettingsStruct.PostambleSequence = POST_SEQ_0101;
  MRSUBG_PacketSettingsStruct.CrcMode = PKT_NO_CRC;
  MRSUBG_PacketSettingsStruct.Coding = CODING_NONE;
  MRSUBG_PacketSettingsStruct.DataWhitening = DISABLE;
  MRSUBG_PacketSettingsStruct.LengthWidth = BYTE_LEN_1;
  MRSUBG_PacketSettingsStruct.SyncPresent = ENABLE;
  HAL_MRSubG_PacketBasicInit(&MRSUBG_PacketSettingsStruct);
  /* USER CODE BEGIN MRSUBG_Init 2 */

  /* USER CODE END MRSUBG_Init 2 */

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
  htim2.Init.Prescaler = 64-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFF-1;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 0;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 0xFFFF-1;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim16, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */

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
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11|GPIO_PIN_10|GPIO_PIN_9|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB11 PB10 PB9 PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_10|GPIO_PIN_9|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA14 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_11|PWR_GPIO_BIT_10|PWR_GPIO_BIT_9|PWR_GPIO_BIT_8);

  /**/
  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_14|PWR_GPIO_BIT_15);

  /**/
  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_11|PWR_GPIO_BIT_10|PWR_GPIO_BIT_9|PWR_GPIO_BIT_8);

  /**/
  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_14|PWR_GPIO_BIT_15);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(GPIOB_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPIOB_IRQn);

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
  //    printf(""rn);
  //
  //  for (size_t i = 0; i < UART_RX_BUFFER_SIZE; i++) {
  //    uartRxBuffer[i] = 0; // Clear the buffer after processing
  //  }
}

void startNormalRx(void) {
  __HAL_MRSUBG_SET_DATABUFFER_SIZE(N_IQ_SAMPLES_PER_BUFFER * sizeof(IQ));
  __HAL_MRSUBG_SET_DATABUFFER0_POINTER((uint32_t)&(buffers.buf0.iq[0].u32));
  __HAL_MRSUBG_SET_DATABUFFER1_POINTER((uint32_t)&(buffers.buf1.iq[0].u32));

  __HAL_MRSUBG_SET_RX_MODE(RX_NORMAL);
  __HAL_MRSUBG_STROBE_CMD(CMD_RX);
}

void startIqRx(void) {
  __HAL_MRSUBG_SET_DATABUFFER_SIZE(N_IQ_SAMPLES_PER_BUFFER * sizeof(IQ));
  __HAL_MRSUBG_SET_DATABUFFER0_POINTER((uint32_t)&(buffers.buf0.iq[0].u32));
  __HAL_MRSUBG_SET_DATABUFFER1_POINTER((uint32_t)&(buffers.buf1.iq[0].u32));

  __HAL_MRSUBG_SET_RX_MODE(RX_IQ_SAMPLING);
  __HAL_MRSUBG_STROBE_CMD(CMD_RX);
}

void startTx(void) {
  /* Payload length config */
  HAL_MRSubG_PktBasicSetPayloadLength(PAYLOAD_LENGTH);
  /* Set TX Mode to Normal Mode*/
  __HAL_MRSUBG_SET_TX_MODE(TX_NORMAL);
  /* Set the pointer to the data buffer */
  __HAL_MRSUBG_SET_DATABUFFER0_POINTER((uint32_t)&buffers);
  /* Send the TX command */
  __HAL_MRSUBG_STROBE_CMD(CMD_TX);
}

void waitForTxDone() {
  while((__HAL_MRSUBG_GET_RFSEQ_IRQ_STATUS() & MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_TX_DONE_F) == 0) {
    HAL_IWDG_Refresh(&hiwdg);
  };
  __HAL_MRSUBG_CLEAR_RFSEQ_IRQ_FLAG(MR_SUBG_GLOB_STATUS_RFSEQ_IRQ_STATUS_TX_DONE_F);
}

void HAL_GPIO_EXTI_Callback(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
  if (GPIOx == GPIOB && GPIO_Pin == GPIO_PIN_7) {
    extiTriggered = true;
  }
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
    printf("ERROR"rn);
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
       ex: printf("Wrong parameters value: file %s on line %d"rn, file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
