#include "lib/grb.h"
#include "lib/lib.h"

t_error grb_read(grb *rb, void *buf, usize len, usize *r_bytes) {
	if (!rb || !buf || !r_bytes)
		return ERROR;
	*r_bytes = 0;
	for (u16 i = 0; rb->count && i < len; i++) {
		((u8 *)buf)[i] = rb->buffer[rb->start];
		rb->count--;
		rb->start = (rb->start + 1) % rb->buf_size;
		(*r_bytes)++;
	}
	return SUCCESS;
}

t_error grb_write(grb *rb, void *buf, usize len, usize *w_bytes) {
	if (!rb || !buf || !w_bytes)
		return ERROR;
	*w_bytes = 0;
	for (u16 i = 0; i < len; i++) {
		rb->buffer[rb->end] = ((u8 *)buf)[i];
		rb->count = (rb->count + 1) % (rb->buf_size + 1);
		rb->end = (rb->end + 1) % rb->buf_size;
		(*w_bytes)++;
	}
	return SUCCESS;
}

bool grb_empty(grb *rb) { return rb->count == 0; }

bool grb_full(grb *rb) { return rb->count == rb->buf_size; }

usize grb_count(grb *rb) { return rb->count; }
