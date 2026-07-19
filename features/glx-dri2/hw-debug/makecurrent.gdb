set pagination off
set height 0
set breakpoint pending on
break PVRDRIInitialiseBuffer
run
set $base = (unsigned)&PVRDRIInitialiseBuffer - 0x1da60
printf "\n### base=%#x ###\n", $base
delete 1

# geometry query — report handle + meminfo/pBase (no deref; pBase may be WC/unreadable by gdb)
break *($base + 0x1c418)
commands
  silent
  set $mi = *(unsigned*)($ebp-0x1c)
  set $pb = *(unsigned*)$mi
  printf "\n[geom] meminfo=%#x pBase=%#x\n", $mi, $pb
  continue
end

# THE crash site state
break KEGLGetDrawableParameters
commands
  silent
  set $drw = *(unsigned*)($ebp+8)
  set $X = *(unsigned*)($drw+0x1c)
  set $x4 = *(unsigned*)($X+4)
  printf "\n[KEGLGetDP] arg3=%#x X=%#x | 0x70=%#x 0x34=%#x 0x3c=%#x 0x38=%#x | (X4)->24=%#x 28=%#x | X24[0..2]=%#x %#x %#x\n", *(unsigned*)($ebp+0x10), $X, *(unsigned*)($X+0x70), *(unsigned*)($X+0x34), *(unsigned*)($X+0x3c), *(unsigned*)($X+0x38), *(unsigned*)($x4+0x24), *(unsigned*)($x4+0x28), *(unsigned*)($X+0x24), *(unsigned*)($X+0x28), *(unsigned*)($X+0x2c)
  continue
end

break PVRDRIMakeCurrentGC
commands
  silent
  printf "\n[MakeCurrentGC] REACHED (surface create)\n"
  continue
end

continue
printf "\n### glx_probe finished — NO SIGSEGV ###\n"
