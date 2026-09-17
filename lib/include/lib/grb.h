#pragma once

typedef struct generic_ring_buffer {
	const usize buf_size;
	usize		count;
	u8 *const	buffer;
	usize		start;
	usize		end;
} grb;

typedef struct grb_input_output {
	grb input;
	grb output;
} grbio;

#define GRB_INIT(BUF) {.buf_size = sizeof(BUF), .count = 0, .buffer = BUF, .start = 0, .end = 0}

#define GRBIO_INIT(IONAME, IBUF, OBUF, ILEN, OLEN, ...)                                            \
	__VA_ARGS__ uint8_t IBUF[ILEN] = {};                                                           \
	__VA_ARGS__ uint8_t OBUF[OLEN] = {};                                                           \
	__VA_ARGS__ grbio	IONAME = {                                                                 \
		.input = GRB_INIT(IBUF),                                                                   \
		.output = GRB_INIT(OBUF),                                                                  \
	}

t_error grb_read(grb *rb, void *buf, usize len, usize *r_bytes);

t_error grb_write(grb *rb, const void *buf, usize len, usize *w_bytes);

void grb_clear(grb *rb);

bool grb_empty(const grb *rb);

bool grb_full(const grb *rb);

usize grb_count(const grb *rb);

usize grb_size(const grb *rb);

usize grb_avail(const grb *rb);
