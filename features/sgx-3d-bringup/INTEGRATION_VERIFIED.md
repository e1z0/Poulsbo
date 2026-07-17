# 3D bring-up scaffold — compile-verified 2026-07-16

sgx_3d.c COMPILES CLEAN against the Vaio kernel (5.4.0-150-generic i686) when
integrated into the gma500 tree via docker/kbuild. gma500_gfx.ko links with
sgx_3d.o (521892 bytes). Closes the "never compiled" caveat.

Integration applied for the compile (see notes.md §2):
  - Makefile: added `sgx_3d.o` to gma500_gfx-y.
  - gtt.c: appended psb_gtt_sgx3d_bind()/psb_gtt_sgx3d_unbind() (from notes.md §2b).
  - gtt.h: added their prototypes.
  - (sysfs hookup in psb_drv.c §2c NOT yet added — needed to actually trigger init;
    add sgx3d_sysfs_register/unregister + sgx3d_fini calls before first hw run.)

Safety fix applied: sgx3d_reset() now always releases SOFT_RESET on the BIF-drain
timeout error path (was the review's one 'major' finding).

Status: code-complete + compile-clean + safety-reviewed (safe_first_boot=true).
NOT yet run on hardware — the Vaio needs a power cycle. First hardware step = M3/M4
per notes.md §6 (sgx3d_enable=1, sysfs `init` trigger, tune sgx3d_code_base /
sgx3d_initstatus_off live via module params until INIT_COMPLETE).
