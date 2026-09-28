#include "lib/grb.h"
#include "lib/lib.h"

t_error grb_peek(grb *rb, void *buf, usize len, usize *p_bytes) {
	if (!rb || (!buf && len != 0))
		return ERROR;
	usize read = 0;
	for (usize i = 0, c = rb->count, s = rb->start; c && i < len; i++) {
		((u8 *)buf)[i] = rb->buffer[s];
		c--;
		s = (s + 1) % rb->buf_size;
		read++;
	}
	if (p_bytes)
		*p_bytes = read;
	return SUCCESS;
}

t_error grb_read(grb *rb, void *buf, usize len, usize *r_bytes) {
	if (!rb || (!buf && len != 0))
		return ERROR;
	usize read = 0;
	for (usize i = 0; rb->count && i < len; i++) {
		((u8 *)buf)[i] = rb->buffer[rb->start];
		rb->count--;
		rb->start = (rb->start + 1) % rb->buf_size;
		read++;
	}
	if (r_bytes)
		*r_bytes = read;
	return SUCCESS;
}

t_error grb_write(grb *rb, const void *buf, usize len, usize *w_bytes) {
	if (!rb || (!buf && len != 0))
		return ERROR;
	usize write = 0;
	for (usize i = 0; i < len; i++) {
		rb->buffer[rb->end] = ((const u8 *)buf)[i];
		rb->end = (rb->end + 1) % rb->buf_size;
		if (rb->count < rb->buf_size)
			rb->count++;
		else
			rb->start = (rb->start + 1) % rb->buf_size;
		write++;
	}
	if (w_bytes)
		*w_bytes = write;
	return SUCCESS;
}

void grb_clear(grb *rb) {
	rb->count = 0;
	rb->start = rb->end;
}

bool grb_empty(const grb *rb) { return rb->count == 0; }

bool grb_full(const grb *rb) { return rb->count == rb->buf_size; }

usize grb_count(const grb *rb) { return rb->count; }

usize grb_size(const grb *rb) { return rb->buf_size; }

usize grb_avail(const grb *rb) { return rb->buf_size - rb->count; }
