#ifndef HOMEBREW_BROWSER_DRAWING_H_
#define HOMEBREW_BROWSER_DRAWING_H_

#include <3ds.h>

void draw_sprite(u8 const* source, u8* framebuffer, u32 x, u32 y);
void draw_solid_background(u8* framebuffer, u32 pixel_count, u8 r, u8 g, u8 b);

#endif  // HOMEBREW_BROWSER_DRAWING_H_
