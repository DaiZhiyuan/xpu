#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#define XPU_IO          251
#define GET_ID          0
#define GET_OP1         1
#define GET_OP2         2
#define GET_OPCODE      3
#define GET_RESULT      4
#define SET_OP1         5
#define SET_OP2         6
#define SET_OPCODE      7

#define XPU_ID          _IOR(XPU_IO, GET_ID, int *)
#define XPU_GET_OP1     _IOR(XPU_IO, GET_OP1, int *)
#define XPU_GET_OP2     _IOR(XPU_IO, GET_OP2, int *)
#define XPU_GET_OPCODE  _IOR(XPU_IO, GET_OPCODE, int *)
#define XPU_GET_RESULT  _IOR(XPU_IO, GET_RESULT, int *)
#define XPU_SET_OP1     _IOW(XPU_IO, SET_OP1, int)
#define XPU_SET_OP2     _IOW(XPU_IO, SET_OP2, int)
#define XPU_SET_OPCODE  _IOW(XPU_IO, SET_OPCODE, int)

#define OPCODE_ADD      0x1
#define OPCODE_MUL      0x2
#define BAR             0
#define XPU_VENDOR_ID   0x1db7
#define XPU_DEVICE_ID   0xdc3d
#define XPU_DMA_BUF     0x40000

static const char xpu_dev_name[] = "xpu";
static int xpu_char_dev_major = -1;
static struct class *xpu_class;
struct device *xpu_device;

static struct pci_dev *pdev;
static void __iomem *mmio;
static int irq;

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

static struct file_operations xpu_fops = {
        .owner   = THIS_MODULE,
        .llseek  = llseek,
        .read    = read,
        .unlocked_ioctl = dev_ioctl,
        .write   = write,
};

static int xpu_chardev_init(void)
{
        int err = 0;

        xpu_char_dev_major = register_chrdev(0, xpu_dev_name, &xpu_fops);
        err = xpu_char_dev_major;
        if (err < 0)
                goto err_register_chrdev;

        xpu_class = class_create(THIS_MODULE, xpu_dev_name);
        err = PTR_ERR(xpu_class);
        if (IS_ERR(xpu_class))
                goto err_class_create;

        xpu_device = device_create(xpu_class, NULL,
                                    MKDEV(xpu_char_dev_major, 0),
                                    NULL, xpu_dev_name);
        err = PTR_ERR(xpu_device);
        if (IS_ERR(xpu_device))
                goto err_device_create;

        return 0;

err_device_create:
        class_destroy(xpu_class);
err_class_create:
        unregister_chrdev(xpu_char_dev_major, xpu_dev_name);
err_register_chrdev:
        return err;
}

static void xpu_chardev_exit(void)
{
        device_destroy(xpu_class, MKDEV(xpu_char_dev_major, 0));
        class_destroy(xpu_class);
        unregister_chrdev(xpu_char_dev_major, xpu_dev_name);
        xpu_device = NULL;
}

static irqreturn_t irq_handler(int irq, void *dev)
{
        u32 irq_status;

        irq_status = ioread32(mmio + 0x40);
        pr_info("xpu_irq_handler: irq = %d, irq_status = %x\n",
                                irq, irq_status);

        /* Must do this ACK, or else the interrupts just keeps firing. */
        iowrite32(irq_status, mmio + 0x48);

        return IRQ_HANDLED;
}

static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
        resource_size_t start, end;
        u8 val, lat;
        unsigned i;
        int nvec = 2;
        void *cpu_from, *cpu_to;
        dma_addr_t dma_from, dma_to;
        int ret;
        int dma_bits = 28;

        pr_info("[XPU] PCI BUS probe.\n");

        pdev = dev;
        if (pcim_enable_device(dev) < 0) {
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

        pci_set_master(pdev);

        ret = dma_set_mask_and_coherent(&(pdev->dev), DMA_BIT_MASK(dma_bits));
        if (ret) {
                dev_err(&(pdev->dev),"failed to set DMA mask & coherent bits\n");
                goto error;
        }

        pci_read_config_byte(dev, PCI_LATENCY_TIMER, &lat);
        dev_info(&(dev->dev), "latency timer = %u clocks\n", lat);

        nvec = pci_alloc_irq_vectors(pdev, 1, nvec, PCI_IRQ_ALL_TYPES);
        if (nvec < 0) {
                pr_info("RETURN: %d\n", nvec);
                goto error;
        }

        if (dev->msi_enabled)
                pr_info("MSI Enabled\n");

        pr_info("MSI Vector: %d\n", nvec);

        irq = pci_irq_vector(pdev, 0);
        pr_info("IRQ Number: %d\n", irq);
        pr_info("pdev->irq: %d\n", pdev->irq);

        ret = request_irq(pdev->irq, irq_handler, IRQF_NO_THREAD, "xpu_dma_interrupt", NULL);
        if (ret) {
                dev_err(&(dev->dev), "request_irq: %d\n", ret);
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

        dev_info(&(dev->dev), "ioremap mmio address: 0x%pK\n", mmio);

        for (i = 0; i < 0x34; i += 4) {
                pr_info("[mmio space] [0x%x] [0x%x]\n", i, ioread32((void*)(mmio + i)));
        }

        dma_set_mask_and_coherent(&(dev->dev), DMA_BIT_MASK(28));
        cpu_from = dma_alloc_coherent(&(dev->dev), 4, &dma_from, GFP_ATOMIC);
        dev_info(&(dev->dev), "CPU address: 0x%pK\n", cpu_from);
        dev_info(&(dev->dev), "DMA address: 0x%llx\n", dma_from);

        *(u32*)cpu_from = 0x87654321; // Prepare data (0x87654321), copy from CPU side DMA to device side.
        iowrite32(dma_from, mmio + 0x80); // Write dma_src register
        iowrite32(XPU_DMA_BUF, mmio + 0x88); // write dma_dst regsiter
        iowrite32(sizeof(u32), mmio + 0x90); // write dma_len regsiter
        iowrite32(0x5, mmio + 0x98); // write dma_cmd regsiter
        mdelay(lat);

        for (i = 0x80; i <= 0x98; i += 8) {
                pr_info("[mmio space] [0x%x] [0x%x]\n", i, ioread32((void*)(mmio + i)));
        }

        cpu_to = dma_alloc_coherent(&(dev->dev), 4, &dma_to, GFP_ATOMIC);
        dev_info(&(dev->dev), "CPU address: 0x%pK\n", cpu_to);
        dev_info(&(dev->dev), "DMA address: 0x%llx\n", dma_to);

        iowrite32(XPU_DMA_BUF, mmio + 0x80); // Write dma_src regsiter
        iowrite32((u32)dma_to, mmio + 0x88); // Wrtie dma_dst register
        iowrite32(sizeof(u32), mmio + 0x90); // Wrtie dma_len regsiter
        iowrite32(0x7, mmio + 0x98); // Write dma_cmd regsiter
        mdelay(lat);

        for (i = 0x80; i <= 0x98; i += 8) {
                pr_info("[mmio space] [0x%x] [0x%x]\n", i, ioread32((void*)(mmio + i)));
        }

        pr_info("[DMA return]: 0x%x\n", *(u32*)cpu_to); // Receive data (0x87654321), copy from device side DMA to CPU side.

        dma_free_coherent(&(dev->dev), 4, cpu_to, dma_to);
        dma_free_coherent(&(dev->dev), 4, cpu_from, dma_from);

        return 0;
error:
        return 1;
}

static void pci_remove(struct pci_dev *dev)
{
        pr_info("[XPU] PCI BUS remove.\n");
        pci_release_region(dev, BAR);
        if (irq)
                free_irq(irq, NULL);
}


#ifdef CONFIG_PM_SLEEP
static int xpu_pci_suspend(struct device *dev)
{
        return 0;
}

static int xpu_pci_resume(struct device *dev)
{
        return 0;
}

static const struct dev_pm_ops xpu_pci_pm_ops = {
        .suspend        = xpu_pci_suspend,
        .resume         = xpu_pci_resume,
};
#endif /* CONFIG_PM_SLEEP */

static struct pci_device_id pci_ids[] = {
        { PCI_DEVICE(XPU_VENDOR_ID, XPU_DEVICE_ID), },
        { 0, }
};

static struct pci_driver xpu_driver = {
        .name     = "xpu_driver",
        .id_table = pci_ids,
        .probe    = pci_probe,
        .remove   = pci_remove,
        .driver = {
                .pm = &xpu_pci_pm_ops,
        },
};

static int __init xpu_init(void)
{
        int rc = 0;

        rc = pci_register_driver(&xpu_driver);
        if (rc)
                return rc;

        rc = xpu_chardev_init();

        return rc;
}

static void __exit xpu_exit(void)
{
        xpu_chardev_exit();
        pci_unregister_driver(&xpu_driver);
}

module_init(xpu_init);
module_exit(xpu_exit);
MODULE_DEVICE_TABLE(pci, pci_ids);
MODULE_DESCRIPTION("XPU Driver");
MODULE_AUTHOR("Jerry Dai <Jerry.zhiyuan.dai@gmail.com>");
MODULE_LICENSE("GPL");
