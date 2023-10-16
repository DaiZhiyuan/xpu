#ifndef __XPU_H__
#define __XPU_H__

#define XPU_IO          251
#define GET_ID		0
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

#define XPU_DEV		"/dev/xpu"

#endif
