#include "hcsr04.h"


static uint32_t cycles_per_us;

static void delay_us(uint32_t us)
{
    const uint32_t start = DWT->CYCCNT;
    const uint32_t ticks = us * cycles_per_us;
    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
        
    }
}

static uint32_t micros_now(void)
{
    return DWT->CYCCNT / cycles_per_us;
}

float HCSR04_GetDistance(GPIO_TypeDef *trigger_port, uint16_t trigger_pin,
                         GPIO_TypeDef *echo_port, uint16_t echo_pin)
{
    if (cycles_per_us == 0U) {
        cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
        if (cycles_per_us == 0U) return 0.0f;
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    HAL_GPIO_WritePin(trigger_port, trigger_pin, GPIO_PIN_RESET);
    delay_us(3U);
    HAL_GPIO_WritePin(trigger_port, trigger_pin, GPIO_PIN_SET);
    delay_us(10U);
    HAL_GPIO_WritePin(trigger_port, trigger_pin, GPIO_PIN_RESET);

    const uint32_t rise_start = micros_now();
    while (HAL_GPIO_ReadPin(echo_port, echo_pin) == GPIO_PIN_RESET) {
        if ((uint32_t)(micros_now() - rise_start) > 30000U) return 0.0f;
    }

    const uint32_t pulse_start = micros_now();
    while (HAL_GPIO_ReadPin(echo_port, echo_pin) == GPIO_PIN_SET) {
        if ((uint32_t)(micros_now() - pulse_start) > 30000U) return 0.0f;
    }
    const uint32_t pulse_us = (uint32_t)(micros_now() - pulse_start);

    /* Speed of sound: distance (cm) ~= echo pulse width (us) / 58. */
    return (float)pulse_us / 58.0f;
}
