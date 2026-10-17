# Emuland - Payloads for the console's ELF loader (port 9021).
# SPDX-License-Identifier: GPL-3.0-or-later
#
# make -C helper PS5_PAYLOAD_SDK=<sdk>   (the hub's copy: ~/emuland/hub/.deps/native/ps5-payload-sdk)

ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
include /opt/ps5-payload-sdk/toolchain/prospero.mk
endif

CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror

all: uninstall_helper.elf

uninstall_helper.elf: uninstall_helper.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f uninstall_helper.elf
