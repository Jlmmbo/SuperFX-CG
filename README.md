# SuperFX-CG

A SNES (Super Famicom) emulator for Casio Prizm/CG graphing calculators, built with [PrizmSDK](https://github.com/Jonimates/PrizmSDK).
It is still very far from functional, so don't expect to be able to play any games for a while.

## Features

- 65C816 CPU emulation (the SNES's processor)
- PPU (Picture Processing Unit) emulation with basic rendering
- ROM loading from the calculator's filesystem
- Configurable keybindings
- Overclock support
- DMA-based display for improved performance
- RAM bank switching

## Building

Requirements: [PrizmSDK](https://github.com/Jonimates/PrizmSDK) (the SDK root should be at or above `../../` relative to the project, or set `FXCGSDK`).

```sh
make
```

This produces `SuperFX-CG.g3a` (the add-in) and `SuperFX-CG.bin`.

## Usage

1. Transfer `SuperFX-CG.g3a` to your Casio Prizm/CG calculator.
2. Launch the add-in.
3. From the main menu, go to **Load ROM** and select an SNES ROM file (`.smc`, `.sfc`, etc.) on the calculator.
4. Use the on-screen keybinding config to map SNES controller buttons to calculator keys.

### Default keybinds

| SNES | Calculator  |
|------|-------------|
| B    | OPTN        |
| Y    | x^2         |
| Select | F6       |
| Start | F6        |
| D-Pad | Arrow keys |
| A    | SHIFT       |
| X    | ALPHA       |
| L    | F1          |
| R    | F2          |

## License

GPLv3 - see [LICENSE](LICENSE).
