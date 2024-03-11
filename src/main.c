/**
 ******************************************************************************
 * @file main.c
 * @brief This file contains the main function for: retarget the C library printf
 * scanf functions to the UART1 example.
 * @author  MCD Application Team, Nicolas Gutierrez
 * @version V1
 * @date   9-December-2023
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#include "delay.h"
#include "display.h"
#include "mirf.h"
#include "stdio.h"
#include "stm8s.h"
#include "stm8s_it.h" /* SDCC patch: required by SDCC for interrupts */

/* SDCC patch: ensure same types as stdio.h */
#if SDCC_VERSION >= 30605  // declaration changed in sdcc 3.6.5 (officially with 3.7.0)
#define PUTCHAR_PROTOTYPE int putchar(int c)
#define GETCHAR_PROTOTYPE int getchar(void)
#else
#define PUTCHAR_PROTOTYPE void putchar(char c)
#define GETCHAR_PROTOTYPE char getchar(void)
#endif
/* Private typedef -----------------------------------------------------------*/
typedef enum {
    STATE_NOT_CONNECTED,
    STATE_WAITING_GAME,
    STATE_LOCAL_PLAYING,
    STATE_REMOTE_PLAYING,
    STATE_LOCAL_MOVE
} states_t;
enum CLOCK_COMMANDS {
    CMD_LOCAL_PLAYER_START = 0x00,
    CMD_REMOTE_PLAYER_START,
    CMD_BTN_PRESS,
    CMD_GAME_OVER,
    CMD_LOCAL_PLAYER_TURN,
    CMD_WRONG_MOVE
};

/* Private define ------------------------------------------------------------*/
#define CCR1_Val ((uint16_t)976)
#define CCR2_Val ((uint16_t)488)
#define CCR3_Val ((uint16_t)244)
/* Private macro -------------------------------------------------------------*/
#define PIN_BUZZZER GPIOD, GPIO_PIN_4
#define PIN_LED_G GPIOB, GPIO_PIN_4
#define PIN_LED_B GPIOB, GPIO_PIN_5
#define PIN_BTN GPIOB, GPIO_PIN_0
/* Private variables ---------------------------------------------------------*/
volatile bool bIntFlag, tIntFlag;
bool PTX;
uint8_t buf[32];
states_t fsmState = STATE_NOT_CONNECTED;
/* Private function prototypes -----------------------------------------------*/
static void TIM2_Config(void);
static void CLK_Config(void);
static void GPIO_Config(void);
static void SPI_Config(void);
static void FSM_Loop(void);
/* Private functions ---------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/**
 * @brief  Main program.
 */
void main(void) {
    CLK_Config();
    TIM2_Config();
    GPIO_Config();
    SPI_Config();

    while (1) {
        FSM_Loop();
    }
}

void beep(uint8_t pulse) {
    GPIO_WriteHigh(PIN_BUZZZER);
    delay_ms(pulse);
    GPIO_WriteLow(PIN_BUZZZER);
    delay_ms(pulse);
}
/**
 * @brief  The main FSM Loop
 */
static void FSM_Loop(void) {
    switch (fsmState) {
        case STATE_NOT_CONNECTED:
            // When the program is received, the received data is output from the serial port
            if (Nrf24_dataReady()) {
                Nrf24_getData(buf);
                // The ESP is printing stuff so it takes a while to switch to RX mode.
                delay_ms(10);

                Nrf24_send(buf, &PTX);

                // Same here, delay a bit.
                delay_ms(10);

                while (!Nrf24_isSend(50, &PTX)) {
                    // If no ACK after the preconfigured retries, send again.
                    Nrf24_send(buf, &PTX);
                    delay_ms(1);
                }
                GPIO_WriteLow(PIN_LED_G);
                GPIO_WriteLow(PIN_LED_B);
                fsmState = STATE_WAITING_GAME;
                disp_setTime(0, PLAYER_LOCAL);
                disp_setTime(0, PLAYER_REMOTE);
            }
            delay_ms(1);
            break;
        case STATE_WAITING_GAME:
            // When the program is received, the received data is output from the serial port
            if (Nrf24_dataReady() && !Nrf24_rxFifoEmpty()) {
                GPIO_WriteLow(PIN_LED_G);
                GPIO_WriteLow(PIN_LED_B);
                Nrf24_getData(buf);
                char *end;
                uint8_t command = (uint8_t)strtoul((char *)buf, &end, 16);  // command is in hexadecimal
                disp_setTime((time_t)strtoul(end, &end, 16), PLAYER_LOCAL);
                disp_setTime((time_t)strtoul(end, end, 16), PLAYER_REMOTE);
                // CLEAR FIFO!
                while (1) {
                    if (Nrf24_dataReady() == FALSE) break;
                    Nrf24_getData(buf);
                }
                memset(buf, 0, sizeof(buf));

                switch (command) {
                    case CMD_LOCAL_PLAYER_START:
                        fsmState = STATE_LOCAL_PLAYING;
                        break;
                    case CMD_REMOTE_PLAYER_START:
                        fsmState = STATE_REMOTE_PLAYING;
                        break;
                    default:
                        fsmState = STATE_WAITING_GAME;
                        disp_error(DISP_ERROR_BAD_CMD);
                        break;
                }
            }
            delay_ms(1);
            break;
        case STATE_LOCAL_PLAYING:
            GPIO_WriteLow(PIN_LED_G);
            GPIO_WriteHigh(PIN_LED_B);

            if (tIntFlag) {
                tIntFlag = 0;
                disp_changeTime(-1, PLAYER_LOCAL);
            }
            if (Nrf24_dataReady() && !Nrf24_rxFifoEmpty()) {
                Nrf24_getData(buf);
                char *end;
                uint8_t command = (uint8_t)strtoul((char *)buf, &end, 16);  // command is in hexadecimal
                switch (command) {
                    case CMD_GAME_OVER:
                        fsmState = STATE_WAITING_GAME;
                        break;
                    default:
                        fsmState = STATE_WAITING_GAME;
                        disp_error(DISP_ERROR_BAD_CMD);
                        break;
                }
            }
            if (bIntFlag) {
                bIntFlag = 0;
                memset(buf, 0, sizeof(buf));
                sprintf((char *)buf, "%01X ", (uint8_t)CMD_LOCAL_PLAYER_TURN);
                delay_ms(10);
                Nrf24_send(buf, &PTX);
                while (!Nrf24_isSend(50, &PTX)) {
                    Nrf24_send(buf, &PTX);
                    delay_ms(10);
                }
                fsmState = STATE_REMOTE_PLAYING;
            }
            delay_ms(1);
            break;
        case STATE_REMOTE_PLAYING:
            bIntFlag = 0;
            GPIO_WriteLow(PIN_LED_B);
            GPIO_WriteHigh(PIN_LED_G);
            if (tIntFlag) {
                tIntFlag = 0;
                disp_changeTime(-1, PLAYER_REMOTE);
            }
            if (Nrf24_dataReady() && !Nrf24_rxFifoEmpty()) {
                Nrf24_getData(buf);
                char *end;
                uint8_t command = (uint8_t)strtoul((char *)buf, &end, 16);  // command is in hexadecimal

                switch (command) {
                    case CMD_GAME_OVER:
                        fsmState = STATE_WAITING_GAME;
                        break;
                    case CMD_LOCAL_PLAYER_TURN:
                        beep(50);
                        disp_setTime((time_t)strtoul(end, &end, 16), PLAYER_LOCAL);
                        disp_setTime((time_t)strtoul(end, end, 16), PLAYER_REMOTE);
                        fsmState = STATE_LOCAL_PLAYING;
                        break;
                    case CMD_WRONG_MOVE:
                        beep(50);
                        beep(50);
                        fsmState = STATE_LOCAL_PLAYING;
                        break;
                    default:
                        fsmState = STATE_WAITING_GAME;
                        disp_error(DISP_ERROR_BAD_CMD);
                        break;
                }
            }
            delay_ms(1);
            break;
        default:
            break;
    }
}

/**
 * @brief  Configure SPI Devices (NRF and 7-Segment Driver)
 */
void SPI_Config(void) {
    SPI_Init(
        SPI_FIRSTBIT_MSB,
        SPI_BAUDRATEPRESCALER_64,  // DO NOT USE HIGH BAUD RATES WITH LONG CABLES, _64 if using jumpers _4 works with good PCB ONLY.
        SPI_MODE_MASTER,
        SPI_CLOCKPOLARITY_LOW,
        SPI_CLOCKPHASE_1EDGE,
        SPI_DATADIRECTION_2LINES_FULLDUPLEX,
        SPI_NSS_SOFT,  // This is not really working properly, should work with HARD but isn't
        (uint8_t)0x07);

    SPI_Cmd(ENABLE);
    disp_initialize();

    // NRF initialization
    Nrf24_init();
    Nrf24_config(&PTX);

    // Set own address using 5 characters
    int ret = Nrf24_setRADDR((uint8_t *)"ABCDE");
    if (ret != SUCCESS) {
        while (1) {
            delay_ms(1);
        }
    }

    // Set the receiver address using 5 characters
    ret = Nrf24_setTADDR((uint8_t *)"FGHIJ");
    if (ret != SUCCESS) {
        while (1) {
            delay_ms(1);
        }
    }

    // This has to be set to 1MBps and 150us delay, if using 250kbps, use at least 1000us delay
    Nrf24_SetSpeedDataRates(RF24_1MBPS);
    Nrf24_setRetransmitDelay(1);

    // Clear RX FiFo
    while (1) {
        if (Nrf24_dataReady() == FALSE) {
            break;
        };
        Nrf24_getData(buf);
    }
}

/**
 * @brief  Configure GPIO
 */
static void GPIO_Config(void) {
    // Debuging LEDs
    GPIO_Init(PIN_LED_B, GPIO_MODE_OUT_OD_HIZ_FAST);  // Open Drain Mode, since it has to sink current
    GPIO_Init(PIN_LED_G, GPIO_MODE_OUT_OD_HIZ_FAST);

    // Button
    GPIO_Init(PIN_BTN, GPIO_MODE_IN_PU_IT);
    EXTI_DeInit();
    EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOB, EXTI_SENSITIVITY_FALL_ONLY);
    EXTI_SetTLISensitivity(EXTI_TLISENSITIVITY_FALL_ONLY);

    // Startup Buzzer
    GPIO_Init(PIN_BUZZZER, GPIO_MODE_OUT_PP_HIGH_FAST);
    delay_ms(1);
    GPIO_WriteLow(PIN_BUZZZER);
}

/**
 * @brief  Configure system clock to work with external Oscillator
 * @param  None
 * @retval None
 */
static void CLK_Config(void) {
    CLK_DeInit();
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
    enableInterrupts();

    // TODO: Check if this is working
    if (FALSE) {
        CLK_CCOConfig(CLK_OUTPUT_MASTER);
        CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSE, DISABLE, CLK_CURRENTCLOCKSTATE_DISABLE);
    }
}

/**
 * @brief  Configure Output Compare Active Mode for TIM2 Channel1, Channel2 and
 *         channel3
 */
static void TIM2_Config(void) {
    /* Time base configuration */
    TIM2_TimeBaseInit(TIM2_PRESCALER_8, 65535);

    /* Prescaler configuration */
    TIM2_PrescalerConfig(TIM2_PRESCALER_256, TIM2_PSCRELOADMODE_IMMEDIATE);

    /* Output Compare Active Mode configuration: Channel1 */
    TIM2_OC1Init(TIM2_OCMODE_TIMING, TIM2_OUTPUTSTATE_DISABLE, CCR1_Val, TIM2_OCPOLARITY_HIGH);
    TIM2_OC1PreloadConfig(DISABLE);
    TIM2_ARRPreloadConfig(ENABLE);
    TIM2_ITConfig(TIM2_IT_CC1, ENABLE);
    /* TIM2 enable counter */
    TIM2_Cmd(ENABLE);
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    (void)file;
    (void)line;
    /* Infinite loop */
    while (1) {
    }
}
#endif

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/