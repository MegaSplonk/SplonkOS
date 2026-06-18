#include <avr/io.h>
#include <util/delay.h>

int main(void) {
  DDRB |= _BV(PB2);

  while (true) {
    _delay_ms(1000);
    PORTB ^= _BV(PB2);
  };
  return 0;
}
