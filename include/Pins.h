#ifndef PINS_H
#define PINS_H


// Pin definitions for ESP32-S3 DevkitC-1 Demo Board
//#define MP3_PLAYER_SERIAL Serial1
const int MP3_TX_PIN = 17;
const int MP3_RX_PIN = 18;
const int MP3_BUSY_PIN = 21;

const int SPI2_SCK_PIN = 12;
const int SPI2_MOSI_PIN = 11;
const int SPI2_MISO_PIN = 13;
const int SPI2_CS_PIN = 10;
const int SPI2_RESET = 47;

const int SPI3_SCK_PIN = 36;
const int SPI3_MOSI_PIN = 35;
const int SPI3_MISO_PIN = 37;
const int SPI3_CS_PIN = 39;

// MCP23S17 SPI IO Expander provided IO
// const uint8_t IO_EXP_IN_0 = GPIOB_BIT_0;
// const uint8_t IO_EXP_IN_1 = GPIOB_BIT_1;
// const uint8_t IO_EXP_IN_2 = GPIOB_BIT_2;
// const uint8_t IO_EXP_IN_3 = GPIOB_BIT_3;
// const uint8_t IO_EXP_IN_4 = GPIOB_BIT_4;
// const uint8_t IO_EXP_IN_5 = GPIOB_BIT_5;
// const uint8_t IO_EXP_IN_6 = GPIOB_BIT_6;
// const uint8_t IO_EXP_IN_7 = GPIOB_BIT_7;
// const uint8_t IO_EXP_OUT_0 = GPIOA_BIT_0;
// const uint8_t IO_EXP_OUT_1 = GPIOA_BIT_1;
// const uint8_t IO_EXP_OUT_2 = GPIOA_BIT_2;
// const uint8_t IO_EXP_OUT_3 = GPIOA_BIT_3;
// const uint8_t IO_EXP_OUT_4 = GPIOA_BIT_4;
// const uint8_t IO_EXP_OUT_5 = GPIOA_BIT_5;
// const uint8_t IO_EXP_OUT_6 = GPIOA_BIT_6;
// const uint8_t IO_EXP_OUT_7 = GPIOA_BIT_7;

const int RGB_LED_DATA_PIN = 48;
const int RGB_LED_BUILTIN_PIN = 48;

const int MOT0_PWM1_PIN = 40;
const int MOT0_PWM2_PIN = 41;
const int MOT0_PWM3_PIN = 42;
const int MOT0_ENC1_PIN = 4;
const int MOT0_ENC2_PIN = 5;

const int ENC1_PIN = 6;
const int ENC2_PIN = 7;


const int I2C0_SDA_PIN = 8;
const int I2C0_SCL_PIN = 9;

const int SPARE_1_PIN = 16;
const int SPARE_2_PIN = 15;

const int SERVO_1_PIN = 1;
const int SERVO_2_PIN = 2;


#endif