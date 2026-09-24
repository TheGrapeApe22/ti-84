# units for TI-84 Plus CE

Input is limited to 64 characters. Inputs and results wrap across multiple display lines, and the latest 6 entries are retained. Numeric values are limited to 8 displayed characters, have trailing zeros removed, and use compact scientific notation such as `1e8` when needed. History entries use only the lines they need; select one with Up/Down and press Clear or Del to delete it without changing the current input.

Addition and subtraction work between compatible quantities, for example `1 m + 20 cm` or `3 ft - 6 inch`. Standard multiplication/division precedence and parentheses are supported.

## Build

* Install [CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/static/getting-started.html)
* `make` to output to `bin/UNITCALC.8xp`

## Transfer to calculator

Use TI Connect CE to transfer:
* `UNITCALC.8xp`
* `UNITDB.8xv`
* `clibs.8xg` (library)

To run, select `prgm`, `UNITCALC`.

If your calculator's OS version is 5.5+, you need to [jailbreak](https://yvantt.github.io/arTIfiCE/) it first.