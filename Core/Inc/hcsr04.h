#ifndef HCSR04_H
#define HCSR04_H

#include "stm32l4xx_hal.h"

/* Reconstructed driver. Returns centimeters, or 0.0f on timeout/invalid data. */
float HCSR04_GetDistance(GPIO_TypeDef *trigger_port, uint16_t trigger_pin,
                         GPIO_TypeDef *echo_port, uint16_t echo_pin);

#endif
