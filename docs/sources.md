# Sources

These are the main public references used to frame the project.

## Kernel

- Linux `gma500` source tree index:
  - https://codebrowser.dev/linux/linux/drivers/gpu/drm/gma500/
- `psb_drv.c`:
  - https://codebrowser.dev/linux/linux/drivers/gpu/drm/gma500/psb_drv.c.html
- `psb_reg.h`:
  - https://codebrowser.dev/linux/linux/drivers/gpu/drm/gma500/psb_reg.h.html
- Linux maintainer entry for `gma500`:
  - https://docs.kernel.org/6.6/process/maintainers.html
- Linux `i915` documentation note excluding SGX-based Intel SoCs:
  - https://docs.kernel.org/6.3/gpu/i915.html

## Xorg

- Xorg `modesetting` manual:
  - https://cgit.freedesktop.org/xorg/xserver/tree/hw/xfree86/drivers/modesetting/modesetting.man
- X.org acceleration notes on `glamor`:
  - https://www.x.org/wiki/Development/Documentation/Performance/

## Mesa And RE Precedent

- Mesa Lima documentation:
  - https://docs.mesa3d.org/drivers/lima.html
- Mesa Freedreno documentation:
  - https://docs.mesa3d.org/drivers/freedreno.html
- Mesa PowerVR documentation for current upstream Rogue support:
  - https://docs.mesa3d.org/drivers/powervr.html

## Recent State

- Phoronix note on March 5, 2026 `gma500` activity:
  - https://www.phoronix.com/news/Intel-GMA500-Driver-In-2026
- Related mailing-list thread:
  - https://www.spinics.net/lists/dri-devel/msg540122.html

## Public SGX Context

- TI OMAP35x public manual references with SGX chapter pointers:
  - https://www.ti.com/lit/ug/spruf98q/spruf98q.pdf
- PowerVR SDK browser showing Series5-era GLES 2.0 focus:
  - https://powervr-graphics.github.io/Native_SDK/Documentation/SDKBrowser/Examples.html

## Additional Historical Leads

- Yocto EMGD patchset carrying Intel-oriented `pvr/services4` SGX trees:
  - https://docs.yoctoproject.org/pipermail/poky/2011-May/006149.html
- Yocto CDV patchset carrying `drivers/staging/cdv/pvr/services4` SGX trees:
  - https://www.mail-archive.com/yocto%40yoctoproject.org/msg05861.html
- OpenPVRSGX kernel-driver effort:
  - https://github.com/openpvrsgx-devgroup/linux_openpvrsgx
- OpenPVRSGX discussion on SGX vs Rogue and matching userspace/kernel stacks:
  - https://lists.goldelico.com/pipermail/openpvrsgx-devgroup/2023-June/000556.html
- TI SGX DDK kernel tree:
  - https://git.ti.com/cgit/graphics/omap5-sgx-ddk-linux/
- TI SGX build guide documenting the open-kernel / closed-userspace split:
  - https://software-dl.ti.com/processor-sdk-linux/esd/AM437X/09_03_05_02/exports/docs/linux/Foundational_Components/Graphics/SGX/Build_Guide.html
- EMGD community archive:
  - https://github.com/EMGD-Community/intel-binaries-linux
- Imagination forum thread pointing to the TI SGX sources:
  - https://forums.imgtec.com/t/request-for-powervr-sgx-535-linux-driver-source-code/4136

## Lower-Value Context

- Windows-era GMA500 overclock blog:
  - https://gma500booster.blogspot.com/2013/06/internal-gma-500-vs-gma-600-overclock.html
- Reddit thread about Rogue ISA docs:
  - https://www.reddit.com/r/linux/comments/2jvwrm/powervr_sdk_v34_released_with_full_isa/
