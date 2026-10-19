#pragma once
#include <stdint.h>
typedef struct {
    volatile uint8_t SADDR, SCTRLA, SCTRLB, SSTATUS, SDATA;
    volatile uint8_t MCTRLA, MCTRLB, MDATA;
} test_twi;
extern test_twi TWI0;
#define TWI_ENABLE_bm 0x01
#define TWI_SMEN_bm 0x02
#define TWI_PIEN_bm 0x04
#define TWI_DIEN_bm 0x20
#define TWI_APIEN_bm 0x40
#define TWI_AP_bm 0x01
#define TWI_DIR_bm 0x02
#define TWI_BUSERR_bm 0x04
#define TWI_COLL_bm 0x08
#define TWI_RXACK_bm 0x10
#define TWI_APIF_bm 0x40
#define TWI_DIF_bm 0x80
#define TWI_ACKACT_bm 0x04
#define TWI_SCMD_COMPTRANS_gc 0x02
#define TWI_SCMD_RESPONSE_gc 0x03
#define TWI_WIEN_bm 0x40
#define TWI_RIEN_bm 0x80
#define TWI_QCEN_bm 0x10
#define TWI_MCMD_STOP_gc 0x03
#define TWI_MCMD_RECVTRANS_gc 0x02
