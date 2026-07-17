cmd_/work/services4/srvkm/env/linux/mutex.o := gcc -Wp,-MD,/work/services4/srvkm/env/linux/.mutex.o.d  -nostdinc -isystem /usr/lib/gcc/i686-linux-gnu/7/include  -I./arch/x86/include -I./arch/x86/include/generated  -I./include -I./arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -Iubuntu/include  -include ./include/linux/compiler_types.h -D__KERNEL__ -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -m32 -msoft-float -mregparm=3 -freg-struct-return -fno-pic -mpreferred-stack-boundary=2 -march=i686 -mtune=generic -Wa,-mtune=generic32 -ffreestanding -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -DCONFIG_AS_CFI_SECTIONS=1 -DCONFIG_AS_SSSE3=1 -DCONFIG_AS_AVX=1 -DCONFIG_AS_AVX2=1 -DCONFIG_AS_AVX512=1 -DCONFIG_AS_SHA1_NI=1 -DCONFIG_AS_SHA256_NI=1 -Wno-sign-compare -fno-asynchronous-unwind-tables -mindirect-branch=thunk-extern -mindirect-branch-register -fno-jump-tables -fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -O2 --param=allow-store-data-races=0 -Wframe-larger-than=1024 -fstack-protector-strong -Wimplicit-fallthrough -Wno-unused-but-set-variable -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -pg -mrecord-mcount -mfentry -DCC_USING_FENTRY -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-array-bounds -Wno-stringop-overflow -Wno-restrict -Wno-maybe-uninitialized -fno-strict-overflow -fno-merge-all-constants -fmerge-constants -fno-stack-check -fconserve-stack -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -I/work/include4 -I/work/services4/include -I/work/services4/srvkm/include -I/work/services4/srvkm/hwdefs -I/work/services4/srvkm/bridged -I/work/services4/srvkm/bridged/sgx -I/work/services4/srvkm/devices/sgx -I/work/services4/srvkm/env/linux -I/work/services4/system/include -I/work/services4/system/plb -I/work/services4/system/tnc -I/work/services4/system/common -DLINUX -DSERVICES4 -DSUPPORT_SGX -DSGX535 -DSGX_CORE_REV=121 -DSUPPORT_SGX535 -DSUPPORT_PERCONTEXT_PB -DTRANSFER_QUEUE -DSYS_USING_INTERRUPTS -DSUPPORT_HW_RECOVERY -DSUPPORT_ACTIVE_POWER_MANAGEMENT -DPVR_SECURE_HANDLES -DSUPPORT_SGX_EVENT_OBJECT -DLDM_PCI -DSUPPORT_SGX_HWPERF -DSUPPORT_LINUX_X86_PAT -DSUPPORT_CPU_CACHED_BUFFERS -DSUPPORT_CACHEFLUSH_ON_ALLOC -DPVR_PROC_USE_SEQ_FILE -DPVR_BUILD_DIR=\"pc_i686_poulsbo_d0_linux\" -DPVR_BUILD_TYPE=\"release\" -DRELEASE  -DMODULE  -DKBUILD_BASENAME='"mutex"' -DKBUILD_MODNAME='"pvrsrvkm"' -c -o /work/services4/srvkm/env/linux/mutex.o /work/services4/srvkm/env/linux/mutex.c

source_/work/services4/srvkm/env/linux/mutex.o := /work/services4/srvkm/env/linux/mutex.c

deps_/work/services4/srvkm/env/linux/mutex.o := \
  include/linux/kconfig.h \
    $(wildcard include/config/cpu/big/endian.h) \
    $(wildcard include/config/booger.h) \
    $(wildcard include/config/foo.h) \
  include/linux/compiler_types.h \
    $(wildcard include/config/have/arch/compiler/h.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/optimize/inlining.h) \
    $(wildcard include/config/cc/has/asm/inline.h) \
  include/linux/compiler_attributes.h \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arm64.h) \
    $(wildcard include/config/retpoline.h) \
    $(wildcard include/config/arch/use/builtin/bswap.h) \

/work/services4/srvkm/env/linux/mutex.o: $(deps_/work/services4/srvkm/env/linux/mutex.o)

$(deps_/work/services4/srvkm/env/linux/mutex.o):
