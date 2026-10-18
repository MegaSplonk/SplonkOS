#pragma once

#include "lib/config.h"
#include "lib/pinout.h"

#ifdef SPLONKLIB_ENABLE_ADC

#include <avr/io.h>
#include <stdint.h>

#if defined(MCU_atmega328p) || defined(MCU_atmega32u4)

typedef enum e_adc_ref : uint8_t {
	ADC_AREF = 0,
	ADC_AVCC = _BV(REFS0),
	ADC_INT = _BV(REFS0) | _BV(REFS1),

	_ADC_REF_MASK = _BV(REFS0) | _BV(REFS1),
} e_adc_ref;

typedef enum e_adc_size : uint8_t {
	ADC_10BIT = 0,
	ADC_8BIT = _BV(ADLAR),
	_ADC_SIZE_MASK = _BV(ADLAR),
} e_adc_size;

typedef enum e_adc_prescaler : uint8_t {
	ADC_PRESCALER_2 = _BV(ADPS0),
	ADC_PRESCALER_4 = _BV(ADPS1),
	ADC_PRESCALER_8 = _BV(ADPS1) | _BV(ADPS0),
	ADC_PRESCALER_16 = _BV(ADPS2),
	ADC_PRESCALER_32 = _BV(ADPS2) | _BV(ADPS0),
	ADC_PRESCALER_64 = _BV(ADPS2) | _BV(ADPS1),
	ADC_PRESCALER_128 = _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0),

	_ADC_PRESCALER_MASK = _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0),

} e_adc_prescaler;

typedef enum e_adc_input : uint8_t {
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
	ADMUX = (ADMUX & (uint8_t)~(_ADC_INPUT_MASK | _ADC_SIZE_MASK)) | input | _BV(ADLAR);
	ADCSRA |= _BV(ADSC);

	while (ADCSRA & _BV(ADSC))
		;
	return ADCH;
}

static inline uint16_t adc_read_pin_10bit(e_adc_input input) {
	ADMUX = (ADMUX & (uint8_t)~(_ADC_SIZE_MASK | _ADC_INPUT_MASK)) | (input & _ADC_INPUT_MASK);
	ADCSRA |= _BV(ADSC);

	while (ADCSRA & _BV(ADSC))
		;
	return ADC;
}

static inline void adc_init(e_adc_ref v_ref, e_adc_prescaler prescaler) {
	ADMUX = (ADMUX & (uint8_t)~(_ADC_REF_MASK)) | v_ref;

	ADCSRA = _BV(ADEN) | (prescaler & _ADC_PRESCALER_MASK);

	(void)adc_read_pin((e_adc_input)0);
}

#elif defined(MCU_attiny1614)

typedef enum e_adc_ref : uint8_t {
	ADC_AREF = ADC_REFSEL_VREFA_gc,
	ADC_AVCC = ADC_REFSEL_VDDREF_gc,
	ADC_INT = ADC_REFSEL_INTREF_gc,

	_ADC_REF_MASK = ADC_REFSEL_gm,
} e_adc_ref;

typedef enum e_adc_size : uint8_t {
	ADC_10BIT = ADC_RESSEL_10BIT_gc,
	ADC_8BIT = ADC_RESSEL_8BIT_gc,

	_ADC_SIZE_MASK = ADC_RESSEL_bm,
} e_adc_size;

typedef enum e_adc_prescaler : uint8_t {
	ADC_PRESCALER_2 = ADC_PRESC_DIV2_gc,
	ADC_PRESCALER_4 = ADC_PRESC_DIV4_gc,
	ADC_PRESCALER_8 = ADC_PRESC_DIV8_gc,
	ADC_PRESCALER_16 = ADC_PRESC_DIV16_gc,
	ADC_PRESCALER_32 = ADC_PRESC_DIV32_gc,
	ADC_PRESCALER_64 = ADC_PRESC_DIV64_gc,
	ADC_PRESCALER_128 = ADC_PRESC_DIV128_gc,

	_ADC_PRESCALER_MASK = ADC_PRESC_gm,

} e_adc_prescaler;

typedef enum e_adc_input : uint8_t {
	ADC_ADC0 = 0x00,
	ADC_ADC1 = 0x01,
	ADC_ADC2 = 0x02,
	ADC_ADC3 = 0x03,
	ADC_ADC4 = 0x04,
	ADC_ADC5 = 0x05,
	ADC_ADC6 = 0x06,
	ADC_ADC7 = 0x07,
	ADC_ADC8 = 0x08,
	ADC_ADC9 = 0x09,
	ADC_ADC10 = 0x0A,
	ADC_ADC11 = 0x0B,

	ADC_PTC = 0x1B,
	ADC_DAC0 = 0x1C,
	ADC_INTREF = 0x1D,
	ADC_TEMPSENSE = 0x1E,
	ADC_GND = 0x1F,

	_ADC_INPUT_MASK = ADC_MUXPOS_gm,

} e_adc_input;

static inline uint8_t adc_read_pin(e_adc_input input) {
	ADC0.MUXPOS = input & _ADC_INPUT_MASK;

	ADC0.COMMAND = ADC_STCONV_bm;

	while (!(ADC0.INTFLAGS & ADC_RESRDY_bm))
		;

	ADC0.INTFLAGS = ADC_RESRDY_bm;

	return (uint8_t)ADC0.RES;
}

static inline uint16_t adc_read_pin_10bit(e_adc_input input) {
	ADC0.MUXPOS = input & _ADC_INPUT_MASK;

	ADC0.COMMAND = ADC_STCONV_bm;

	while (!(ADC0.INTFLAGS & ADC_RESRDY_bm))
		;

	ADC0.INTFLAGS = ADC_RESRDY_bm;

	return ADC0.RES;
}

static inline void adc_init(e_adc_ref ref, e_adc_prescaler prescaler) {
	const pin adc_pins[] = {ADC_BAR1};

	for (usize i = 0; i < tabsize(adc_pins); i++) {
		pin p = adc_pins[i];
		pin_input(p);
		*(u8 *)((p.port == PORT_A ? PORTA_PIN0CTRL : PORTB_PIN0CTRL) + p.pin) =
			PORT_ISC_INPUT_DISABLE_gc;
		// ^^^ Disable digital input buffer for better precision/less noise
	}

	ADC0.CTRLA = 0;
	ADC0.CTRLC = (prescaler & _ADC_PRESCALER_MASK) | (ref & _ADC_REF_MASK); // ADC_SAMPCAP_bm |
	ADC0.CTRLA = ADC_RESSEL_10BIT_gc | ADC_ENABLE_bm;
	// ADC0.CALIB = ADC_DUTYCYC_bm;

	ADC0.MUXPOS = ADC_ADC0;

	(void)adc_read_pin(ADC_ADC0);
}

#endif

#endif
