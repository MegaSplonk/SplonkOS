#pragma once

#include "lib/font_8x6.h"
#include "lib/images.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/spi.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

void send_command(uint8_t x);
void send_data(uint8_t x);
void set_column(uint8_t col);
void display_screen();
void full_buff(uint8_t page, int column, int symbol);
int	 ft_strlen(const char *str);
void send_sentence(const char *str, uint8_t page, int column);
void clear_screen(void);
void draw(uint8_t page, uint8_t col, t_image img);
void screen_init(void);
