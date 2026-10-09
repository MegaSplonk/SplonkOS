#include <util/delay.h>
#if defined(SPLONKLIB_UART_USB)

#include "lib/grb.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include "lib/usb/cdc.h"

GRBIO_INIT(UART_BUF, UART_IBUF, UART_OBUF, UART_IBUF_LEN, UART_OBUF_LEN);

static void _inner_uart_send(void) {
	int avail = usb_serial_availableForWrite();

	if (avail > 0) {
		uint8_t buf[UART_OBUF_LEN];
		usize	read_len;
		grb_read(&UART_BUF.output, buf, (usize)avail, &read_len);
		usb_serial_write(buf, read_len);
	}
}

void uart_init(void) {
	pu_cdc();

	timer_ontick_register(T_ONTICK_SLOT_UART, _inner_uart_send);
}

void uart_poll(void) {}

void uart_tx(u8 data) { usb_serial_write1(data); }
// should this be impled like this ?
u8 uart_rx(void) {
	int ret;
	while ((ret = usb_serial_read()) < 0) {
		_delay_ms(1);
	}
	return (u8)ret;
}

#endif
