# Units for the TI-84 Plus CE

A native, GNU Units-style conversion program with a replaceable unit database.
The calculator program and its data are separate: update `UNITDB.8xv` to add or
change units without rebuilding `UNITCALC.8xp`.

## Using the calculator

1. At `You have:`, enter a number and unit, such as `10 ft`, then press `ENTER`.
2. At `You want:`, enter the destination unit, such as `m`, then press `ENTER`.
3. The conversion or a descriptive error appears in history.

Supported expressions include prefixes, multiplication, division, parentheses,
and integer powers. Examples:

- `1 mi` to `km`
- `32 F` to `C`
- `1 kg*m/s^2` to `N`
- `60 mph` to `m/s`
- `1 acre` to `ft^2`

Names and prefixes are case-sensitive. For example, `m` is a meter while `M`
can be used by a database as a different prefix; temperature symbols `C`, `F`,
and `K` require a capital letter.

## Controls

- Letter keys type the lowercase green letter printed above each key.
- `2ND` toggles `(abc)` and one-shot `(ABC)`; one capital returns to lowercase.
- `ALPHA` toggles `(abc)` and `(123)` for numbers and operators.
- `0` in `(abc)` inserts a space.
- `LEFT` / `RIGHT` move the insertion cursor.
- `DEL` backspaces before the cursor.
- `CLEAR` clears the current prompt.
- `UP` / `DOWN` select conversion history. `ENTER` inserts that conversion's
  `have` or `want` text into the active prompt.
- `2ND`, then `ON` (`OFF`) exits while `(ABC)` is displayed.

Input is limited to 30 characters. The latest six conversions are retained until
exit.

## Dynamic unit database

Edit [`data/units.dat`](data/units.dat) on the computer. Blank lines and lines
starting with `#` are ignored. The two record forms are:

```text
P|prefix|scale
U|name|scale|offset|L|M|T|Temp|Current|Amount|Luminosity
```

A unit converts to SI base units using:

```text
base_value = (input_value + offset) * scale
```

The seven final fields are integer dimensional exponents. For example:

```text
P|kilo|1000
U|m|1|0|1|0|0|0|0|0|0
U|meters|1|0|1|0|0|0|0|0|0
U|N|1|0|1|1|-2|0|0|0|0
U|F|0.5555555555555556|459.67|0|0|0|1|0|0|0
```

Aliases, abbreviations, and plurals are separate `U` records, so all naming is
customizable in the database. A direct unit match takes priority over splitting
a name into prefix + unit. Affine units such as temperatures must stand alone.
The current limits are 128 unit names, 20 prefixes, names shorter than 20 bytes,
and a database smaller than 12 KiB.

## Build

Install the [CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/),
then run:

```sh
make
```

This produces:

- `bin/UNITCALC.8xp` — program
- `bin/UNITDB.8xv` — replaceable database AppVar

Running `make database` rebuilds only the AppVar after editing `data/units.dat`.

## Transfer

Send these files to the calculator or CEmu:

1. `bin/clibs.8xg` (once, or when updating the CE libraries)
2. `bin/UNITDB.8xv`
3. `bin/UNITCALC.8xp`

Launch `prgmUNITCALC`. If the database is missing, malformed, or too large, the
program stays open and displays the database error.
