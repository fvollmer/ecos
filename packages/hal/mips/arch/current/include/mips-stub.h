#ifndef CYGONCE_HAL_MIPS_STUB_H
#define CYGONCE_HAL_MIPS_STUB_H

/* mips-stub.h - MIPS-specific definitions for generic stub
 * 
 * Copyright (c) 1998,1999 Cygnus Solutions
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

#include <pkgconf/system.h>

#define NUMREGS    90

#if defined(__mips64)
  #define REGSIZE(X) 8
  typedef unsigned long long target_register_t;
#else
  #define REGSIZE(X) 4
  typedef unsigned long target_register_t;
#endif

enum regnames {
        ZERO,   AT,     V0,     V1,     A0,     A1,     A2,     A3,
        T0,     T1,     T2,     T3,     T4,     T5,     T6,     T7,
        S0,     S1,     S2,     S3,     S4,     S5,     S6,     S7,
        T8,     T9,     K0,     K1,     GP,     SP,     S8,     RA,
        SR,     LO,     HI,     BAD,    CAUSE,  PC,
        CONFIG = 84,    CACHE,  DEBUG,  DEPC,   EPC
};

typedef enum regnames regnames_t;

/* Given a trap value TRAP, return the corresponding signal. */
extern int __computeSignal (unsigned int trap_number);

/* Return the SPARC trap number corresponding to the last-taken trap. */
extern int __get_trap_number (void);

/* Return the currently-saved value corresponding to register REG. */
extern target_register_t get_register (regnames_t reg);

/* Store VALUE in the register corresponding to WHICH. */
extern void put_register (regnames_t which, target_register_t value);

/* Set the currently-saved pc register value to PC. This also updates NPC
   as needed. */
extern void set_pc (target_register_t pc);

/* Set things up so that the next user resume will execute one instruction.
   This may be done by setting breakpoints or setting a single step flag
   in the saved user registers, for example. */
void __single_step (void);

/* Clear the single-step state. */
void __clear_single_step (void);

/* If the breakpoint we hit is in the breakpoint() instruction, return a
   non-zero value. */
extern int __is_breakpoint_function (void);

/* Skip the current instruction. */
extern void __skipinst (void);

extern void __install_breakpoints (void);

extern void __clear_breakpoints (void);

#endif // ifndef CYGONCE_HAL_MIPS_STUB_H
