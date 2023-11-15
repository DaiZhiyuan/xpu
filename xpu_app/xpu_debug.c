#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>

#define XPU_IO_SIZE 		0x1000
#define XPU_ID_OFFSET		0x00
#define XPU_OP1_OFFSET		0x10
#define XPU_OP2_OFFSET		0x14
#define XPU_OPCODE_OFFSET	0x18
#define XPU_RESULT_OFFSET	0x30
#define XPU_DMA_SRC_OFFSET	0x80
#define XPU_DMA_DST_OFFSET	0x88
#define XPU_DMA_CNT_OFFSET	0x90
#define XPU_DMA_CMD_OFFSET	0x98

static inline void xpu_write_mmio(uint32_t *bar, uint32_t offset, uint32_t value) 
{
	*((volatile uint32_t *)(bar + offset)) = value;
}

static inline uint32_t xpu_read_mmio(uint32_t *bar, uint32_t offset) 
{
	return *((volatile uint32_t *)(bar + offset));
}

static inline int xpu_read_config(int config_fd, void *buf, size_t len, off_t offset)
{
	return pread(config_fd, buf, len, offset);
}

static inline int xpu_write_config(int config_fd, const void *buf, size_t len, off_t offset)
{
	return pwrite(config_fd, buf, len, offset);
}

int main(int argc, char **argv)
{
	int uiofd, configfd, bar0fd;
	uint32_t *bar0, buffer;

	uiofd = open("/dev/uio0", O_RDWR);
	if (uiofd < 0) {
		perror("uio open:");
		return errno;
	}

	configfd = open("/sys/class/uio/uio0/device/config", O_RDWR);
	if (configfd < 0) {
		perror("config open:");
		return errno;
	}

	fprintf(stdout, "-----------XPU configure space----------\n");

	buffer = 0;
	xpu_read_config(configfd, &buffer, 2, 0x00);
	fprintf(stdout, "[Vendor ID]: 0x%04X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 2, 0x02);
	fprintf(stdout, "[Device ID]: 0x%04X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 2, 0x04);
	fprintf(stdout, "[Command]: 0x%04X\n", buffer);

#define IO_SPACE_ENABLE_SHIFT 0
#define IO_SPACE_MASK (0x1 << IO_SPACE_ENABLE_SHIFT)
#define MEM_SPACE_ENABLE_SHIFT 1
#define MEM_SPACE_MASK (0x1 << MEM_SPACE_ENABLE_SHIFT)
#define BUS_MASTER_ENABLE_SHIFT 2
#define BUS_MASTER_MASK (0x1 << BUS_MASTER_ENABLE_SHIFT)
#define INTERRUPT_DISABLE_SHIFT 10
#define INTERRUPT_MASK (0x1 << INTERRUPT_DISABLE_SHIFT)

	fprintf(stdout, "\t[IO Space Enable]: 0x%04X\n", (buffer & IO_SPACE_MASK) >> IO_SPACE_ENABLE_SHIFT);
	fprintf(stdout, "\t[MMIO Space Enable]: 0x%04X\n", (buffer & MEM_SPACE_MASK) >> MEM_SPACE_ENABLE_SHIFT);
	fprintf(stdout, "\t[BUS Master Enable]: 0x%04X\n", (buffer & BUS_MASTER_MASK) >> BUS_MASTER_ENABLE_SHIFT);
	fprintf(stdout, "\t[Interrupt Disable]: 0x%04X\n", (buffer & INTERRUPT_MASK) >> INTERRUPT_DISABLE_SHIFT);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 2, 0x06);
	fprintf(stdout, "[Status]: 0x%04X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x08);
	fprintf(stdout, "[Revision ID]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 3, 0x09);
	fprintf(stdout, "[Class Code]: 0x%06X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x0c);
	fprintf(stdout, "[Cache Line]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x0d);
	fprintf(stdout, "[Latency Timer]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x0e);
	fprintf(stdout, "[Header Type]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x0f);
	fprintf(stdout, "[BIST]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x10);
	fprintf(stdout, "[Base Address 0]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x14);
	fprintf(stdout, "[Base Address 1]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x18);
	fprintf(stdout, "[Base Address 2]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x1c);
	fprintf(stdout, "[Base Address 3]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x20);
	fprintf(stdout, "[Base Address 4]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x24);
	fprintf(stdout, "[Base Address 5]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x28);
	fprintf(stdout, "[CardBus CIS pointer]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 2, 0x2c);
	fprintf(stdout, "[Subsystem Vendor ID]: 0x%04X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 2, 0x2e);
	fprintf(stdout, "[Subsystem Device ID]: 0x%04X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x30);
	fprintf(stdout, "[Expansion ROM Base Address]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x34);
	fprintf(stdout, "[Reserved(Capability List)]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 4, 0x38);
	fprintf(stdout, "[Reserved]: 0x%08X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x3c);
	fprintf(stdout, "[IRQ Line]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x3d);
	fprintf(stdout, "[IRQ Pin]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x3e);
	fprintf(stdout, "[Min_Gnt]: 0x%02X\n", buffer);

	buffer = 0;
	xpu_read_config(configfd, &buffer, 1, 0x3f);
	fprintf(stdout, "[Max_Lat]: 0x%02X\n", buffer);

	bar0fd = open("/sys/class/uio/uio0/device/resource0", O_RDWR);
	if (bar0fd < 0) {
		perror("bar0fd open:");
		return errno;
	}

	bar0 = (uint32_t *)mmap(NULL, XPU_IO_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, bar0fd, 0);
	if (bar0 == MAP_FAILED) {
		perror("Error mapping bar0!");
		return errno;
	}

	fprintf(stdout, "-----------XPU MMIO space----------\n");
	fprintf(stdout, "[0x00]: 0x%08X\n", xpu_read_mmio(bar0, XPU_ID_OFFSET));
	fprintf(stdout, "[0x10]: 0x%08X\n", xpu_read_mmio(bar0, XPU_OP1_OFFSET));
	fprintf(stdout, "[0x14]: 0x%08X\n", xpu_read_mmio(bar0, XPU_OP2_OFFSET));
	fprintf(stdout, "[0x18]: 0x%08X\n", xpu_read_mmio(bar0, XPU_OPCODE_OFFSET));
	fprintf(stdout, "[0x30]: 0x%08X\n", xpu_read_mmio(bar0, XPU_RESULT_OFFSET));
	fprintf(stdout, "[0x80]: 0x%08X\n", xpu_read_mmio(bar0, XPU_DMA_SRC_OFFSET));
	fprintf(stdout, "[0x88]: 0x%08X\n", xpu_read_mmio(bar0, XPU_DMA_DST_OFFSET));
	fprintf(stdout, "[0x90]: 0x%08X\n", xpu_read_mmio(bar0, XPU_DMA_CNT_OFFSET));
	fprintf(stdout, "[0x98]: 0x%08X\n", xpu_read_mmio(bar0, XPU_DMA_CMD_OFFSET));

	munmap(bar0, XPU_IO_SIZE);
	close(bar0fd);
	close(configfd);
	close(uiofd);

	return errno;
}

