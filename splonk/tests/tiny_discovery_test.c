#include "../../lib/src/proto/i2c.c"
#include <assert.h>
#include <stdio.h>

test_twi TWI0;
GRBIO_INIT(gc, gc_in, gc_out, 2 * sizeof(gcall), 2 * sizeof(gcall));
GRBIO_INIT(local, local_in, local_out, 2, 2);
timer_t timer_reset(void) { return (timer_t){}; }
bool timer_wait(timer_t *timer, milis_t delay) {
    (void)timer; (void)delay;
    return false;
}
static void event(u8 status, u8 data) {
    TWI0.SSTATUS = status;
    TWI0.SDATA = data;
    TWI0_TWIS_vect();
}
static void probe(u8 instruction, u8 value, bool matches) {
    event(TWI_APIF_bm | TWI_AP_bm, 0);
    event(TWI_DIF_bm, instruction);
    assert(TWI0.SCTRLB == TWI_SCMD_RESPONSE_gc);
    event(TWI_DIF_bm, value);
    // The ATtiny ACKs the byte just read. The probe's padding must arrive
    // before it can NACK; otherwise the command remains only two bytes long.
    assert(TWI0.SCTRLB == TWI_SCMD_RESPONSE_gc);
    event(TWI_DIF_bm, 1);
    assert(TWI0.SCTRLB == (matches ? TWI_SCMD_RESPONSE_gc
                                 : TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc));
    assert(grb_count(&gc.input) == sizeof(gcall_ack_));
    event(TWI_APIF_bm, 0);
    handle_gcall();
    assert(grb_empty(&gc.input));
}
int main(void) {
    i2c_peer *self;
    assert(i2c_register_peer(nullptr, &gc, (ipi){}) == SUCCESS);
    assert(i2c_register_peer(&self, &local, (ipi){96, IPD_NONE, IPT_SPLONKO}) == SUCCESS);
    i2c_set_addr(96, true);
    i2c_listen();
    probe(GCI_ACK_TYPE, IPT_SPLONKNOB, false);
    probe(GCI_ACK_TYPE, IPT_SPLONKO, true);
    event(TWI_APIF_bm | TWI_AP_bm, 0);
    event(TWI_DIF_bm, GCI_USE_ADDR);
    event(TWI_DIF_bm, 44);
    event(TWI_APIF_bm, 0);
    handle_gcall();
    assert(self->info.addr == 44 && TWI0.SADDR == ((44 << 1) | 1));
    probe(GCI_ACK_ADDR, 45, false);
    probe(GCI_ACK_ADDR, 44, true);
    puts("ATtiny discovery probe framing and address assignment tests passed");
}
