#include <asm/uaccess.h> 
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ioctl.h>

#define XPU_IO          251
#define GET_ID		0
#define GET_OP1         1
#define GET_OP2         2
#define GET_OPCODE      3
#define GET_RESULT      4
#define SET_OP1         5
#define SET_OP2         6
#define SET_OPCODE      7

#define XPU_ID		_IOR(XPU_IO, GET_ID, int *)
#define XPU_GET_OP1     _IOR(XPU_IO, GET_OP1, int *)
#define XPU_GET_OP2     _IOR(XPU_IO, GET_OP2, int *)
#define XPU_GET_OPCODE  _IOR(XPU_IO, GET_OPCODE, int *)
#define XPU_GET_RESULT  _IOR(XPU_IO, GET_RESULT, int *)
#define XPU_SET_OP1     _IOW(XPU_IO, SET_OP1, int)
#define XPU_SET_OP2     _IOW(XPU_IO, SET_OP2, int)
#define XPU_SET_OPCODE  _IOW(XPU_IO, SET_OPCODE, int)

#define OPCODE_ADD      0x1
#define OPCODE_MUL      0x2

#define BAR 0
#define XPU_CDEV_NAME "xpu"
#define XPU_VENDOR_ID 0x1db7
#define XPU_DEVICE_ID 0xdc3d

static int major;
static struct pci_dev *pdev;
static void __iomem *mmio;

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0, *get, set;

	if (GET_ID == cmd) {
		get = (int *)arg;
		*get = ioread32((void*)(mmio + 0x0));
	}

	if (GET_OP1 == cmd) {
		get = (int *) arg;	
		*get = ioread32((void*)(mmio + 0x10));
	}

	if (GET_OP2 == cmd) {
		get = (int *) arg;	
		*get = ioread32((void*)(mmio + 0x14));
	}

	if (GET_OPCODE == cmd) {
		get = (int *) arg;	
		*get = ioread32((void*)(mmio + 0x18));
	}

	if (GET_RESULT == cmd) {
		get = (int *) arg;	
		*get = ioread32((void*)(mmio + 0x30));
	}

	if (SET_OP1 == cmd) {
		set = (int) arg;
		iowrite32(set, mmio + 0x10);
	}

	if (SET_OP2 == cmd) {
		set = (int) arg;
		iowrite32(set, mmio + 0x14);
	}

	if (SET_OPCODE == cmd) {
		set = (int) arg;
		iowrite32(set, mmio + 0x18);
	}

	return ret;
}

static ssize_t read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	ssize_t ret;
	u32 kbuf;

	if (*off % 4 || len == 0) {
		ret = 0;
	} else {
		kbuf = ioread32(mmio + *off);
		if (raw_copy_to_user(buf, (void *)&kbuf, 4)) {
			pr_alert("fault on raw_copy_to_user\n");
			ret = -EFAULT;
		} else {
			ret = 4;
			(*off)++;
		}
	}

	return ret;
}

static ssize_t write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	ssize_t ret;
	u32 kbuf;

	ret = len;
	if (!(*off % 4)) {
		if (raw_copy_from_user((void *)&kbuf, buf, 4) || len != 4) {
			ret = -EFAULT;
		} else {
			iowrite32(kbuf, mmio + *off);
		}
	}

	return ret;
}

static loff_t llseek(struct file *filp, loff_t off, int whence)
{
	filp->f_pos = off;

	return off;
}

static struct file_operations fops = {
	.owner   = THIS_MODULE,
	.llseek  = llseek,
	.read    = read,
	.unlocked_ioctl = dev_ioctl,
	.write   = write,
};

static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	resource_size_t start, end;
	u8 val;
	unsigned i;

	pr_info("[XPU] PCI BUS probe.\n");

	major = register_chrdev(0, XPU_CDEV_NAME, &fops);
	pdev = dev;

	if (pci_enable_device(dev) < 0) {
		dev_err(&(pdev->dev), "pci_enable_device\n");
		goto error;
	}

	if (pci_request_region(dev, BAR, "xpu_region0")) {
		dev_err(&(pdev->dev), "pci_request_region\n");
		goto error;
	}

	mmio = pci_iomap(pdev, BAR, pci_resource_len(pdev, BAR));

	if ((pci_resource_flags(dev, BAR) & IORESOURCE_MEM) != IORESOURCE_MEM) {
		dev_err(&(dev->dev), "pci_resource_flags\n");
		goto error;
	}

	start = pci_resource_start(pdev, BAR);
	end = pci_resource_end(pdev, BAR);
	pr_info("BAR[0]: address=0x%llx\n", start);
	pr_info("BAR[0]: length=0x%llx\n", (unsigned long long)(end + 1 - start));

	for (i = 0; i < 64; ++i) {
		pci_read_config_byte(pdev, i, &val);
		pr_info("[config space] [0x%x] [0x%x]\n", i, val);
	}

	for (i = 0; i < 0x34; i += 4) {
		pr_info("[mmio space] [0x%x] [0x%x]\n", i, ioread32((void*)(mmio + i)));
	}

	return 0;
error:
	return 1;
}

static void pci_remove(struct pci_dev *dev)
{
	pr_info("[XPU] PCI BUS remove.\n");
	pci_release_region(dev, BAR);
	unregister_chrdev(major, XPU_CDEV_NAME);
}


static struct pci_device_id pci_ids[] = {
	{ PCI_DEVICE(XPU_VENDOR_ID, XPU_DEVICE_ID), },
	{ 0, }
};

static struct pci_driver pci_driver = {
	.name     = "xpu_driver",
	.id_table = pci_ids,
	.probe    = pci_probe,
	.remove   = pci_remove,
};

static int xpu_init(void)
{
	int err;

	err = pci_register_driver(&pci_driver);
	if (err)
		goto out_regsiter;
	
	return 0;

out_regsiter:
	pr_err("failed to register xpu: %d\n", err);
	return err;
}

static void xpu_exit(void)
{
	pci_unregister_driver(&pci_driver);
}

module_init(xpu_init);
module_exit(xpu_exit);
MODULE_DEVICE_TABLE(pci, pci_ids);
MODULE_DESCRIPTION("XPU Driver");
MODULE_AUTHOR("Jerry Dai");
MODULE_LICENSE("GPL");