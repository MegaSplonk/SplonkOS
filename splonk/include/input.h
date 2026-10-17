#pragma once
#include "lib/lib.h"

// Physical identity, never an I2C address, layer or HID usage.
typedef enum input_module : u8 {
	INPUT_NONE,
	INPUT_SPLONK_L,
	INPUT_SPLONK_R,
	INPUT_SPLONKO_L,
	INPUT_SPLONKO_R,
	INPUT_SPLONKUS,
	INPUT_SPLONKNOB,
	INPUT_SPLONKBAR,
	INPUT_SPLONKPAD,
	INPUT_MODULE_COUNT,
} input_module;

typedef enum input_kind : u8 {
	INPUT_BUTTON,
	INPUT_STEP,
	INPUT_ABSOLUTE,
	INPUT_POINTER,
	INPUT_TAP,
	INPUT_RESET,
} input_kind;

enum input_wiring {
	INPUT_ROWS_FIRST = 0,
	INPUT_COLUMNS_FIRST = 1,
	INPUT_MIRROR_COLUMNS = 2,
	INPUT_MIRROR_ROWS = 4,
};

typedef struct input_event {
	u8			 source;
	input_module module;
	input_kind	 kind;
	union {
		struct {
			u8	 position;
			bool pressed;
		} button;
		u8	step;	  // Knob position: l1/r1/b1/l2/r2/b2 (buttons are positions 2/5).
		u16 absolute; // Source units; ADC calibration/HID scaling is a later policy.
		struct {
			i16 dx, dy;
			u8	buttons;
		} pointer; // Relative movement, button levels.
		u8 taps;   // Semantic single/double/triple tap; not synthesized mouse clicks.
	};
} input_event;
typedef void (*input_emit)(const input_event *event, void *context);

typedef struct input_device {
	input_module module;
	u8			 wiring;
	union {
		u8 held[3];
		struct {
			u16	 value;
			bool valid;
		} axis;
		u8 buttons;
	};
} input_device;

// Zero-initialize state. Callbacks are synchronous, non-reentrant, and must
// consume/copy events before returning. No transport, queue or USB assumptions.
// Rebinding releases old buttons, then emits RESET for the old module. A caller
// changing source identity must first bind INPUT_NONE, even if the role is equal.
bool input_bind(input_device *device, u8 source, input_module module, u8 wiring, input_emit emit,
				void *context);
// Exact packed lengths: 3 bytes for 4x6 matrices, 1 for thumbs/knob. Matrix
// padding is ignored. Knob directions are one step PER PACKET, including
// consecutive identical packets; only buttons are held. Invalid frames do not
// change state. Matrix positions are visual row * columns + column.
bool input_feed(input_device *device, u8 source, const u8 *bytes, usize length, input_emit emit,
				void *context);
// Typed extension points for unmerged bar/pad firmware. They intentionally do
// not define an I2C wire ABI. A future packet adapter calls these after decoding.
bool input_absolute(input_device *device, u8 source, u16 value, input_emit emit, void *context);
bool input_pointer(input_device *device, u8 source, i16 dx, i16 dy, u8 buttons, input_emit emit,
				   void *context);
bool input_tap(input_device *device, u8 source, u8 count, input_emit emit, void *context);

// Optional transport-independent snapshot cache. The adapter supplies a binding
// token (zero = absent); changing it releases state and discards old samples.
// This preserves held matrix/button levels received before a role is resolved,
// without replaying transient encoder steps when roles or USB ownership change.
typedef struct input_source {
	input_device decoder;
	u8			 identity;
	u8			 cached[3];
	u8			 length;
} input_source;
bool input_source_bind(input_source *source, u8 slot, u8 identity, input_module module, u8 wiring,
					   input_emit emit, void *context);
bool input_source_feed(input_source *source, u8 slot, const u8 *bytes, u8 length, bool knob,
					   input_emit emit, void *context);
