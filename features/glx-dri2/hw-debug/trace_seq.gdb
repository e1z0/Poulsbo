set pagination off
set height 0
set breakpoint pending on
break PVRDRIInitialiseBuffer
commands
  silent
  printf "[seq] PVRDRIInitialiseBuffer\n"
  continue
end
break KEGLCreateRenderSurface
commands
  silent
  printf "[seq] >>> KEGLCreateRenderSurface (creates surface)\n"
  continue
end
break KEGLSurfaceBind
commands
  silent
  printf "[seq] >>> KEGLSurfaceBind (binds surface to ctx)\n"
  continue
end
break PVRDRIMakeCurrentGC
commands
  silent
  printf "[seq] >>> PVRDRIMakeCurrentGC\n"
  continue
end
break PVRDRIRecreateEGLDrawable
commands
  silent
  printf "[seq] PVRDRIRecreateEGLDrawable\n"
  continue
end
run
printf "\n### finished, no crash ###\n"
