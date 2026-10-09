#pragma once

#include "lib/usb/api.h"

struct pu_module_vtable;
struct pu_module;
struct pu;

struct pu_module_vtable {
	bool (*setup)(struct pu_module *self, struct usb_setup *setup);
	int (*get_interface)(struct pu_module *self, u8 *interfaceCount);
	int (*get_descriptor)(struct pu_module *self, struct usb_setup *setup);
	u8 (*get_shortname)(struct pu_module *self, char *name);
	void (*flush)(struct pu_module *self);
};

u8 pu_module_get_shortname_default(struct pu_module *self, char *name);

struct pu_module {
	struct pu_module_vtable *vtable;

	u8 pluggedInterface;
	u8 pluggedEndpoint;

	const u8  numEndpoints;
	const u8  numInterfaces;
	const u8 *endpointType;

	struct pu_module *next;
};

struct pu {
	u8				  lastIf;
	u8				  lastEp;
	struct pu_module *root;
};

bool pu_plug(struct pu_module *node);
int	 pu_get_interface(u8 *interfaceCount);
int	 pu_get_descriptor(struct usb_setup *setup);
bool pu_setup(struct usb_setup *setup);
void pu_get_shortname(char *iSerialNum);
void pu_flush(void);

struct pu *pu_instance(void);
