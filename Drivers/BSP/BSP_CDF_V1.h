/*
 * pinout_cdf_v1.h
 *
 *  Created on: Mar 26, 2026
 *      Author: Mathis
 */

#ifndef BSP_BSP_CDF_V1_H_
#define BSP_BSP_CDF_V1_H_

#include "stm32wl3x_hal.h"


void BSP_RF_ENABLE_MASTER(void) {
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_14, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
}

void BSP_RF_ENABLE_ARRAY(void) {
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_14, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
}


#endif /* BSP_BSP_CDF_V1_H_ */
