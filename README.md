# GBVideoPlayer2
A new version of GBVideoPlayer with higher resolution, 3-bit stereo PCM audio and video compression.

Version 2 increases the horizontal resolution by up to 4, replaces the chiptune music with ~9KHz, 3-bit PCM audio, introduces simple video compression with configurable quality settings, and uses a faster and easier to use encoding routines that can directly re-encode FFMPEG-compatible video to GBVP2 format.

## Examples

You can grab compiled example ROMs from the [releases page](https://github.com/LIJI32/GBVideoPlayer2/releases) or [watch a video](https://youtu.be/iDd_aqpLf5Q)

## Requirements

For playing a GBVP2 video ROM on hardware, an MBC5-compatible flash cart is required. Remember that your cartridge's capacity must be big enough for your ROM – if your ROM is over 4MBs, you will need a cartridge that can store a 8MB ROM. (Note: the common EMS 64M flash carts can only store two 4MB (32 megabits) ROMs, not a single 8MB (64 megabits) ROM)

For playing a GBVP2 video ROM on an emulator, you must use an accurate Game Boy Color emulator, such as recent versions of [SameBoy](https://sameboy.github.io) or BGB. GBVP2 will not work on inaccurate emulators, such as VisualBoyAdvance or GameBoy Online.

For encoding and building a video ROM, you will need a Make, a C compiler (Clang recommended), [rgbds](https://github.com/bentley/rgbds/releases/), and a recent version of FFMPEG.

## Format Specifications

* Irregular horizontal resolution, from effectively 120 pixels to 160 pixels wide, stretched to fill the 160 pixels wide Game Boy screen
* Vertical resolution of 144 pixels, same as the Game Boy screen
* Effectively up to 528 different colors per frame
* Stereo PCM audio at 9198 Hz and 3-bits per channel
* A frame rate of 29.86 frames per second
* In-frame compression of successive similar rows of pixels, customizable compression quality
* Can repeat a frame up to 255 times to avoid re-encoding highly similar frames

## Building a ROM

You can encode and build a ROM simply by running `make`:

```
make SOURCE=/path/to/my_video.mp4
```

`SOURCE` may be any file compatible with your copy of FFMPEG. Your output will be at `output/my_video/my_video.gbc`.

Optionally, you may specify `QUALITY` to reduce the file size or, alternatively, improve video quality:

```
make SOURCE=/path/to/my_video.mp4 QUALITY=8
```

`QUALITY` can be any non-negative integer. The higher the value, the more aggressive the compression. A value of 0 performs no lossy compression after converting a frame to the player's format. The default value is 4.