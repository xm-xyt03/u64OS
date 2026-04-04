#ifndef X86_ASM_COM_H
#define X86_ASM_COM_H

#define COM1_BASE 0x3f8
#define COM2_BASE 0x2f8

#define COM_REG_TX 0
#define COM_REG_RX 0
#define COM_REG_IER 1
#define COM_REG_DLL 0
#define COM_REG_DLM 1
#define COM_REG_IIR 2
#define COM_REG_FCR 2
#define COM_REG_LCR 3
#define COM_REG_MCR 4
#define COM_REG_LSR 5
#define COM_REG_MSR 6
#define COM_REG_SR 7

#endif // !X86_ASM_COM_H