# units for TI-84 Plus CE

Input is limited to 64 characters. Inputs and results wrap across multiple display lines, and the latest 6 entries are retained. Numeric results longer than 8 characters use compact scientific notation such as `1e8`.

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