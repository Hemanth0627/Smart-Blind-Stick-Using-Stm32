
#include "main.h"
#include "hcsr04.h"
#include "gps.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

#define OBSTACLE_THRESHOLD_CM 70.0f 
#define BUZZER_DURATION_MS    250U  
#define MOTOR_DURATION_MS     250U  
#define SOS_HOLD_MS           3000U 

static GPS_Handle gps;
static uint8_t gps_rx_byte;
static GPIO_PinState prevIrState = GPIO_PIN_SET;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void uart_send(UART_HandleTypeDef *uart, const char *text);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    GPS_Init(&gps);

    if (HAL_UART_Receive_IT(&huart1, &gps_rx_byte, 1U) != HAL_OK) Error_Handler();

    uint8_t isBuzzerActive = 0U;
    uint8_t isMotorActive = 0U;
    uint32_t buzzerOffTime = 0U;
    uint32_t motorOffTime = 0U;
    uint32_t button_start_time = 0U;
    uint8_t sos_triggered = 0U;

    while (1) {
        const uint32_t currentMillis = HAL_GetTick();
        GPS_ProcessLine(&gps);
        if (gps.update_ready && gps.fix_valid) {
            char bt_buffer[64];
            const int len = snprintf(bt_buffer, sizeof(bt_buffer),
                                     "Track: %f, %f\r\n", gps.latitude, gps.longitude);
            gps.update_ready = 0U;
            if (len > 0) {
                (void)HAL_UART_Transmit(&huart2, (uint8_t *)bt_buffer,
                                        (uint16_t)len, 1000U);
            }
        }

     
        const float distFront = HCSR04_GetDistance(TRIG_FRONT_PORT, TRIG_FRONT_PIN,
                                                   ECHO_FRONT_PORT, ECHO_FRONT_PIN);
        HAL_Delay(15U);
        const float distLeft = HCSR04_GetDistance(TRIG_LEFT_PORT, TRIG_LEFT_PIN,
                                                  ECHO_LEFT_PORT, ECHO_LEFT_PIN);
        HAL_Delay(15U);
        const float distRight = HCSR04_GetDistance(TRIG_RIGHT_PORT, TRIG_RIGHT_PIN,
                                                   ECHO_RIGHT_PORT, ECHO_RIGHT_PIN);

        
        const uint8_t obstacleDetected =
            (distFront > 0.1f && distFront < OBSTACLE_THRESHOLD_CM) ||
            (distLeft  > 0.1f && distLeft  < OBSTACLE_THRESHOLD_CM) ||
            (distRight > 0.1f && distRight < OBSTACLE_THRESHOLD_CM);

        
        const GPIO_PinState currIrState = HAL_GPIO_ReadPin(IR_SENSOR_PORT, IR_SENSOR_PIN);
        uint8_t irTriggered = 0U;
        if (prevIrState == GPIO_PIN_SET && currIrState == GPIO_PIN_RESET) {
            irTriggered = 1U;
        }
        prevIrState = currIrState;

        
        if (obstacleDetected || irTriggered) {
            if (isBuzzerActive == 0U) {
                HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
                isBuzzerActive = 1U;
            }
            buzzerOffTime = currentMillis + BUZZER_DURATION_MS;
        }
        if (obstacleDetected || irTriggered) {
            if (isMotorActive == 0U) {
                HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN, GPIO_PIN_SET);
                isMotorActive = 1U;
            }
            motorOffTime = currentMillis + MOTOR_DURATION_MS;
        }
        if (isBuzzerActive && (int32_t)(currentMillis - buzzerOffTime) >= 0) {
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
            isBuzzerActive = 0U;
        }
        if (isMotorActive && (int32_t)(currentMillis - motorOffTime) >= 0) {
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN, GPIO_PIN_RESET);
            isMotorActive = 0U;
        }

        
        if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET) {
            if (button_start_time == 0U) button_start_time = currentMillis;
            if (((currentMillis - button_start_time) > SOS_HOLD_MS) && !sos_triggered) {
                sos_triggered = 1U;
                char bt_buffer[128];
                if (gps.fix_valid) {
                    (void)snprintf(bt_buffer, sizeof(bt_buffer),
                                   "!!! SOS !!!\r\nUser Help Needed!\r\nlat: %.6f\r\nlon: %.6f\r\n",
                                   gps.latitude, gps.longitude);
                } else {
                    (void)snprintf(bt_buffer, sizeof(bt_buffer),
                                   "!!! SOS !!!\r\nGPS Searching...\r\n");
                }
                uart_send(&huart2, bt_buffer);
                uart_send(&huart1, bt_buffer);
                uart_send(&huart2, "SOS Sent\r\n");
            }
        } else {
            button_start_time = 0U;
            sos_triggered = 0U;
        }

        HAL_Delay(5U);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        GPS_HandleRxByte(&gps, gps_rx_byte);
        (void)HAL_UART_Receive_IT(&huart1, &gps_rx_byte, 1U);
    }
}

static void uart_send(UART_HandleTypeDef *uart, const char *text)
{
    (void)HAL_UART_Transmit(uart, (uint8_t *)text, (uint16_t)strlen(text), 1000U);
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState = RCC_MSI_ON;
    osc.MSICalibrationValue = 0U;
    osc.MSIClockRange = RCC_MSIRANGE_6; /* 4 MHz MSI */
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    osc.PLL.PLLM = 1U;
    osc.PLL.PLLN = 40U;
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2; /* 80 MHz SYSCLK */
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOB, TRIG_FRONT_PIN | TRIG_LEFT_PIN | TRIG_RIGHT_PIN |
                      BUZZER_PIN | MOTOR_PIN, GPIO_PIN_RESET);

    gpio.Pin = TRIG_FRONT_PIN | TRIG_LEFT_PIN | TRIG_RIGHT_PIN | BUZZER_PIN | MOTOR_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = ECHO_FRONT_PIN | ECHO_LEFT_PIN | ECHO_RIGHT_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = IR_SENSOR_PIN | BUTTON_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);
}

static void uart_init(UART_HandleTypeDef *uart, USART_TypeDef *instance)
{
    uart->Instance = instance;
    uart->Init.BaudRate = 9600U;
    uart->Init.WordLength = UART_WORDLENGTH_8B;
    uart->Init.StopBits = UART_STOPBITS_1;
    uart->Init.Parity = UART_PARITY_NONE;
    uart->Init.Mode = UART_MODE_TX_RX;
    uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart->Init.OverSampling = UART_OVERSAMPLING_16;
    uart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    uart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(uart) != HAL_OK) Error_Handler();
}

static void MX_USART1_UART_Init(void) { uart_init(&huart1, USART1); }
static void MX_USART2_UART_Init(void) { uart_init(&huart2, USART2); }


void HAL_UART_MspInit(UART_HandleTypeDef *uart)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    if (uart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    } else if (uart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    } else return;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio);
    if (uart->Instance == USART1) {
        HAL_NVIC_SetPriority(USART1_IRQn, 5U, 0U);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
