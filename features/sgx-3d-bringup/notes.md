# SGX535 3D bring-up — first-cut integration + hardware procedure

Target tree: `/Projects/Poulsbo/build/devnull-vaio-gma500-src` (root-owned).
Files here: `sgx_fw.h`, `sgx_3d.c`, `mk_fw_header.py` (+ generated `sgx_fw_blob.h`).

This is **deploy-and-test-later** scaffold. The Vaio is down; nothing has run on
hardware. Optimise for safe, observable, incremental bring-up — expect the first
several boots to time out. The design is built so a timeout is recoverable
(reset + bail to 2D-only), never fatal.

---

## 1. What this maps to in the driver

| Design piece | Driver primitive used | Location |
|---|---|---|
| Render MMU context | `dev_priv->pf_pd` (ctx1, already allocated) | psb_drv.c:26728, mmu.c:208 |
| Program PD into BIF | `psb_mmu_set_pd_context(pf_pd,1)` → BIF_DIR_LIST_BASE1 | mmu.c:168 |
| Map BO at a VA | `psb_mmu_insert_pages(pf_pd, pages, va, npage,…)` | mmu.c:745 |
| Allocate GPU BO | `psb_gtt_alloc_range(dev,len,name,0,align)` | gtt.c:443 |
| Pin pages | `psb_gtt_pin_locked` | gtt.c:253 |
| Flush WC/UC pages | `psb_mmu_flush(dev_priv->mmu)` | mmu.c:141 |
| Power window | `gma_power_begin(dev,true)/gma_power_end` | power.c:249/288 |
| Doorbell | `PSB_WSGX32(_PSB_CE_KICK_NOW, PSB_CR_EVENT_KICK)` | psb_reg.h:123/124 |
| CORE_REVISION check | `PSB_RSGX32(PSB_CR_CORE_REVISION)` | psb_reg.h:37 |

The whole path is gated by `sgx3d_enable` (default 0). With it off the driver
boots exactly as today. Init is **never** run at load — only via the sysfs
trigger `/sys/bus/pci/devices/<BDF>/poulsbo_sgx_3d`.

---

## 2. Integration steps (edits to the target tree)

### 2a. Add the two source files to the build
Add `sgx_3d.c` to the psb/gma500 `Makefile` object list. Drop `sgx_fw.h` next to
`psb_reg.h`. (If you prefer embedded firmware over `request_firmware`, generate
`sgx_fw_blob.h` with `mk_fw_header.py --emit-blob-header` and `#include` it — but
the default path is `request_firmware`, see 2d.)

### 2b. Expose the one GTT helper `sgx_3d.c` depends on
`psb_gtt_pin_locked` is `static`. Add this small exported helper to `gtt.c`
(mirrors `psb_gtt_lab_bind_pf_locked` but honours an arbitrary GPU VA so the
ukernel code can live at `sgx3d_code_base` instead of `gatt_start+offset`):

```c
/* gtt.c — new, exported. Prototype in gtt.h. */
int psb_gtt_sgx3d_bind(struct drm_device *dev, struct gtt_range *gt,
		       unsigned long gpu_va)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	int ret;

	if (gt->stolen || !dev_priv->pf_pd)
		return -EINVAL;

	mutex_lock(&dev_priv->gtt_mutex);
	ret = psb_gtt_pin_locked(gt);		/* attaches pages + GTT */
	if (ret)
		goto out;
	ret = psb_mmu_insert_pages(dev_priv->pf_pd, gt->pages, gpu_va,
				   gt->npage, 0, 0, PSB_MMU_CACHED_MEMORY);
	if (ret)
		psb_gtt_unpin_locked(gt, false);
out:
	mutex_unlock(&dev_priv->gtt_mutex);
	return ret;
}

void psb_gtt_sgx3d_unbind(struct drm_device *dev, struct gtt_range *gt,
			  unsigned long gpu_va)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	mutex_lock(&dev_priv->gtt_mutex);
	psb_mmu_remove_pages(dev_priv->pf_pd, gpu_va, gt->npage, 0, 0);
	psb_gtt_unpin_locked(gt, false);
	mutex_unlock(&dev_priv->gtt_mutex);
}
```
Add prototypes to `gtt.h`. (`psb_mmu_remove_pages` is already exported, mmu.h:104.)

### 2c. Register the sysfs trigger
In `psb_driver_load` (near the other `device_create_file` calls, psb_drv.c:26778+):
```c
    sgx3d_sysfs_register(dev);           /* ignore/So warn on error */
```
In `psb_driver_unload` (with the other `device_remove_file`, psb_drv.c:26545+):
```c
    sgx3d_sysfs_unregister(dev);
    sgx3d_fini(dev);
```
Declare `sgx3d_sysfs_register/unregister/init/fini` in a small `sgx_3d.h`
(or extern in psb_drv.c).

### 2d. Install firmware
```
sudo mkdir -p /lib/firmware/sgx535
sudo cp /Projects/Poulsbo/firmware/sgx535-emgd-1.18/pbuKernelProgram.bin \
        /Projects/Poulsbo/firmware/sgx535-emgd-1.18/pbuKernelTAProgram.bin \
        /Projects/Poulsbo/firmware/sgx535-emgd-1.18/pbuKernel3DProgram.bin \
        /lib/firmware/sgx535/
```
Validate first: `python3 mk_fw_header.py` (sizes/sha must be OK).

---

## 3. The milestones as coded

- **M3** `sgx3d_setup_buffers` — alloc + upload main/TA/3D code (contiguous in the
  code heap at `sgx3d_code_base`), plus host-ctl / kernel-CCB / kicker BOs, plus
  the reset PD/PT scratch pages. Verifiable by readback with no kick.
- **M4** `sgx3d_reset` (bounded SGXReset) → `sgx3d_boot` (USE_CODE_BASE(15),
  host-ctl init + service-addr patch, one EVENT_KICK, bounded poll for
  `INIT_COMPLETE`).
- **M5** (NOT in this file yet) — the first TA kick at
  `poulsbo_sgx_apply_submit_sgx_ta_commit_locked` (psb_drv.c:5910) and the
  SW_EVENT/TA_FINISHED IRQ latch in `psb_sgx_interrupt` (psb_irq.c:471) +
  enable in `psb_irq_postinstall` (OR `_PSB_CE_SW_EVENT|_PSB_CE_TA_FINISHED`
  into `PSB_SGX_LAB_EVENT_MASK`, psb_irq.c:22/616). Left as the next increment;
  do it only after M4 reaches INIT_COMPLETE on hardware.

M5 wire-up sketch (for when M4 passes) — inside the commit-locked seam, right
after the queue-header write-offset advance:
```c
    wmb();
    psb_mmu_flush(dev_priv->mmu);
    /* increment device kicker dword, then: */
    PSB_WSGX32(_PSB_CE_KICK_NOW, PSB_CR_EVENT_KICK);
    /* release the submit spinlock BEFORE waiting; IRQ latches completion */
    /* wait_event_timeout(...) with a jiffies deadline; on timeout: reset+bail */
```

---

## 4. Safety properties (verify these hold before every deploy)

1. `sgx3d_enable=0` ⇒ no SGX 3D MMIO at all. Bad build still boots 2D.
2. Never at load — only the sysfs `init` trigger runs it.
3. Every wait bounded: reset drain `SGX_RESET_DRAIN_TRIES×50us`; init poll
   `SGX_POLL_TRIES(10000)×50us ≈ 500ms`. No bare loops.
4. On any timeout: exactly one `sgx3d_reset`, then bail `-EIO`, 2D stays alive.
5. Two consecutive failed boots ⇒ `latched_off = true` for the module lifetime.
6. All MMIO inside `gma_power_begin(true)…gma_power_end`.
7. Reset PD/PT scratch allocated before the first reset (fault-drain can't deref
   NULL).
8. `wmb()+psb_mmu_flush` before the doorbell.

---

## 5. What is GUESSED (must be confirmed on hardware)

Search `sgx_3d.c`/`sgx_fw.h` for "GUESSED". The load-bearing ones:

- **`sgx3d_code_base` / USE_CODE_BASE(15) encoding** (open q c/d). We map main
  code at `0xF8000000` and write that raw value to USE_CODE_BASE(15). The real
  register uses a 25-bit ADDR field + DM select; if the encoding is wrong the
  USSE fetches garbage and the boot times out. Module param, tune live.
- **`sgx3d_initstatus_off`** (open q a). 0 vs +8 depending on
  `PVRSRV_USSE_EDM_BREAKPOINTS`. Wrong ⇒ poll the wrong dword forever (but still
  bounded). Module param.
- **host-ctl service-addr struct offsets** `0x2c/0x30/0x34/0x38/0x3c`
  (`sgx3d_patch_service_addrs`, open q b). Template layout only partially
  recovered. These are the first thing to fix after the code-base encoding.
- **reset PD PDE format** — the transient reset PDE `= pt_phys | 1` is a
  placeholder; only needs to be a walkable non-NULL page for the drain, but the
  VALID-bit / align encoding may need the real MMU format.
- **reset settle `udelay(100)`** and drain iteration counts — timing guesses,
  tune against real BIF behaviour.
- **EUR_CR_POWER** is intentionally NOT written (undefined for SGX535 in
  sgx535defs.h; the DDK guards it `#if defined`). Design P4's value-0 is a no-op
  on this core.
- **`PSB_CR_EVENT_KICKER` pre-poke** before the doorbell — harmless if wrong.
- Whether the EMGD 1.18 blobs are microcode-compatible with rev 1.2.1 — only a
  boot proves it (open q f).

---

## 6. Hardware bring-up procedure (once the Vaio is back)

1. **Build** in the docker/kbuild env against the target kernel; install the
   module + firmware (section 2d).
2. **Fresh device / blacklist method**: blacklist the in-tree gma500, load the
   patched module against a fresh device bind so you control probe. Confirm 2D +
   display still work (safety property 1) *before* touching 3D.
3. **M3 first, no kick**: `modprobe … sgx3d_enable=1`. Do NOT trigger init yet;
   instead confirm the module loaded and 2D is healthy.
4. **Trigger init on demand**:
   ```
   echo init > /sys/bus/pci/devices/<BDF>/poulsbo_sgx_3d
   dmesg | tail -40
   cat  /sys/bus/pci/devices/<BDF>/poulsbo_sgx_3d
   ```
   Look for: `uploaded … at gpu_va`, `SGXReset done`, and either
   `INIT_COMPLETE (status=…)` or `INIT_COMPLETE timeout`.
5. **On timeout** (expected early): the driver already did one reset + bailed;
   the box stays alive. Iterate the guessed knobs without rebuilding:
   ```
   echo 0x0E400000 > /sys/module/<mod>/parameters/sgx3d_code_base   # try multi-ctx base
   echo 8          > /sys/module/<mod>/parameters/sgx3d_initstatus_off
   echo reset      > /sys/…/poulsbo_sgx_3d
   echo init       > /sys/…/poulsbo_sgx_3d
   ```
   (After two consecutive fails 3D latches off; `rmmod`/`modprobe` to reset the
   latch, or add an `off` then reload.)
6. **Once INIT_COMPLETE lands**, freeze `sgx3d_code_base` + `sgx3d_initstatus_off`
   as the confirmed constants, then start M5 (the TA kick + IRQ latch).

Watch `dmesg` for BIF faults (psb_sgx_interrupt already logs them) — a fault
right after the kick means the USE code-base encoding or a service-addr offset is
wrong, not that the box is wedged.
