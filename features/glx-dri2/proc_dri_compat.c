/* proc_dri_compat.c - recreate the legacy /proc/dri/N/name interface that the
 * closed PowerVR DRI driver (emgd_dri.so) reads to find its DRM device, but
 * which modern kernels removed.  proc-only: safe to insmod live. */
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static char *name = "emgd 0000:00:02.0";
module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "content of /proc/dri/N/name (driver busid)");

#define NCARD 8
static struct proc_dir_entry *dri_dir;

static int name_show(struct seq_file *m, void *v) { seq_printf(m, "%s\n", name); return 0; }
static int name_open(struct inode *i, struct file *f) { return single_open(f, name_show, NULL); }
static const struct file_operations name_fops = {
	.owner = THIS_MODULE, .open = name_open, .read = seq_read,
	.llseek = seq_lseek, .release = single_release,
};

static int __init pdc_init(void)
{
	int k; char b[8];
	dri_dir = proc_mkdir("dri", NULL);
	if (!dri_dir) return -ENOMEM;
	for (k = 0; k < NCARD; k++) {
		struct proc_dir_entry *d;
		snprintf(b, sizeof b, "%d", k);
		d = proc_mkdir(b, dri_dir);
		if (d) proc_create("name", 0444, d, &name_fops);
	}
	pr_info("proc_dri_compat: /proc/dri/N/name = \"%s\"\n", name);
	return 0;
}
static void __exit pdc_exit(void) { proc_remove(dri_dir); }
module_init(pdc_init); module_exit(pdc_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("legacy /proc/dri/N/name compat for closed PVR DRI driver");
