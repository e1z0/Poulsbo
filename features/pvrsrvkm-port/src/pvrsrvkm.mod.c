#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x3654b8c2, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xa0c1b9ea, "kmem_cache_destroy" },
	{ 0xd9fe29ea, "kmalloc_caches" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0x70538484, "__pm_runtime_idle" },
	{ 0x29f3a126, "up_read" },
	{ 0xf20d5589, "pci_release_region" },
	{ 0x562cae, "single_open" },
	{ 0x399d7dc2, "param_ops_int" },
	{ 0xa17d9352, "seq_open" },
	{ 0xad73041f, "autoremove_wake_function" },
	{ 0x78e340f9, "__x86_indirect_thunk_ebx" },
	{ 0x60b4748e, "boot_cpu_data" },
	{ 0xb6e11811, "pci_disable_device" },
	{ 0x32ae5741, "_raw_read_lock" },
	{ 0x57bfce4d, "seq_printf" },
	{ 0x8321e211, "remove_proc_entry" },
	{ 0x993a016e, "device_destroy" },
	{ 0x36e91934, "__register_chrdev" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x99b76c81, "mutex_unlock" },
	{ 0x88ce1c15, "__pm_runtime_resume" },
	{ 0x999e8297, "vfree" },
	{ 0xe8cac288, "pm_runtime_allow" },
	{ 0xe2724a34, "pm_runtime_forbid" },
	{ 0x4629334c, "__preempt_count" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x5dba6cb0, "seq_read" },
	{ 0xc809e4b1, "__alloc_pages_nodemask" },
	{ 0xd60c2042, "pv_ops" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xc9bba4b6, "down_read" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x658def44, "PDE_DATA" },
	{ 0x1ac4a519, "pci_set_master" },
	{ 0xea1b6b22, "kmap_atomic" },
	{ 0x97934ecf, "del_timer_sync" },
	{ 0x2bc95bd4, "memset" },
	{ 0xb16341a, "__default_kernel_pte_mask" },
	{ 0x9f9c718a, "proc_mkdir" },
	{ 0x72bcb10, "pci_restore_state" },
	{ 0x3812050a, "_raw_spin_unlock_irqrestore" },
	{ 0x60a92c84, "current_task" },
	{ 0xaffbb4f, "__mutex_init" },
	{ 0x8026fa61, "__x86_indirect_thunk_esi" },
	{ 0xecc9cdd8, "printk" },
	{ 0xfaef0ed, "__tasklet_schedule" },
	{ 0xb6ed1e53, "strncpy" },
	{ 0xf58ad361, "kmem_cache_free" },
	{ 0x1b58e04c, "mutex_lock" },
	{ 0x5d367805, "mem_section" },
	{ 0xed93f29e, "__kunmap_atomic" },
	{ 0x95ea79eb, "device_create" },
	{ 0x9545af6d, "tasklet_init" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x24d273d1, "add_timer" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0xf8a6fe12, "__x86_indirect_thunk_edi" },
	{ 0xc01464aa, "pci_enable_msi" },
	{ 0x12291e5, "pci_clear_master" },
	{ 0x4948c0fa, "vm_insert_page" },
	{ 0x82072614, "tasklet_kill" },
	{ 0x635cbb82, "find_vma" },
	{ 0xc6cbbc89, "capable" },
	{ 0xadd565e7, "kmem_cache_alloc" },
	{ 0x2d5e3f8f, "__free_pages" },
	{ 0x93a219c, "ioremap_nocache" },
	{ 0x738803e6, "strnlen" },
	{ 0xb665f56d, "__cachemode2pte_tbl" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xea0dd8b3, "get_user_pages" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0x4292364c, "schedule" },
	{ 0xbc435770, "dump_stack" },
	{ 0xe3460c96, "__x86_indirect_thunk_ecx" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x97dee519, "__x86_indirect_thunk_edx" },
	{ 0xf0df950a, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x51760917, "_raw_spin_lock_irqsave" },
	{ 0x71c6d403, "kmem_cache_create" },
	{ 0x2c9d3756, "vsnprintf" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x31408a22, "pci_set_power_state" },
	{ 0x8ad29bab, "_raw_write_unlock_bh" },
	{ 0xf77e2b5d, "proc_create_data" },
	{ 0xfd4b04a4, "seq_lseek" },
	{ 0x78bb33ac, "__vmalloc" },
	{ 0x37a0cba, "kfree" },
	{ 0xd7863fa2, "remap_pfn_range" },
	{ 0x2e60bace, "memcpy" },
	{ 0xd5fd90f1, "prepare_to_wait" },
	{ 0xb343581, "pci_disable_msi" },
	{ 0xedc03953, "iounmap" },
	{ 0x556422b3, "ioremap_cache" },
	{ 0xd5ed41b1, "class_destroy" },
	{ 0xe1ed698d, "_raw_write_lock_bh" },
	{ 0xe7a87ea1, "pci_get_device" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x31794991, "pci_choose_state" },
	{ 0x8c980b8, "seq_release" },
	{ 0x8b9b0d7, "vmalloc_to_page" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x294b9ea1, "on_each_cpu" },
	{ 0x1fb9149e, "pci_enable_device" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x5b75551e, "__class_create" },
	{ 0xaf403462, "pci_request_region" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xe7b767ab, "__put_page" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x84ee5bac, "pci_save_state" },
	{ 0xe914e41e, "strcpy" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("pci:v00008086d00008108sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00004108sv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "7B4CF6E5580290634B47BC6");
