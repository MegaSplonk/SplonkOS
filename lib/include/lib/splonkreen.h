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

void send_command(u8 x);
void send_data(u8 x);
void set_column(u8 col);
void display_screen();
void full_buff(u8 page, int column, int symbol);
int	 ft_strlen(const char *str);
void send_sentence(const char *str, u8 page, int column);
void clear_screen(void);
void draw(u8 page, u8 col, t_image img);
void screen_init(void);
