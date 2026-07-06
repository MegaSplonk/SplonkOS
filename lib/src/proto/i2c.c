#include <avr/io.h>
#include <util/twi.h>

#include "lib/config.h"
#include "lib/grb.h"
#include "lib/proto/i2c.h"

#ifdef SPLONKLIB_ENABLE_I2C

#define I2C_CLOCK_SPEED ((F_CPU / I2C_CLOCK) - 16) / 2

GRBIO_INIT(i2c_io, i2c_in, i2c_out, 64, 64)

static void print_status_reg(const char *prefix);

static t_error i2c_check_start() { return (TWSR & 0xF8) != TW_START; }

static t_error i2c_check_sla_ack() { return (TWSR & 0xF8) != TW_MT_SLA_ACK; }

// static t_error i2c_check_sla_nack() { return (TWSR & 0xF8) != TW_MT_SLA_NACK;
// }

static t_error i2c_check_data_ack() { return (TWSR & 0xF8) != TW_MT_DATA_ACK; }

// static t_error i2c_check_data_nack() {
// 	return (TWSR & 0xF8) != TW_MT_DATA_NACK;
// }

static void i2c_wait_int() {
	// wait until done
	while (!(TWCR & _BV(TWINT)))
		;
}

void i2c_init(void) {
	// clear the status registers
	TWSR = 0x00;
	// set the i2c clock speed
	TWBR = I2C_CLOCK_SPEED;
}

t_error i2c_start(void) {
	//     CLR INT    | SET MASTER |  ENABLE  ;
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

	i2c_wait_int();

	return i2c_check_start();
}

void i2c_stop(void) {
	//     CLR INT    | ENABLE    |  STOP     ;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
}

t_error i2c_select(uint8_t addr, uint8_t rw) {
	// set addr to write
	TWDR = (addr << 1) | rw;
	//     CLR INT    | ENABLE   ;
	TWCR = _BV(TWINT) | _BV(TWEN);

	i2c_wait_int();
	return i2c_check_sla_ack();
}

t_error i2c_write(uint8_t data) {
	// set data to write
	TWDR = data;
	//     CLR INT    | ENABLE   ;
	TWCR = _BV(TWINT) | _BV(TWEN);

	i2c_wait_int();
	return i2c_check_data_ack();
}

uint8_t i2c_read_ack(void) {
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
	i2c_wait_int();
	return TWDR;
}

uint8_t i2c_read_nack(void) {
	TWCR = _BV(TWINT) | _BV(TWEN);
	i2c_wait_int();
	return TWDR;
}

t_error i2c_send(u8 addr, u8 *data, usize len) {
	if (!len || i2c_start() || i2c_select(addr, TW_WRITE))
		return ERROR;

	t_error e = SUCCESS;
	for (usize i = 0; e == SUCCESS && i < len; i++)
		e = i2c_write(data[i]); // TODO - check bus errors?
	i2c_stop();
	return SUCCESS;
}

t_error i2c_query(u8 addr, u8 *data, usize *len) {
	if (!*len || i2c_start() || i2c_select(addr, TW_WRITE))
		return ERROR;

	i2c_start(); // repeated start
	print_status_reg("[I2C] Start - ");
	i2c_select(addr, TW_READ);
	print_status_reg("[I2C] Selct - ");
	for (usize i = 0; i < *len; i++) {
		data[i] = i + 1 == *len ? i2c_read_nack() : i2c_read_ack();
		print_status_reg("[I2C] Write - ");
	}
	// TODO - check bus errors?
	i2c_stop();
	// print_status_reg("[I2C] Stop  - ");
	return SUCCESS;
}

// ASYNC STUFF

t_error i2c_send_async(grbio *gio, u8 addr, u8 *data, usize len) {
	grbio *g = gio ? gio : &i2c_io;
	(void)addr; // use a table of grbio's, one for each addr?

	if (grb_full(&g->output) || grb_avail(&g->output) < len)
		return ERROR;

	usize useless;
	return grb_write(&g->output, data, len, &useless);
}

t_error i2c_query_async(grbio *gio, u8 addr, u8 *data, usize len) {
	grbio *g = gio ? gio : &i2c_io;
	(void)addr; // use a table of grbio's, one for each addr?

	if (grb_empty(&g->input) || grb_count(&g->input) < len)
		return ERROR;

	usize useless;
	return grb_read(&g->input, data, len, &useless);
}

void i2c_poll(grbio *gio, u8 addr) {
	grbio *g = gio ? gio : &i2c_io;
	// iterate over table of grbio's?
	if (grb_empty(&g->output) && grb_full(&g->input))
		return;

	// add Slave Recv check here

	if (i2c_start() != SUCCESS)
		return;

	if (!grb_empty(&g->output)) {
		if (i2c_select(addr, TW_WRITE))
			return;

		t_error e = SUCCESS;

		for (usize i = 0; e == SUCCESS && !grb_empty(&g->output); i++) {
			u8	  data;
			usize useless;
			grb_read(&g->output, &data, 1, &useless);
			e = i2c_write(data); // TODO - check bus errors?
		}
	}

	i2c_stop();
}

#ifdef SPLONKLIB_ENABLE_UART
#include "lib/proto/uart.h"

static void print_status_reg(const char *prefix) {
	const char *data = nullptr;

	if (prefix)
		uart_sendstring(prefix);

	switch (TW_STATUS) {
	case (TW_START): {
		data = "start condition transmitted";
		break;
	}
	case (TW_REP_START): {
		data = "repeated start condition transmitted";
		break;
	}
	case (TW_MT_SLA_ACK): {
		data = "SLA+W transmitted, ACK received";
		break;
	}
	case (TW_MT_SLA_NACK): {
		data = "SLA+W transmitted, NACK received";
		break;
	}
	case (TW_MT_DATA_ACK): {
		data = "data transmitted, ACK received";
		break;
	}
	case (TW_MT_DATA_NACK): {
		data = "data transmitted, NACK received";
		break;
	}
	case (TW_MT_ARB_LOST): {
		data = "arbitration lost in SLA+W or data";
		break;
	}
	case (TW_MR_SLA_ACK): {
		data = "SLA+R transmitted, ACK received";
		break;
	}
	case (TW_MR_SLA_NACK): {
		data = "SLA+R transmitted, NACK received";
		break;
	}
	case (TW_MR_DATA_ACK): {
		data = "data received, ACK returned";
		break;
	}
	case (TW_MR_DATA_NACK): {
		data = "data received, NACK returned";
		break;
	}

	case (TW_ST_SLA_ACK): {
		data = "SLA+R received, ACK returned";
		break;
	}
	case (TW_ST_ARB_LOST_SLA_ACK): {
		data = "arbitration lost in SLA+RW, SLA+R received, ACK returned";
		break;
	}
	case (TW_ST_DATA_ACK): {
		data = "data transmitted, ACK received";
		break;
	}
	case (TW_ST_DATA_NACK): {
		data = "data transmitted, NACK received";
		break;
	}
	case (TW_ST_LAST_DATA): {
		data = "last data byte transmitted, ACK received";
		break;
	}
	case (TW_SR_SLA_ACK): {
		data = "SLA+W received, ACK returned";
		break;
	}
	case (TW_SR_ARB_LOST_SLA_ACK): {
		data = "arbitration lost in SLA+RW, SLA+W received, ACK returned";
		break;
	}
	case (TW_SR_GCALL_ACK): {
		data = "general call received, ACK returned";
		break;
	}
	case (TW_SR_DATA_ACK): {
		data = "general call received, ACK returned";
		break;
	}
	case (TW_SR_ARB_LOST_GCALL_ACK): {
		data =
			"arbitration lost in SLA+RW, general call received, ACK returned";
		break;
	}
	case (TW_SR_DATA_NACK): {
		data = "data received, NACK returned";
		break;
	}
	case (TW_SR_GCALL_DATA_ACK): {
		data = "general call data received, ACK returned";
		break;
	}
	case (TW_SR_GCALL_DATA_NACK): {
		data = "general call data received, NACK returned";
		break;
	}
	case (TW_SR_STOP): {
		data = "stop or repeated start condition received while selected";
		break;
	}
	case (TW_NO_INFO): {
		data = "no state information available";
		break;
	}
	case (TW_BUS_ERROR): {
		data = "illegal start or stop condition";
		break;
	}
	}

	if (data == nullptr)
		data = "(no status)";
	uart_sendstring(data);
	uart_sendstring("\r\n");
}

#elifndef SPLONKLIB_ENABLE_UART

static void print_status_reg(const char *prefix) {
	(void)prefix;
	return;
}

#endif

#endif
