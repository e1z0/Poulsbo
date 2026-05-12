# Hardware Notes

## Known Platform Facts

- Intel GMA 500 / Poulsbo corresponds to SGX535-class render hardware paired with Intel display hardware.
- Upstream `gma500` still recognizes Poulsbo PCI IDs `8086:8108` and `8086:8109`.
- The kernel source identifies Poulsbo and related variants directly in `psb_drv.c`.
- Upstream `gma500` exposes modesetting and GEM, but not a modern render interface.

## Key Register Blocks Seen In Upstream Source

The upstream `psb_reg.h` file already exposes a useful starting taxonomy.

### Core Reset And Eventing

- `PSB_CR_SOFT_RESET` at `0x0080`
- `PSB_CR_EVENT_HOST_ENABLE` at `0x0130`
- `PSB_CR_EVENT_HOST_CLEAR` at `0x0134`
- `PSB_CR_EVENT_STATUS` at `0x012C`
- `PSB_CR_EVENT_KICKER` at `0x0AC4`
- `PSB_CR_EVENT_KICK` at `0x0AC8`

### PDS And USE

- `PSB_CR_USE_CODE_BASE0..15` starting at `0x0A0C`
- `PSB_CR_PDS_EXEC_BASE` at `0x0AB8`

### BIF And MMU

- `PSB_CR_BIF_CTRL` at `0x0C00`
- `PSB_CR_BIF_INT_STAT` at `0x0C04`
- `PSB_CR_BIF_FAULT` at `0x0C08`
- `PSB_CR_BIF_BANK0` at `0x0C78`
- `PSB_CR_BIF_BANK1` at `0x0C7C`
- `PSB_CR_BIF_DIR_LIST_BASE0` at `0x0C84`
- `PSB_CR_BIF_TWOD_REQ_BASE` at `0x0C88`
- `PSB_CR_BIF_3D_REQ_BASE` at `0x0CAC`

### 2D Block

- `PSB_CR_2D_BLIT_STATUS` at `0x0E04`
- `PSB_CR_2D_SOCIF` at `0x0E18`

## Known Event Bits Worth Tracking

From upstream source, the event block already names several interesting conditions:

- 2D complete
- TA finished
- pixel backend end render
- several DPM out-of-memory conditions
- BIF requester fault

These are enough to build a first fault and completion decoder.

## Implication For RE Work

We do not start from zero:

- register names already exist
- reset sequence hints already exist
- MMU and BIF plumbing already exists in upstream code

The missing information is how the proprietary stack programs those pieces for real SGX rendering.
