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
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */
uint8_t recv_bin[1000] = {0};
uint8_t recv_cnt = 0;
uint8_t reception_started = 0;    //开始接收
uint32_t laser_on_start_time = 0;  //激光开始亮的时间
uint8_t is_count = 0;
uint8_t flag = 0;
uint8_t start = 0;
uint32_t three_second_counter = 0;
uint8_t is_show = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

uint8_t light_recv_bin(uint8_t *recv_bin, uint8_t *recv_cnt);
void bin_to_hex_display(uint8_t *recv_bin, uint8_t *recv_cnt);
uint8_t wait_for_laser_start_on_signal(void);    //检测激光开始信号
//void bin_to_hex_display(uint8_t *recv_bin);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t light_recv_bin(uint8_t *recv_bin, uint8_t *recv_cnt)  {
	 if (*recv_cnt >= 10000) {
	        *recv_cnt = 0;
	        memset(recv_bin, 0, sizeof(recv_bin)); // 清空缓存
	    }
   // 读取光敏DO电平
   recv_bin[*recv_cnt] = (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13) == 0) ? 1 : 0;
   (*recv_cnt)++;
   if(*recv_cnt == 4 * 40){
	   is_show = 1;
   }
   HAL_Delay(500);
   return 1;
}

char nibble_to_hex_char(uint8_t nibble) {
    if (nibble < 10) {
        return '0' + nibble;
    } else {
        return 'A' + (nibble - 10);
    }
}

void bin_to_hex_display(uint8_t *recv_bin, uint8_t *recv_cnt) {
    #define MAX_CHARS_PER_LINE 16
    #define MAX_LINES 4
    uint16_t total_bits = *recv_cnt;
    uint16_t hex_len = (total_bits + 3) / 4;
    char hex_str[hex_len + 1];
    uint16_t hex_index = 0;

    memset(hex_str, 0, sizeof(hex_str));

    for (uint16_t i = 0; i < total_bits; i += 4) {
        uint8_t nibble = 0;

        for (int j = 0; j < 4; j++) {
            if ((i + j) < total_bits) {
                nibble |= (recv_bin[i + j] & 0x01) << j;
            }
        }
        if (hex_index < hex_len) {
            hex_str[hex_index++] = nibble_to_hex_char(nibble);
        }
    }


    OLED_Clear();
    OLED_ShowString(0,0,"receive start",8,1);

    uint16_t current_str_pos = 0;

    for (uint8_t line = 0; line < MAX_LINES && current_str_pos < hex_len; line++) {
        char display_line[MAX_CHARS_PER_LINE + 1];
        uint8_t chars_to_copy = MAX_CHARS_PER_LINE;

        if (hex_len - current_str_pos < MAX_CHARS_PER_LINE) {
            chars_to_copy = hex_len - current_str_pos;
        }

        strncpy(display_line, hex_str + current_str_pos, chars_to_copy);
        display_line[chars_to_copy] = '\0';

        OLED_ShowString(0, 15 + line * 15, display_line, 8, 2);

        current_str_pos += chars_to_copy;
    }

    if (current_str_pos < hex_len) {
        OLED_ShowString(0, 15 + MAX_LINES * 15, "...", 8, 1);
    }

    OLED_Refresh();
}

uint8_t wait_for_laser_start_signal(void) {
    uint32_t current_time = HAL_GetTick();
    static uint8_t waiting_start = 0;

    // 检测激光是否亮
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == 0) {
        if (!waiting_start) {
            // 第一次检测到激光亮，记录开始时间
            laser_on_start_time = current_time;
            waiting_start = 1;
            __HAL_TIM_SET_COUNTER(&htim2,0);
            is_count = 1;
            OLED_Clear();
            OLED_ShowString(0,0,"Laser Detected",8,1);
            OLED_ShowString(0,15,"Wait 3s...",8,1);
            OLED_Refresh();
        } else {
            // 持续检测激光亮
            if (start) {
                // 激光持续亮3秒，开始信号有效
                waiting_start = 0;
                is_count = 0;
                OLED_Clear();
                OLED_ShowString(0,0,"Start Signal OK!",8,1);
                OLED_Clear();
                return 1; // 开始信号有效
            }
        }
    } else {
        // 激光熄灭，重置检测
        waiting_start = 0;
        is_count = 0;
        OLED_ShowString(0,0,"Wait for Laser",8,1);
        OLED_ShowString(0,15,"3s Start Signal",8,1);
        OLED_Refresh();
    }

    return 0; // 开始信号未完成
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // 检查是不是我们配置的 TIM2
  if (htim->Instance == TIM2)
  {
	  if(is_count){
			three_second_counter++; // 每次中断（1秒）加 1
			if(three_second_counter >= 3){
				three_second_counter = 0;
				is_count = 0;
				start = 1;
			}
	  }
  }
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
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13)==GPIO_PIN_SET)
	  {
		  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_RESET);
	  }
	  else{
		  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
		  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_SET);
	  }

  // 等待激光开始信号（持续亮3秒）
	 	if (!reception_started) {
	 		  if (wait_for_laser_start_signal()) {
	 			  reception_started = 1;
	 			  recv_cnt = 0; // 重置接收计数器
	 			  //OLED_Clear();
	 			  OLED_ShowString(0,0,"Receiving...",8,1);
	 			  OLED_Refresh();
	 		  }
	 		  HAL_Delay(250);
	 		  continue;
	 	  }
	 	  // 开始接收数据
	 	  if (light_recv_bin(recv_bin, &recv_cnt) == 1) {
	 		  // 可以在这里添加接收完成的条件判断
//	 		  if (recv_cnt >= 20) { // 例如接收20位后显示
	 		  if(is_show == 1){
	 			  bin_to_hex_display(recv_bin, &recv_cnt);
	 		  }
//	 			  //reception_started = 0; // 重置为等待开始信号状态
//	 			  HAL_Delay(3000);
//	 			  OLED_Clear();
//	 			  OLED_ShowString(0,0,"Wait for Laser",8,1);
//	 			  OLED_ShowString(0,15,"3s Start Signal",8,1);
//	 			  OLED_Refresh();
//	 		  }
	 		  // 实时计算并显示十六进制（收多少位算多少位）
//	 		        uint32_t hex_result = 0;
//	 		        for (uint8_t i = 0; i < recv_cnt; i++) {
//	 		            hex_result |= (uint32_t)recv_bin[i] << i;
//	 	  }
//
//	 		       // 用字符串显示十六进制（避免原OLED_ShowString显示数值错误）
//	 		           char hex_str[10] = {0}, hex_char[] = "0123456789ABCDEF";
//	 		           uint32_t temp_hex = hex_result;
//	 		           uint8_t str_idx = 0;
//	 		           for (int i = 28; i >= 0; i -= 4) { // 32位数值，每次取4位
//	 		               uint8_t digit = (temp_hex >> i) & 0x0F;
//	 		               if (digit != 0 || str_idx != 0) hex_str[str_idx++] = hex_char[digit];
//	 		           }
//	 		           if (str_idx == 0) hex_str[str_idx++] = '0';
//	 		           OLED_ShowString(40, 32, hex_str, 8, 1);
//
//	 		           OLED_Refresh();
	 	  }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
