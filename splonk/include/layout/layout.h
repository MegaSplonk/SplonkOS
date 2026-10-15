#pragma once

#include "lib/usb/hid.h"
enum e_kkind : u8 {
	KK_SCANCODE,
	KK_CC,
	KK_MOUSE,
	KK_BUILTIN,
	KK_NONE = 255,
};

#define KK(_K, _S)                                                                                 \
	{                                                                                              \
		{.kind = KK_##_K, .payload = (_S)},                                                        \
	}

typedef struct s_key {
	union {
		u16 raw;
		struct {
			enum e_kkind kind;
			u8			 payload;
		};
	};
} key;

typedef struct s_layout {
	u8			  w, h;
	struct s_key *key;
} layout;

// get a key from the layout
key kk_getkey(layout *layout, u8 x, u8 h);

void kk_apply_key(key key);
void kk_unapply_key(key key);

enum e_kk_mouse : u8 {
	KK_MOUSE_UP,
	KK_MOUSE_DOWN,
	KK_MOUSE_LEFT,
	KK_MOUSE_RIGHT,
	KK_MOUSE_LBTN,
	KK_MOUSE_RBTN,
	KK_MOUSE_MBTN,
	_KK_MOUSE_LAST,
};

enum e_kk_cc : u8 {
	KK_CC_VOL_UP,
	KK_CC_VOL_DOWN,
	KK_CC_MUTE,
	KK_CC_PAUSE_PLAY,
	KK_CC_NEXT,
	KK_CC_PREVIOUS,
	_KK_CC_LAST,
};

enum e_kk_builtin : u8 {
	KK_BUILTIN_SERIAL,
	KK_BUILTIN_LED_MODE,
	_KK_BUILTIN_LAST,
};

#define KK_MOUSE_SENSITIVITY 20
