#include "../../lib/src/proto/i2c.c"
#include <assert.h>
#include <string.h>

#ifdef SPLONKLIB_ENABLE_USB
#error "Splonkus must not enable USB"
#endif
#ifdef BUS_ENABLE1
#error "Splonkus must not expose bus-enable pins"
#endif

volatile uint8_t TWCR, TWDR, TWAR, TWSR, TWBR;
timer_t			 timer_reset(void) { return (timer_t){}; }
bool			 timer_wait(timer_t *timer, milis_t delay) {
	(void)timer;
	(void)delay;
	return false;
}
void uart_sendstring(const char *s) { (void)s; }
I2C_SLOT(gc, gcall, gcall);
I2C_SLOT(local, rgb_control, splonkus);

static void event(u8 status) {
	i2c_state_machine(status);
	assert(!(TWCR & (_BV(TWSTA) | _BV(TWSTO))));
}

static void poll(void) {
	// Model hardware clearing the write-one-to-clear interrupt flag.
	TWCR &= (u8)~_BV(TWINT);
	i2c_poll();
	assert(!(TWCR & (_BV(TWSTA) | _BV(TWSTO))));
}

static void general_call(const u8 *bytes, usize count) {
	event(TW_SR_GCALL_ACK);
	for (usize i = 0; i < count; i++) {
		TWDR = bytes[i];
		event(TW_SR_GCALL_DATA_ACK);
	}
	event(TW_SR_STOP);
	poll();
}

static void read_snapshot(const splonkus *expected) {
	event(TW_ST_SLA_ACK);
	for (usize i = 0; i < sizeof(*expected); i++) {
		assert(TWDR == ((const u8 *)expected)[i]);
		if (i + 1 < sizeof(*expected))
			event(TW_ST_DATA_ACK);
	}
	event(TW_ST_DATA_NACK);
	assert(state.mode == TW_YE && (TWCR & _BV(TWEA)));
}

int main(void) {
	i2c_peer *self;
	assert(i2c_register_peer(nullptr, &gc, (ipi){}) == SUCCESS);
	assert(i2c_register_peer(&self, &local, (ipi){0x60, IPD_NONE, IPT_SPLONKUS}) == SUCCESS);
	i2c_init();
	i2c_set_addr(0x60, true);
	i2c_listen();
	assert(TWAR == ((0x60 << 1) | 1));
	poll();

	// Discovery probes ACK only our type, then accept the host's address.
	for (u8 type = IPT_SPLONK; type <= IPT_SPLONKBAR; type++) {
		event(TW_SR_GCALL_ACK);
		TWDR = GCI_ACK_TYPE;
		event(TW_SR_GCALL_DATA_ACK);
		TWDR = type;
		event(TW_SR_GCALL_DATA_ACK);
		assert(!!(TWCR & _BV(TWEA)) == (type == IPT_SPLONKUS));
		TWDR = 1;
		event(type == IPT_SPLONKUS ? TW_SR_GCALL_DATA_ACK : TW_SR_GCALL_DATA_NACK);
		event(TW_SR_STOP);
		poll();
	}
	const u8 assign[] = {GCI_USE_ADDR, 44};
	general_call(assign, sizeof(assign));
	assert(self->info.addr == 44 && TWAR == ((44 << 1) | 1));

	splonkus released = {}, pressed = {.keys = {0x81, 0x42, 0x24}};
	read_snapshot(&released); // Safe even before the first matrix publication.
	assert(i2c_send(self, (u8 *)&pressed, sizeof(pressed)) == SUCCESS);
	poll(); // A queued response must never initiate a controller transfer.
	read_snapshot(&pressed);
	read_snapshot(&pressed); // Faster reads repeat the last complete snapshot.

	assert(i2c_send(self, (u8 *)&pressed, sizeof(pressed)) == SUCCESS);
	event(TW_ST_SLA_ACK);
	assert(i2c_send(self, (u8 *)&released, sizeof(released)) == SUCCESS);
	assert(TWDR == pressed.keys[0]);
	event(TW_ST_DATA_NACK); // Early end cannot misalign the next packet.
	read_snapshot(&released);
	read_snapshot(&released);

	// One-byte RGB writes arrive in the command ring without controller traffic.
	event(TW_SR_SLA_ACK);
	TWDR = 2;
	event(TW_SR_DATA_ACK);
	event(TW_SR_STOP);
	poll();
	rgb_control command;
	assert(grb_read(&local.input, &command, sizeof(command), nullptr) == SUCCESS);
	assert(command.led_mode == 2);
	gcall forbidden = {.use_addr = {GCI_USE_ADDR, 45}};
	assert(i2c_gcall(&forbidden) == ERROR);
	assert(i2c_query(self, nullptr, sizeof(pressed)) == ERROR);
	event(TW_BUS_ERROR);
	assert(state.mode == TW_YE && (TWCR & _BV(TWEA)));
	read_snapshot(&released);
	puts("Splonkus slave discovery, snapshots, RGB reception and no-START tests passed");
}
