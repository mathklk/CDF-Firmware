/*
 * checksum.h
 *
 *  Created on: Mar 24, 2026
 *      Author: Mathis
 */

#ifndef INC_CHECKSUM_H_
#define INC_CHECKSUM_H_


#include <stdint.h>
#include <stddef.h>

uint32_t crc32(size_t len, const uint8_t* data) {
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xEDB88320u;  // reversed poly
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFFu;
}

uint32_t splitCrc32(size_t lenA, const uint8_t* dataA,
                    size_t lenB, const uint8_t* dataB) {
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < lenA; ++i) {
        crc ^= dataA[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }

    for (size_t i = 0; i < lenB; ++i) {
        crc ^= dataB[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFFu;
}


#endif /* INC_CHECKSUM_H_ */
