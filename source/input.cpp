#include "input.h"

#include <3ds.h>

void handle_input(u32 kDown, BrowserState& state) {
  //Exit the app on home button
  if (kDown & KEY_START) {
      aptSetStatus(APP_EXITING);
  }

  
}