/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdarg.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    int16_t ch[5];  
    uint8_t s[2];   
} rc_info_t;


typedef struct {
    int16_t x;      
    int16_t y;      
    int16_t z;     
    uint8_t press_l; 
    uint8_t press_r; 
} mouse_info_t;
typedef struct{
		uint16_t v;
}KEY;

typedef struct {
    rc_info_t rc;
    mouse_info_t mouse;
    KEY key;       
} RC_ctrl_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SBUS_RX_BUF_NUM 32
#define RC_FRAME_LENGTH 18
#define RC_CH_VALUE_OFFSET 1024
extern DMA_HandleTypeDef hdma_usart3_rx;
uint8_t uart1_tx_busy;
uint8_t rx1_buf[32];	
uint8_t rx2_buf[32];
RC_ctrl_t rc_ctrl;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    if(huart == &huart1){
        uart1_tx_busy = 0;
    }
}

void usart1_tx_dma_enable(uint8_t* tx_buf,uint16_t len){
	if(uart1_tx_busy)return;
	HAL_UART_Transmit_DMA(&huart1,tx_buf,len);
	uart1_tx_busy=1;
}

void RC_Init(uint8_t*rx1_buf,uint8_t*rx2_buf,uint16_t dma_buf_num){
	SET_BIT(huart3.Instance->CR3,USART_CR3_DMAR);
	
	__HAL_UART_ENABLE_IT(&huart3,UART_FLAG_IDLE);
	__HAL_DMA_DISABLE(&hdma_usart3_rx);
	
	while(hdma_usart3_rx.Instance->CR&DMA_SxCR_EN){
		__HAL_DMA_DISABLE(&hdma_usart3_rx);
	}
	
	hdma_usart3_rx.Instance->PAR=(uint32_t)&USART3->DR;
	hdma_usart3_rx.Instance->M0AR=(uint32_t)rx1_buf;
	hdma_usart3_rx.Instance->M1AR=(uint32_t)rx2_buf;
	hdma_usart3_rx.Instance->NDTR=dma_buf_num;
	
	SET_BIT(hdma_usart3_rx.Instance->CR,DMA_SxCR_DBM);
	__HAL_DMA_ENABLE(&hdma_usart3_rx);
}

void uart_printf(const char*fmt,...){

	  if(uart1_tx_busy) {
        return;
    }
		uint8_t tx_buf[256]={0};
		va_list ap;
		uint16_t len;
		va_start(ap,fmt);
		len=vsprintf((char*)tx_buf,fmt,ap);
		va_end(ap);
		usart1_tx_dma_enable(tx_buf,len);
		
}


static void sbus_to_rc(volatile const uint8_t *sbus_buf, RC_ctrl_t *rc_ctrl)
{
 if (sbus_buf == NULL || rc_ctrl == NULL)
 {
 return;
 }
 rc_ctrl->rc.ch[0] = (sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff; //!< Channel 0
 rc_ctrl->rc.ch[1] = ((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff; //!< Channel 1
 rc_ctrl->rc.ch[2] = ((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) | //!< Channel 2
 (sbus_buf[4] << 10)) &0x07ff;
 rc_ctrl->rc.ch[3] = ((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff; //!< Channel 3
 rc_ctrl->rc.s[0] = ((sbus_buf[5] >> 4) & 0x0003); //!< Switch left
 rc_ctrl->rc.s[1] = ((sbus_buf[5] >> 4) & 0x000C) >> 2; //!< Switch right
 rc_ctrl->mouse.x = sbus_buf[6] | (sbus_buf[7] << 8); //!< Mouse X axis
 rc_ctrl->mouse.y = sbus_buf[8] | (sbus_buf[9] << 8); //!< Mouse Y axis
 rc_ctrl->mouse.z = sbus_buf[10] | (sbus_buf[11] << 8); //!< Mouse Z axis
 rc_ctrl->mouse.press_l = sbus_buf[12]; //!< Mouse Left Is Press ?
 rc_ctrl->mouse.press_r = sbus_buf[13]; //!< Mouse Right Is Press ?
 rc_ctrl->key.v = sbus_buf[14] | (sbus_buf[15] << 8); //!< KeyBoard value
 rc_ctrl->rc.ch[4] = sbus_buf[16] | (sbus_buf[17] << 8); //NULL
 rc_ctrl->rc.ch[0] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[1] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[2] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[3] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[4] -= RC_CH_VALUE_OFFSET;

}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
	if(huart==&huart3){
		
//		HAL_GPIO_TogglePin(GPIOH,GPIO_PIN_10);
//		HAL_GPIO_TogglePin(GPIOH,GPIO_PIN_11);
		__HAL_DMA_DISABLE(&hdma_usart3_rx);
		
		static uint8_t real_time_length;
		
		if((hdma_usart3_rx.Instance->CR&DMA_SxCR_CT)==RESET){
			
			real_time_length=SBUS_RX_BUF_NUM-hdma_usart3_rx.Instance->NDTR;
			hdma_usart3_rx.Instance->NDTR=SBUS_RX_BUF_NUM;
			hdma_usart3_rx.Instance->CR |= DMA_SxCR_CT;
		
			if(real_time_length==RC_FRAME_LENGTH){
				sbus_to_rc(rx1_buf, &rc_ctrl);
        uart_printf("RC Ch0: %d, Ch1: %d, Ch2: %d, Ch3: %d\r\n",
                   rc_ctrl.rc.ch[0], rc_ctrl.rc.ch[1], 
                   rc_ctrl.rc.ch[2], rc_ctrl.rc.ch[3]);
        uart_printf("Mouse X: %d, Y: %d, Press L: %d, R: %d\r\n",
                   rc_ctrl.mouse.x, rc_ctrl.mouse.y,
                   rc_ctrl.mouse.press_l, rc_ctrl.mouse.press_r);
        uart_printf("Key: 0x%04X, Switch L: %d, R: %d\r\n\r\n",
                   rc_ctrl.key.v, rc_ctrl.rc.s[0], rc_ctrl.rc.s[1]);
			}
			__HAL_DMA_ENABLE(&hdma_usart3_rx);
		}
		else if(hdma_usart3_rx.Instance->CR&DMA_SxCR_CT){
			real_time_length=SBUS_RX_BUF_NUM-hdma_usart3_rx.Instance->NDTR;
			hdma_usart3_rx.Instance->NDTR=SBUS_RX_BUF_NUM;
			hdma_usart3_rx.Instance->CR &= ~(DMA_SxCR_CT);
		
			if(real_time_length==RC_FRAME_LENGTH){
				sbus_to_rc(rx2_buf, &rc_ctrl);
        uart_printf("RC Ch0: %d, Ch1: %d, Ch2: %d, Ch3: %d\r\n",
                   rc_ctrl.rc.ch[0], rc_ctrl.rc.ch[1], 
                   rc_ctrl.rc.ch[2], rc_ctrl.rc.ch[3]);
        uart_printf("Mouse X: %d, Y: %d, Press L: %d, R: %d\r\n",
                   rc_ctrl.mouse.x, rc_ctrl.mouse.y,
                   rc_ctrl.mouse.press_l, rc_ctrl.mouse.press_r);
        uart_printf("Key: 0x%04X, Switch L: %d, R: %d\r\n\r\n",
                   rc_ctrl.key.v, rc_ctrl.rc.s[0], rc_ctrl.rc.s[1]);
			}
			__HAL_DMA_ENABLE(&hdma_usart3_rx);
			
		}
	}
}


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  RC_Init(rx1_buf, rx2_buf, SBUS_RX_BUF_NUM);

  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx1_buf, SBUS_RX_BUF_NUM);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
uart_printf("Key: 0x%04X, Switch L: %d, R: %d\r\n\r\n",
                   rc_ctrl.key.v, rc_ctrl.rc.s[0], rc_ctrl.rc.s[1]);

		HAL_Delay(250);
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
