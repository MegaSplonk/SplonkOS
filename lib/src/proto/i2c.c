#include <avr/interrupt.h>
#include <avr/io.h>
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

// define an 'initial' addr that devices can use temporarily? yep
// just have the devices ask the main mcu using general call? nuh uh
// other way around, have the main mcu ask the devices with gc? yep
// devices stop listening to gc after main mcu finds them? nuh uh
void i2c_set_addr(u8 addr, bool answer_gc) {
	// need to set Slave Receive address here,
	// and with the LSB we can choose to respond (1) to the General Call or
	// ignore it (0)
#ifdef TWI_UNIFIED
	TWAR = (addr << 1) | answer_gc;
#elifdef TWI_SPLIT
	TWI0.SADDR = (u8)(addr << 1) | answer_gc;
	debug_print("addr %d gc %d", addr, answer_gc);
#endif
}

void i2c_init(void) {
#ifdef TWI_UNIFIED
	// // clear the status registers
	// TWSR = _BV(TWPS1); // _BV(TWPS0) |  SHIT WAS GETTING STUCK BECAUSE THE PRESCALER WASNT BIG
	// ENOUGH
	// // set the i2c clock speed
	// TWBR = I2C_CLOCK_SPEED;
	TWSR = 0; // TWPS = 0
	TWBR = ((F_CPU / I2C_CLOCK) - 16) / 2;
#elifdef TWI_SPLIT
	// TODO - setup stuff here
#endif
	// timer_ontick_register(T_ONTICK_SLOT_I2C, i2c_poll);
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

#ifdef SPLONK
const pin bus_enable[] = I2C_BE_PINS;
const u8  be_logic[] = I2C_BE_LOGIC;

static void i2c_bus_enable(void) {
	ipd dir = IPD_ALL; // state.peer_idx == I2C_GCALL_IDX ? state.bus_enable : IPD_ALL;
	for (u8 be = 0; be < tabsize(bus_enable); be++)
		pin_output(bus_enable[be]), pin_set(bus_enable[be], (bool)(be_logic[dir] & (1 << be)));
}

// TODO - test
// static ipd i2c_bus_sense(void) {
//   ipd dir = IPD_NONE;
//   for (u8 d = 0; d < tabsize(directions); d++) {
//     for (u8 be = 0; be < tabsize(bus_enable); be++) {
// 		  pin_input(bus_enable[be]);
//       if ((be_logic[d] & (1 << be)) && pin_get(bus_enable[be]))
//         dir = directions[d];
//     }
//   }
//   for (u8 be = 0; be < tabsize(bus_enable); be++)
// 		pin_output(bus_enable[be]);
//   return dir;
// }
#elifdef TWI_UNIFIED
static void i2c_bus_enable(void) {}
#endif

#ifdef TWI_UNIFIED
static void i2c_start(void) {
	i2c_bus_enable();
	//     CLR INT    | SET MASTER |  ENABLE   | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
	// debug_print("start %d", peers[state.peer_idx].timeout);
}

// Use when master does not ever get called as slave?
// static void i2c_stop(void) {
// 	state.mode = TW_UH;
// 	//     CLR INT    | ENABLE    |  STOP     | INTERRUPT;
// 	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO) | _BV(TWIE);
// 	// debug_print("stop %d", state.mode);
// }

static void i2c_stop_listen(void) {
	state.mode = TW_YE;
	//     CLR INT    | LISTEN    | ENABLE    |  STOP     | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWSTO) | _BV(TWIE);
	// debug_print("stop %d", state.mode);
}

static void i2c_int(void) {
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
}

static void i2c_int_listen(void) {
	//     CLR INT    | LISTEN    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
}

static void i2c_select() {
	// set addr to write
	TWDR = (u8)I2C_ADDR(peers[state.peer_idx].info.addr, state.mode == TW_MR ? TW_READ : TW_WRITE);
	//     CLR INT    | ENABLE    | INTERRUPT;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
	// debug_print("select %d", peers[state.peer_idx].snack);
}
#endif

static void handle_bus_error() {
#ifdef TWI_UNIFIED
	// To recover from a bus error, the TWSTO Flag must set
	// and TWINT must be cleared by writing a logic one to it
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO) | _BV(TWIE);
#endif
	grb_clear(&peers[state.peer_idx].io->input);
	grb_clear(&peers[state.peer_idx].io->output);
}

static void handle_gcall() {
	u8	  peek[sizeof(gcall_)];
	usize p = 0;
	grb_peek(&peers[I2C_GCALL_IDX].io->input, peek, sizeof(gcall_), &p);
	if (!p || grb_count(&peers[I2C_GCALL_IDX].io->input) < gcall_size[peek[0]])
		return;
	gcall call;
	usize r;
	grb_read(&peers[I2C_GCALL_IDX].io->input, &call, gcall_size[peek[0]], &r);
	switch (call.base.instruction) {
	case GCI_RESET:
		debug_print("RESET %d", call.reset.data);
		if (call.reset.data != 0 && call.reset.data != peers[I2C_SELF_IDX].info.addr)
			break;
		// soft_reset();
		break;
	case GCI_USE_ADDR:
		i2c_set_addr(call.use_addr.data, 1);
		peers[I2C_SELF_IDX].info.addr = call.use_addr.data;
		debug_print("NEW ADDR %d", call.use_addr.data);
		break;
	case GCI_ACK_TYPE:
		debug_print("ACK TYPE %d %d %d", call.ack_type.instruction, call.ack_type.data,
					call.ack_type.more[0]);
		// ack or nack handled during data transfer
		break;
	case GCI_ACK_ADDR:
		debug_print("ACK ADDR %d %d %d", call.ack_addr.instruction, call.ack_addr.data,
					call.ack_addr.more[0]);
		// ack or nack handled during data transfer
		break;
	default:
		debug_print("UNKNOWN GCALL %d", call.base.data);
		break;
	}
}

bool addr_used[tabsize(i2c_addr_pool)] = {};

static t_error i2c_new_addr(u8 *addr) {
	for (usize a = 0; addr && a < tabsize(i2c_addr_pool); a++) {

		if (addr_used[a])
			continue;

		*addr = i2c_addr_pool[a];
		addr_used[a] = true;

		return SUCCESS;
	}
	return ERROR;
}

static t_error i2c_free_addr(u8 addr) {
	for (usize a = 0; a < tabsize(i2c_addr_pool); a++) {

		if (i2c_addr_pool[a] != addr)
			continue;

		addr_used[a] = false;

		return SUCCESS;
	}
	return ERROR;
}

t_error i2c_get_peer(i2c_peer **p, ipi info) {
	usize match = 0;
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {
		usize score = ((info.addr && info.addr == peers[peer].info.addr) ? 3u : 0) +
					  (info.direction != IPD_NONE && info.direction == peers[peer].info.direction) +
					  (info.type == peers[peer].info.type);

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

void i2c_get_peers(i2c_peer **p, usize *s) {
	*p = peers;
	*s = tabsize(peers);
}

t_error i2c_register_peer(i2c_peer **p, grbio *io, ipi info) {
	for (usize peer = 0; peer < I2C_MAX_PEERS; peer++) {

		if (peer != 0 && peers[peer].info.addr == info.addr)
			return ERROR;

		if (peers[peer].io)
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

		if (i2c_register_peer(p, slots[slot].io, info))
			return ERROR;
		slots[slot].taken = true;
		return SUCCESS;
	}
	return ERROR;
}

static void del_and_shift_peers(i2c_peer *p) {
	usize pos = (usize)(((uintptr_t)p - (uintptr_t)peers) / sizeof(i2c_peer *));
	for (; pos + 1 < sizeof(peers); pos++) {
		peers[pos] = peers[pos + 1];
	}
	peers[pos] = (i2c_peer){};
}

t_error i2c_unregister(i2c_peer *p) {
	for (usize slot = 0; slot < I2C_MAX_SLOTS; slot++) {

		if (slots[slot].io != p->io)
			continue;
		debug_print("I forgor %d\n", p->info.addr);

		for (usize d = 0; d < tabsize(peers[I2C_SELF_IDX].downstream); d++) {
			if (peers[I2C_SELF_IDX].downstream[d] != p)
				continue;
			peers[I2C_SELF_IDX].downstream[d] = NULL;
		}

		slots[slot].taken = false;
		i2c_free_addr(p->info.addr);
		del_and_shift_peers(p);
		return SUCCESS;
	}
	return ERROR;
}

static void i2c_alzheimer(void) {
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
	static u8	 addr = 0;

	p %= tabsize(peer->downstream);
	for (; p < tabsize(peer->downstream); p++, t = 0, addr = 0) {
		if (peer->downstream[p]) { // peer already discovered
			if (!addr) {
				addr = peer->downstream[p]->info.addr;
				gcall data = {.ack_addr = {GCI_ACK_ADDR, addr, .more = {1}}};
				i2c_gcall((gcall *)&data);
				return SUCCESS;
			}
			if (!grb_empty(&peers[I2C_GCALL_IDX].io->output)) // keep waiting
				return SUCCESS;
			if (peers[I2C_GCALL_IDX].snack || peers[I2C_GCALL_IDX].dnack) {
				debug_print("PURGED %d", addr);
				i2c_unregister(peer->downstream[p]);
				peer->downstream[p] = NULL;
				p = 0, t = 0, addr = 0;
				return SUCCESS;
			}
			continue;
		}
		if (state.bus_enable != ipdirections[p + 1]) { // start assessing peer type
			state.bus_enable = ipdirections[p + 1];
			gcall data = {.ack_type = {GCI_ACK_TYPE, iptypes[t++], .more = {1}}};
			i2c_gcall((gcall *)&data);
			return SUCCESS;
		}
		if (!grb_empty(&peers[I2C_GCALL_IDX].io->output)) // keep waiting
			return SUCCESS;
		if (peers[I2C_GCALL_IDX].snack) { // next peer
			peers[I2C_GCALL_IDX].snack = 0;
			continue;
		}
		if (t >= tabsize(iptypes)) {
			debug_print("Unknown peer type???");
			continue;
		}
		gcall data = {.ack_type = {GCI_ACK_TYPE, iptypes[t], .more = {1}}}; // padding
		if (peers[I2C_GCALL_IDX].dnack) {									// try another type
			peers[I2C_GCALL_IDX].dnack = 0;
			i2c_gcall((gcall *)&data);
			t++;
			return SUCCESS;
		}
		if (!addr) {
			if (!i2c_has_slot(iptypes[t - 1])) {
				debug_print("Cannot register peer, no slot of type %d available", iptypes[t - 1]);
				continue;
			}
			debug_print("Found peer type %d, assigning addr\r\n", iptypes[t - 1]);
			if (i2c_new_addr(&addr)) {
				debug_print("No new i2c addr available");
				return ERROR;
			}
			i2c_gcall(&(gcall){.use_addr = {GCI_USE_ADDR, addr}});
			return SUCCESS;
		}
		i2c_peer *discovered = NULL;
		if (i2c_register(&discovered, (ipi){addr, state.bus_enable, iptypes[t - 1]})) {
			debug_print("Failed to register peer, no slot available(?)");
			i2c_free_addr(addr);
			continue;
		}
		peer->downstream[p] = discovered;
		debug_print("Registered new peer addr %d dir %d", addr, ipdirections[p + 1]);
		// FIXME does weird stuff like finding a nonexistent peer of type 0
	}
	return ERROR; // Done interrogating, no peer found
}

t_error i2c_gcall(gcall *call) {
	return i2c_send(&peers[I2C_GCALL_IDX], (u8 *)call, gcall_size[call->base.instruction]);
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
		//            DATA INTRPT | STOP INTRPT  | SMART MODE  | ENABLE       ;
		TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
	} else if (state.mode == TW_MT) {
		TWI0.MDATA = data;
		//            WRIT INTRPT | QUICK COMND | SMART MODE  | ENABLE       ;
		TWI0.MCTRLA = TWI_WIEN_bm | TWI_QCEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
	}
#endif
	return SUCCESS;
}

static bool read_should_nack(i2c_peer *p) {
	if (p->query == 1)
		return true;
	if (state.peer_idx == I2C_GCALL_IDX) {
		u8	  peek[sizeof(gcall_ack_)];
		usize p_bytes = 0;
		grb_peek(&peers[I2C_GCALL_IDX].io->input, peek, sizeof(peek), &p_bytes);
		if (p_bytes <
			sizeof(peek) - 1) // FIXME NEEDS TO BEHAVE DIFFERENTLY ATMEGA (size-1) VS ATTINY (size)
			return false;
		bool ret = ((gcall_instruction)peek[0] == GCI_ACK_TYPE &&
					(ipt)peek[1] != peers[I2C_SELF_IDX].info.type) ||
				   ((gcall_instruction)peek[0] == GCI_ACK_ADDR &&
					(ipt)peek[1] != peers[I2C_SELF_IDX].info.addr);
		// debug_print("shouldnack%d==%d, %d!=%d -> %d", peek[0], GCI_ACK_TYPE, peek[1],
		// peers[I2C_SELF_IDX].info.type, ret);
		// if (ret)
		//   grb_clear(&peers[I2C_GCALL_IDX].io->input);
		return ret;
	}
	return false;
}

static t_error i2c_read(i2c_peer *p) {
	grbio *g = p->io;
	if (grb_full(&g->input)) {
		// debug_print("INBUFFULL");
		return ERROR;
	}
	usize	useless;
	u8		data = 0;
	t_error ret = SUCCESS;
#ifdef TWI_UNIFIED
	data = TWDR;
	ret = grb_write(&g->input, &data, 1, &useless);
	if (state.mode == TW_MR && !--p->query) // TW_MR_DATA_NACK
		i2c_stop_listen();
	else {
		bool should_nack = read_should_nack(p);
		// debug_print("shouldnack %d", should_nack);
		TWCR = _BV(TWINT) | _BV(TWEN) | (should_nack ? 0 : _BV(TWEA)) | _BV(TWIE);
	}
#elifdef TWI_SPLIT
	if (state.mode == TW_SR) {
		TWI0.SCTRLA &= (u8)~TWI_SMEN_bm;
		// smart mode sends ack/nack automatically when sdata is read, so I need to
		// disable it to make a decision on whether to nack or ack before proceeding
		data = TWI0.SDATA;
		ret = grb_write(&g->input, &data, 1, &useless);
		bool should_nack = read_should_nack(p);
		TWI0.SCTRLB = (should_nack ? TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc);
		TWI0.SCTRLA |= TWI_SMEN_bm; // re-enable smart mode
	} else if (state.mode == TW_MR) {
		data = TWI0.MDATA;
		ret = grb_write(&g->input, &data, 1, &useless);
		//            READ INTRPT | QUICK COMND | SMART MODE  | ENABLE       ;
		TWI0.MCTRLA = TWI_RIEN_bm | TWI_QCEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;

		if (!--p->query)
			TWI0.MCTRLB = TWI_MCMD_STOP_gc;
		else
			TWI0.MCTRLB = (grb_avail(&g->input) == 1 || p->query == 1 ? TWI_ACKACT_bm : 0) |
						  TWI_MCMD_RECVTRANS_gc;
	}
#endif
	return ret;
}
#ifdef TWI_UNIFIED
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
#else
// ATTINY is never master so never needs to poll for possible starts (for now)
static void i2c_keep_or_rel_bus() {}
#endif

void i2c_debug() {
#ifdef TWI_UNIFIED
	debug_print("R%xS%xM%dA%d-I%d I%d O%d", TW_STATUS, TWCR, state.mode,
				peers[I2C_SELF_IDX].info.addr, grb_count(&peers[I2C_GCALL_IDX].io->input),
				grb_count(&peers[state.peer_idx].io->input),
				grb_count(&peers[state.peer_idx].io->output));
#elifdef TWI_SPLIT
	debug_print("R%xSA%xSB%xM%dA%d-I%d I%d O%d", TWI0.SSTATUS, TWI0.SCTRLA, TWI0.SCTRLB, state.mode,
				peers[I2C_SELF_IDX].info.addr, grb_count(&peers[I2C_GCALL_IDX].io->input),
				grb_count(&peers[state.peer_idx].io->input),
				grb_count(&peers[state.peer_idx].io->output));
#endif
}

void i2c_poll(void) {
	static timer_t disco_timer = {}, info_timer = {};
	static usize   disco_timeout = 1;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (state.mode == TW_UH || state.mode == TW_YE) {
			i2c_keep_or_rel_bus();
			handle_gcall();
			if (peers[I2C_SELF_IDX].info.type == IPT_SPLONK) {
				if (timer_wait(&disco_timer, disco_timeout)) {
					disco_timer = timer_reset();
					if (i2c_discover(&peers[I2C_SELF_IDX]))
						disco_timeout = 500;
					else
						disco_timeout = 1;
				}
				i2c_alzheimer();
			}
		}
		if (timer_wait(&info_timer, 2000)) {
			info_timer = timer_reset();
			i2c_debug();
		}
	}
}

#ifdef TWI_UNIFIED
static void i2c_state_machine(u8 status) {
	const char *data = nullptr;
	switch (status) {
	case (TW_START): {
		data = "start condition tx";
		if (state.mode != TW_MR)
			debug_print("%s %d %d", data, state.mode, i2c_is_listening());
		// cool bro now select a Slave
		i2c_select();
		break;
	}
	case (TW_REP_START): {
		// data = "repeated start condition tx";
		// debug_print("%s %d %d", data, state.mode, i2c_is_listening());
		// cool bro now select a slave
		i2c_select();
		break;
	}
	case (TW_MT_SLA_ACK): {
		// data = "SLA+W tx, ACK rx";
		// debug_print("%s", data);
		state.mode = TW_MT;
		peers[state.peer_idx].snack = 0;
		peers[state.peer_idx].dnack = 0;
		peers[state.peer_idx].timeout = 0;
		// ok now either load a data byte
		// or set TWSTA or set TWSTO or both
		// and set TWINT
		if (i2c_write(&peers[state.peer_idx]))
			i2c_keep_or_rel_bus();
		break;
	}
	case (TW_MT_SLA_NACK): {
		data = "SLA+W tx, NACK rx";
		debug_print("%s", data);
		peers[state.peer_idx].snack++;
		peers[state.peer_idx].timeout = 100;
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
		peers[state.peer_idx].dnack++;
		if (i2c_write(&peers[state.peer_idx])) {
			// debug_print("last byte sent n");
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
		(!grb_full(&peers[state.peer_idx].io->input) || peers[state.peer_idx].query == 0
			 ? i2c_int_listen
			 : i2c_int)();
		break;
	}
	case (TW_MR_SLA_NACK): {
		// data = "SLA+R tx, NACK rx";
		// debug_print("%s", data);
		peers[state.peer_idx].snack++;
		peers[state.peer_idx].timeout = 100;
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
		peers[state.peer_idx].dnack = 0;
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
		peers[state.peer_idx].dnack = 0;
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
		peers[state.peer_idx].dnack = 0;
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
		i2c_read(&peers[state.peer_idx]);
		break;
	}
	case (TW_SR_ARB_LOST_GCALL_ACK): {
		// data = "arbitration lost in SLA+RW, general call rx, ACK returned";
		// debug_print("%s", data);
		state.mode = TW_SR;
		state.peer_idx = I2C_GCALL_IDX;
		peers[state.peer_idx].dnack = 0;
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

ISR(TWI_INTERRUPT) { i2c_state_machine(TW_STATUS); }
#elifdef TWI_SPLIT
ISR(TWI_INTERRUPT_S) {
	u8		  status = TWI0.SSTATUS;
	u8		  action = 0;
	static u8 rxack_mask = 0;
	if (status & TWI_COLL_bm || status & TWI_BUSERR_bm) {
		u8 errors = status & (TWI_BUSERR_bm | TWI_COLL_bm);
		TWI0.SSTATUS = errors;
		handle_bus_error();
		action = TWI_SCMD_COMPTRANS_gc;
	} else if (status & TWI_APIF_bm) {
		if (status & TWI_AP_bm) { // ADDR
			u8 addr = TWI0.SDATA;
			// debug_print("ADDR RECV %d", addr);
			if (addr == 0x00) { // GCALL
				state.mode = TW_SR;
				state.peer_idx = I2C_GCALL_IDX;
				peers[state.peer_idx].dnack = 0;
				action = grb_full(&peers[state.peer_idx].io->input)
							 ? TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc
							 : TWI_SCMD_RESPONSE_gc;

			} else if ((addr >> 1) == (TWI0.SADDR >> 1)) {
				if (addr & 1) {
					state.mode = TW_ST;
					state.peer_idx = I2C_SELF_IDX;
					action = grb_empty(&peers[state.peer_idx].io->output)
								 ? TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc
								 : TWI_SCMD_RESPONSE_gc;
				} else {
					state.mode = TW_SR;
					state.peer_idx = I2C_SELF_IDX;
					action = grb_full(&peers[state.peer_idx].io->input)
								 ? TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc
								 : TWI_SCMD_RESPONSE_gc;
				}
			} else { // 10 bit addr or whatever else
				state.mode = TW_YE;
				action = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;
			}
		} else { // STOP
			state.mode = TW_YE;
			action = TWI_SCMD_COMPTRANS_gc;
		}
	} else if (status & TWI_DIF_bm) {
		if (!(status & TWI_DIR_bm)) {
			if (i2c_read(&peers[state.peer_idx]))
				action = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;
			else
				return; // return since SCTRLB is set inside read()

		} else if (status & rxack_mask) {
			rxack_mask = 0;
			state.mode = TW_YE;
			action = TWI_SCMD_COMPTRANS_gc;
		} else {
			if (i2c_write(&peers[state.peer_idx])) {
				rxack_mask = 0;
				state.mode = TW_YE;
				action = TWI_SCMD_COMPTRANS_gc;
			} else {
				rxack_mask = TWI_RXACK_bm;
				return;
			}
		}
	}
	TWI0.SCTRLB = action;
}
ISR(TWI_INTERRUPT_M) {
	// TODO (optional) finish impl master stuff (not that we'll ever use those)
}
#endif

#endif
