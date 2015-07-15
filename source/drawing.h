#ifndef HOMEBREW_BROWSER_DRAWING_H_
#define HOMEBREW_BROWSER_DRAWING_H_

#include <3ds.h>
namespace homebrew_browser {
// Draws a sprite that has leading metadata.
void draw_sprite(u8 const* source, u8* framebuffer, u32 x, u32 y);
// Draws a sprite that is just the image data.
void draw_raw_sprite(u8 const* source, u8* framebuffer, u32 x, u32 y, u32 width, u32 height);
void draw_solid_background(u8* framebuffer, u32 pixel_count, u8 r, u8 g, u8 b);

void draw_sprite_from_atlas(u8 const* const source, u8* framebuffer,
    s32 screen_x, s32 screen_y, u32 atlas_x, u32 atlas_y, u32 width,
    u32 height);

u32 get_image_width(u8 const* const image);
u32 get_image_height(u8 const* const image);

namespace fx {
void darken_background(u8* framebuffer, u32 pixel_count);
void fade_to_black();
}  // namespace fx
}  // namespace homebrew_browser

#endif  // HOMEBREW_BROWSER_DRAWING_H_
