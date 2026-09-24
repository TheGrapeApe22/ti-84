# ----------------------------
# CE C Toolchain options
# ----------------------------

NAME = UNITCALC
DESCRIPTION = "Unit calculator console"
COMPRESSED = NO
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Werror -Oz

# ----------------------------

include $(shell cedev-config --makefile)
