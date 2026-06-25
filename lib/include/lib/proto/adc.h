#ifndef ADC_H
#define ADC_H

#include <avr/io.h>

typedef enum e_adc_ref {
	ADC_AREF = 0,
	ADC_AVCC = _BV(REFS0),
	ADC_INT = _BV(REFS0) | _BV(REFS1),

	_ADC_REF_MASK = _BV(REFS0) | _BV(REFS1),
} e_adc_ref;

typedef enum e_adc_size {
	ADC_10BIT = 0,
	ADC_8BIT = _BV(ADLAR),
	_ADC_SIZE_MASK = _BV(ADLAR),
} e_adc_size;

typedef enum e_adc_prescaler {
	ADC_PRESCALER_2 = _BV(ADPS0),
	ADC_PRESCALER_4 = _BV(ADPS1),
	ADC_PRESCALER_8 = _BV(ADPS1) | _BV(ADPS0),
	ADC_PRESCALER_16 = _BV(ADPS2),
	ADC_PRESCALER_32 = _BV(ADPS2) | _BV(ADPS0),
	ADC_PRESCALER_64 = _BV(ADPS2) | _BV(ADPS1),
	ADC_PRESCALER_128 = _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0),

	_ADC_PRESCALER_MASK = _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0),

} e_adc_prescaler;

typedef enum e_adc_input {
	ADC_ADC0 = 0,
	ADC_ADC1 = _BV(MUX0),
	ADC_ADC2 = _BV(MUX1),
	ADC_ADC3 = _BV(MUX0) | _BV(MUX1),
	ADC_ADC4 = _BV(MUX2),
	ADC_ADC5 = _BV(MUX2) | _BV(MUX0),
	ADC_ADC6 = _BV(MUX2) | _BV(MUX1),
	ADC_ADC7 = _BV(MUX2) | _BV(MUX1) | _BV(MUX0),
	ADC_TEMP = _BV(MUX3),
	ADC_11V = _BV(MUX3) | _BV(MUX2) | _BV(MUX1),
	ADC_0V = _BV(MUX3) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0),
	_ADC_INPUT_MASK = _BV(MUX0) | _BV(MUX1) | _BV(MUX2) | _BV(MUX3),
} e_adc_input;

static inline uint8_t adc_read_pin(e_adc_input input) {
	ADMUX = (ADMUX & ~(_ADC_INPUT_MASK | _ADC_SIZE_MASK)) | input | _BV(ADLAR);
	ADCSRA |= _BV(ADSC);

	while (ADCSRA & _BV(ADSC))
		;
	return ADCH;
}

static inline uint16_t adc_read_pin_10bit(e_adc_input input) {
	ADMUX = (ADMUX & ~(_ADC_SIZE_MASK | _ADC_INPUT_MASK)) |
			(input & _ADC_INPUT_MASK);
	ADCSRA |= _BV(ADSC);

	while (ADCSRA & _BV(ADSC))
		;
	return ADC;
}

static inline void adc_init(e_adc_ref v_ref, e_adc_prescaler prescaler) {
	ADMUX = (ADMUX & ~(_ADC_REF_MASK)) | v_ref;

	ADCSRA = _BV(ADEN) | (prescaler & _ADC_PRESCALER_MASK);

	(void)adc_read_pin((e_adc_input)0);
}
#endif /* ADC_H */
