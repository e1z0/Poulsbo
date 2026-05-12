# Mesa Driver Design

## Driver Family Choice

Use Gallium.

Reasons:

- modern Mesa infrastructure expects Gallium or Vulkan-first paths
- Gallium gives a realistic path to GLES 2.0 and some desktop GL 2.x
- shader lowering, state tracking, and test coverage are much better than reviving a classic DRI driver

## API Target

Initial target:

- GLES 2.0

Secondary target:

- desktop OpenGL 2.x where hardware limits and Mesa lowering make it practical

Out of scope for first bring-up:

- Vulkan
- GLES 3.x
- modern desktop GL

## Architecture

The Mesa side will need:

- Gallium screen implementation
- Gallium context implementation
- winsys talking to the `gma500` render uAPI
- resource and transfer handling
- shader compiler backend for SGX535
- format table and capability table

## Shader Plan

Assume a NIR-based pipeline feeding a custom backend.

Required outputs:

- translated SGX535 shader binaries
- relocation handling
- uniform and varying layout control

Do not try to outsmart the backend early. First target codegen for:

1. pass-through vertex shader
2. flat color fragment shader
3. basic textured fragment shader

## Winsys Contract

The winsys must cover:

- BO allocation
- BO map and unmap
- BO import or export if scanout sharing requires it
- submit with relocations
- wait and fence handling

## Bring-Up Ladder

### G0

- `eglInitialize`
- context creation
- query basic caps

### G1

- clear color

### G2

- triangle with fixed vertex layout

### G3

- textures and sampler state

### G4

- FBO render and readback

### G5

- dEQP GLES2 smoke pass set

### G6

- enough stability for Xorg `glamor`

## Validation

Test in this order:

1. tiny internal demos
2. piglit subset
3. dEQP GLES2 subset
4. `kmscube`-style sample equivalent for the driver
5. Xorg with `glamor`

## Naming

Do not commit to a final Mesa driver name until the kernel ABI is stable. For docs, `psb` is the least confusing placeholder.
