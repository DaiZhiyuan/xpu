#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <time.h>

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

#define XPU_DEV         "/dev/xpu"


#ifdef HAVE_XPU

unsigned int add(unsigned int op1, unsigned int op2)
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

unsigned int mul(unsigned int op1, unsigned int op2)
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

#else

unsigned int add(unsigned int op1, unsigned int op2)
{
	unsigned int result;

	result = op1 + op2;

	return result;
}

unsigned int mul(unsigned int op1, unsigned int op2)
{
	unsigned int result;

	result = op1 * op2;

	return result;
}

#endif
