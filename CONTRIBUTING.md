# Contributing

Thanks for your interest! This fork welcomes bug reports, feature ideas and pull requests.

## Before writing code

- **Bugs**: open an issue with what you did, what you expected, what happened, and your console model (DS / DS Lite / DSi). A photo of the screen helps.
- **Features**: open an issue first so we can talk about the design before you invest time. Small quality-of-life features in the spirit of the existing ones are the best fit.
- **Upstream or here?** Improvements to the launcher's core (display modes, themes, cheats, file associations) belong in [upstream Pico Launcher](https://github.com/LNH-team/pico-launcher) — please send them there so every user benefits. This fork focuses on the library features listed in [docs/Enhanced.md](docs/Enhanced.md).

## Building

See the [Compiling](README.md#compiling) section of the README. The short version:

```bash
git submodule update --init
docker run --rm -v "$PWD":/work -w /work skylyrac/blocksds:slim-v1.16.0 make
```

The build must finish without new warnings.

## Pull requests

- One change per PR. Small, focused PRs get reviewed fast; big ones stall.
- **Test on real hardware** before opening the PR and say in the description which console model you tested on. Emulators do not model the DSpico, the SD timing, or the power management hardware.
- Match the style of the surrounding code (4-space indent, braces on their own line, member fields prefixed `_`). Comments explain *why*, not *what*.
- Update the docs that your change touches: [docs/Enhanced.md](docs/Enhanced.md) for user-facing behavior, [docs/GameData.md](docs/GameData.md) if you change the `gamedata.json` format, and [CHANGELOG.md](CHANGELOG.md).
- Don't commit build artifacts (`LAUNCHER.nds`, `build/`).

## Things to know about this codebase

- The ARM9 does everything (UI, filesystem, decoding); the ARM7 is a minimal servant (touch, SD sectors, RTC, power chip). Keep it that way — and keep all ARM7 SPI access on its main thread, the bus is shared with the touch screen.
- `gamedata.json` is read by external tools ([PicoDex](https://github.com/rasalopa/picodex)). Changing its format needs a matching change there; unknown keys are dropped on save, so additions must be coordinated.
- Launching a game is a cold restart: no launcher state survives except what is written to the SD card.
- `docs/Enhanced.md` and `docs/GameData.md` document every feature and format in detail.

## License

By contributing you agree that your code is released under the same license as the project (see [LICENSE.txt](LICENSE.txt)).
