#!/usr/bin/env python3

# This file was taken from smealum/3ds_hb_menu, commit
# ab2982254a4785805f6a45a940a5b67555150700. It was originally called font.py,
# and was written by smealum.
# It has been adapted to our workflow.

import struct
import math
import os
import sys
from PIL import Image

fntfn = sys.argv[1]
fontName = sys.argv[2]

def readLine(l):
  l=l.split()
  carac={}
  if len(l)>0 and l[0]!="info":
    for w in l[1:]:
      v = w.split("=")
      carac[v[0]]=v[1].replace("\"","")
  return (l[0], carac)

fontData=[]
fontDesc={}

def outputChar(p, c):
  global fontData, fontDesc, fontName
  # im = p[c["page"]][1]
  x, y = int(c["x"]), int(c["y"])
  w, h = int(c["width"]), int(c["height"])
  xo, yo = int(c["xoffset"]), int(c["yoffset"])
  id, xa = int(c["id"]), int(c["xadvance"])
  # if id<164:
  #   c = chr(id)
  #   c = c if c!="'" else "\\'"
  #   c = c if c!="\\" else "\\\\"
  #   c = "\'"+c+"\'"
  # else:
  #   c = str(id)
  c = str(id)
  data = []
  # for i in range(w):
  #   data.extend([im.getpixel((x+i,y+h-1-j)) for j in range(h)])
  # fontDesc[id] = (" {%s, %d, %d, %d, %d, %d, %d, %d, &%s_data[%d]},"%(c,x,y,w,h,xo,yo,xa,fontName,len(fontData)))
  fontDesc[id] = (" {%s, %d, %d, %d, %d, %d, %d, %d},"%(c,x,y,w,h,xo,yo,xa))
  fontData.extend(data)

def outputFontDesc():
  global fontDesc
  for i in range(128):
    if i in fontDesc:
      print(fontDesc[i])
    else:
      print(" {0, 0, 0, 0, 0, 0, 0, 0},")

def outputFontData():
  global fontData, fontName
  print("u8 "+fontName+"_data[] = {"+"".join([hex(v)+", " for v in fontData])+"0x00};")

f = open(fntfn, "r")
pages = {}
for l in f:
  l=readLine(l)
  if len(l)>0:
    # if l[0]=="page":
    #   pages[l[1]["id"]]=(l[1]["file"], Image.open(l[1]["file"]))
    # elif l[0]=="char":
    if l[0] == "char":
      outputChar(pages, l[1])


print("#include <3ds.h>")
print("#include \"font.h\"")
# print("extern u8 fontData[];")
# print("typedef struct {char c; int x, y, w, h, xo, yo, xa; u8* data;}CharacterDescription;")
print("std::array<CharacterDescription, 128> "+fontName+"_desc{{")
outputFontDesc()
print("}};")
# outputFontData()