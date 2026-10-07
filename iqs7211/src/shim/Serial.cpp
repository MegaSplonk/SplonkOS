#include "shim/Serial.h"

extern "C" {
#include "usb_cdc.h"
#include <avr/io.h>
#include <math.h>
#include <string.h>
#include <util/delay.h>
}

static usize ft_strlen(const char *s) {
	if (s == nullptr)
		return 0;
	usize i = 0;
	while (s[i])
		i++;
	return i;
}

// Public Methods //////////////////////////////////////////////////////////////

/* default implementation: may be overridden */
usize Serial_::write(const uint8_t *buffer, usize size) { return usb_serial_write(buffer, size); }
usize Serial_::write(uint8_t n) { return usb_serial_write(&n, 1); }

usize Serial_::print(const char str[]) {
	return Serial_::write((const uint8_t *)str, ft_strlen(str));
}

usize Serial_::print(char c) { return write(c); }

usize Serial_::print(unsigned char b, int base) { return print((unsigned long)b, base); }

usize Serial_::print(int n, int base) { return print((long)n, base); }

usize Serial_::print(unsigned int n, int base) { return print((unsigned long)n, base); }

usize Serial_::print(long n, int base) {
	if (base == 0) {
		return write(n);
	} else if (base == 10) {
		if (n < 0) {
			int t = print('-');
			n = -n;
			return printNumber(n, 10) + t;
		}
		return printNumber(n, 10);
	} else {
		return printNumber(n, base);
	}
}

usize Serial_::print(unsigned long n, int base) {
	if (base == 0)
		return write(n);
	else
		return printNumber(n, base);
}

usize Serial_::println(void) { return Serial_::write((const uint8_t *)"\r\n", 2); }

usize Serial_::println(const char c[]) {
	usize n = print(c);
	n += println();
	return n;
}

usize Serial_::println(char c) {
	usize n = print(c);
	n += println();
	return n;
}

usize Serial_::println(unsigned char b, int base) {
	usize n = print(b, base);
	n += println();
	return n;
}

usize Serial_::println(int num, int base) {
	usize n = print(num, base);
	n += println();
	return n;
}

usize Serial_::println(unsigned int num, int base) {
	usize n = print(num, base);
	n += println();
	return n;
}

usize Serial_::println(long num, int base) {
	usize n = print(num, base);
	n += println();
	return n;
}

usize Serial_::println(unsigned long num, int base) {
	usize n = print(num, base);
	n += println();
	return n;
}

// Private Methods /////////////////////////////////////////////////////////////

usize Serial_::printNumber(unsigned long n, uint8_t base) {
	char  buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
	char *str = &buf[sizeof(buf) - 1];

	*str = '\0';

	// prevent crash if called with base == 1
	if (base < 2)
		base = 10;

	do {
		char c = n % base;
		n /= base;

		*--str = c < 10 ? c + '0' : c + 'A' - 10;
	} while (n);

	return write((const uint8_t *)str, ft_strlen(str));
}

bool Serial_::available(void) { return usb_serial_available() > 0; }

void Serial_::begin(long int baud_rate) {
	(void)(baud_rate);
	pu_cdc();
}

Serial_::operator bool(void) {
	bool result = false;
	if (usb_serial_line_state() > 0)
		result = true;
	_delay_ms(10);
	return result;
}

uint8_t Serial_::read(void) { return usb_serial_read(); }

// ah yes, arduino's `.` namespacing
Serial_ Serial;
