# One-shot diagnostic for the glXMakeCurrent crash in emgd_dri.so.
# Goal: capture, in a single (fragile-box) session, every runtime value the
# static analysis could not resolve, to decide the real fix.
#
# Symbol-relative so it survives load-base changes; run with randomize_va_space=0.
set pagination off
set breakpoint pending on
set $base = (unsigned)&KEGLGetDrawableParameters - 0x1eb40

# --- geometry query 0x1c3b0: what buffer does emgd map, and what does it read? ---
break *($base + 0x1c40c)     
# just before PVR2DMemMap; $edi = the buffer handle it maps
commands
  printf "\n[1c3b0] mapping DRI2 buffer handle=%#x\n", $edi
  finish
  printf "[1c3b0] PVR2DMemMap ret=%d\n", $eax
end
break *($base + 0x1c418)      
# after PVR2DMemMap: -0x1c(ebp) = &meminfo
commands
  set $mi = *(unsigned*)($ebp-0x1c)
  set $pb = *(unsigned*)$mi
  printf "[1c3b0] meminfo=%#x pBase=%#x  *(pBase)=%#x  *(pBase+0x10)=%#x\n", $mi, $pb, *(unsigned*)$pb, *(unsigned*)($pb+0x10)
  continue
end

# --- KEGLGetDrawableParameters: dump the drawable/surface state each call ---
break KEGLGetDrawableParameters
commands
  set $drw = *(unsigned*)($esp+4)
  set $X = *(unsigned*)($drw+0x1c)
  printf "\n[KEGLGetDP] arg1=%#x arg3=%#x  X(=arg1->0x1c)=%#x\n", $drw, *(unsigned*)($esp+0xc), $X
  printf "   X->0x70(state)=%#x  X->0x34=%#x  X->0x3c=%#x  X->0x38=%#x  X->0x4=%#x\n", *(unsigned*)($X+0x70), *(unsigned*)($X+0x34), *(unsigned*)($X+0x3c), *(unsigned*)($X+0x38), *(unsigned*)($X+4)
  set $x4 = *(unsigned*)($X+4)
  printf "   (X->0x4)->0x24=%#x  ->0x28=%#x   X->0x24[0..2]=%#x %#x %#x\n", *(unsigned*)($x4+0x24), *(unsigned*)($x4+0x28), *(unsigned*)($X+0x24), *(unsigned*)($X+0x28), *(unsigned*)($X+0x2c)
  continue
end

# --- does the surface-creating path run at all before the crash? ---
break PVRDRIMakeCurrentGC
commands
  printf "\n[MakeCurrentGC] REACHED (surface-create) ctx=%#x draw=%#x read=%#x\n", *(unsigned*)($esp+4), *(unsigned*)($esp+8), *(unsigned*)($esp+0xc)
  continue
end

run
printf "\n=== if we get here without SIGSEGV, MakeCurrent progressed ===\n"
bt 6
