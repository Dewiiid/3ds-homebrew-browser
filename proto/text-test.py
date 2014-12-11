import os, sys
from PIL import Image

input_string = "Hello World!"

def decode_fnt_file(filename):
  glyph_list = {}

  with open(filename) as fp:
    lines = fp.readlines()

    #skip over the first 4 lines in the file; we don't care about them right now
    for line in lines[4:]:
      elements = line.split()
      char_data = {}
      # the first element on each line is simply "char" which isn't useful,
      # so we skip it here
      for element in elements[1:]:
        components = element.split("=")
        key = components[0]
        value = components[1]
        char_data[key] = int(value)
      if 'id' in char_data:
        glyph_list[int(char_data['id'])] = char_data
  return glyph_list

font_atlas = Image.open("../assets/fonts/ubuntu_condensed_16px_0.png")
font_data = decode_fnt_file("../assets/fonts/ubuntu_condensed_16px.fnt")

print("Number of glyphs: ", len(font_data))

output_image = Image.new("RGB", (256,256))

def draw_string_into_image(string, dest, font_atlas, font_data):
  x_pos = 0
  for char in string:
    if ord(char) in font_data:
      char_data = font_data[ord(char)]
      char_image = font_atlas.crop((char_data["x"], char_data["y"], char_data["x"] + char_data["width"], char_data["y"] + char_data["height"]))
      dest.paste(char_image, (x_pos,0))
      x_pos += char_data["xadvance"]
    else:
      print("Bad glyph: ", ord(char))
      x_pos += 1 # do something halfway useful with non-printable characters

draw_string_into_image(input_string, output_image, font_atlas, font_data)
output_image.save("TEST_OUTPUT.png")