/*
 * Smart Blind Stick - STM32L476 HAL application
 *
 * Pin assignments in this file are reconstructed examples, not recovered
 * from the source document. Update them to match the actual PCB/wiring.
 */
#ifndef SMART_BLIND_STICK_MAIN_H
#define SMART_BLIND_STICK_MAIN_H

#include "stm32l4xx_hal.h"

/* Ultrasonic sensor trigger pins (reconstructed pin map). */
#define TRIG_FRONT_PORT GPIOB
#define TRIG_FRONT_PIN  GPIO_PIN_0
#define TRIG_LEFT_PORT  GPIOB
#define TRIG_LEFT_PIN   GPIO_PIN_1
#define TRIG_RIGHT_PORT GPIOB
#define TRIG_RIGHT_PIN  GPIO_PIN_2

/* Ultrasonic echo pins (3.3 V safe level required). */
#define ECHO_FRONT_PORT GPIOC
#define ECHO_FRONT_PIN  GPIO_PIN_0
#define ECHO_LEFT_PORT  GPIOC
#define ECHO_LEFT_PIN   GPIO_PIN_1
#define ECHO_RIGHT_PORT GPIOC
#define ECHO_RIGHT_PIN  GPIO_PIN_2

#define IR_SENSOR_PORT  GPIOC
#define IR_SENSOR_PIN   GPIO_PIN_3
#define BUZZER_PORT     GPIOB
#define BUZZER_PIN      GPIO_PIN_10
#define MOTOR_PORT      GPIOB
#define MOTOR_PIN       GPIO_PIN_11
#define BUTTON_PORT     GPIOC
#define BUTTON_PIN      GPIO_PIN_13

/* CubeMX-generated UART handles are declared by main.c. */
extern UART_HandleTypeDef huart1; /* GPS NEO-6M */
extern UART_HandleTypeDef huart2; /* HC-05 Bluetooth */

void Error_Handler(void);

#endif
