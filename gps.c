#include "gps.h"
#include <stdlib.h>
#include <string.h>

void GPS_Init(GPS_Handle *gps)
{
    memset(gps, 0, sizeof(*gps));
}

void GPS_HandleRxByte(GPS_Handle *gps, uint8_t byte)
{
    if (byte == '\n') {
        gps->line[gps->length] = '\0';
        memcpy(gps->pending, gps->line, gps->length + 1U);
        gps->pending_length = gps->length;
        gps->length = 0U;
        gps->line_ready = 1U;
        return;
    }
    if (byte == '\r') return;

    if (gps->length < (GPS_LINE_CAPACITY - 1U)) {
        gps->line[gps->length++] = (char)byte;
    } else {
        /* Drop an overlong sentence and begin collecting the next one. */
        gps->length = 0U;
    }
}

static float nmea_to_decimal(const char *value, char hemisphere)
{
    const float raw = strtof(value, NULL);
    const int degrees = (int)(raw / 100.0f);
    const float minutes = raw - ((float)degrees * 100.0f);
    float decimal = (float)degrees + minutes / 60.0f;
    if (hemisphere == 'S' || hemisphere == 'W') decimal = -decimal;
    return decimal;
}

void GPS_ProcessLine(GPS_Handle *gps)
{
    if (gps->line_ready == 0U) return;
    gps->line_ready = 0U;

    char sentence[GPS_LINE_CAPACITY];
    const uint16_t n = gps->pending_length;
    if (n >= GPS_LINE_CAPACITY) {
        gps->length = 0U;
        return;
    }
    memcpy(sentence, gps->pending, n + 1U);

    /* The source screenshot specifically checks $GPGGA. Accept GN GGA too. */
    if (strncmp(sentence, "$GPGGA,", 7U) != 0 &&
        strncmp(sentence, "$GNGGA,", 7U) != 0) return;

    char *fields[15] = {0};
    unsigned count = 0U;
    char *cursor = sentence;
    while (count < 15U) {
        fields[count++] = cursor;
        char *comma = strchr(cursor, ',');
        if (comma == NULL) break;
        *comma = '\0';
        cursor = comma + 1;
    }

    /* GGA: 0=tag, 2=latitude, 3=N/S, 4=longitude, 5=E/W, 6=fix quality. */
    if (count < 7U || fields[2][0] == '\0' || fields[3][0] == '\0' ||
        fields[4][0] == '\0' || fields[5][0] == '\0') {
        gps->fix_valid = 0U;
        return;
    }
    if (atoi(fields[6]) <= 0) {
        gps->fix_valid = 0U;
        return;
    }

    gps->latitude = nmea_to_decimal(fields[2], fields[3][0]);
    gps->longitude = nmea_to_decimal(fields[4], fields[5][0]);
    gps->fix_valid = 1U;
    gps->update_ready = 1U;
}
