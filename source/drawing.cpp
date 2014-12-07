#include "drawing.h"

#include <cstring>

#include <3ds.h>

void blit_column(u8 const* source, u8* dest, u32 pixel_count) {
  memcpy(dest, source, 3 * pixel_count);
}

void blit(u8 const* source, u8* dest, u32 rows, u32 columns) {
  for (u32 i = 0; i < columns; ++i) {
    blit_column(source + 3 * rows * i, dest + 3 * 240 * i, rows);
  }
}

void blit_sprite(u8 const* source, u8* dest) {
  u32 width = *reinterpret_cast<u32 const*>(source);
  u32 height = *reinterpret_cast<u32 const*>(source + 4);
  blit(source + 8, dest, height, width);
}

void draw_sprite(u8 const* source, u8* framebuffer, u32 x, u32 y) {
  u32 height = *reinterpret_cast<u32 const*>(source + 4);
  // Convert from typical screen coordinates to framebuffer coordinates.
  blit_sprite(source, framebuffer + 3 * (240 - y - height + x * 240));
}

void draw_solid_background(u8* framebuffer, u32 pixel_count, u8 r, u8 g, u8 b) {
  for (u32 i = 0; i < pixel_count; ++i) {
    framebuffer[3 * i + 0] = b;
    framebuffer[3 * i + 1] = g;
    framebuffer[3 * i + 2] = r;
  }
}
