# golden-knight

A tiny terminal intro: a knight in a golden-hour wheat field, with a
harpsichord and string arrangement synthesized in real time. Written in C
for 32-bit Linux, with no libc. Target: 4 KB compressed.

## Requirements

- Linux on x86 (32-bit programs enabled) or WSL2
- `gcc` with 32-bit support (`gcc-multilib`), `make`, `xz`
- a terminal with 24-bit colour, at least 120×30 characters
- audio: `aplay` on Linux, or `ffplay.exe` (FFmpeg for Windows) on WSL

## Build and run

```sh
make            # builds build/knight
make run        # Linux: sound through aplay
make run-wsl    # WSL: sound through Windows' ffplay.exe
make size       # stripped and compressed size
```

The picture goes to stderr (the terminal) and the music to stdout as raw
16-bit, 44.1 kHz mono samples, so any player can be piped in:

```sh
./build/knight | aplay -q -f S16_LE -r 44100 -c 1
```

## Music

`song.h` is generated from a MIDI file of the song *Golden Brown* 
by *The Stranglers*, which is not included.

## Files

| File              | Contents                                               |
|-------------------|--------------------------------------------------------|
| `main.c`          | the picture, the frame loop, the audio process         |
| `sound.c`         | the synthesizer: plucked and bowed strings             |
| `sys.h`           | system calls, x87 math and `_start` (replaces libc)    |
