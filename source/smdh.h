#ifndef HOMEBREW_BROWSER_SMDH_H_
#define HOMEBREW_BROWSER_SMDH_H_

// This file was taken from smealum/3ds_hb_menu/source/smdh.h, commit
// ab2982254a4785805f6a45a940a5b67555150700. It was written by smealum, fincs,
// and gemisis.

#include <3ds.h>

namespace homebrew_browser {

typedef struct
{
  u32 magic;
  u16 version;
  u16 reserved;
}smdhHeader_s;

typedef struct
{
  u16 shortDescription[0x40];
  u16 longDescription[0x80];
  u16 publisher[0x40];
}smdhTitle_s;

typedef struct
{
  u8 gameRatings[0x10];
  u32 regionLock;
  u8 matchMakerId[0xC];
  u32 flags;
  u16 eulaVersion;
  u16 reserved;
  u32 defaultFrame;
  u32 cecId;
}smdhSettings_s;

typedef struct
{
  smdhHeader_s header;
  smdhTitle_s applicationTitles[16];
  smdhSettings_s settings;
  u8 reserved[0x8];
  u8 smallIconData[0x480];
  u16 bigIconData[0x900];
}smdh_s;

int extractSmdhData(smdh_s* s, char* name, char* desc, char* auth, u8* iconData);

}  // namespace homebrew_browser

#endif  // HOMEBREW_BROWSER_SMDH_H_
