#include "display.h"

#include "delay.h"
#include "spi.h"

time_t initialTime[2];

void send_command(uint8_t addr, uint8_t data) {
    GPIO_WriteLow(GPIOB, GPIO_PIN_1);
    uint8_t pld[2] = {addr, data};
    spi_write_byte(pld, 2);
    GPIO_WriteHigh(GPIOB, GPIO_PIN_1);
}

void disp_turnOn() {
    send_command(ADDR_SHUTDOWN, 1);
}

void disp_turnOff() {
    send_command(ADDR_SHUTDOWN, 0);
}

void disp_setSegments(uint8_t* disp, player_t player) {
    for (uint8_t i = 0; i < 4; i++) {
        send_command(i + (player * 4) + 1, disp[i]);
    }
}

void disp_changeTime(int16_t changeSeconds, player_t player) {
    disp_setTime(initialTime[player] + changeSeconds, player);
}

void disp_setTime(time_t time, player_t player) {
    initialTime[player] = time;
    struct tm* convTime = gmtime(&initialTime[player]);
    uint8_t disp[] = {
        convTime->tm_min / 10,
        convTime->tm_min % 10,
        convTime->tm_sec / 10,
        convTime->tm_sec % 10,
    };
    disp_setSegments(disp, player);
}

void disp_error(uint8_t num) {
    send_command(ADDR_DIGIT_0, 0xB);
    send_command(ADDR_DIGIT_1, 0xB);
    send_command(ADDR_DIGIT_2, num / 10);
    send_command(ADDR_DIGIT_3, num % 10);
}

void disp_clear(void) {
    for (uint8_t i = ADDR_DIGIT_0; i <= ADDR_DIGIT_7; i++)
    {
        send_command(i, 0x0F);
    }
    
}

void disp_initialize() {
    delay_ms(1);

    GPIO_Init(
        GPIOB,
        GPIO_PIN_1,
        GPIO_MODE_OUT_PP_HIGH_FAST);
    GPIO_Init(
        GPIOB,
        GPIO_PIN_3,
        GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_WriteHigh(GPIOB, GPIO_PIN_3);

    send_command(ADDR_DECODE_MODE, 0xFF);
    send_command(ADDR_SCAN_LIMIT, 0x07);
    send_command(ADDR_INTENSITY, 0x09);
    disp_clear();
    disp_turnOn();
    disp_error(DISP_ERROR_NO_CONN);
}
