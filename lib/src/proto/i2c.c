#include <avr/io.h>
#include <util/twi.h>

#include "lib/config.h"
#include "lib/grb.h"
#include "lib/lib.h"
#include "lib/proto/i2c.h"

#ifdef SPLONKLIB_ENABLE_I2C

#define I2C_CLOCK_SPEED ((F_CPU / I2C_CLOCK) - 16) / 2

GRBIO_INIT(i2c_io, i2c_in, i2c_out, 64, 64)
// Own grbio dedicated to outbound general call data
// or inbound addressed or general call data

i2c_peer peers[I2C_MAX_PEERS] = {{0, &i2c_io, 0}};

i2c_state state = {};

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

// define an 'initial' addr that devices can use temporarily?
// just have the devices ask the main mcu using general call?
// other way around, have the main mcu ask the devices with gc?
// devices stop listening to gc after main mcu finds them?
void i2c_set_addr(u8 addr, bool answer_gc) {
	// need to set Slave Receive address here,
	// and with the LSB we can choose to respond (1) to the General Call or
	// ignore it (0)
	TWAR = (addr << 1) | answer_gc;
}

void i2c_init(void) {
	// clear the status registers
	TWSR = 0x00;
	// set the i2c clock speed
	TWBR = I2C_CLOCK_SPEED;
}

// remember to set a slave address before listening
void i2c_listen(void) {
	// TWEN must be written to one to enable the TWI.
	// The TWEA bit must be written to one to enable the
	// acknowledgement of the device’s own slave address or the general call
	// address
	//     SET SLAVE |  ENABLE  ;
	TWCR = _BV(TWEA) | _BV(TWEN);
	state.mode = TW_YE;
}

bool i2c_is_listening() {
	return ((TWCR & (_BV(TWEA) | _BV(TWEN))) == (_BV(TWEA) | _BV(TWEN)));
}

void i2c_ignore(void) {
	// While TWEA is zero, the TWI does not acknowledge its own slave address.
	TWCR &= (u8)~_BV(TWEA);
	state.mode = TW_UH;
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
	// status code can either be sla ack, sla nack or arbitration lost
	// -> nack: send stop since slave unreachable
	// -> lost arbitration: either get addressed as slave or wait and then send
	// start again
}

t_error i2c_write(uint8_t data) {
	// set data to write
	TWDR = data;
	//     CLR INT    | ENABLE   ;
	TWCR = _BV(TWINT) | _BV(TWEN);

	i2c_wait_int();
	return i2c_check_data_ack();
	// status code can either be data ack, data nack or arbitration lost
	// -> nack: send data again? stop transmission?
	// -> lost arbitration: either get addressed as slave or wait and then send
	// start again
}

uint8_t i2c_read_ack(void) {
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
	i2c_wait_int();
	return TWDR;
}

uint8_t i2c_read_nack(void) {
	TWCR = _BV(TWINT) | _BV(TWEN);
	i2c_wait_int();
	// need to handle case when other master nacks
	return TWDR;
}

t_error i2c_send(u8 addr, u8 *data, usize len) {
	if (!len || i2c_start() || i2c_select(addr, TW_WRITE))
		return ERROR;

	t_error e = SUCCESS;
	for (usize i = 0; e == SUCCESS && i < len; i++)
		e = i2c_write(data[i]); // TODO - check bus errors?
	if (e)
		switch (TW_STATUS) {
		case (TW_MT_DATA_NACK):
			break;
		case (TW_MT_ARB_LOST):
			return ERROR;
		};
	i2c_stop();
	return SUCCESS;
}

t_error i2c_query(u8 addr, u8 *data, usize *len) {
	// if (!*len || i2c_start() || i2c_select(addr, TW_WRITE))
	// 	return ERROR;
	if (!*len || i2c_start() || i2c_select(addr, TW_READ))
		return ERROR;
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

t_error i2c_send_async(i2c_peer *p, u8 *data, usize len) {
	grbio *g = p->io;

	if (grb_full(&g->output) || grb_avail(&g->output) < len)
		return ERROR;

	usize useless;
	return grb_write(&g->output, data, len, &useless);
}

t_error i2c_query_async(i2c_peer *p, u8 *data, usize len) {
	grbio *g = p->io;

	if (grb_empty(&g->input) || grb_count(&g->input) < len) {
		p->query = true;
		return ERROR;
	}
	p->query = false;

	usize useless;
	return grb_read(&g->input, data, len, &useless);
}

static t_error i2c_write_async(i2c_peer *p) {
	grbio *g = p->io;
	if (grb_empty(&g->output))
		return ERROR;
	u8	  data;
	usize useless;
	grb_read(&g->output, &data, 1, &useless);
	// set data to write
	TWDR = data;
	//     CLR INT    | ENABLE   ;
	TWCR = _BV(TWINT) | _BV(TWEN);
	i2c_wait_int(); // wait?
	return SUCCESS;
}

static t_error i2c_read_async(i2c_peer *p) {
	grbio *g = p->io;
	if (grb_full(&g->input))
		return ERROR;
	TWCR = _BV(TWINT) | _BV(TWEN) | (grb_avail(&g->input) == 1 ? 0 : _BV(TWEA));
	i2c_wait_int();
	usize useless;
	u8	  data = TWDR;
	return grb_write(&g->input, &data, 1, &useless);
}

static void find_peer() {
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {
		grbio *g = peers[peer].io;

		if (!g || (grb_empty(&g->output) && !peers[peer].query))
			continue;

		state.peer_idx = peer;
		return;
	}
	state.peer_idx = 0;
}

static void i2c_keep_or_rel_bus() {
	find_peer();
	grbio *g = peers[state.peer_idx].io;
	if (!grb_empty(&g->output)) {
		state.mode = TW_MT;
	} else if (peers[state.peer_idx].query) {
		state.mode = TW_MR;
	} else {
		if (state.mode != TW_MR && state.mode != TW_MT)
			i2c_stop();
		return;
	}
	i2c_start();
}

static void handle_bus_error() {
	// To recover from a bus error, the TWSTO Flag must set
	// and TWINT must be cleared by writing a logic one to it
	TWCR |= _BV(TWINT) | _BV(TWSTO);
	i2c_wait_int(); // Never wait for int? So that polling is truly 'async'?
}

static bool hear_ye_hear_ye() {
	if (!i2c_is_listening())
		return false;
	// TWINT is set to signify something happened while listening
	return (TWCR & _BV(TWINT));
}

void i2c_poll(void) {
	switch (TW_STATUS) {
	case (TW_START): {
		// data = "start condition transmitted";
		// cool bro now select a Slave
		i2c_select(peers[state.peer_idx].addr,
				   state.mode == TW_MR ? TW_READ : TW_WRITE);
		break;
	}
	case (TW_REP_START): {
		// data = "repeated start condition transmitted";
		// cool bro now select a slave
		i2c_select(peers[state.peer_idx].addr,
				   state.mode == TW_MR ? TW_READ : TW_WRITE);
		break;
	}
	case (TW_MT_SLA_ACK): {
		// data = "SLA+W transmitted, ACK received";
		state.mode = TW_MT;
		// ok now either load a data byte
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		if (i2c_write_async(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MT_SLA_NACK): {
		// data = "SLA+W transmitted, NACK received";
		// ok now either load a data byte
		// (apparently you can still load data after a NACK?)
		// (you can talk to a wall, cool)
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		if (i2c_write_async(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MT_DATA_ACK): {
		// data = "data transmitted, ACK received";
		// ok now either load a data byte
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		if (i2c_write_async(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MT_DATA_NACK): {
		// data = "data transmitted, NACK received";
		// ok now either load a data byte
		// (apparently you can still load data after a NACK?)
		// (you can talk to a wall, cool)
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MT_ARB_LOST): {
		// data = "arbitration lost in SLA+W or data";
		// just set TWINT to accept fate
		// can optionally set TWSTA to try and get a START asap
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MR_SLA_ACK): {
		// data = "SLA+R transmitted, ACK received";
		state.mode = TW_MR;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_MR_SLA_NACK): {
		// data = "SLA+R transmitted, NACK received";
		// Either send Repeated start or Stop or both
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MR_DATA_ACK): {
		// data = "data received, ACK returned";
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_MR_DATA_NACK): {
		// data = "data received, NACK returned";
		// Either send Repeated start or Stop or both
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_SLA_ACK): {
		// data = "SLA+R received, ACK returned";
		state.mode = TW_ST;
		// Load data byte and either set TWEA to signify more data is coming
		// or unset it to signify last byte (in which case NACK is expected from
		// master)
		if (i2c_write_async(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_ARB_LOST_SLA_ACK): {
		// data = "arbitration lost in SLA+RW, SLA+R received, ACK returned";
		state.mode = TW_ST;
		state.peer_idx = 0;
		// Load data byte and either set TWEA to signify more data is coming
		// or unset it to signify last byte (in which case NACK is expected)
		if (i2c_write_async(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_DATA_ACK): {
		// data = "data transmitted, ACK received";
		// Load data byte and either set TWEA to signify more data is coming
		// or unset it to signify last byte (in which case NACK is expected)
		if (i2c_write_async(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_DATA_NACK): {
		// data = "data transmitted, NACK received";
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_LAST_DATA): {
		// data = "last data byte transmitted, ACK received";
		// Can either keep listening
		// Or stop listening
		// Or prepare to send start asap while not listening
		// Or prepare to send start asap while listening
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_SLA_ACK): {
		// data = "SLA+W received, ACK returned";
		state.mode = TW_SR;
		state.peer_idx = 0;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_ARB_LOST_SLA_ACK): {
		// data = "arbitration lost in SLA+RW, SLA+W received, ACK returned";
		state.mode = TW_SR;
		state.peer_idx = 0;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_GCALL_ACK): {
		// data = "general call received, ACK returned";
		state.mode = TW_SR;
		state.peer_idx = 0;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_DATA_ACK): {
		// data = "data received, ACK returned";
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_ARB_LOST_GCALL_ACK): {
		// data = "arbitration lost in SLA+RW, general call received, ACK "
		// "returned";
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_DATA_NACK): {
		// data = "data received, NACK returned";
		// Can either keep listening
		// Or stop listening
		// Or prepare to send start asap while not listening
		// Or prepare to send start asap while listening
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_GCALL_DATA_ACK): {
		// data = "general call data received, ACK returned";
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read_async(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_GCALL_DATA_NACK): {
		// data = "general call data received, NACK returned";
		// Can either keep listening
		// Or stop listening
		// Or prepare to send start asap while not listening
		// Or prepare to send start asap while listening
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_STOP): {
		// data = "stop or repeated start condition received while selected";
		// Either listen or don't, and/or get a START asap or don't.
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_NO_INFO): {
		// data = "no state information available";
		switch (state.mode) {
		case (TW_YE): {
			if (hear_ye_hear_ye())
				break;
			[[fallthrough]];
		}
		case (TW_UH): {
			i2c_keep_or_rel_bus();
			break;
		}
		default:
			break;
		}
		break;
	}
	case (TW_BUS_ERROR): {
		// data = "illegal start or stop condition";
		handle_bus_error();
		break;
	}
	}
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
		data = "data received, ACK returned";
		break;
	}
	case (TW_SR_ARB_LOST_GCALL_ACK): {
		data = "arbitration lost in SLA+RW, general call received, ACK "
			   "returned";
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
