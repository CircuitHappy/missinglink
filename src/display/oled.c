/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 *
 * I2C interface for NHD-0216AW-IB3 OLED character display.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "display/i2c.h"

static const int OLED_I2C_BUS = 1;
static const int OLED_I2C_ADDR = 0x3C;

static void oled_command(unsigned char command) {
  unsigned char tx_buf[2] = {0x00, command};
  i2c_write(OLED_I2C_BUS, OLED_I2C_ADDR, tx_buf, 2);
}

static void oled_data(unsigned char data) {
  unsigned char tx_buf[2] = {0x40, data};
  i2c_write(OLED_I2C_BUS, OLED_I2C_ADDR, tx_buf, 2);
}

static void delay(int msec) {
  usleep(msec * 1000);
}

void oled_init() {
  delay(1);
  oled_command(0x2A);  //function set (extended command set)
  oled_command(0x71);  //function selection A
  oled_data(0x00);     //disable internal Vdd regualtor
  oled_command(0x28);  //function set (fundamental command set)
  oled_command(0x08);  //display off, cursor off, blink off
  oled_command(0x2A);  //function set (extended command set)
  oled_command(0x79);  //OLED command set enabled
  oled_command(0xD5);  //set display clock divide ratio/oscillator frequency
  oled_command(0x70);  //set display clock divide ratio/oscillator frequency
  oled_command(0x78);  //OLED command set disabled
  oled_command(0x08);  //extended function set (2-lines)
  oled_command(0x06);  //COM SEG direction
	oled_command(0x72);  //function selection B, disable internal Vdd regualtor
	oled_data(0x00);     //ROM CGRAM selection
	oled_command(0x2A);  //function set (extended command set)
	oled_command(0x79);  //OLED command set enabled
	oled_command(0xDA);  //set SEG pins hardware configuration
	oled_command(0x00);
	oled_command(0xDC);  //function selection C
	oled_command(0x00);  //function selection C
	oled_command(0x81);  //set contrast control
	oled_command(0x7F);  //set contrast control
	oled_command(0xD9);  //set phase length
	oled_command(0xF1);  //set phase length
	oled_command(0xDB);  //set VCOMH deselect level
	oled_command(0x40);  //set VCOMH deselect level
	oled_command(0x78);  //OLED command set disabled
	oled_command(0x28);  //function set (fundamental command set)
	oled_command(0x01);  //clear display
	oled_command(0x80);  //set DDRAM address to 0x00
	oled_command(0x0C);  //display ON
  delay(100);
}

void oled_write(unsigned int line, const char *text) {
  oled_command(0x28); // fundamental command set

  // set DDRAM address to start of selected line
  unsigned char ddram_addr = 0x80 | (line << 6);
  oled_command(ddram_addr);

  int len = strlen(text);
  for (int i=0; i < len; i++) {
    oled_data((unsigned char)text[i]);
  }
}

void oled_clear() {
  oled_command(0x28); // fundamental command set
  oled_command(0x01); // clear display
}

