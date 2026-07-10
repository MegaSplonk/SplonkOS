#include "lib/grb.h"
#include "lib/lib.h"

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

t_error grb_write(grb *rb, void *buf, usize len, usize *w_bytes) {
	if (!rb || (!buf && len != 0))
		return ERROR;
	usize write = 0;
	for (usize i = 0; i < len; i++) {
		rb->buffer[rb->end] = ((u8 *)buf)[i];
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

bool grb_empty(grb *rb) { return rb->count == 0; }

bool grb_full(grb *rb) { return rb->count == rb->buf_size; }

usize grb_count(grb *rb) { return rb->count; }
