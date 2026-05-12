# Kernel Driver Design

## Starting Point

Upstream `gma500` is the right display base. Replacing it would be wasted effort.

The practical plan is to extend or fork the `gma500` kernel path to grow a real render interface for SGX535.

## What Upstream Already Gives Us

- PCI probe and device identification
- display/KMS support
- GEM object support
- GTT and MMU-related infrastructure
- register definitions for SGX, BIF, and 2D blocks
- interrupt plumbing and power hooks

## What Is Missing

- a stable userspace render submission uAPI
- command-buffer validation rules
- sync objects or at least fence/event semantics
- hang detection and reset policy for render jobs
- devcoredump-style failure capture
- buffer-sharing contract for Mesa scanout and textures

## Driver Shape

Keep one DRM device for now.

Reasons:

- display and render live behind the same PCI function
- existing `gma500` memory-management code already knows about the device layout
- early bring-up gets simpler if KMS, GEM, and SGX submission share one kernel driver

If render/display separation becomes useful later, it can be revisited after a working open stack exists.

## Proposed Incremental Kernel Work

### Step 1: Observability

Add:

- debugfs register dump helpers
- SGX/BIF fault decode helpers
- structured tracepoints for submit, IRQ, reset, and fault
- optional devcoredump capture for SGX state

### Step 2: Reset Safety

Add:

- explicit SGX idle detection
- reset sequencing for soft reset and BIF fault clear
- post-reset reinitialization of required base registers

### Step 3: Minimal Submission Path

Add a narrow ioctl for an internal test client first:

- submit one command buffer
- optional wait or fence return
- strict limitations on buffer count and job shape

Do not start with a general-purpose userspace ABI. Start with the smallest workable ABI and grow it only after traces justify it.

### Step 4: Memory Contract

Define:

- buffer flags for command, shader, texture, and scanout usage
- mapping rules
- cache or flush behavior
- page-table expectations

### Step 5: Production ABI

Only after test jobs work:

- expose submit objects
- expose sync or fence objects
- document fault semantics

## First Kernel Bring-Up Targets

1. Read status and fault registers from a test module path.
2. Confirm reset sequence leaves the engine alive.
3. Submit a no-op or trivially terminating job.
4. Receive completion or deterministically time out.
5. Trigger and decode a deliberate fault.

## UAPI Sketch

This is intentionally small:

- `PSB_SGX_GET_PARAM`
- `PSB_SGX_CREATE_BO`
- `PSB_SGX_SUBMIT`
- `PSB_SGX_WAIT`

The names are placeholders. The important point is to keep the first ABI narrow enough that it can be replaced before upstreaming if the RE model changes.

## Upstreaming Strategy

Do not aim upstream immediately.

Use three stages:

1. experimental branch for instrumentation and unsafe assumptions
2. private or project branch with stabilized ABI and cleanup
3. upstream-ready split patch series after Mesa proves the model
