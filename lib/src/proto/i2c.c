#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include "lib/config.h"
#include "lib/grb.h"
#include "lib/lib.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/time.h"

#ifdef SPLONKLIB_ENABLE_I2C

#define I2C_CLOCK_SPEED ((F_CPU / I2C_CLOCK) - 16) / 2

#if defined(MCU_atmega328p) || defined(MCU_atmega32u4)

#define TWI_INTERRUPT TWI_vect

#elif defined(MCU_attiny1614)

#define TWI_INTERRUPT TWI0_TWIS_vect

#endif

GRBIO_INIT(i2c_io, i2c_in, i2c_out, 32, 32);
// Own grbio dedicated to outbound general call data
// or inbound addressed or general call data

static i2c_peer peers[I2C_MAX_PEERS] = {{0, &i2c_io, 0, 0}};

i2c_state state = {};

static void print_status_reg(const char *prefix);

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
	TWSR =  _BV(TWPS1) | _BV(TWPS0); // SHIT WAS GETTING STUCK BECAUSE THE PRESCALER WASNT BIG ENOUGH
	// set the i2c clock speed
	TWBR = I2C_CLOCK_SPEED;

	timer_ontick_register(T_ONTICK_SLOT_I2C, i2c_poll);
}

// remember to set a slave address before listening
void i2c_listen(void) {
	// TWEN must be written to one to enable the TWI.
	// The TWEA bit must be written to one to enable the
	// acknowledgement of the device’s own slave address or the general call
	// address
	//     SET SLAVE |  ENABLE   | INTERRUPT;
	TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
	state.mode = TW_YE;
}

bool i2c_is_listening() { return ((TWCR & (_BV(TWEA) | _BV(TWEN))) == (_BV(TWEA) | _BV(TWEN))); }

void i2c_ignore(void) {
	// While TWEA is zero, the TWI does not acknowledge its own slave address.
	TWCR &= (u8)~_BV(TWEA);
	state.mode = TW_UH;
}

static void i2c_start(void) {
	//     CLR INT    | SET MASTER |  ENABLE   | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
	// debug_print("start %d", state.peer_idx);
}

static void i2c_stop(void) {
	//     CLR INT    | ENABLE    |  STOP     | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO)| _BV(TWIE);
	// debug_print("stop %d", state.mode);
}

static void i2c_int(void) {
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
}

static void i2c_select(uint8_t addr, uint8_t rw) {
	// set addr to write
	TWDR = (addr << 1) | rw;
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);

	// debug_print("select %d", state.mode);
}

t_error i2c_register_peer(i2c_peer **p, grbio *io, u8 addr) {
	if (!io) {
		*p = &peers[0];
		return SUCCESS;
	}
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {

		if (peers[peer].addr || peers[peer].io)
			continue;

		peers[peer].io = io;
		peers[peer].addr = addr;
		peers[peer].query = 0;
		*p = &peers[peer];

		return SUCCESS;
	}
	return ERROR;
}

t_error i2c_send(i2c_peer *p, u8 *data, usize len) {
	grbio *g = p->io;

	if (grb_full(&g->output) || grb_avail(&g->output) < len){
    // debug_print("OUTBUFFULL%d-%d", state.mode, TWCR);
		return ERROR;
  }

	usize	useless;
	t_error e = grb_write(&g->output, data, len, &useless);
	debug_print("I'm alive %p, %d, %p, %d", (void *)p, p->addr, (void *)p->io, useless);
	return e;
}

t_error i2c_query(i2c_peer *p, u8 *data, usize len) {
	grbio *g = p->io;

	if (grb_empty(&g->input) || grb_count(&g->input) < len) {
		p->query = true;
		return ERROR;
	}
	p->query = false;

	usize useless;
	return grb_read(&g->input, data, len, &useless);
}

static t_error i2c_write(i2c_peer *p) {
	grbio *g = p->io;
	if (grb_empty(&g->output)){
    // debug_print("OUTBUFEMPTY");
		return ERROR;
  }
	u8	  data;
	usize useless;
	grb_read(&g->output, &data, 1, &useless);
	// set data to write
	TWDR = data;
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
	return SUCCESS;
}

static t_error i2c_read(i2c_peer *p) {
	grbio *g = p->io;
	if (grb_full(&g->input)) {
    // debug_print("INBUFFULL");
		return ERROR;
  }
	TWCR = _BV(TWINT) | _BV(TWEN) | (grb_avail(&g->input) == 1 ? 0 : _BV(TWEA)) | _BV(TWIE);
	usize useless;
	u8	  data = TWDR;
	return grb_write(&g->input, &data, 1, &useless);
}

static void find_peer() {
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {
		grbio *g = peers[peer].io;

		if (!g || (peers[peer].nack && peers[peer].nack--) ||
			(grb_empty(&g->output) && !peers[peer].query))
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
		i2c_start();
	} else if (peers[state.peer_idx].query) {
		state.mode = TW_MR;
		i2c_start();
	} else if (state.mode == TW_MR || state.mode == TW_MT) {
		i2c_stop();
		i2c_listen();
	} else if (state.mode == TW_ST || state.mode == TW_SR) {
		i2c_listen();
	}
}

static void handle_bus_error() {
	// To recover from a bus error, the TWSTO Flag must set
	// and TWINT must be cleared by writing a logic one to it
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO) | _BV(TWIE);
}

void i2c_poll(void) {
  if (state.mode == TW_UH || state.mode == TW_YE)
		i2c_keep_or_rel_bus();
  // if (test++ > 1000) {
  //   test = 0;
  //   debug_print("%d %d %d I%d I%d O%d", TW_STATUS, TWCR, state.mode, grb_count(&peers[0].io->input), grb_count(&peers[state.peer_idx].io->input), grb_count(&peers[state.peer_idx].io->output));
  //   // print_status_reg("poll: ");
  //   if (TWCR == 37 && reset++ > 5) {
  //     reset = 0;
  //     i2c_listen();
  //     timeout = 10;
  //   }
  //   if (timeout) timeout--;
  // }
}

ISR(TWI_INTERRUPT) {
	const char *data = nullptr;
	switch (TW_STATUS) {
	case (TW_START): {
		// data = "start condition tx";
		// debug_print("%s", data);
		// cool bro now select a Slave
		i2c_select(peers[state.peer_idx].addr, state.mode == TW_MR ? TW_READ : TW_WRITE);
		break;
	}
	case (TW_REP_START): {
		// data = "repeated start condition tx";
		// debug_print("%s", data);
		// cool bro now select a slave
		i2c_select(peers[state.peer_idx].addr, state.mode == TW_MR ? TW_READ : TW_WRITE);
		break;
	}
	case (TW_MT_SLA_ACK): {
		// data = "SLA+W tx, ACK rx";
		// debug_print("%s", data);
		state.mode = TW_MT;
		peers[state.peer_idx].nack = 0;
		// ok now either load a data byte
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		if (i2c_write(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MT_SLA_NACK): {
		// data = "SLA+W tx, NACK rx";
		// debug_print("%s", data);
		peers[state.peer_idx].nack = 10;
		// ok now either load a data byte
		// (apparently you can still load data after a NACK?)
		// (you can talk to a wall, cool)
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		i2c_stop();
		i2c_listen();
		break;
	}
	case (TW_MT_DATA_ACK): {
		// data = "data tx, ACK rx";
		// debug_print("%s", data);
		// ok now either load a data byte
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		if (i2c_write(&peers[state.peer_idx])) {
			// debug_print("last byte sent");
			i2c_keep_or_rel_bus();
		}
		break;
	}
	case (TW_MT_DATA_NACK): {
		// data = "data tx, NACK rx";
		// debug_print("%s", data);
		// ok now either load a data byte
		// (apparently you can still load data after a NACK?)
		// (you can talk to a wall, cool)
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		i2c_stop();
		i2c_listen();
		break;
	}
	case (TW_MT_ARB_LOST): {
		// data = "arbitration lost in SLA+W or data";
		// debug_print("%s", data);
		// just set TWINT to accept fate
		// can optionally set TWSTA to try and get a START asap
		i2c_int();
		i2c_listen();
		break;
	}
	case (TW_MR_SLA_ACK): {
		// data = "SLA+R tx, ACK rx";
		// debug_print("%s", data);
		peers[state.peer_idx].nack = 0;
		state.mode = TW_MR;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_MR_SLA_NACK): {
		// data = "SLA+R tx, NACK rx";
		// debug_print("%s", data);
		peers[state.peer_idx].nack = 10000;
		// Either send Repeated start or Stop or both
		i2c_stop();
		i2c_listen();
		break;
	}
	case (TW_MR_DATA_ACK): {
		// data = "data rx, ACK returned";
		// debug_print("%s", data);
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_MR_DATA_NACK): {
		// data = "data rx, NACK returned";
		// debug_print("%s", data);
		// Either send Repeated start or Stop or both
		i2c_stop();
		i2c_listen();
		break;
	}
	case (TW_ST_SLA_ACK): {
		// data = "SLA+R rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_ST;
		// Load data byte and either set TWEA to signify more data is coming
		// or unset it to signify last byte (in which case NACK is expected from
		// master)
		if (i2c_write(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_ARB_LOST_SLA_ACK): {
		// data = "arbitration lost in SLA+RW, SLA+R rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_ST;
		state.peer_idx = 0;
		// Load data byte and either set TWEA to signify more data is coming
		// or unset it to signify last byte (in which case NACK is expected)
		if (i2c_write(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_DATA_ACK): {
		// data = "data tx, ACK rx";
		// debug_print("%s", data);
		// Load data byte and either set TWEA to signify more data is coming
		// or unset it to signify last byte (in which case NACK is expected)
		if (i2c_write(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_DATA_NACK): {
		// data = "data tx, NACK rx";
		// debug_print("%s", data);
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_ST_LAST_DATA): {
		// data = "last data byte tx, ACK rx";
		// debug_print("%s", data);
		// Can either keep listening
		// Or stop listening
		// Or prepare to send start asap while not listening
		// Or prepare to send start asap while listening
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_SLA_ACK): {
		// data = "SLA+W rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_SR;
		state.peer_idx = 0;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_ARB_LOST_SLA_ACK): {
		// data = "arbitration lost in SLA+RW, SLA+W rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_SR;
		state.peer_idx = 0;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_GCALL_ACK): {
		// data = "general call rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_SR;
		state.peer_idx = 0;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_DATA_ACK): {
		// data = "data rx, ACK returned";
		// debug_print("%s", data);
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_ARB_LOST_GCALL_ACK): {
		// data = "arbitration lost in SLA+RW, general call rx, ACK returned";
		// debug_print("%s", data);
    state.mode = TW_SR;
		state.peer_idx = 0;
		// "returned";
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_DATA_NACK): {
		// data = "data rx, NACK returned";
		// debug_print("%s", data);
		// Can either keep listening
		// Or stop listening
		// Or prepare to send start asap while not listening
		// Or prepare to send start asap while listening
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_GCALL_DATA_ACK): {
		// data = "general call data rx, ACK returned";
		// debug_print("%s", data);
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_GCALL_DATA_NACK): {
		// data = "general call data rx, NACK returned";
		// debug_print("%s", data);
		// Can either keep listening
		// Or stop listening
		// Or prepare to send start asap while not listening
		// Or prepare to send start asap while listening
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_STOP): {
		// data = "stop or repeated start condition rx while selected";
		// Either listen or don't, and/or get a START asap or don't.

		if (state.mode != TW_YE && state.mode != TW_UH) {
			// debug_print("%s %d %d", data, state.mode, i2c_is_listening());
			i2c_keep_or_rel_bus();
			// debug_print("%s %d %d", data, state.mode, i2c_is_listening());
		}
		break;
	}
	case (TW_NO_INFO): {
		data = "no state information available";
		break;
	}
	case (TW_BUS_ERROR): {
		// data = "illegal start or stop condition";
		// debug_print("%s", data);
		handle_bus_error();
		break;
	}
	}
	(void)data;
	// debug_print("I2C POLL: %s", data);
}

#ifdef SPLONKLIB_ENABLE_UART

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
