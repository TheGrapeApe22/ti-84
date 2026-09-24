# units for TI-84 Plus CE

modeled after the amazing unit calculator, GNU Units!!

featuring...
![demo image](demo.png)

### Quick install
1. [Jailbreak](https://yvantt.github.io/arTIfiCE/) your calculator if its OS version >=5.5
2. use [TI Connect CE](https://education.ti.com/en/products/computer-software/ti-connect-ce-sw) to upload the files from `bin/` to your calculator.
3. To run, select `prgm`, `UNITCALC`.

### Usage (mostly self explanatory)
* Type unit expressions and press enter
* Arrows to navigate cursor. Press enter while selecting a command in history to paste it onto your cursor.
* Alpha to toggle between letters and numbers/operators, and 2nd for uppercase lettters
* 2nd+off to quit


### Workflow
* Install [CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/static/getting-started.html)
* Install CeMU emulator for easy testing (requires a ROM dump from your physical calculator)
* `make` to compile to `bin/`

Note: `bin/clibs.8xg` is a library downloaded from [here](https://tiny.cc/clibs)