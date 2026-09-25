#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <util/twi.h>

#include "lib/config.h"
#include "lib/grb.h"
#include "lib/lib.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/time.h"

#ifdef SPLONKLIB_ENABLE_I2C

#define I2C_CLOCK_SPEED ((F_CPU / I2C_CLOCK) - 16) / 2

#if defined(MCU_atmega328p) || defined(MCU_atmega32u4)

#define TWI_UNIFIED
#define TWI_INTERRUPT TWI_vect

#elif defined(MCU_attiny1614)

#define TWI_SPLIT

#define TWI_INTERRUPT_S TWI0_TWIS_vect
#define TWI_INTERRUPT_M TWI0_TWIM_vect

#endif

static i2c_peer peers[I2C_MAX_PEERS] = {};
static i2c_slot slots[I2C_MAX_SLOTS] = {};

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
#ifdef TWI_UNIFIED
	TWAR = (addr << 1) | answer_gc;
#elifdef TWI_SPLIT
	TWI0.SADDR = (u8)(addr << 1) | answer_gc;
#endif
}

void i2c_init(void) {
#ifdef TWI_UNIFIED
	// // clear the status registers
	// TWSR = _BV(TWPS1); // _BV(TWPS0) |  SHIT WAS GETTING STUCK BECAUSE THE PRESCALER WASNT BIG ENOUGH
	// // set the i2c clock speed
	// TWBR = I2C_CLOCK_SPEED;
  TWSR = 0;       // TWPS = 0
  TWBR = ((F_CPU / I2C_CLOCK) - 16) / 2;
#elifdef TWI_SPLIT
  // TODO - setup stuff here
#endif
	timer_ontick_register(T_ONTICK_SLOT_I2C, i2c_poll);
}

// remember to set a slave address before listening
void i2c_listen(void) {
	// TWEN must be written to one to enable the TWI.
	// The TWEA bit must be written to one to enable the
	// acknowledgement of the device’s own slave address or the general call
	// address
#ifdef TWI_UNIFIED
	//     SET SLAVE |  ENABLE   | INTERRUPT;
	TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
#elifdef TWI_SPLIT
	//            DATA INTRPT | ADDR INTRPT  | STOP INTRPT | SMART MODE  | ENABLE       ;
	TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
#endif
	state.mode = TW_YE;
}

bool i2c_is_listening() {
#ifdef TWI_UNIFIED
  return ((TWCR & (_BV(TWEA) | _BV(TWEN))) == (_BV(TWEA) | _BV(TWEN)));
#elifdef TWI_SPLIT
  return ((TWI0.SCTRLA & TWI_ENABLE_bm) != 0);
#else
  return false;
#endif
}

void i2c_ignore(void) {
	// While TWEA is zero, the TWI does not acknowledge its own slave address.
#ifdef TWI_UNIFIED
	TWCR &= (u8)~_BV(TWEA);
#elifdef TWI_SPLIT
	// disable slave TWI;
	TWI0.SCTRLA = 0;
#endif
	state.mode = TW_UH;
}

// #ifdef SPLONK
const ipd directions[] = {IPD_NONE, IPD_NORTH, IPD_EAST, IPD_SOUTH, IPD_WEST, IPD_ALL};
const ipt types[] = {IPT_SPLONK, IPT_SPLONKO, IPT_SPLONKUS, IPT_SPLONKPAD, IPT_SPLONKNOB, IPT_SPLONKREEN, IPT_SPLONKBAR};
const pin bus_enable[] = (pin[])I2C_BE_PINS;
const u8 be_logic[] = I2C_BE_LOGIC;
const u8 addr_pool[] = I2C_ADDR_POOL;
bool addr_used[tabsize(addr_pool)] = {};

static void i2c_bus_enable(void) {
  ipd dir = state.peer_idx == I2C_GCALL_IDX ? state.bus_enable : IPD_ALL;
  for (u8 be = 0; be < tabsize(bus_enable); be++)
		pin_output(bus_enable[be]),
      pin_set(bus_enable[be], (bool)(be_logic[dir] & (1 << be)));
}

static ipd i2c_bus_sense(void) {
  ipd dir = IPD_NONE;
  for (u8 d = 0; d < tabsize(directions); d++) {
    for (u8 be = 0; be < tabsize(bus_enable); be++) {
		  pin_input(bus_enable[be]);
      if ((be_logic[d] & (1 << be)) && pin_get(bus_enable[be]))
        dir = directions[d];
    }
  }
  for (u8 be = 0; be < tabsize(bus_enable); be++)
		pin_output(bus_enable[be]);
  return dir;
}
// #else
// static void i2c_bus_enable(void) {}
// #endif

static void i2c_start(void) {
  // i2c_bus_enable();
#ifdef TWI_UNIFIED
	//     CLR INT    | SET MASTER |  ENABLE   | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
	// debug_print("start %d", peers[state.peer_idx].timeout);
#elifdef TWI_SPLIT
	TWI0.MADDR =
		(u8)I2C_ADDR(peers[state.peer_idx].info.addr, state.mode == TW_MR ? TW_READ : TW_WRITE);
	// TWI0.MCTRLB = TWI_MCMD_REPSTART_gc; // how?
#endif
}

static void i2c_stop(void) {
	state.mode = TW_UH;
#ifdef TWI_UNIFIED
	//     CLR INT    | ENABLE    |  STOP     | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO) | _BV(TWIE);
	// debug_print("stop %d", state.mode);
#elifdef TWI_SPLIT
	TWI0.MCTRLB = TWI_MCMD_STOP_gc;
#endif
}

static void i2c_stop_listen(void) {
	state.mode = TW_YE;
#ifdef TWI_UNIFIED
	//     CLR INT    | LISTEN    | ENABLE    |  STOP     | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWSTO) | _BV(TWIE);
	// debug_print("stop %d", state.mode);
#elifdef TWI_SPLIT
	i2c_stop();
	i2c_listen();
#endif
}

static void i2c_int(void) {
#ifdef TWI_UNIFIED
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
#elifdef TWI_SPLIT
	TWI0.SCTRLB = TWI_ACKACT_bm | TWI_SCMD_RESPONSE_gc;
#endif
}

static void i2c_int_listen(void) {
#ifdef TWI_UNIFIED
	//     CLR INT    | LISTEN    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
#elifdef TWI_SPLIT
	TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
#endif
}

static void i2c_select() {
#ifdef TWI_UNIFIED
	// set addr to write
	TWDR = (u8)I2C_ADDR(peers[state.peer_idx].info.addr, state.mode == TW_MR ? TW_READ : TW_WRITE);
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
#endif
	// debug_print("select %d", peers[state.peer_idx].snack);
}

static void handle_bus_error() {
#ifdef TWI_UNIFIED
	// To recover from a bus error, the TWSTO Flag must set
	// and TWINT must be cleared by writing a logic one to it
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO) | _BV(TWIE);
#elifdef TWI_SPLIT
  if (state.mode == TW_MR || state.mode == TW_MT) {
    TWI0.MSTATUS |= TWI_BUSERR_bm;
  } else if (state.mode == TW_SR || state.mode == TW_ST) {
    TWI0.SSTATUS |= TWI_BUSERR_bm;
    if (TWI0.SSTATUS & TWI_COLL_bm)
      TWI0.SSTATUS |= TWI_COLL_bm;
  }
#endif
  grb_clear(&peers[state.peer_idx].io->input);
  grb_clear(&peers[state.peer_idx].io->output);
}

static void handle_gcall() {
  u8 peek[2];
  usize p = 0;
  grb_peek(&peers[I2C_GCALL_IDX].io->input, peek, 2, &p);
  if (!p || grb_count(&peers[I2C_GCALL_IDX].io->input) < gcall_size[peek[0]])
    return;
  gcall call;
  usize r;
  grb_read(&peers[I2C_GCALL_IDX].io->input, &call, gcall_size[peek[0]], &r);
  switch (call.instruction) {
    case GCI_RESET:
	    debug_print("RESET %d", call.data);
      if (call.data != 0 && call.data != peers[I2C_SELF_IDX].info.addr)
        break;
      wdt_enable(WDTO_30MS);
      while(1) {};
    break;
    case GCI_USE_ADDR:
      i2c_set_addr(call.data, 1);
      peers[I2C_SELF_IDX].info.addr = call.data;
	    debug_print("NEW ADDR %d", call.data);
    break;
    case GCI_ACK_TYPE:
	    debug_print("ACK TYPE %d", r);
      // ack or nack handled during data transfer
    break;
  }
}

static t_error i2c_new_addr(u8 *addr) {
	for (usize a = 0; addr && a < tabsize(addr_pool); a++) {

		if (addr_used[a])
			continue;

		*addr = addr_pool[a];
    addr_used[a] = true;

		return SUCCESS;
	}
	return ERROR;
}

static t_error i2c_free_addr(u8 addr) {
	for (usize a = 0; a < tabsize(addr_pool); a++) {

		if (addr_pool[a] != addr)
			continue;

    addr_used[a] = false;

		return SUCCESS;
	}
	return ERROR;
}

t_error i2c_get_peer(i2c_peer **p, ipi info) {
  usize match = 0;
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {
    usize score = (info.addr && info.addr == peers[peer].info.addr)
      + (info.direction != IPD_NONE && info.direction == peers[peer].info.direction)
      + (info.type == peers[peer].info.type);

    if (score <= match)
      continue;

    match = score;
		
		if (p)
			*p = &peers[peer];	
	}
  if (match)
    return SUCCESS;
	return ERROR;
}

t_error i2c_register_peer(i2c_peer **p, grbio *io, ipi info) {
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {

		if (peers[peer].io) //  && peers[peer].info.addr != info.addr
			continue;

		if (io)
      peers[peer].io = io;
		peers[peer].info = info;
		peers[peer].query = 0;
		if (p) {
			*p = &peers[peer];
      debug_print("a%dt%d", (*p)->info.addr, (*p)->info.type);
    }

		return SUCCESS;
	}
	return ERROR;
}

t_error i2c_register_slot(grbio *io, ipt type) {
	for (usize slot = 0; slot < I2C_MAX_SLOTS; slot++) {

		if (slots[slot].io)
			continue;

		slots[slot].io = io;
		slots[slot].type = type;
		slots[slot].taken = false;
		return SUCCESS;
	}
	return ERROR;
}

static bool i2c_has_slot(ipt type) {
	for (usize slot = 0; slot < I2C_MAX_SLOTS; slot++) {

		if (!slots[slot].io || slots[slot].taken || slots[slot].type != type)
			continue;

		return true;
	}
	return false;
}

t_error i2c_register(i2c_peer **p, ipi info) {
	for (usize slot = 0; slot < I2C_MAX_SLOTS; slot++) {

		if (!slots[slot].io || slots[slot].taken || slots[slot].type != info.type)
			continue;

		slots[slot].taken = true;
		return i2c_register_peer(p, slots[slot].io, info);
	}
	return ERROR;
}

t_error i2c_unregister(i2c_peer *p) {
	for (usize slot = 0; slot < I2C_MAX_SLOTS; slot++) {

		if (slots[slot].io != p->io)
			continue;
    debug_print("I forgor %d\n", p->info.addr);

		slots[slot].taken = false;
		i2c_free_addr(p->info.addr);
		*p = (i2c_peer){};
		return SUCCESS;
	}
	return ERROR;
}

void i2c_alzheimer(void) {
	for (usize peer = I2C_SELF_IDX + 1; peer < I2C_MAX_PEERS; peer++) {

		if (!peers[peer].io || !peers[peer].snack)
			continue;

    if (peers[peer].snack > 5)
      i2c_unregister(&peers[peer]);
	}
}

static t_error i2c_discover(i2c_peer *peer) { // use with &peers[I2C_SELF_IDX]
	// send a gcall to use a random addr
  // then query said addr? how?
  static usize p = 0, t = 0;
  static u8 addr = 0;

  p %= tabsize(peer->downstream);
  for (; p < tabsize(peer->downstream); p++, t = 0, addr = 0) {
    if (peer->downstream[p]) // peer already discovered
      continue;
    if (state.bus_enable != directions[p + 1]) { // start assessing peer type
      state.bus_enable = directions[p + 1];
      struct gcall_ack_type {
        gcall g; u8 more[1]; // thank you gcc
      } data = {GCI_ACK_TYPE, types[t++], .more = {1}};
      i2c_gcall((gcall *)&data);
      return SUCCESS;
    }
    if (!grb_empty(&peers[I2C_GCALL_IDX].io->output)) // keep waiting
      return SUCCESS;
    if (peers[I2C_GCALL_IDX].snack) // next peer
      continue;
    if (t >= tabsize(types)) {
      debug_print("Unknown peer type???");
      continue;
    }
    struct gcall_ack_type {
      gcall g; u8 more[1]; // thank you gcc
    } data = {GCI_ACK_TYPE, types[t], .more = {1}}; // padding
    if (peers[I2C_GCALL_IDX].dnack) { // try another type
      i2c_gcall((gcall *)&data);
      t++;
      return SUCCESS;
    }
    if (!addr) {
      if (!i2c_has_slot(types[t - 1])) {
        debug_print("Cannot register peer, no slot of type %d available", types[t - 1]);
        continue;
      }
      debug_print("Found peer type %d, assigning addr\r\n", types[t - 1]);
      if (i2c_new_addr(&addr)) {
        debug_print("No new i2c addr available");
        return ERROR;
      }
      i2c_gcall(&(gcall){GCI_USE_ADDR, addr});
      return SUCCESS;
    }
    i2c_peer *discovered = NULL;
    if (i2c_register(&discovered, (ipi){addr, state.bus_enable, types[t - 1]})) {
      debug_print("Failed to register peer, no slot available(?)");
      i2c_free_addr(addr);
      continue;
    }
    peer->downstream[p] = discovered;
    debug_print("Registered new peer addr %d dir %d", addr, directions[p + 1]);
    // FIXME does weird stuff like finding a nonexistent peer of type 0
  }
  return ERROR; // Done interrogating, no peer found
}

t_error i2c_gcall(gcall *call) {
  i2c_send(&peers[I2C_GCALL_IDX], (u8 *)call, sizeof(call) + (call->instruction == GCI_ACK_TYPE ? 1 : 0));
}

t_error i2c_send(i2c_peer *p, u8 *data, usize len) {
	grbio  *g = p->io;
	t_error e = ERROR;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_full(&g->output) || grb_avail(&g->output) < len) {
			// debug_print("OUTBUFFULL%d-%d", state.mode, TWCR);
			return e;
		}
		usize useless;
		e = grb_write(&g->output, data, len, &useless);
		// debug_print("I'm alive %p, %d, %p, %d", (void *)p, p->addr, (void *)p->io, useless);
	}
	return e;
}

t_error i2c_query(i2c_peer *p, u8 *data, usize len) {
	grbio  *g = p->io;
	t_error e = ERROR;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_empty(&g->input) || grb_count(&g->input) < len) {
			p->query = len;
			return e;
		}

		usize useless;
    if (data)
		  e = grb_read(&g->input, data, len, &useless);
	}
	return e;
}

static t_error i2c_write(i2c_peer *p) {
	grbio *g = p->io;
	if (grb_empty(&g->output)) {
		// debug_print("OUTBUFEMPTY");
		return ERROR;
	}
	u8	  data;
	usize useless;
	grb_read(&g->output, &data, 1, &useless);
#ifdef TWI_UNIFIED
	// set data to write
	TWDR = data;
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | (state.mode != TW_ST ? 0 : _BV(TWEA));
#elifdef TWI_SPLIT
	// set data to write
	if (state.mode == TW_ST) {
		TWI0.SDATA = data;
		//            DATA INTRPT | STOP INTRPT | SMART MODE  | ENABLE       ;
		TWI0.SCTRLA = TWI_DIEN_bm | TWI_PIEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
	} else if (state.mode == TW_MT) {
		TWI0.MDATA = data;
		//            WRIT INTRPT | QUICK COMND | SMART MODE  | ENABLE       ;
		TWI0.MCTRLA = TWI_WIEN_bm | TWI_QCEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
	}
#endif
	return SUCCESS;
}

static t_error i2c_read(i2c_peer *p) {
	grbio *g = p->io;
	if (grb_full(&g->input)) {
		// debug_print("INBUFFULL");
		return ERROR;
	}
	usize useless;
	u8	  data = 0;
  static u8 prev = 0;
#ifdef TWI_UNIFIED
	data = TWDR;
  if (state.mode == TW_MR && !--p->query) // TW_MR_DATA_NACK
		i2c_stop_listen();
  else {
    bool should_nack = (p->query == 1) || (state.peer_idx == I2C_GCALL_IDX && (gcall_instruction)prev == GCI_ACK_TYPE && (ipt)data != peers[I2C_SELF_IDX].info.type);
	  TWCR = _BV(TWINT) | _BV(TWEN) | (should_nack ? 0 : _BV(TWEA)) | _BV(TWIE);
    // debug_print("shouldnack%d-%d-%d=%d+%d", state.peer_idx == I2C_GCALL_IDX , (gcall_instruction)prev == GCI_ACK_TYPE, data, should_nack, TWCR);
  }
#elifdef TWI_SPLIT
	if (state.mode == TW_SR) {
		data = TWI0.SDATA;
		//            DATA INTRPT | STOP INTRPT | SMART MODE  | ENABLE       ;
		TWI0.SCTRLA = TWI_DIEN_bm | TWI_PIEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
		TWI0.SCTRLB = ((state.peer_idx == I2C_GCALL_IDX && (gcall_instruction)prev == GCI_ACK_TYPE && (ipt)data != peers[I2C_SELF_IDX].info.type) ? TWI_ACKACT_bm : 0) | TWI_SCMD_RESPONSE_gc;
	} else if (state.mode == TW_MR) {
		data = TWI0.MDATA;
		//            READ INTRPT | QUICK COMND | SMART MODE  | ENABLE       ;
		TWI0.MCTRLA = TWI_RIEN_bm | TWI_QCEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;

    if (!--p->query)
      TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    else
		  TWI0.MCTRLB = (grb_avail(&g->input) == 1 || p->query == 1 ? TWI_ACKACT_bm : 0) | TWI_MCMD_RECVTRANS_gc;
	}
#endif
  prev = data;
	return grb_write(&g->input, &data, 1, &useless);
}

static void find_peer() {
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {
		grbio *g = peers[peer].io;

		if (!g || (peers[peer].timeout && peers[peer].timeout--) ||
			(grb_empty(&g->output) && !peers[peer].query))
			continue;

		state.peer_idx = peer;
		return;
	}
	state.peer_idx = I2C_SELF_IDX;
}

static void i2c_keep_or_rel_bus() {
	find_peer();
	grbio *g = peers[state.peer_idx].io;
	if (state.peer_idx != I2C_SELF_IDX && !grb_empty(&g->output)) {
		state.mode = TW_MT;
		i2c_start();
	} else if (peers[state.peer_idx].query) {
		state.mode = TW_MR;
		i2c_start();
	} else if (state.mode == TW_MR || state.mode == TW_MT) {
    state.mode = TW_YE;
		i2c_stop_listen();
    return;
	} else if (state.mode == TW_SR || state.mode == TW_ST) {
    state.mode = TW_YE;
    i2c_int_listen();
    return;
  }
}

void i2c_poll(void) {
  static bool disco_timeout = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
	  if (state.mode == TW_UH || state.mode == TW_YE) {
		  i2c_keep_or_rel_bus();
      handle_gcall();
      if (peers[I2C_SELF_IDX].info.type == IPT_SPLONK) {
        if (!disco_timeout && i2c_discover(&peers[I2C_SELF_IDX]))
          disco_timeout = true;
        i2c_alzheimer();
      }
    }
    static u16 test = 0;
    if (test++ > 4000) {
      test = 0;
      disco_timeout = false;
      debug_print("R%xS%xM%dA%d-I%d I%d O%d", TW_STATUS, TWCR, state.mode, peers[I2C_SELF_IDX].info.addr,
      grb_count(&peers[0].io->input), grb_count(&peers[state.peer_idx].io->input),
      grb_count(&peers[state.peer_idx].io->output));
    }
  }
}

static void i2c_state_machine(u8 status) {
	const char *data = nullptr;
	switch (status) {
	case (TW_START): {
		// data = "start condition tx";
		//   if (state.mode != TW_MR)
		//   debug_print("%s %d %d", data, state.mode, i2c_is_listening());
		// cool bro now select a Slave
		i2c_select();
		break;
	}
	case (TW_REP_START): {
		data = "repeated start condition tx";
		debug_print("%s %d %d", data, state.mode, i2c_is_listening());
		// cool bro now select a slave
		i2c_select();
		break;
	}
	case (TW_MT_SLA_ACK): {
		// data = "SLA+W tx, ACK rx";
		// debug_print("%s", data);
		state.mode = TW_MT;
		peers[state.peer_idx].snack = 0;
		peers[state.peer_idx].timeout = 0;
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
		peers[state.peer_idx].snack++;
		peers[state.peer_idx].timeout = 1000;
		// ok now either load a data byte
		// (apparently you can still load data after a NACK?)
		// (you can talk to a wall, cool)
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MT_DATA_ACK): {
		// data = "data tx, ACK rx";
		// debug_print("%s", data);
		// ok now either load a data byte
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		peers[state.peer_idx].dnack = 0;
		if (i2c_write(&peers[state.peer_idx])) {
			debug_print("last byte sent");
			i2c_keep_or_rel_bus();
		}
		break;
	}
	case (TW_MT_DATA_NACK): {
		data = "data tx, NACK rx";
		debug_print("%s", data);
		// ok now either load a data byte
		// (apparently you can still load data after a NACK?)
		// (you can talk to a wall, cool)
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		peers[state.peer_idx].dnack++;
		if (i2c_write(&peers[state.peer_idx])) {
			debug_print("last byte sent n");
			i2c_keep_or_rel_bus();
		}
		break;
	}
	case (TW_MT_ARB_LOST): {
		// data = "arbitration lost in SLA+W or data";
		// debug_print("%s", data);
		// just set TWINT to accept fate
		// can optionally set TWSTA to try and get a START asap
		i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MR_SLA_ACK): {
		// data = "SLA+R tx, ACK rx";
		// debug_print("%s", data);
		peers[state.peer_idx].snack = 0;
		peers[state.peer_idx].timeout = 0;
		state.mode = TW_MR;
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		(!grb_full(&peers[state.peer_idx].io->input) || peers[state.peer_idx].query == 0 ? i2c_int_listen : i2c_int)();
		break;
	}
	case (TW_MR_SLA_NACK): {
		// data = "SLA+R tx, NACK rx";
		// debug_print("%s", data);
		peers[state.peer_idx].snack++;
		peers[state.peer_idx].timeout = 1000;
		// Either send Repeated start or Stop or both
		i2c_keep_or_rel_bus();
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
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_ST_SLA_ACK): {
		// data = "SLA+R rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_ST;
		state.peer_idx = I2C_SELF_IDX;
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
		state.peer_idx = I2C_SELF_IDX;
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
		state.peer_idx = I2C_SELF_IDX;
		// No action to TWDR, just set TWINT to start getting DATA
		// Or send NACK by setting TWEA to 0
		(!grb_full(&peers[state.peer_idx].io->input) ? i2c_int_listen : i2c_int)();
		break;
	}
	case (TW_SR_ARB_LOST_SLA_ACK): {
		// data = "arbitration lost in SLA+RW, SLA+W rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_SR;
		state.peer_idx = I2C_SELF_IDX;
		// No action to TWDR, just set TWINT to start getting DATA
		// Or send NACK by setting TWEA to 0
		(!grb_full(&peers[state.peer_idx].io->input) ? i2c_int_listen : i2c_int)();
		break;
	}
	case (TW_SR_GCALL_ACK): {
		// data = "general call rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_SR;
		state.peer_idx = I2C_GCALL_IDX;
		// No action to TWDR, just set TWINT to start getting DATA
		// Or send NACK by setting TWEA to 0
		(!grb_full(&peers[state.peer_idx].io->input) ? i2c_int_listen : i2c_int)();
		break;
	}
	case (TW_SR_DATA_ACK): {
		// data = "data rx, ACK returned";
		// debug_print("%s", data);
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		peers[state.peer_idx].dnack = 0;
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_ARB_LOST_GCALL_ACK): {
		// data = "arbitration lost in SLA+RW, general call rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_SR;
		state.peer_idx = I2C_GCALL_IDX;
		// No action to TWDR, just set TWINT to start getting DATA
		// Or send NACK by setting TWEA to 0
		(!grb_full(&peers[state.peer_idx].io->input) ? i2c_int_listen : i2c_int)();
		break;
	}
	case (TW_SR_DATA_NACK): {
		// data = "data rx, NACK returned";
		// debug_print("%s", data);
		// Can either keep listening
		// Or stop listening
		// Or prepare to send start asap while not listening
		// Or prepare to send start asap while listening
		peers[state.peer_idx].dnack++;
		i2c_read(&peers[state.peer_idx]);
    i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_GCALL_DATA_ACK): {
		// data = "general call data rx, ACK returned";
		// debug_print("%s", data);
		// Either read one byte and send ACK by setting TWEA to 1
		// Or read and send NACK by setting TWEA to 0
		peers[state.peer_idx].dnack = 0;
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
		peers[state.peer_idx].dnack++;
		i2c_read(&peers[state.peer_idx]);
    i2c_keep_or_rel_bus();
		break;
	}
	case (TW_SR_STOP): {
		data = "stop or repeated start condition rx while selected";
		// Either listen or don't, and/or get a START asap or don't.

		if (state.mode != TW_YE && state.mode != TW_UH) {
			debug_print("%s %d %d", data, state.mode, i2c_is_listening());
			i2c_keep_or_rel_bus();
			debug_print("%s %d %d", data, state.mode, i2c_is_listening());
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

#ifdef TWI_UNIFIED
ISR(TWI_INTERRUPT) { i2c_state_machine(TW_STATUS); }
#elifdef TWI_SPLIT
ISR(TWI_INTERRUPT_S) {
	u8 status = TWI0.SSTATUS;
  if (status & TWI_COLL_bm || status & TWI_BUSERR_bm) {
    i2c_state_machine(TW_BUS_ERROR);
    return;
  }

	if (status & TWI_APIF_bm) {
		if (status & TWI_AP_ADR_gc) {
      u8 addr = TWI0.SDATA;
      if ((addr & 0xFE) == 0x00)
        i2c_state_machine(TW_SR_GCALL_ACK);
      else if ((addr >> 1) == (TWI0.SADDR >> 1))
			  i2c_state_machine(addr & 1 ? TW_ST_SLA_ACK : TW_SR_SLA_ACK);
		} else if (status & TWI_AP_STOP_gc)
			i2c_state_machine(TW_SR_STOP);
	}

	if (status & TWI_DIF_bm) {
		if (state.peer_idx == I2C_GCALL_IDX) {
			i2c_state_machine(TWI0.SCTRLB & TWI_ACKACT_bm ? TW_SR_GCALL_DATA_NACK : TW_SR_GCALL_DATA_ACK);
		} else {
      if (state.mode == TW_SR) // or use DIR bm?
        i2c_state_machine(TWI0.SCTRLB & TWI_ACKACT_bm ? TW_SR_DATA_NACK : TW_SR_DATA_ACK);
      else
        i2c_state_machine(status & TWI_RXACK_bm ? TW_ST_DATA_NACK : TW_ST_DATA_ACK); 
		}
	}
}
ISR(TWI_INTERRUPT_M) {
  // TODO (optional) finish impl master stuff (not that we'll ever use those)

}
#endif

#if defined(SPLONKLIB_ENABLE_UART) && defined(TWI_UNIFIED)

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
