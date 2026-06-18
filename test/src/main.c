#include <avr/io.h>
#include <util/delay.h>

int main(void) {
  DDRB |= _BV(PB5);
  while (true) {
    _delay_ms(500);
    PORTB ^= _BV(PB5);
  };
  return 0;
}
