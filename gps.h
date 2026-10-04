#ifndef GPS_H
#define GPS_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

#define GPS_LINE_CAPACITY 128U

typedef struct {
    char line[GPS_LINE_CAPACITY];
    char pending[GPS_LINE_CAPACITY];
    uint16_t length;
    uint16_t pending_length;
    volatile uint8_t line_ready;
    volatile uint8_t update_ready;
    float latitude;
    float longitude;
    uint8_t fix_valid;
} GPS_Handle;

void GPS_Init(GPS_Handle *gps);
void GPS_HandleRxByte(GPS_Handle *gps, uint8_t byte);
void GPS_ProcessLine(GPS_Handle *gps);

#endif
