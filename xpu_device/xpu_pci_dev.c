#include "qemu/osdep.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/hw.h"
#include "hw/pci/msi.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include "qemu/main-loop.h"
#include "qemu/module.h"
#include "qapi/visitor.h"

#define TYPE_PCI_XPU_DEVICE "xpudev"
#define DMA_START   0x40000
#define DMA_SIZE	4096

typedef struct XpudevState XpudevState;

DECLARE_INSTANCE_CHECKER(XpudevState, XPUDEV, TYPE_PCI_XPU_DEVICE)

struct XpudevState {
	PCIDevice pdev;
	MemoryRegion mmio;
	uint32_t op1;
	uint32_t op2;
	uint32_t opcode;
	uint32_t result;
	uint32_t error_code;

	uint32_t irq_status;

#define XPU_DMA_RUN             0x1
#define XPU_DMA_DIRECTION(cmd) (((cmd) & 0x2) >> 1)
#define XPU_DMA_FROM_PCI		0
#define XPU_DMA_TO_PCI			1
#define XPU_DMA_IRQ				0x4

	struct dma_state {
		dma_addr_t src;
		dma_addr_t dst;
		dma_addr_t cnt;
		dma_addr_t cmd;
	} dma;

	QEMUTimer dma_timer;
	char dma_buffer[DMA_SIZE];
	uint64_t dma_mask;
};

static void xpu_raise_irq(XpudevState *xpu, uint32_t val)
{
	xpu->irq_status |= val;

	if (xpu->irq_status) {
		if (msi_enabled(&xpu->pdev))
			msi_notify(&xpu->pdev, 0);
		else
			pci_set_irq(&xpu->pdev, 1);
	}
}

static void xpu_lower_irq(XpudevState *xpu, uint32_t val)
{
	xpu->irq_status &= ~val;

	if (!xpu->irq_status && (!msi_enabled(&xpu->pdev)))
		pci_set_irq(&xpu->pdev, 0);
}

static void dma_rw(XpudevState *xpu, bool write, dma_addr_t *val, dma_addr_t *dma, bool timer)
{
	if (write && (xpu->dma.cmd & XPU_DMA_RUN))
		return;

	if (write)
		*dma = *val;
	else
		*val = *dma;

	if (timer)
		 timer_mod(&xpu->dma_timer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + 100);
}

static uint64_t xpudev_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
	XpudevState *xpudev = opaque;
	uint64_t val = ~0ULL;

	if (addr < 0x80 && size != 4) {
		return val;
	}

	if (addr >= 0x80 && size != 4 && size != 8) {
		return val;
	}

	switch (addr) {
		case 0x00:
			val = 0x12345678;
			break;

		case 0x10:
			val = xpudev->op1;
			break;

		case 0x14:
			val = xpudev->op2;
			break;

		case 0x18:
			val = xpudev->opcode;
			break;

		case 0x30:
			if(xpudev->opcode == 0x1) {
				xpudev->result = xpudev->op1 + xpudev->op2;
				xpudev->error_code = 0x0;
			} else if(xpudev->opcode == 0x2) {
				xpudev->result = xpudev->op1 * xpudev->op2;
				xpudev->error_code = 0x0;
			} else {
				xpudev->result = 0xff;
				xpudev->error_code = 0x1;
			}

			val = xpudev->result;
			break;

		case 0x40:
			val = xpudev->irq_status;
			break;

		case 0x80:
			dma_rw(xpudev, false, &val, &xpudev->dma.src, false);
			break;

		case 0x88:
			dma_rw(xpudev, false, &val, &xpudev->dma.dst, false);
			break;

		case 0x90:
			dma_rw(xpudev, false, &val, &xpudev->dma.cnt, false);
			break;

		case 0x98:
			dma_rw(xpudev, false, &val, &xpudev->dma.cmd, false);
			break;
	}

	return val;
}

static void xpudev_mmio_write(void *opaque, hwaddr addr, uint64_t val,
		unsigned size)
{
	XpudevState *xpudev = opaque;

	if (addr < 0x80 && size != 4) {
		return;
	}

	if (addr < 0x80 && size != 4 && size != 8) {
		return;
	}

	switch (addr) {
		case 0x10:
			xpudev->op1 = val;
			break;

		case 0x14:
			xpudev->op2 = val;
			break;

		case 0x18:
			xpudev->opcode = val;
			break;

		case 0x44:
			xpu_raise_irq(xpudev, val);
			break;

		case 0x48:
			xpu_lower_irq(xpudev, val);
			break;

		case 0x80:
			dma_rw(xpudev, true, &val, &xpudev->dma.src, false);
			break;

		case 0x88:
			dma_rw(xpudev, true, &val, &xpudev->dma.dst, false);
			break;

		case 0x90:
			dma_rw(xpudev, true, &val, &xpudev->dma.cnt, false);
			break;

		case 0x98:
			if (!(val & XPU_DMA_RUN))
				break;

			dma_rw(xpudev, true, &val, &xpudev->dma.cmd, true);
			break;
	}
}

static const MemoryRegionOps xpudev_mmio_ops = {
	.read = xpudev_mmio_read,
	.write = xpudev_mmio_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
	.valid = {
		.min_access_size = 4,
		.max_access_size = 8,
	},
	.impl = {
		.min_access_size = 4,
		.max_access_size = 8,
	},
};

static void xpu_check_range(uint64_t xfer_start, uint64_t xfer_size, uint64_t dma_start, uint64_t dma_size)
{
	uint64_t xfer_end = xfer_start + xfer_size;
	uint64_t dma_end = dma_start + dma_size;

	if (dma_end >= dma_start && xfer_end >= xfer_start && xfer_start >= dma_start && xfer_end <= dma_end)
		return;

	hw_error("XPU: DMA range 0x%016"PRIx64"-0x%016"PRIx64
			" out of bounds (0x%016"PRIx64"-0x%016"PRIx64")!",
			xfer_start, xfer_end-1, dma_start, dma_end-1);
}

static dma_addr_t xpu_clamp_addr(const XpudevState *xpu, dma_addr_t addr)
{
	dma_addr_t res = addr & xpu->dma_mask;

	if (addr != res) {
		printf("XPU: clamping DMA %#.16"PRIx64" to %#.16"PRIx64"!\n", addr, res);
	}

	return res;
}

static void xpu_dma_timer(void *opaque)
{
	XpudevState *xpu = opaque;
	bool raise_irq = false;

	if (!(xpu->dma.cmd & XPU_DMA_RUN))
		return;

	/* DMA_DIRECTION_TO_DEVICE */
	if (XPU_DMA_DIRECTION(xpu->dma.cmd) == XPU_DMA_FROM_PCI) {
		uint64_t dst = xpu->dma.dst;
		xpu_check_range(dst, xpu->dma.cnt, DMA_START, DMA_SIZE);

		dst -= DMA_START;
		pci_dma_read(&xpu->pdev, xpu_clamp_addr(xpu, xpu->dma.src), 
			xpu->dma_buffer + dst, xpu->dma.cnt);
	} 
	
	/* DMA_DIRECTION_FROM_DEVICE */
	if (XPU_DMA_DIRECTION(xpu->dma.cmd) == XPU_DMA_TO_PCI ) {
		uint64_t src = xpu->dma.src;
		xpu_check_range(src, xpu->dma.cnt, DMA_START, DMA_SIZE);

		src -= DMA_START;
		pci_dma_write(&xpu->pdev, xpu_clamp_addr(xpu, xpu->dma.dst), 
			xpu->dma_buffer + src, xpu->dma.cnt);
	}

	/* Clear EDU_DMA_RUN bit */
	xpu->dma.cmd &= ~XPU_DMA_RUN;

	if (xpu->dma.cmd & XPU_DMA_IRQ)
		raise_irq = true;

	/* Raise IRQ */
	if (raise_irq)
		xpu_raise_irq(xpu, XPU_DMA_IRQ);
}

static void pci_xpudev_realize(PCIDevice *pdev, Error **errp)
{
	XpudevState *xpudev = XPUDEV(pdev);
	uint8_t *pci_conf = pdev->config;

	pci_config_set_interrupt_pin(pci_conf, 1);

	if (msi_init(pdev, 0, 1, true, false, errp)) 
		return;

	timer_init_ms(&xpudev->dma_timer, QEMU_CLOCK_VIRTUAL, xpu_dma_timer, xpudev);

	xpudev->op1 = 0x1;
	xpudev->op2 = 0x2;
	xpudev->opcode = 0x1;
	xpudev->result = 0x3;
	xpudev->error_code = 0x0;

	memory_region_init_io(&xpudev->mmio, OBJECT(xpudev), &xpudev_mmio_ops, xpudev, "xpudev-mmio", 1 * MiB);
	pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &xpudev->mmio);

	/* PCI Latency Timer */
	pci_set_byte(pci_conf + PCI_LATENCY_TIMER, 0x80);   /* latency timer = 128 clocks */
}

static void pci_xpudev_uninit(PCIDevice *pdev)
{
	XpudevState *xpu = XPUDEV(pdev);

	timer_del(&xpu->dma_timer);
	msi_uninit(pdev);
}

static void xpudev_instance_init(Object *obj)
{
	XpudevState *xpu = XPUDEV(obj);

	/* 256MiB */
	xpu->dma_mask = (1L << 28) - 1;
	object_property_add_uint64_ptr(obj, "dma_mask",
                               &xpu->dma_mask, OBJ_PROP_FLAG_READWRITE);
}

static void xpudev_class_init(ObjectClass *class, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(class);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

	k->realize = pci_xpudev_realize;
	k->exit = pci_xpudev_uninit;
	k->vendor_id = 0x1db7;
	k->device_id = 0xdc3d;
	k->revision = 0x01;
	k->class_id = PCI_CLASS_OTHERS;

	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static void pci_xpu_device_register_types(void)
{
	static InterfaceInfo interfaces[] = {
		{ INTERFACE_CONVENTIONAL_PCI_DEVICE },
		{ },
	};

	static const TypeInfo xpu_pci_device_info = {
		.name          = TYPE_PCI_XPU_DEVICE,
		.parent        = TYPE_PCI_DEVICE,
		.instance_size = sizeof(XpudevState),
		.instance_init = xpudev_instance_init,
		.class_init    = xpudev_class_init,
		.interfaces = interfaces,
	};

	type_register_static(&xpu_pci_device_info);
}

type_init(pci_xpu_device_register_types)
