#include "../include/splonkreen.h"

#include "../include/font_8x6.h"

#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR	126
#define FONT_WIDTH		6

#define DC	PD4
#define RES PD7
#define CS	PD6

static uint8_t screenBuffer[8][128];




void screen_init(void) {
	// Power up Sequence (4.1)
	DDRD |= (1 << DC) | (1 << RES); // DC et RES en sortie

	// pulse of reset
	PORTD &= (uint8_t)~(1 << RES);
	_delay_ms(10);
	PORTD |= (uint8_t)(1 << RES);
	_delay_ms(10);

	spi_init();

	send_command(0xAE); // display off
	send_command(0xAD);
	send_command(0x8B); // charge pump ON
	send_command(0x33); // VPP
	_delay_ms(100);
	send_command(0xAF); // display on

	clear_screen();
}


void send_command(uint8_t x) {
	PORTD &= (uint8_t)~(1 << DC);
	spi_write(x);
}

void send_data(uint8_t x) {
	PORTD |= (uint8_t)(1 << DC);
	spi_write(x);
}

void clear_screen(void) {
	for (uint8_t page = 0; page < 8; page++) {
		send_command(0xB0 | page);
		send_command(0x02);
		send_command(0x10);
		for (uint8_t col = 0; col < 128; col++) {
			send_data(0x00);
		}
	}
}

void set_column(uint8_t col) {
	col += 2;
	send_command(0x00 | (col & 0x0F));
	send_command(0x10 | ((col >> 4) & 0x0F));
}

void display_screen() {
	for (uint8_t page = 0; page < 8; page++) {
		send_command(0xB0 | page);
		send_command(0x02);
		send_command(0x10);
		for (int i = 0; i < 128; i++) {
			send_data(screenBuffer[page][i]);
		}
	}
}

void full_buff(uint8_t page, int column, int symbol) {
	if (page > 7 || column < 0 || column > 127) {
		return;
	}
	if (symbol < FONT_FIRST_CHAR || symbol > FONT_LAST_CHAR) {
		return;
	}

	uint8_t index = (uint8_t)(symbol - FONT_FIRST_CHAR);

	for (uint8_t i = 0; i < FONT_WIDTH; i++) {
		if (column + i > 127) {
			break;
		}
		screenBuffer[page][column + i] = pgm_read_byte(&font_8x6[index][i]);
	}
}

int ft_strlen(const char *str) {
	int i;
	for (i = 0; str[i] != 0; i++)
		;
	return (i);
}

void send_sentence(const char *str, uint8_t page, int column) {
	if (((ft_strlen(str) * 6) + column > 127) || page > 7)
		return;

	for (int i = 0; str[i] != 0; i++) {
		full_buff(page, column, str[i]);
		column += 6;
	}
}

void draw(uint8_t page, uint8_t col, struct s_img img) {
	uint8_t nb_pages = img.height / 8;

	if (page + nb_pages > 8 || col + img.width > 128)
		return;

	for (uint8_t p = 0; p < nb_pages; p++)
		for (uint8_t c = 0; c < img.width; c++)
			screenBuffer[page + p][col + c] = pgm_read_byte(&img.data[p * img.width + c]);
}
