#!/bin/sh
# compiz-enable.sh - enable a GL-composited (compiz) wobbly desktop on the
# GMA500/SGX535 with the closed emgd GL stack.  Run once (idempotent).
#
# Requires the emgd GLX/DRI2 stack already enabled (see glx-enable.sh): that
# provides the format-match un-hang and the texture_from_pixmap fix so compiz
# can composite our DDX's window pixmaps at all.
#
# THIS script fixes the last blocker -- "idle windows go black".  Root cause:
# compiz does damage-optimized PARTIAL rendering, but the closed emgd present
# (a kernel display-class swap, not reachable from userspace) shows partially-
# rendered frames, so anything not freshly damaged reads black.  The open
# libcomposite.so is patchable: CompositeScreen::damageRegion() sets
# damageMask |= DAMAGE_REGION_MASK (partial); we flip that to DAMAGE_ALL_MASK so
# every change triggers a FULL-screen repaint -> emgd never presents a partial
# frame.  It only repaints when something actually changes (idle stays idle), so
# the SGX535 is not overloaded (forcing continuous full repaint panic-reboots it).
# Trade-off: laggier during on-screen changes (full 1600x768 recomposite each).
#
# Patch (verified against compiz 0.9.13.1 / Ubuntu 18.04 i386 libcomposite.so):
#   CompositeScreen::damageRegion @0x12730 -> `or dword[edx+0x64],2` @file-off
#   0x127d5 (83 4A 64 02).  Flip imm8 02->04 (byte @0x127d8): REGION -> ALL.
set -e
LIB=$(ls /usr/lib/*/compiz/libcomposite.so 2>/dev/null | head -1)
[ -n "$LIB" ] || { echo "compiz-enable: libcomposite.so not found (is compiz installed?)"; exit 1; }

perl -e '
  my ($f)=@ARGV;
  my $off=0x127d8;                    # imm8 of `or [edx+0x64], N` in damageRegion
  open(F,"+<",$f) or die "open $f: $!"; binmode F;
  # sanity: verify the enclosing instruction bytes 83 4A 64 0x at 0x127d5
  seek(F,0x127d5,0); my $ins; read(F,$ins,3);
  if ($ins ne "\x83\x4a\x64") { warn "compiz-enable: damageRegion signature not found; skipping\n"; exit 0; }
  seek(F,$off,0); my $b; read(F,$b,1);
  if    ($b eq "\x02") { seek(F,$off,0); print F "\x04"; print "compiz-enable: damageRegion -> DAMAGE_ALL (patched)\n"; }
  elsif ($b eq "\x04") { print "compiz-enable: already patched\n"; }
  else  { warn sprintf("compiz-enable: unexpected imm 0x%02x at %#x; skipping\n", ord($b), $off); }
  close F;
' "$LIB"

echo "compiz-enable: done. Launch compiz with: compiz --replace core composite opengl decor wobbly move resize place"
