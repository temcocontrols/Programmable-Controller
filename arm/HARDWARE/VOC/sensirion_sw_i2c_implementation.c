/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

//#include <stm32_hal_legacy.h>
//#include <stm32f1xx_hal.h>

#include "sensirion_arch_config.h"
#include "sensirion_sw_i2c_gpio.h"
#include "myiic.h"
#include "delay.h"

/*
 * We use the following names for the two I2C signal lines:
 * SCL for the clock line
 * SDA for the data line
 *
 * Both lines must be equipped with pull-up resistors appropriate to the bus
 * frequency.
 */

/**
 * Initialize all hard- and software components that are needed to set the
 * SDA and SCL pins.
 */
void sensirion_init_pins() {
    //__GPIOB_CLK_ENABLE();
    sensirion_SDA_in();
    sensirion_SCL_in();
}

/**
 * Configure the SDA pin as an input. With an external pull-up resistor the line
 * should be left floating, without external pull-up resistor, the input must be
 * configured to use the internal pull-up resistor.
 */
void sensirion_SDA_in() {
	
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	GPIO_SetBits(GPIOB, GPIO_Pin_11);
//    GPIO_InitTypeDef GPIO_InitStruct = {
//        .Pin = GPIO_PIN_9,
//        .Mode = GPIO_MODE_INPUT,
//        .Pull = GPIO_NOPULL,
//        .Speed = GPIO_SPEED_HIGH,
//    };
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * Configure the SDA pin as an output and drive it low or set to logical false.
 */
void sensirion_SDA_out() {
	
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_2);
	
//    GPIO_InitTypeDef GPIO_InitStruct = {
//        .Pin = GPIO_PIN_9,
//        .Mode = GPIO_MODE_OUTPUT_PP,
//        .Pull = GPIO_NOPULL,
//        .Speed = GPIO_SPEED_HIGH,
//    };
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
}

/**
 * Read the value of the SDA pin.
 * @returns 0 if the pin is low and 1 otherwise.
 */
uint8_t sensirion_SDA_read() {
	//return READ_SDA();
	u8 status;
	status = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);  		  
	return status;
    //return (uint8_t)HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_SET;
}

/**
 * Configure the SCL pin as an input. With an external pull-up resistor the line
 * should be left floating, without external pull-up resistor, the input must be
 * configured to use the internal pull-up resistor.
 */
void sensirion_SCL_in() {
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//    GPIO_InitTypeDef GPIO_InitStruct = {
//        .Pin = GPIO_PIN_8,
//        .Mode = GPIO_MODE_INPUT,
//        .Pull = GPIO_NOPULL,
//        .Speed = GPIO_SPEED_FREQ_HIGH,
//    };
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * Configure the SCL pin as an output and drive it low or set to logical false.
 */
void sensirion_SCL_out() {
	
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_3);
//    GPIO_InitTypeDef GPIO_InitStruct = {
//        .Pin = GPIO_PIN_8,
//        .Mode = GPIO_MODE_OUTPUT_PP,
//        .Pull = GPIO_NOPULL,
//        .Speed = GPIO_SPEED_FREQ_HIGH,
//    };
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
}

/**
 * Read the value of the SCL pin.
 * @returns 0 if the pin is low and 1 otherwise.
 */
uint8_t sensirion_SCL_read() {
	u8 status;
	status = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3);  		  
	return status;
    //return (uint8_t)HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_SET;
}

/**
 * Sleep for a given number of microseconds. The function should delay the
 * execution approximately, but no less than, the given time.
 *
 * The precision needed depends on the desired i2c frequency, i.e. should be
 * exact to about half a clock cycle (defined in
 * `SENSIRION_I2C_CLOCK_PERIOD_USEC` in `sensirion_arch_config.h`).
 *
 * Example with 400kHz requires a precision of 1 / (2 * 400kHz) == 1.25usec.
 *
 * @param useconds the sleep time in microseconds
 */
void sensirion_sleep_usec(uint32_t useconds) {
	delay_us(useconds);
    //HAL_Delay(useconds / 1000 + 1);
}
