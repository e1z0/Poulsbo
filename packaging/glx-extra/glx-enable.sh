#!/bin/sh
# glx-enable.sh - finish enabling HW-accelerated OpenGL/GLX/DRI2 (emgd) on top of
# the base poulsbo-sgx userspace.  Run from the userspace package postinst; safe
# to re-run.  The pieces here are the ones that live OUTSIDE /opt/emgd (system
# Mesa paths) or need a binary tweak, so they are applied at install time.
set -e
SHARE=/opt/poulsbo-sgx-glx        # payload staged by the package

# 1. Put the glapi-linked emgd DRI driver on the Mesa loader search path.
if [ -f "$SHARE/emgd_dri.so" ]; then
    install -D -m0644 "$SHARE/emgd_dri.so" \
        /usr/lib/i386-linux-gnu/dri/emgd_dri.so
fi

# 1b. Compiz/GLX-compositing enable: PVRDRICreateContext (@0x1d440) rejects any
#     GLX context whose config format doesn't match the screen's ("Window format
#     doesn't match that of screen") and then hard-hangs the box -- which is what
#     made every compiz start wedge the machine.  glxgears etc. match and are
#     unaffected; only mismatched-format compositor contexts hit it.  Bypass the
#     four format-match branches by offset (verified against the shipped binary):
#       0x1d470  je  0x1d4a0  (74 2e) -> jmp 0x1d4a0 (eb 2e)
#       0x1d4a6  jne err      (75 ca) -> nop nop     (90 90)
#       0x1d4ae  jne err      (75 c2) -> nop nop     (90 90)
#       0x1d4b6  jne err      (75 ba) -> nop nop     (90 90)
#     Idempotent: only patches bytes still at their original values.
DRI=/usr/lib/i386-linux-gnu/dri/emgd_dri.so
if [ -f "$DRI" ] && command -v perl >/dev/null 2>&1; then
    perl -e '
      my ($f)=@ARGV;
      my @p=([0x1d470,"\x74\x2e","\xeb\x2e"],
             [0x1d4a6,"\x75\xca","\x90\x90"],
             [0x1d4ae,"\x75\xc2","\x90\x90"],
             [0x1d4b6,"\x75\xba","\x90\x90"]);
      open(F,"+<",$f) or die "open $f: $!"; binmode F;
      for my $x (@p) {
          seek(F,$x->[0],0); my $b; read(F,$b,2);
          if ($b eq $x->[1]) { seek(F,$x->[0],0); print F $x->[2]; }
          elsif ($b ne $x->[2]) {
              warn sprintf("emgd_dri compiz-patch: unexpected byte at %#x\n",$x->[0]);
          }
      }
      close F;' "$DRI" || true
fi

# 2. Tell the Mesa GLX/DRI2 loader to use the emgd driver (PCI-id resolution
#    otherwise picks the in-kernel gma500 name and falls back to llvmpipe).
if ! grep -q '^MESA_LOADER_DRIVER_OVERRIDE=' /etc/environment 2>/dev/null; then
    echo 'MESA_LOADER_DRIVER_OVERRIDE=emgd' >> /etc/environment
fi

# 3. Work around a Mesa-20 libGLX_mesa NULL-deref: it calls an optional
#    DRI2_Config_Query screen hook unconditionally, which emgd does not provide.
#    Neutralise the two unguarded `call *0x8(%eax)` (ff 50 08) inside the
#    `ff 75 30 ff 50 08 83 c4 10` sequence -> `xor %eax,%eax; nop` (31 c0 90).
LIBGLX=/usr/lib/i386-linux-gnu/libGLX_mesa.so.0
if [ -f "$LIBGLX" ] && [ ! -f "$LIBGLX.orig" ] && command -v perl >/dev/null 2>&1; then
    perl -0777 -i.orig -pe \
      's/\xff\x75\x30\xff\x50\x08\x83\xc4\x10/\xff\x75\x30\x31\xc0\x90\x83\xc4\x10/g' \
      "$LIBGLX" || true
fi

# 4. Install the AIGLX/DRI2 X config and make it the default so `startx` /
#    the display manager brings up the accelerated gma500blt DDX.
if [ -f "$SHARE/xorg.conf.glx" ]; then
    install -D -m0644 "$SHARE/xorg.conf.glx" /etc/X11/xorg.conf.glx
    [ -f /etc/X11/xorg.conf ] || cp "$SHARE/xorg.conf.glx" /etc/X11/xorg.conf
fi

echo "poulsbo-sgx: HW OpenGL/GLX/DRI2 enabled (reboot or restart X to use it)."
