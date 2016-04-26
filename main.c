/*
 * main.c
 *
 * Description: This file contains the reference application file Arduino Mega
 * (or similar devices running AVR8 MCUs) targeting the Tiny RTC PCB.
 * This application will use the Arduino twi-library to create an i2c-wrapper
 * and implement functionalities which explores the PCBs functions such as
 * real-time clock handling, usage of an internal RAM buffer and storing/reading
 * data from an external EEPROM.
 *
 * Created: 2016-04-05
 * Author : alex.rodzevski@gmail.com
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "uart/uart.h"
#include "i2c/i2c.h"

/* DS1307 RTC register for start/stop time counter */
#define RTC_REG_START_TIME      (uint8_t)0x00
/* DS1307 RTC register address pointing at the internal RAM-buffer */
#define RTC_REG_RAM_BUF_START   (uint8_t)0x08
/* Memory block sizes */
#define RTC_RAM_SIZE            (uint8_t)56     /* Bytes */
#define EEPROM_SIZE             (uint16_t)4096   /* Bytes */

/* RTC time variable struct */
struct rtc_time_var {
        uint8_t sec_10;
        uint8_t sec_1;
        uint8_t min_10;
        uint8_t min_1;
};

/* Dummy debug strings */
static const char Dummy_EEPROM[] = "EEPROM_Dummy_data";
static const char Dummy_RTC_RAM[] = "RTC_RAM_Dummy_data";


static void rtc_init(void)
{
        uint8_t rtc_reg[RTC_RAM_SIZE];

        /* Clear the RTC RAM buffer */
        memset(rtc_reg, 0, RTC_RAM_SIZE);
        i2c_wr_addr_blk(DS1307, RTC_REG_RAM_BUF_START, rtc_reg, RTC_RAM_SIZE);

        /* Start the RTC clock counter */
        i2c_wr_addr_byte(DS1307, RTC_REG_START_TIME, 0);
}

static void rtc_get_time_var(struct rtc_time_var *var)
{
	uint8_t sec, min, rtc_dat[2];

        i2c_rd_addr_blk(DS1307, RTC_REG_START_TIME, rtc_dat, sizeof(rtc_dat));
	sec = rtc_dat[0];
	min = rtc_dat[1];

	var->sec_1 = (sec & 0x0F);
	var->sec_10 = ((sec & 0x70) >> 4);
	var->min_1 = (min & 0x0F);
	var->min_10 = ((min & 0x70) >> 4);
}

static int rtc_get_ram_buf(uint8_t *buf, uint8_t len)
{
        if (len >= RTC_RAM_SIZE)
                return -1;
        return i2c_rd_addr_blk(DS1307, RTC_REG_RAM_BUF_START, buf, len);
}

static int rtc_set_ram_buf(uint8_t *buf, uint8_t len)
{
        if (len >= RTC_RAM_SIZE)
                return -1;
        return i2c_wr_addr_blk(DS1307, RTC_REG_RAM_BUF_START, buf, len);
}

static int eeprom_get_data(uint16_t reg_idx, uint8_t *buf, uint16_t len)
{
        if (len >= EEPROM_SIZE)
                return -1;
        /* TODO: add page handling */
        return i2c_rd_addr16_blk(AT24C32, reg_idx, buf, len);
}

static int eeprom_set_data(uint16_t reg_idx, uint8_t *buf, uint8_t len)
{
        if (len >= EEPROM_SIZE)
                return -1;
        /* TODO: add page handling */
        return i2c_wr_addr16_blk(AT24C32, reg_idx, buf, len);
}

int main(void)
{
        uint8_t PORTB_shadow, DDRB_shadow;
        struct rtc_time_var rtc;
        uint8_t i2c_buf[32];

        /* GPIO toggle-test snippet (Blinking LED on Arduino Mega) */
        DDRB_shadow = DDRB;
        DDRB = DDRB_shadow | (1 << DDB7);
        PORTB_shadow = PORTB | (1 << PB7);
        PORTB = PORTB_shadow;

        /* Initialize UART0, serial printing over USB on Arduino Mega */
        uart0_init();

        /* Initialize I2C */
        i2c_init();

        /* Enable global interrupts (used in I2C) */
        sei();
        _delay_ms(1000);

        rtc_init();
        /* Write some dummy data to the RTC RAM & the EEPROM */
        rtc_set_ram_buf((uint8_t *)Dummy_RTC_RAM, strlen(Dummy_RTC_RAM));
        eeprom_set_data(0x0000, (uint8_t *)Dummy_EEPROM, strlen(Dummy_EEPROM));

        /* Print the 7-bit I2C-client addresses */
        printf("RTC DS1307 I2C-addr:0x%x\n", DS1307);
        printf("EEPROM AT24C32 I2C-addr:0x%x\n\n", AT24C32);
        _delay_ms(1000);

        /* Read the Dummy data from RTC RAM & the EEPROM and print it */
        memset(i2c_buf, 0, sizeof(i2c_buf));
        rtc_get_ram_buf(i2c_buf, strlen(Dummy_RTC_RAM));
        printf("RTC RAM read result:%s\n", i2c_buf);
        memset(i2c_buf, 0, sizeof(i2c_buf));
        eeprom_get_data(0x0000, i2c_buf, strlen(Dummy_EEPROM));
        printf("EEPROM read result:%s\n\n", i2c_buf);

        /* Main loop */
        while (1) {
                _delay_ms(1000);
                /* Blink LED connected to Arduino pin 13 (PB7 on Mega) */
                PINB = PINB | 1<<PINB7;
		rtc_get_time_var(&rtc);
		printf("Elapsed RTC time - min:%d%d sec:%d%d\n",
			rtc.min_10, rtc.min_1, rtc.sec_10, rtc.sec_1);
        }
}
