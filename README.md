# Unit Calculator for the TI-84 Plus CE

This is the first, input/output-only stage of a native unit calculator. It lets
you enter a message, prints `hello [message]`, and recalls earlier input lines.
History exists until you quit the program; persistent history can be added
later.

## Controls

- Letter keys: type the lowercase form of the green letter printed above each key.
- `2ND`: toggle between `(abc)` and one-shot `(ABC)` mode. After one capital
  letter, input returns to lowercase.
- `ALPHA`: toggle between `(abc)` and `(123)` input modes. The current mode appears
  in the top-right corner.
- `0` in `(abc)` mode: type a space.
- `DEL`: delete the final input character.
- `ENTER`: submit the current input.
- `UP` / `DOWN`: select a previous input. Press `ENTER` to append the selected
  text to the current input. Pressing `DOWN` past the newest item cancels the
  selection.
- `CLEAR`: clear the current input.
- `2ND`, then `ON` (`OFF`): exit. This works only while `(ABC)` is shown.

Input is limited to 30 characters and the latest 12 entries are retained.

## Build

Install the [CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/)
for your operating system, open a terminal in this directory, and run:

```sh
make
```

The calculator program will be generated as `bin/UNITCALC.8xp`.

## Put it on the calculator

Use TI Connect CE to transfer both:

1. `bin/UNITCALC.8xp`
2. The CE toolchain libraries (`clibs.8xg`), unless they are already installed
   on the calculator.

Then run `prgmUNITCALC` from the calculator's program menu. On newer calculator
OS versions, a shell such as arTIfiCE may be required to launch assembly/native
programs.
