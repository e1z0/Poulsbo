#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

#define POULSBO_PCI_ID_0 0x8108
#define POULSBO_PCI_ID_1 0x8109

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenAI Codex");
MODULE_DESCRIPTION("Poulsbo lab helper module for Ubuntu 18.04 build bring-up");

static bool verbose = true;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Log matching Poulsbo PCI devices during module init");

static int log_matching_devices(u16 device_id)
{
	struct pci_dev *pdev = NULL;
	int found = 0;

	for_each_pci_dev(pdev) {
		if (pdev->vendor != PCI_VENDOR_ID_INTEL || pdev->device != device_id)
			continue;

		found++;
		if (verbose) {
			pr_info(
				"poulsbo_lab: found device %04x:%02x:%02x.%d vendor=%04x device=%04x class=%06x\n",
				pci_domain_nr(pdev->bus),
				pdev->bus->number,
				PCI_SLOT(pdev->devfn),
				PCI_FUNC(pdev->devfn),
				pdev->vendor,
				pdev->device,
				pdev->class
			);
		}
	}

	return found;
}

static int __init poulsbo_lab_init(void)
{
	int total = 0;

	total += log_matching_devices(POULSBO_PCI_ID_0);
	total += log_matching_devices(POULSBO_PCI_ID_1);

	pr_info("poulsbo_lab: loaded, detected %d Poulsbo-class PCI device(s)\n", total);
	return 0;
}

static void __exit poulsbo_lab_exit(void)
{
	pr_info("poulsbo_lab: unloaded\n");
}

module_init(poulsbo_lab_init);
module_exit(poulsbo_lab_exit);
