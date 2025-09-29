# Hexgame

Nothing fancy, just a simple educational command line game to teach conversions from/to decimal/hexadecimal/binary conversions.

For each unique combination of base conversions user is prompted to do as many conversions as they can in 30 seconds. Most conversions are worth two points, trivial conversions (same representation in both bases) are worth one point to teach keybooard navigation.

The idea is that a systems level programmer should be able to instantly recognize bit patterns for all nibbles. Therefore, the numbers to be converted are limited to 15/0xF/0b1111.

## Dependencies

`cc` and `make` (you probably have those if Linux). On Windows you will need [MSYS2](https://www.msys2.org/) as your environment.

## Build/Install/Run

Clone repository:

```bash
git clone https://github.com/PrinssiFiestas/hexgame.git
cd hexgame
```

Run one of the following:

```bash
make           # Build the game
make run       # Build and run the game
make install   # Build and install the game (may require sudo)
make uninstall # Remove all hexgame files for all users (may require sudo)
```
