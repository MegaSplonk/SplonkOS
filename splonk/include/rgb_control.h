#pragma once

#include "keycodes.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/rgb_matrix.h"

void rgb_control_init(u8 *keys);
bool rgb_control_key(u16 code, bool pressed);
void rgb_control_set_mode(u8 mode);
led_mode rgb_control_get_mode(void);
void rgb_control_update(i2c_peer *peers, usize count);
