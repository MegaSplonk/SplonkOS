#include "lib/config.h"
#include "lib/grb.h"
#include "lib/lib.h"
#include "lib/proto/uart.h"
#include <util/atomic.h>

#if defined(SPLONKLIB_ENABLE_UART)

extern grbio UART_BUF;

void uart_sendstring(const char *str) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		usize len = 0;
		if (!str)
			return;
		while (str[len])
			len++;
		(void)grb_write(&UART_BUF.output, (const void *)str, len, nullptr);
	}
}
void uart_send_u8(uint8_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	char	buf[4] = {0, 0, 0, 0};
	uint8_t idx = 0;
	bool	print = false;

	uint8_t modulus = 100;
	while (modulus) {
		uint8_t digit = val / modulus;
		if (print || digit != 0) {
			print = true;
			buf[idx++] = (char)('0' + digit);
		}
		val %= modulus;
		modulus /= 10;
	}

	uart_sendstring(buf);
}

void uart_send_u16(uint16_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	char	buf[6] = {0, 0, 0, 0, 0, 0};
	uint8_t idx = 0;
	bool	print = false;

	uint16_t modulus = 10000;
	while (modulus) {
		uint8_t digit = (uint8_t)(val / modulus);
		if (print || digit != 0) {
			print = true;
			buf[idx++] = (char)('0' + digit);
		}
		val %= modulus;
		modulus /= 10;
	}
	uart_sendstring(buf);
}

void uart_send_i16(int16_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	if (val == -32768) {
		uart_sendstring("-32768");
		return;
	}
	if (val < 0) {
		uart_tx('-');
		val = -val;
	}
	uart_send_u16((uint16_t)val);
}

void uart_send_u32(uint32_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	char	buf[11] = {};
	uint8_t idx = 0;
	bool	print = false;

	uint32_t modulus = 1000000000;
	while (modulus) {
		uint8_t digit = (uint8_t)(val / modulus);
		if (print || digit != 0) {
			print = true;
			buf[idx++] = (char)('0' + digit);
		}
		val %= modulus;
		modulus /= 10;
	}
	uart_sendstring(buf);
}

void uart_send_u8_hex(uint8_t val) {
	char buf[3] = {0, 0, 0};
	buf[0] = "0123456789abcdef"[(val >> 4) & 0x0F];
	buf[1] = "0123456789abcdef"[(val >> 0) & 0x0F];

	uart_sendstring(buf);
}

void uart_send_u16_hex(uint16_t val) {
	char buf[5] = {0, 0, 0, 0, 0};

	buf[0] = "0123456789abcdef"[(val >> 12) & 0x0F];
	buf[1] = "0123456789abcdef"[(val >> 8) & 0x0F];
	buf[2] = "0123456789abcdef"[(val >> 4) & 0x0F];
	buf[3] = "0123456789abcdef"[(val >> 0) & 0x0F];
	uart_sendstring(buf);
}

#endif
