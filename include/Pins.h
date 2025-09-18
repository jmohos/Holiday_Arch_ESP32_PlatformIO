#ifndef PINS_H
#define PINS_H

#if defined(ESP32C3_BOARD)  
// Pin definitions for Seeed Studio Xiao-ESP32-C3 Board
const int MP3_TX_PIN =   21;
const int MP3_RX_PIN =   20;
const int MP3_BUSY_PIN = 10; /* Optional */
const int RGB_LED_DATA_PIN = 5;
const int I2C_SDA_PIN = 6;
const int I2C_SCL_PIN = 7;
const int SERVO_1_PIN = 3;
const int SERVO_2_PIN = 4;

#elif defined(ESP32C6_BOARD)
// Pin definitions for Seeed Studio Xiao-ESP32-C6 Board
const int MP3_TX_PIN =   16;
const int MP3_RX_PIN =   17;
const int MP3_BUSY_PIN = 18; /* Optional */
const int RGB_LED_DATA_PIN = 21;
const int I2C_SDA_PIN = 22;
const int I2C_SCL_PIN = 23;
const int SERVO_1_PIN = 1;
const int SERVO_2_PIN = 2;

#endif

#endif