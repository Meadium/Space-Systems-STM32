
#ifndef INC_WS2812B_H_
#define INC_WS2812B_H_

#include "stm32g4xx.h"

void ws2812b_init(TIM_HandleTypeDef *tim);

void ws2812b_set_color(uint32_t led, uint8_t red, uint8_t green, uint8_t blue);

void ws2812b_update(TIM_HandleTypeDef *tim);

void ws2812b_wait(TIM_HandleTypeDef *tim);

#endif /* INC_WS2812B_H_ */
