#include "display.h"

#include "delay.h"
#include "spi.h"

void send_command(uint8_t addr, uint8_t data) {
    GPIO_WriteLow(GPIOB, GPIO_PIN_1);
    delay_ms(10);
    uint8_t pld[2] = {addr, data};
    spi_write_byte(pld, 2);
    delay_ms(10);
    GPIO_WriteHigh(GPIOB, GPIO_PIN_1);

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

    send_command(ADDR_SHUTDOWN, 1);
    send_command(ADDR_DECODE_MODE, 0xFF);
    send_command(ADDR_SCAN_LIMIT, 0x07);
    send_command(ADDR_INTENSITY, 0x08);

    send_command(ADDR_DIGIT_0, 0x00);
    send_command(ADDR_DIGIT_1, 0x01);
    send_command(ADDR_DIGIT_2, 0x02);
    send_command(ADDR_DIGIT_3, 0x03);
    send_command(ADDR_DIGIT_4, 0x04);
    send_command(ADDR_DIGIT_5, 0x05);
    send_command(ADDR_DIGIT_6, 0x06);
    send_command(ADDR_DIGIT_7, 0x07);
}
void disp_turnOn() {
}
