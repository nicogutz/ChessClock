#include <stdint.h>

#ifndef DISPLAY_H
#define DISPLAY_H
/**
 ******************************************************************************
 * @file display.h
 * @brief Header for display.
 * @author  Nicolas Gutierrez
 * @version V1
 * @date   9-December-2023
 ******************************************************************************
 */
#include <time.h>

/* Memory Map */
#define ADDR_NO_OP 0x00
#define ADDR_DIGIT_0 0x01
#define ADDR_DIGIT_1 0x02
#define ADDR_DIGIT_2 0x03
#define ADDR_DIGIT_3 0x04
#define ADDR_DIGIT_4 0x05
#define ADDR_DIGIT_5 0x06
#define ADDR_DIGIT_6 0x07
#define ADDR_DIGIT_7 0x08
#define ADDR_DECODE_MODE 0x09
#define ADDR_INTENSITY 0x0A
#define ADDR_SCAN_LIMIT 0x0B
#define ADDR_SHUTDOWN 0x0C
#define ADDR_TEST 0x0F
#define SHUTDOWN_MODE ((ADDR_SHUTDOWN << 8) | 0x00)
#define NORMAL_MODE ((ADDR_SHUTDOWN << 8) | 0x01)

#define NO_DECODE ((ADDR_DECODE_MODE << 8) | 0x00)
#define B_DECODE ((ADDR_DECODE_MODE << 8) | 0x00)

#define SCAN_ALL ((ADDR_SCAN_LIMIT << 8) | 0x07)

enum DISPLAY_ERROR{
    DISP_ERROR_NO_CONN = 0x01,
    DISP_ERROR_BAD_MOVE,
    DISP_ERROR_BAD_CMD
};

typedef enum {
    PLAYER_LOCAL,
    PLAYER_REMOTE
} player_t;

void disp_initialize();
void disp_clear();
void disp_turnOn();
void disp_turnOff();
void disp_error(uint8_t num);
void disp_setTime(time_t time, player_t player);
void disp_setSegments(uint8_t* disp, player_t player);
void disp_changeTime(int16_t changeSeconds, player_t player);

#endif /* DISPLAY_H */
