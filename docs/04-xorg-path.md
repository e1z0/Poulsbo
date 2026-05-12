# Xorg Path

## The Right Xorg Strategy

Use the generic Xorg `modesetting` driver with `glamor`.

That is the correct path once Mesa acceleration exists, because `glamor` translates X rendering onto OpenGL instead of requiring a large chip-specific 2D DDX.

## Requirements

To get accelerated Xorg on Poulsbo, we need:

- working DRM/KMS scanout from `gma500`
- working Mesa driver exposing enough GL or GLES for `glamor`
- stable buffer allocation and page-flip behavior

## Bring-Up Sequence

1. Boot with upstream-style `gma500` KMS.
2. Confirm render acceleration in a direct Mesa test app.
3. Start Xorg with `modesetting`.
4. Verify `glamor` loads instead of software fallback.
5. Measure basic desktop operations before attempting compositors.

## Minimal Xorg Configuration

Example starting point:

```conf
Section "Device"
    Identifier "Poulsbo"
    Driver "modesetting"
    Option "AccelMethod" "glamor"
EndSection
```

Keep configuration minimal until the render path is stable.

## Why Not Write A Poulsbo DDX First

Because that duplicates work in the wrong layer.

A custom DDX would still leave:

- no accelerated GL stack
- no reusable path for compositors
- no modern Mesa integration

If a custom X extension path becomes necessary later for a missing overlay or video feature, that can be added after the core stack works.
