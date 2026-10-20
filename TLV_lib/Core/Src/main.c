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

#include "tlv.h"
#include "msg_handling.h"
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
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
typedef struct
{
    protocol_app_t *app;
    UART_HandleTypeDef *uart;
} tlv_context_t;  // Typ rozróżniający kontekst protocol_app od UART

static tlv_t tlv;
static tlv_context_t tlv_context;
static protocol_app_t protocol;

// Bufor cykliczny
#define UART_RX_BUFFER_SIZE 256U
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1U) // Maska, która pozwala przesuwać cyklicznie wskaźnik

static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

static volatile uint16_t uart_rx_head = 0U;
static volatile uint16_t uart_rx_tail = 0U;

// Utracone bajty
static volatile uint32_t uart_rx_overflow_count = 0U;
static volatile uint32_t uart_rx_error_count = 0U;

static uint8_t uart_rx_byte;

// Debug
volatile uint32_t uart_rx_count = 0;
volatile uint8_t uart_last_byte = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static int uart_transmit(void *context, size_t lenght, const uint8_t* data) // Wysyła dane przez UART
{
	  tlv_context_t *ctx = (tlv_context_t *)context;

	  HAL_StatusTypeDef status = HAL_UART_Transmit(ctx->uart, (uint8_t*)data, (uint16_t)lenght, 100U);

	  return (status == HAL_OK) ? 0 : -1;
}

static void uart_receive_start(void) // Rozpoczyna odbiór danych przez UART
{
	  if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) != HAL_OK)
	  {
		  uart_rx_error_count++;
	  }
}

static void uart_process_rx_buffer(void) // Obsługuje dane z bufora
{
    while (uart_rx_tail != uart_rx_head) // Jeżeli w buforze znajduje się nieobsłużony element
    {
        uint16_t tail = uart_rx_tail;
        uint8_t byte = uart_rx_buffer[tail]; // Bierzemy najstarszy element

        uart_rx_tail = (tail + 1U) & UART_RX_BUFFER_MASK; // Przesuwamy tail o 1 i zapetlamy za pomocą maski (jeżeli tail + 1 = 256 to uart_rx_tail = 0)


        tlv_feed(&tlv, byte);
        // Dzięki takiej obsłudze parser działa poza przerwaniem
    }
}

static void tlv_on_frame(void *context, uint8_t type, uint8_t length, const uint8_t *payload) // Wywołuje protocol_app_on_frame z odpowiednim typem kontekstu
{
    tlv_context_t *ctx = (tlv_context_t *)context;

    protocol_app_on_frame(ctx->app, type, length, payload);
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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  protocol_app_init(&protocol, &tlv);

  tlv_context.app = &protocol;
  tlv_context.uart = &huart1;

  if (tlv_init(&tlv, TLV_MAX_PAYLOAD_SIZE, tlv_on_frame, uart_transmit, &huart1) != TLV_OK)
  {
	  Error_Handler();
  }

  uart_receive_start();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	 uart_process_rx_buffer();

	 protocol_app_process(&protocol, HAL_GetTick());
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1) // Implementacja z buforem cyklicznym chroniącym przed utratą bajtów
	{
		uint16_t head = uart_rx_head;
		uint16_t next = (head + 1U) & UART_RX_BUFFER_MASK;

		uart_last_byte = uart_rx_byte;
		uart_rx_count++;

		if (next != uart_rx_tail) // Jeżeli w buforze jest miejsce bajt jest zapisywany
		{
			uart_rx_buffer[head] = uart_rx_byte;
			uart_rx_head = next;
		}
		else
		{
			// Bufor pełny — bajt zostaje utracony
			uart_rx_overflow_count++;
		}

		// Ponownie uruchom odbiór jednego bajtu
		if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) != HAL_OK)
		{
			uart_rx_error_count++;
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
