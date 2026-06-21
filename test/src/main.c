#include "lib/uart.h"
#include <avr/io.h>

#define COL1 PORTC2
#define COL2 PORTC3
#define COL3 PORTC4
#define COL4 PORTC5
#define COLS {COL1, COL2, COL3, COL4}

#define ROW1 PIND4
#define ROW2 PIND5
#define ROW3 PIND6
#define ROW4 PIND7
#define ROWS {ROW1, ROW2, ROW3, ROW4}

void delay_ms(uint16_t ms);

static bool btn_pressed(int cols[], int col) {
  bool yes = (PINC & _BV(cols[col]));
  delay_ms(50);
  if (yes && (PINC & _BV(cols[col])))
    return true;
  return false;
}

int main(void) {
  uart_init();

  const int rows[] = ROWS;
  const int cols[] = COLS;

  for (uint8_t r = 0; r < sizeof(rows) / sizeof(rows[0]); r++) 
    DDRD |= (_BV(rows[r]));

  DDRC = 0;
  while (true) {
    for (uint8_t r = 0; r < sizeof(rows) / sizeof(rows[0]); r++) {
      PORTD = (_BV(rows[r]));
      for (uint8_t c = 0; c < sizeof(cols) / sizeof(cols[0]); c++) {
        if (btn_pressed(cols, c))
          uart_tx('('), uart_send_u8(r), uart_tx(','), uart_send_u8(c), uart_tx(')'), uart_tx('\r'), uart_tx('\n');
        else
          uart_tx(',' + c), uart_tx('\r');
      }
    }
  }

  return 0;
}
