#include "pluggable_usb.h"
#include <util/atomic.h>

extern uint8_t _initEndpoints[];

u8 pu_module_get_shortname_default(struct pu_module *self, char *name) {
	name[0] = 'A' + (char)self->pluggedInterface;
	return 1;
}

int pu_get_interface(u8 *interfaceCount) {
	struct pu *self = pu_instance();

	int sent = 0;

	struct pu_module *node = nullptr;
	for (node = self->root; node != nullptr; node = node->next) {
		int res = node->vtable->get_interface(node, interfaceCount);
		if (res < 0)
			return -1;
		sent += res;
	}
	return sent;
}

int pu_get_descriptor(struct usb_setup *setup) {
	struct pu		 *self = pu_instance();
	struct pu_module *node = nullptr;
	for (node = self->root; node != nullptr; node = node->next) {
		int res = node->vtable->get_descriptor(node, setup);
		if (res)
			return res;
	}
	return 0;
}

void pu_get_shortname(char *iSerialNum) {
	struct pu		 *self = pu_instance();
	struct pu_module *node = nullptr;
	for (node = self->root; node; node = node->next) {
		iSerialNum += node->vtable->get_shortname(node, iSerialNum);
	}
	*iSerialNum = 0;
}

bool pu_setup(struct usb_setup *setup) {
	struct pu		 *self = pu_instance();
	struct pu_module *node = nullptr;
	for (node = self->root; node != nullptr; node = node->next) {
		if (node->vtable->setup(node, setup))
			return false;
	}
	return true;
}

void pu_flush(void) {
	struct pu		 *self = pu_instance();
	struct pu_module *node = nullptr;
	for (node = self->root; node != nullptr; node = node->next) {
		if (node->vtable->flush)
			node->vtable->flush(node);
	}
}

bool pu_plug(struct pu_module *node) {
	struct pu *self = pu_instance();

	if ((self->lastEp + node->numEndpoints) > USB_ENDPOINTS) {
		return false;
	}

	node->next = nullptr;

	if (self->root == nullptr) {
		self->root = node;
	} else {
		struct pu_module *current = self->root;
		while (current->next) {
			current = current->next;
		}
		current->next = node;
	}

	node->pluggedInterface = self->lastIf;
	node->pluggedEndpoint = self->lastEp;
	self->lastIf += node->numInterfaces;
	for (uint8_t i = 0; i < node->numEndpoints; i++) {
		_initEndpoints[self->lastEp] = node->endpointType[i];
		self->lastEp++;
	}
	return true;
	// restart USB layer???
}

struct pu *pu_instance(void) {
	static bool		 init = false;
	static struct pu pu = {};

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (!init) {
			init = true;
			pu.lastIf = 1; // CDC_ACM_INTERFACE + CDC_INTERFACE_COUNT;
			pu.lastEp = 1; // CDC_FIRST_ENDPOINT + CDC_ENPOINT_COUNT;
			pu.root = nullptr;
		}
	}
	return &pu;
}
