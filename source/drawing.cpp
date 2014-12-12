#include "drawing.h"

#include <cstring>

#include <3ds.h>

u32 get_image_width(u8 const* const image) {
  return *reinterpret_cast<u32 const*>(image);
}

u32 get_image_height(u8 const* const image) {
  return *reinterpret_cast<u32 const*>(image + 4);
}

u32 const kImageHeaderSizeBytes = 8;

void blit_column(u8 const* source, u8* dest, u32 pixel_count) {
  memcpy(dest, source, 3 * pixel_count);
}

void blit(u8 const* source, u8* dest, u32 rows, u32 columns) {
  for (u32 i = 0; i < columns; ++i) {
    blit_column(source + 3 * rows * i, dest + 3 * 240 * i, rows);
  }
}

void blit_sprite(u8 const* source, u8* dest) {
  u32 width = get_image_width(source);
  u32 height = get_image_height(source);
  blit(source + 8, dest, height, width);
}

void draw_sprite(u8 const* source, u8* framebuffer, u32 x, u32 y) {
  u32 const height = get_image_height(source);
  // Convert from typical screen coordinates to framebuffer coordinates.
  blit_sprite(source, framebuffer + 3 * (240 - y - height + x * 240));
}

void draw_sprite_from_atlas(u8 const* const source, u8* framebuffer,
    s32 screen_x, s32 screen_y, u32 atlas_x, u32 atlas_y, u32 width,
    u32 height) {
  u32 const atlas_height = get_image_height(source);
  // u32 const atlas_width = get_image_width(source);
  for (u32 i = 0; i < width; ++i) {
    blit_column(
        source + kImageHeaderSizeBytes + 3 * ((atlas_x + i) * atlas_height +
        atlas_height - atlas_y - height),
        framebuffer + 3 * ((screen_x + i) * 240 + 240 - screen_y - height), height);
  }
}

void draw_solid_background(u8* framebuffer, u32 pixel_count, u8 r, u8 g, u8 b) {
  for (u32 i = 0; i < pixel_count; ++i) {
    framebuffer[3 * i + 0] = b;
    framebuffer[3 * i + 1] = g;
    framebuffer[3 * i + 2] = r;
  }
}
