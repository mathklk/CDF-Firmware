/*
 * frame.h
 *
 *  Created on: Mar 27, 2026
 *      Author: Mathis
 */

#ifndef INC_FRAME_H_
#define INC_FRAME_H_

#include <stdint.h>
#include "stm32wl3x_hal.h"

extern uint16_t FRAME_ID;

void transmitFrame(uint16_t lenBufA, uint8_t const*const bufA, uint16_t lenBufB, uint8_t const*const bufB, IWDG_HandleTypeDef* watchdog);


#endif /* INC_FRAME_H_ */
