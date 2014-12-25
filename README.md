# 3DS Homebrew Browser

Download homebrew from the internet!

## What does this do?

The homebrew browser allows for easy loading of new homebrew software onto a
homebrew enabled 3DS. You should already be able to run the Homebrew
Menu/Launcher to run this code. Your 3DS should be connected to the internet for
this to work.

## How to use

Drop the `homebrew-browser` folder into the `/3ds` directory on your 3DS's SD
card. Don't rename the folder or the application - there are some paths that are
presently hard-coded and rely on these paths existing.

The basic use of the browser is via the `+Pad` or `OPad` and the `A` button. Use
the pad to navigate up and down the list, and the `A` button to download the
highlighted app. The `L` and `R` buttons filter by category - they are games,
media, emulators, utility, and miscelaneous. `Select` will change the sort
order. `X` opens the debug console on the top screen.

Similarly, the main controls can be touched as well - to navigate, drag the
scroll bar. To download a title, tap on it.

## Submitting/removing an app

Presently, the server's source is not yet available, but if you would like your
app to show up in the browser, please send an email to
zeta0134@darknovagames.com. Similarly, if you've added an app and would like to
remove it, please send an email to the same address stating that.

## Warning - this is early stage software!

This code was written relatively quickly, meaning that there's not a whole lot
of error checking. We plan to smooth this out as time goes on and we continue
to work on it, but realize that it is still subject to lock ups and freezing -
particularly regarding network operations.

The early nature of this software also means that while, theoretically, the
Homebrew Browser should be able to download itself to upgrade to newer versions,
the API may change break backwards compatibility, requiring a manual upgrade. We
will try to avoid this until the API solidifies, but until then, this is a
potential issue.

## Planned features

* Custom homebrew sources, so that people can easily host their own app
  repositories.
* Release of the server source code.

## Building

There is a `Dockerfile` available, which will set up the devlopment environment
using Docker. If you prefer to get set up manually, you'll need devkitARM r43
and a recent ctrulib - this was developed against a recent pull of its master
branch.

Once the environment is set up, just run `make`. It will generate
`homebrew-browser.3dsx` and `homebrew-browser.smdh`, which can be loaded onto
the SD card using your preferred method so that the Homebrew Menu/Launcher can
run it.

BMFont is used to generate the font atlases as PNGs and offset information as
fnt files. The fnt file is already pre-generated, and has been passed through
`tools/convert-fnt.py` and stored in the repo as C++ source files.

Images are manually run through `tools/image-to-3ds.py` script, which converts
them to a raw BGR image with some metadata prepended.

## Bugs and feature requests

We are using the issues on this repository to keep track of things that need to
be done. Whether there's something you'd like to see or something doesn't work
quite right, please open an issue there and we'll take a look at it.

## License

The license for the source is a standard BDS 3-clause license with an additional
clause to make explicit our desire that this software not be used for piracy.
The full license, including additional clause, is in the `LICENSE` file.

The font used throughout the app is the Ubuntu font. The original TTF files and
license are in `assets/fonts`.

## Credits

* cromo for the initial development
* zeta0134 for development on both the client and server
* smealum, fincs, and gemisis for the SMDH reading code, which was taken from
  the Homebrew Menu/Launcher
* smealum, whose code was used as a basis for the fnt file conterter
