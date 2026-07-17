cmd_/work/pvrsrvkm.ko := ld -r -m elf_i386 -z noexecstack   --build-id  -T ./scripts/module-common.lds -o /work/pvrsrvkm.ko /work/pvrsrvkm.o /work/pvrsrvkm.mod.o;  true
