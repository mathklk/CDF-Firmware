/*
 * frame.c
 *
 *  Created on: Mar 27, 2026
 *      Author: Mathis
 */
#include "frame.h"

#include <stdio.h>
#include "checksum.h"

uint16_t FRAME_ID = 1;

void transmitFrame(uint16_t lenBufA, uint8_t const*const bufA, uint16_t lenBufB, uint8_t const*const bufB, IWDG_HandleTypeDef* watchdog) {
  uint32_t const totalBytes = lenBufA + lenBufB;
  if (totalBytes > UINT16_MAX) {
    printf("FRAME TOO LARGE TO REPRESENT AS UINT16\r\n");
  }
//  uint32_t const checksum = crc32((uint8_t*)buffers.buf0, sizeof(buffers));
  uint32_t const checksum = splitCrc32(lenBufA, bufA, lenBufB, bufB);

  // Start of Header
  putchar(0x01);
  // ID
  putchar((FRAME_ID >> 8) & 0xFF);
  putchar( FRAME_ID       & 0xFF);
  FRAME_ID++;
  // Length / Number of Bytes in Data
  putchar((totalBytes >> 8) & 0xFF);
  putchar( totalBytes       & 0xFF);
  // Checksum
  putchar((checksum >> 24) & 0xFF);
  putchar((checksum >> 16) & 0xFF);
  putchar((checksum >>  8) & 0xFF);
  putchar( checksum        & 0xFF);
  // Data (TODO: buffer order switching)
  for (uint32_t i = 0; i < lenBufA; ++i) {
    putchar(bufA[i]);
    HAL_IWDG_Refresh(watchdog);
  }
  for (uint32_t i = 0; i < lenBufB; ++i) {
    putchar(bufB[i]);
    HAL_IWDG_Refresh(watchdog);
  }
}
