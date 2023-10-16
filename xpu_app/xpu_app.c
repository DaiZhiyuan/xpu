#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <time.h>

#include "xpu.h"

unsigned int get_xpu_id(void)
{
	unsigned int id;
	int fd;

	fd = open(XPU_DEV, O_RDWR);
	if (fd < 0)
		perror("open()");

	ioctl(fd, GET_ID, &id);
	close(fd);

	return id;
}

unsigned int sw_add(unsigned int op1, unsigned int op2)
{
	unsigned int result;

	result = op1 + op2;

	return result;
}

unsigned int hw_add(unsigned int op1, unsigned int op2)
{
	unsigned int result;
	int fd;

	fd = open(XPU_DEV, O_RDWR);
	if (fd < 0)
		perror("open()");

	ioctl(fd, SET_OP1, op1);
	ioctl(fd, SET_OP2, op2);
	ioctl(fd, SET_OPCODE, OPCODE_ADD);
	ioctl(fd, GET_RESULT, &result);
	close(fd);

	return result;
}

unsigned int sw_mul(unsigned int op1, unsigned int op2)
{
	unsigned int result;

	result = op1 * op2;

	return result;
}

unsigned int hw_mul(unsigned int op1, unsigned int op2)
{
	unsigned int result;
	int fd;

	fd = open(XPU_DEV, O_RDWR);
	if (fd < 0)
		perror("open()");

	ioctl(fd, SET_OP1, op1);
	ioctl(fd, SET_OP2, op2);
	ioctl(fd, SET_OPCODE, OPCODE_MUL);
	ioctl(fd, GET_RESULT, &result);
	close(fd);

	return result;
}

int main(int argc, char **argv)
{
        unsigned int op1, op2;

	srand((unsigned int)time(NULL));
	op1 = rand() % 100 + 1;
	op2 = rand() % 100 + 1;

	printf("\nXPU accelerater device ID: 0x%x\n\n", get_xpu_id());
	printf("[mode]    [op1]  [action]  [op2]  [result]\n");
	printf("------------------------------------------\n");
	printf("  SW        %d      ADD      %d      %d\n", op1, op2, sw_add(op1, op2));
	printf("  HW        %d      ADD      %d      %d\n", op1, op2, hw_add(op1, op2));
	printf("  SW        %d      MUL      %d      %d\n", op1, op2, sw_mul(op1, op2));
	printf("  HW        %d      MUL      %d      %d\n", op1, op2, hw_mul(op1, op2));

        return 0;
}
