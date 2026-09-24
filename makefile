# ----------------------------
# CE C Toolchain options
# ----------------------------

NAME = UNITS
DESCRIPTION = "Unit calculator engine"
COMPRESSED = NO
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Werror -Oz

# ----------------------------

include $(shell cedev-config --makefile)

.PHONY: database

build: database

database: data/units.dat
	@mkdir -p bin
	@convbin -j bin -k 8xv -n UNITDB -r -i $< -o bin/UNITDB.8xv
	@echo "[database] bin/UNITDB.8xv"
