/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "i2c.h"
#include "quadspi.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "quadspi_1.h"
#include "SX1278.h"
#include "ring_buffer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define esp_uart huart1
#define ESP_TRANSMIT_BUFF_SIZE 64
#define ESP_PARSE_BUFF_SIZE 64

char echo_off[] = "ATE0";
char init_wifi[] = "AT+CWINIT=1";
char wifi_mode[] = "AT+CWMODE=1";
char wifi_no_autoconnect[] = "AT+CWAUTOCONN=0";
char wifi_connect[] = "AT+CWJAP=\"DM_TT\",\"W0B35d8y\"";
char ntp_pool[] = "AT+CIPSNTPCFG=1,2,\"pl.pool.ntp.org\"";
char query_time[] = "AT+CIPSNTPTIME?";
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
RingBuffer_t RxBuffer;
uint8_t UartRxTmp;
uint8_t NewLine;

SX1278_hw_t SX1278_hw;
SX1278_t SX1278;

uint8_t master = 1;
int ret;

char buffer[512];

int message;
int message_length;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void ESP_Transmit_Log(char* s);
void ESP_Receive_Log(char* s);

void ESP_Transmit(char* s);

char* EspGetLine(void);
void ESP_WaitForOK(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//uint8_t writebuf[] = "Hello world from QSPI";
uint8_t writebuf[] = "Giga ZYGA";
uint8_t Readbuf[100];
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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_QUADSPI_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_GPIO_ReadPin(BTN_UNLOCK_GPIO_Port, BTN_UNLOCK_Pin) == GPIO_PIN_RESET)
  {
    HAL_GPIO_TogglePin(LED_VALVE_GPIO_Port, LED_VALVE_Pin);
  }

  if (HAL_GPIO_ReadPin(FLOW_BTN_IN_GPIO_Port, FLOW_BTN_IN_Pin) == GPIO_PIN_RESET)
  {
    HAL_GPIO_TogglePin(LED_WIFI_GPIO_Port, LED_WIFI_Pin);
  }

  HAL_UART_Receive_IT(&esp_uart, &UartRxTmp, 1);

  ESP_Transmit("AT");
  ESP_WaitForOK();

  ESP_Transmit(echo_off);
  ESP_WaitForOK();

  ESP_Transmit(init_wifi);
  ESP_WaitForOK();

  ESP_Transmit(wifi_mode);
  ESP_WaitForOK();

  ESP_Transmit(wifi_no_autoconnect);
  ESP_WaitForOK();

  ESP_Transmit(wifi_connect);
  ESP_WaitForOK();

  ESP_Transmit(ntp_pool);
  ESP_WaitForOK();

  ESP_Transmit(query_time);
  ESP_WaitForOK();

  ESP_Transmit(query_time);
  ESP_WaitForOK();

  //initialize LoRa module
  SX1278_hw.dio0.port = LORA_DIO0_GPIO_Port;
  SX1278_hw.dio0.pin = LORA_DIO0_Pin;
  SX1278_hw.nss.port = LORA_NSS_GPIO_Port;
  SX1278_hw.nss.pin = LORA_NSS_Pin;
  SX1278_hw.reset.port = LORA_RESET_GPIO_Port;
  SX1278_hw.reset.pin = LORA_RESET_Pin;
  SX1278_hw.spi = &hspi1;

  SX1278.hw = &SX1278_hw;

  printf("Configuring LoRa module\r\n");
  SX1278_init(&SX1278, SX1278_FREQ_433MHz, SX1278_POWER_20DBM, SX1278_LORA_SF_7,
              SX1278_LORA_BW_125KHZ, SX1278_LORA_CR_4_5, SX1278_LORA_CRC_DIS, 8, SX127X_SYNC_WORD);
  printf("Done configuring LoRaModule\r\n");

  if (master == 1)
  {
    SX1278_LoRaEntryTx(&SX1278, 16, 2000);
  }
  else
  {
    SX1278_LoRaEntryRx(&SX1278, 16, 2000);
  }

  if (CSP_QUADSPI_Init() != HAL_OK) Error_Handler();

  // if (CSP_QSPI_Erase_Chip() != HAL_OK) Error_Handler();

  if (CSP_QSPI_EnableMemoryMappedMode() != HAL_OK) Error_Handler();

  HAL_QSPI_Abort(&hqspi);

  if (CSP_QSPI_WriteMemory(writebuf, 0, sizeof(writebuf)) != HAL_OK) Error_Handler();

  if (CSP_QSPI_EnableMemoryMappedMode() != HAL_OK) Error_Handler();

  //  if (CSP_QSPI_Read(Readbuf, 0, 100) != HAL_OK) Error_Handler();

  memcpy(Readbuf, (uint8_t *) 0x90000000, sizeof(writebuf));

  /*if (HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0, sizeof(uint16_t), writebuf, sizeof(writebuf), 1000) == HAL_OK)
  {
    HAL_GPIO_TogglePin(LED_OK_GPIO_Port, LED_OK_Pin);
  }*/
  char wbuf[30] = {0};
  if (HAL_I2C_Mem_Read(&hi2c2, 0xA0, 0, sizeof(uint16_t), wbuf, sizeof(wbuf), 1000) == HAL_OK)
  {
    HAL_GPIO_TogglePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  for (uint16_t i = 0; i < 500; i++)
  {
    // HAL_GPIO_TogglePin(BUZZER_OUT_GPIO_Port, BUZZER_OUT_Pin);
    HAL_GPIO_TogglePin(VALVE_OUT_GPIO_Port, VALVE_OUT_Pin);
    HAL_GPIO_TogglePin(LED_IMP_GPIO_Port, LED_IMP_Pin);
    HAL_Delay(1000);
  }
  while (1)
  {
    if (master == 1)
    {
      printf("Master ...\r\n");
      HAL_Delay(1000);
      printf("Sending package...\r\n");

      message_length = sprintf(buffer, "Hello %d", message);
      ret = SX1278_LoRaEntryTx(&SX1278, message_length, 2000);
      printf("Entry: %d\r\n", ret);

      printf("Sending %s\r\n", buffer);
      ret = SX1278_LoRaTxPacket(&SX1278, (uint8_t*) buffer, message_length, 2000);
      message += 1;

      printf("Transmission: %d\r\n", ret);
      printf("Package sent...\r\n");
    }
    else
    {
      printf("Slave ...\r\n");
      HAL_Delay(800);
      printf("Receiving package...\r\n");

      ret = SX1278_LoRaRxPacket(&SX1278);
      printf("Received: %d\r\n", ret);
      if (ret > 0) {
        SX1278_read(&SX1278, (uint8_t*) buffer, ret);
        printf("Content (%d): %s\r\n", ret, buffer);
      }
      printf("Package received ...\r\n");
    }

    HAL_GPIO_TogglePin(LED_WIFI_GPIO_Port, LED_WIFI_Pin);
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin);
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(LED_OK_GPIO_Port, LED_OK_Pin);
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(LED_VALVE_GPIO_Port, LED_VALVE_Pin);
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(LED_IMP_GPIO_Port, LED_IMP_Pin);
    HAL_Delay(1000);
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void ESP_Transmit_Log(char* s)
{
  // HAL_UART_Transmit(&huart2, (uint8_t*)"T: ", 4, 1000);
  // HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen((char*)s), 1000);
}

void ESP_Receive_Log(char* s)
{
  // HAL_UART_Transmit(&huart2, (uint8_t*)"R: ", 4, 1000);
  // HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen((char*)s), 1000);
  // HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 4, 1000);
}

void ESP_Transmit(char* s)
{
  uint8_t Tmp[ESP_TRANSMIT_BUFF_SIZE] = {};
  strcat(Tmp, (char*)s);
  strcat(Tmp, "\r\n");
  ESP_Transmit_Log(Tmp);
  HAL_UART_Transmit(&esp_uart, (uint8_t*)Tmp, strlen(Tmp), 1000);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &esp_uart)
  {
    RB_Write(&RxBuffer, UartRxTmp);

    if (UartRxTmp == '\n')
    {
      NewLine++;
    }
    HAL_UART_Receive_IT(&esp_uart, &UartRxTmp, 1);
  }
}

char* EspGetLine(void)
{
  if (NewLine)
  {
    static char BuffTmp[ESP_PARSE_BUFF_SIZE];
    uint16_t i = 0;

    while (i < ESP_PARSE_BUFF_SIZE)
    {
      RB_Read(&RxBuffer, (uint8_t*)&BuffTmp[i]);

      if (BuffTmp[i] == '\r')
      {
        BuffTmp[i] = '\0';
      }

      if (BuffTmp[i] == '\n')
      {
        BuffTmp[i] = '\0';
        break;
      }
      i++;
    }
    NewLine--;

    return BuffTmp;
  }
  return 0;
}

void ESP_WaitForOK(void)
{
  char* BufferToParse;

  while (1)
  {
    BufferToParse = EspGetLine();

    if (BufferToParse)
    {
      ESP_Receive_Log(BufferToParse);

      if (0 == strncmp((char*)BufferToParse, "OK", ESP_PARSE_BUFF_SIZE))
      {
        break;
      }
    }
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
  __disable_irq();
  while (1)
  {
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
