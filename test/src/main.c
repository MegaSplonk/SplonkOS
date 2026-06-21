#include "lib/proto/uart.h"
#include "lib/pinout.h"
#include <avr/io.h>
#include <util/delay.h>

#define COL1 A2
#define COL2 A3
#define COL3 A4
#define COL4 A5
#define COLS {COL1, COL2, COL3, COL4}

#define ROW1 D4
#define ROW2 D5
#define ROW3 D6
#define ROW4 D7
#define ROWS {ROW1, ROW2, ROW3, ROW4}

// void delay_ms(uint16_t ms);

static bool btn_pressed(pin cols[], int col) {
  bool yes = pin_get(cols[col]);
  _delay_ms(50);
  if (yes && pin_get(cols[col]))
    return true;
  return false;
}

int main(void) {
  uart_init();

  const pin rows[] = ROWS;
  const pin cols[] = COLS;

  for (uint8_t r = 0; r < sizeof(rows) / sizeof(rows[0]); r++) 
    pin_output(rows[r]), pin_set(rows[r], false);
  for (uint8_t c = 0; c < sizeof(cols) / sizeof(cols[0]); c++) 
    pin_input(cols[c]);

  while (true) {
    for (uint8_t r = 0; r < sizeof(rows) / sizeof(rows[0]); r++) {
      pin_set(rows[r], true);
      for (uint8_t c = 0; c < sizeof(cols) / sizeof(cols[0]); c++) {
        if (btn_pressed(cols, c))
          uart_tx('('), uart_send_u8(r), uart_tx(','), uart_send_u8(c), uart_tx(')'), uart_tx('\r'), uart_tx('\n');
        else
          uart_tx(',' + c), uart_tx('\r');
      }
      pin_set(rows[r], false);
    }
  }

  return 0;
}
