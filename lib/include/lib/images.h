#pragma once

typedef struct s_img {
	const u8 *data;
	u8		  height;
	u8		  width;
} t_image;

extern const t_image maieul;
extern const t_image erase_modifier;
extern const t_image alt_modifier;
extern const t_image shift_modifier;
extern const t_image super_modifier;
extern const t_image ctrl_modifier;

extern const t_image default_layer;
extern const t_image nav_layer;
extern const t_image numbers_layer;
extern const t_image rgb_layer;

extern const t_image hollow_vassal;

typedef enum image_mod_types : u8 {
	IMG_MOD_ERASE,
	IMG_MOD_LCTRL,
	IMG_MOD_LSHIFT,
	IMG_MOD_LALT,
	IMG_MOD_LSUPER,
	IMG_MOD_RCTRL,
	IMG_MOD_RSHIFT,
	IMG_MOD_RALT,
	IMG_MOD_RSUPER,
	__IMG_MOD_SIZE,
} image_mod_types;

typedef enum image_lay_types : u8 {
	IMG_LAY_DEF,
	IMG_LAY_NAV,
	IMG_LAY_NUM,
	IMG_LAY_RGB,
	__IMG_LAY_SIZE,
} image_lay_types;

extern t_image const *image_modifiers[];
extern t_image const *image_layers[];

#define SPLONKREEN_DEFAULT_IMAGE hollow_vassal
