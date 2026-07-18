# Makefile for xf86-video-gma500blt
#
# Builds the DDX directly against the installed Xorg 1.20 SDK (video ABI 24)
# and libdrm on the target (Ubuntu 18.04, i686). Autotools is not required:
#
#   make                 # -> gma500blt_drv.so
#   sudo make install    # -> /usr/lib/xorg/modules/drivers/gma500blt_drv.so
#
# Requires: pkg-config, xorg-server SDK (xserver-xorg-dev), libdrm (libdrm-dev).

CC       ?= gcc
PKGCONFIG ?= pkg-config

# xorg-server.pc lays down /usr/include/xorg (SDK headers) and sdkdir.
XORG_CFLAGS := $(shell $(PKGCONFIG) --cflags xorg-server)
DRM_CFLAGS  := $(shell $(PKGCONFIG) --cflags libdrm)
DRM_LIBS    := $(shell $(PKGCONFIG) --libs libdrm)

WARNINGS := -Wall -Wextra -Wno-unused-parameter
CFLAGS   ?= -O2 -g
CFLAGS   += -std=gnu99 -fPIC -fvisibility=hidden $(WARNINGS) \
            $(XORG_CFLAGS) $(DRM_CFLAGS) -Isrc

LDFLAGS  += -shared
LIBS     := $(DRM_LIBS)

MODULE   := gma500blt_drv.so
SRCS     := src/gma500blt_driver.c \
            src/gma500blt_drmmode.c \
            src/gma500blt_exa.c \
            src/gma500blt_dri2.c
OBJS     := $(SRCS:.c=.o)

# Install location (video ABI 24 driver dir on Ubuntu 18.04 / i686).
MODULEDIR ?= /usr/lib/xorg/modules/drivers
DESTDIR   ?=

.PHONY: all clean install
all: $(MODULE)

$(MODULE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(MODULE)
	install -d $(DESTDIR)$(MODULEDIR)
	install -m 0755 $(MODULE) $(DESTDIR)$(MODULEDIR)/$(MODULE)

clean:
	rm -f $(OBJS) $(MODULE)

# header deps
$(OBJS): src/gma500blt.h src/gma500blt_drmmode.h
src/gma500blt_exa.o: src/gma_blt_drm.h
