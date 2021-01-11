#ifndef _INITSTD_H
#define _INITSTD_H

// processor modes
#define _usr_mode 16
#define _fiq_mode 17
#define _irq_mode 18
#define _svc_mode 19
#define _abt_mode 23
#define _und_mode 27
#define _sys_mode 31

// stack pointer
#define _sp_offset 1024 // stacks have 1 KB of space
#define _svc_sp (0x00300000) // kernel stacks start underneath first addr of user space
#define _fiq_sp (_svc_sp - _sp_offset)
#define _irq_sp (_fiq_sp - _sp_offset)
#define _abt_sp (_irq_sp - _sp_offset)
#define _und_sp (_abt_sp - _sp_offset)
#define _sys_sp (_und_sp - _sp_offset)
#define _tcb_sp_base (0x00600000) // user stacks start underneath first addr of unused memory space
#define _pcb_heap_base (0x00400000)
#define _tcb_size 32
#define _pcb_size 8

#endif // _INITSTD_H
