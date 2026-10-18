#pragma once

typedef struct s_img {
	const u8 *data;
	u8		  height;
	u8		  width;
} t_image;

extern const t_image maieul;
extern const t_image alt_modifier;
extern const t_image shift_modifier;
extern const t_image super_modifier;
extern const t_image ctrl_modifier;

extern const t_image default_layer;
extern const t_image nav_layer;
extern const t_image numbers_layer;
extern const t_image rgb_layer;

extern const t_image hollow_vassal;