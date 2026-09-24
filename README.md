# units for TI-84 Plus CE

modeled after the amazing unit calculator, GNU Units!!

featuring...
![demo image](demo.png)

### Quick install
1. [Jailbreak](https://yvantt.github.io/arTIfiCE/) your calculator if its OS version >=5.5
2. use [TI Connect CE](https://education.ti.com/en/products/computer-software/ti-connect-ce-sw) to transfer the files in `quick-install/` to your calculator to install as a `prgm`.
3. To run, select `prgm`, `UNITCALC`. (2nd+off to quit)

### Usage
* Arrow keys to navigate cursor and copy/paste from history
* Press alpha to toggle between letters and numbers/operators, and 2nd for uppercase lettters
* 2nd+off to quit


### Workflow

* Install [CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/static/getting-started.html)
* Install CeMU emulator for easy testing (requires a ROM dump from your physical calculator)
* `make` to compile to `bin/`
* CEmu emulator

Use TI Connect CE to transfer:
* `UNITCALC.8xp` (in `bin/`)
* `UNITDB.8xv` (in `bin/`)
* `clibs.8xg` (downloaded from [here](https://tiny.cc/clibs))
