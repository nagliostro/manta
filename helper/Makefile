# Manta - Payloads for the console's ELF loader (port 9021).
# SPDX-License-Identifier: GPL-3.0-or-later
#
#   make -C helper PS5_PAYLOAD_SDK=<sdk>                       manta_helper.elf (ships in the app)
#   make -C helper PS5_PAYLOAD_SDK=<sdk> installer ZIP=<zip>   manta-installer.elf (for the release)
#
# The SDK is the hub build's copy: ~/emuland/hub/.deps/native/ps5-payload-sdk.

ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
include /opt/ps5-payload-sdk/toolchain/prospero.mk
endif

MINIZ := ../hub/src/third_party/miniz
VERSION := $(shell sed -n 's/^\#define MANTA_VERSION "\(.*\)"/\1/p' ../hub/src/emuland/model.hpp)
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror
MINIZ_CFLAGS := -std=c11 -O2 -w -I$(MINIZ)
MINIZ_SOURCES := $(MINIZ)/miniz.c $(MINIZ)/miniz_tinfl.c $(MINIZ)/miniz_tdef.c $(MINIZ)/miniz_zip.c

.PHONY: all installer clean

all: manta_helper.elf

manta_helper.elf: manta_helper.c payload.c payload.h
	$(CC) $(CFLAGS) -o $@ manta_helper.c payload.c

installer: manta-installer.elf

# The ZIP is built in with .incbin: the ELF is rebuilt whenever it changes.
manta-installer.elf: manta_installer.c payload.c payload.h $(ZIP)
	@test -n "$(ZIP)" || { echo "ZIP=<Manta release zip> is required"; exit 2; }
	mkdir -p build
	for source in $(MINIZ_SOURCES); do \
		$(CC) $(MINIZ_CFLAGS) -c -o build/$$(basename $$source .c).o $$source || exit 1; done
	$(CC) $(CFLAGS) -I$(MINIZ) -DMANTA_ZIP='"$(abspath $(ZIP))"' -DMANTA_VERSION='"$(VERSION)"' \
		-o $@ manta_installer.c payload.c build/miniz*.o

clean:
	rm -rf manta_helper.elf manta-installer.elf build
