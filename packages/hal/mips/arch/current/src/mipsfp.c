//==========================================================================
//
//      mipsfp.c
//
//      HAL miscellaneous functions
//
//==========================================================================
//####COPYRIGHTBEGIN####
//
// -------------------------------------------
// The contents of this file are subject to the Cygnus eCos Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://sourceware.cygnus.com/ecos
// 
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
// License for the specific language governing rights and limitations under
// the License.
// 
// The Original Code is eCos - Embedded Cygnus Operating System, released
// September 30, 1998.
// 
// The Initial Developer of the Original Code is Cygnus.  Portions created
// by Cygnus are Copyright (C) 1998,1999 Cygnus Solutions.  All Rights Reserved.
// -------------------------------------------
//
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    jlarmour
// Contributors: jlarmour
// Date:         1999-07-13
// Purpose:      Emulate unimplemented FP operations on MIPS architectures
// Description:  This catches the unimplemented operation excetion only,
//               and if possible deals with it so processing can continue
//               as if the MIPS had a proper IEEE FPU
//               
//
//####DESCRIPTIONEND####
//
//========================================================================*/

// CONFIGURATION

#include <pkgconf/hal.h>

#ifdef CYGHWR_HAL_MIPS_FPU

// INCLUDES

#include <cyg/infra/cyg_type.h>    // Standard eCos types
#include <cyg/infra/cyg_ass.h>     // Standard eCos assertion support
#include <cyg/infra/cyg_trac.h>    // Standard eCos tracing support
#include <cyg/hal/hal_intr.h>      // HAL interrupt vectors
#include <cyg/hal/hal_arch.h>      // Architecture types such as
                                   // HAL_SavedRegisters
#define CYGARC_HAL_COMMON_EXPORT_CPU_MACROS
#include <cyg/hal/mips-regs.h>     // MIPS register and bitmask definitions

// TYPES

// The following types were taken from <sys/ieeefp.h> from libm.

#if (CYG_BYTEORDER == CYG_MSBFIRST) // Big endian

// Note: there do not seem to be any current machines which are Big Endian but
// have a mixed up double layout. 

typedef union 
{
    cyg_int32 asi32[2];

    cyg_int64 asi64;
    
    double value;
    
    struct 
    {
        unsigned int sign : 1;
        unsigned int exponent: 11;
        unsigned int fraction0:4;
        unsigned int fraction1:16;
        unsigned int fraction2:16;
        unsigned int fraction3:16;
        
    } number;
    
    struct 
    {
        unsigned int sign : 1;
        unsigned int exponent: 11;
        unsigned int quiet:1;
        unsigned int function0:3;
        unsigned int function1:16;
        unsigned int function2:16;
        unsigned int function3:16;
    } nan;
    
    struct 
    {
        cyg_uint32 msw;
        cyg_uint32 lsw;
    } parts;

    
} Cyg_libm_ieee_double_shape_type;


typedef union
{
    cyg_int32 asi32;
    
    float value;

    struct 
    {
        unsigned int sign : 1;
        unsigned int exponent: 8;
        unsigned int fraction0: 7;
        unsigned int fraction1: 16;
    } number;

    struct 
    {
        unsigned int sign:1;
        unsigned int exponent:8;
        unsigned int quiet:1;
        unsigned int function0:6;
        unsigned int function1:16;
    } nan;
    
} Cyg_libm_ieee_float_shape_type;


#else // Little endian

typedef union 
{
    cyg_int32 asi32[2];

    cyg_int64 asi64;
    
    double value;

    struct 
    {
#if (CYG_DOUBLE_BYTEORDER == CYG_MSBFIRST) // Big endian
        unsigned int fraction1:16;
        unsigned int fraction0: 4;
        unsigned int exponent :11;
        unsigned int sign     : 1;
        unsigned int fraction3:16;
        unsigned int fraction2:16;
#else
        unsigned int fraction3:16;
        unsigned int fraction2:16;
        unsigned int fraction1:16;
        unsigned int fraction0: 4;
        unsigned int exponent :11;
        unsigned int sign     : 1;
#endif
    } number;

    struct 
    {
#if (CYG_DOUBLE_BYTEORDER == CYG_MSBFIRST) // Big endian
        unsigned int function1:16;
        unsigned int function0:3;
        unsigned int quiet:1;
        unsigned int exponent: 11;
        unsigned int sign : 1;
        unsigned int function3:16;
        unsigned int function2:16;
#else
        unsigned int function3:16;
        unsigned int function2:16;
        unsigned int function1:16;
        unsigned int function0:3;
        unsigned int quiet:1;
        unsigned int exponent: 11;
        unsigned int sign : 1;
#endif
    } nan;

    struct 
    {
#if (CYG_DOUBLE_BYTEORDER == CYG_MSBFIRST) // Big endian
        cyg_uint32 msw;
        cyg_uint32 lsw;
#else
        cyg_uint32 lsw;
        cyg_uint32 msw;
#endif
    } parts;
    
} Cyg_libm_ieee_double_shape_type;


typedef union
{
    cyg_int32 asi32;
  
    float value;

    struct 
    {
        unsigned int fraction0: 7;
        unsigned int fraction1: 16;
        unsigned int exponent: 8;
        unsigned int sign : 1;
    } number;

    struct 
    {
        unsigned int function1:16;
        unsigned int function0:6;
        unsigned int quiet:1;
        unsigned int exponent:8;
        unsigned int sign:1;
    } nan;

} Cyg_libm_ieee_float_shape_type;

#endif // little-endian

typedef enum {
    ADD_INSN=0,       // 0
    SUB_INSN,
    MUL_INSN,
    DIV_INSN,
    SQRT_INSN,
    ABS_INSN,         // 5
    MOV_INSN,
    NEG_INSN,
    ROUNDL_INSN,
    TRUNCL_INSN,
    CEILL_INSN,       // 10
    FLOORL_INSN,
    ROUNDW_INSN,
    TRUNCW_INSN,
    CEILW_INSN,
    FLOORW_INSN,      // 15
    // ...
    CVTS_INSN=32,
    CVTD_INSN,
    // ...
    CVTW_INSN=36,
    CVTL_INSN,
    // ...
    // 
    // 48-63 are floating point compare - treated separately
    
} fp_operation;

typedef enum {
    S_FORMAT=16,
    D_FORMAT=17,
    W_FORMAT=20,
    L_FORMAT=21
} fp_format;

// FUNCTIONS

#define issubnormal(_x_) ((_x_)->number.exponent == 0)

// This function returns non-zero if the exception has been handled
// successfully.

// FIXME: Arguably we should raise underflow exceptions in some of the cases
// below e.g. sqrt(subnormal). And perhaps we should round appropriately to
// +/- 2^^Emin if round to +/- infinity is enabled, as per the FS bit. Not sure.

externC cyg_uint8
cyg_hal_mips_process_fpe( HAL_SavedRegisters *regs )
{
    CYG_WORD insn;
    CYG_HAL_FPU_REG *srcreg1, *srcreg2, *dstreg;
    cyg_uint8 handled=0;
    fp_format format;
    cyg_uint8 fp64bit=0;             // true if format is 64bit, false if 32bit
    cyg_uint8 fixedpoint=0;          // true if format is fixed point, false if
                                     // floating point
    cyg_uint8 computational_insn=1;  // computational FP instruction
    cyg_bool delay_slot;             // did it happen in a delay slot

    CYG_REPORT_FUNCNAMETYPE("cyg_hal_mips_process_fpe", "returning %d");

    CYG_CHECK_DATA_PTR( regs,
                    "cyg_hal_mips_process_fpe() called with invalid regs ptr");

    CYG_PRECONDITION( (regs->vector>>2) == CYGNUM_HAL_VECTOR_FPE,
                      "Asked to process non-FPE exception");

    // First of all, we only handle the unimplemented operation exception
    // here, so if we don't have that, we just exit
    if ((regs->fcr31 & FCR31_CAUSE_E) == 0) {
        CYG_REPORT_RETVAL(0);
        return 0;
    }

    // Get the contents of the instruction that caused the exception. This
    // may have been in a branch delay slot however, so we have to check
    // the BD bit in the cause register first.
    if (regs->cause & CAUSE_BD) {
        insn = *(((CYG_WORD *) regs->pc) + 1);
        delay_slot = true;
    } else {
        insn = *(CYG_WORD *) regs->pc;
        delay_slot = false;
    }

    CYG_TRACE2(true, "exception at pc %08x containing %08x", regs->pc, insn);

    CYG_ASSERT( (insn>>26) == 0x11,
                "Instruction at pc doesn't have expected opcode COP1");

    // Determine the format
    format = (insn >> 21) & 0x1f;

    switch (format)
    {
    case S_FORMAT:
        break;
    case D_FORMAT:
        fp64bit++;
        break;
    case W_FORMAT:
        fixedpoint++;
        break;
    case L_FORMAT:
        fixedpoint++;
        fp64bit++;
        break;
    default:
        computational_insn=0;
        break;
    } // switch

    // This module only emulates computational floating point instructions
    if (computational_insn && !fixedpoint) {

        // Decode the registers used
        dstreg  = &regs->f[ (insn >>  6) & 0x1f ];
        srcreg1 = &regs->f[ (insn >> 11) & 0x1f ];
        srcreg2 = &regs->f[ (insn >> 16) & 0x1f ];
    
        // Determine the operation requested
        switch (insn & 0x3f)
        {
        case ADD_INSN:
        case SUB_INSN:
        case MUL_INSN:
        case DIV_INSN:

            if (fp64bit) {
                Cyg_libm_ieee_double_shape_type *s1, *s2;

                s1 = (Cyg_libm_ieee_double_shape_type *)srcreg1;
                s2 = (Cyg_libm_ieee_double_shape_type *)srcreg2;

                if (issubnormal(s1)) {  // flush to 0 and restart
                    s1->value=0.0;
                    handled++;
                }

                // We could try flushing both to 0 at the same time, but
                // that's inadvisable if both numbers are very small.
                // Particularly if this is DIV_INSN, when we could therefore
                // get 0/0 even when the program explicitly checked for
                // denominator != 0. That's also why we check s1 first.

                else if (issubnormal(s2)) {  // flush to 0 and restart
                    s2->value=0.0;
                    handled++;
                }
            } else { // 32-bit
                Cyg_libm_ieee_float_shape_type *s1, *s2;

                s1 = (Cyg_libm_ieee_float_shape_type *)srcreg1;
                s2 = (Cyg_libm_ieee_float_shape_type *)srcreg2;

                if (issubnormal(s1)) {  // flush to 0 and restart
                    s1->value=0.0;
                    handled++;
                }
                else if (issubnormal(s2)) {  // flush to 0 and restart
                    s2->value=0.0;
                    handled++;
                }
            }
            break;

        case SQRT_INSN:
            if (fp64bit) {
                Cyg_libm_ieee_double_shape_type *d, *s;

                d = (Cyg_libm_ieee_double_shape_type *)dstreg;
                s = (Cyg_libm_ieee_double_shape_type *)srcreg1;

                if (issubnormal(s)) {  // Sqrt of something tiny is 0
                    // if this is a delay slot, we can't restart properly
                    // so clear the source register instead
                    if (delay_slot) 
                        s->value=0.0; 
                    else {
                        d->value=0.0;
                        regs->pc += 4; // We've dealt with this so move on
                    }
                    handled++;
                }

            } else { // 32-bit
                Cyg_libm_ieee_float_shape_type *d, *s;

                d = (Cyg_libm_ieee_float_shape_type *)dstreg;
                s = (Cyg_libm_ieee_float_shape_type *)srcreg1;

                if (issubnormal(s)) {  // Sqrt of something tiny is 0
                    // if this is a delay slot, we can't restart properly
                    // so clear the source register instead
                    if (delay_slot) 
                        s->value=0.0; 
                    else {
                        d->value=0.0;
                        regs->pc += 4; // We've dealt with this so move on
                    }
                    handled++;
                }
            }
            break;

        case ABS_INSN:
            // We may as well do this right if we can
            if (fp64bit) {
                Cyg_libm_ieee_double_shape_type *d, *s;

                d = (Cyg_libm_ieee_double_shape_type *)dstreg;
                s = (Cyg_libm_ieee_double_shape_type *)srcreg1;

                // if this is a delay slot, we can't restart properly
                // so clear the source register instead
                if (delay_slot) {
                    s->value=0.0;
                } else {
                    d->asi64 = s->asi64;
                    d->number.sign = 0;
                    regs->pc += 4;
                }
            } else { // 32-bit
                Cyg_libm_ieee_float_shape_type *d, *s;

                d = (Cyg_libm_ieee_float_shape_type *)dstreg;
                s = (Cyg_libm_ieee_float_shape_type *)srcreg1;

                // if this is a delay slot, we can't restart properly
                // so clear the source register instead
                if (delay_slot) {
                    s->value=0.0;
                } else {
                    d->asi32 = s->asi32;
                    d->number.sign = 0;
                    regs->pc += 4;
                }
            }
            handled++;
            break;

        case NEG_INSN:
            // We may as well do this right if we can
            if (fp64bit) {
                Cyg_libm_ieee_double_shape_type *d, *s;

                d = (Cyg_libm_ieee_double_shape_type *)dstreg;
                s = (Cyg_libm_ieee_double_shape_type *)srcreg1;

                // if this is a delay slot, we can't restart properly
                // so clear the source register instead
                if (delay_slot) {
                    s->value=0.0;
                } else {
                    d->asi64 = s->asi64;
                    d->number.sign = s->number.sign ? 0 : 1;
                    regs->pc += 4;
                }
            } else { // 32-bit
                Cyg_libm_ieee_float_shape_type *d, *s;

                d = (Cyg_libm_ieee_float_shape_type *)dstreg;
                s = (Cyg_libm_ieee_float_shape_type *)srcreg1;

                // if this is a delay slot, we can't restart properly
                // so clear the source register instead
                if (delay_slot) {
                    s->value=0.0;
                } else {
                    d->asi32 = s->asi32;
                    d->number.sign = s->number.sign ? 0 : 1;
                    regs->pc += 4;
                }
            }
            handled++;
            break;

        default:
            break;
        } // switch

        // As well as all the other opcodes in the enum, there are also all
        // the floating point compare operations between 48 and 63
    }
    
    if (handled) {
        // We must clear the cause and flag bits before restoring FPCR31
        regs->fcr31 &= ~(FCR31_CAUSE_E | FCR31_CAUSE_V | FCR31_CAUSE_Z |
                         FCR31_CAUSE_O | FCR31_CAUSE_U | FCR31_CAUSE_I |
                         FCR31_FLAGS_V | FCR31_FLAGS_Z |
                         FCR31_FLAGS_O | FCR31_FLAGS_U | FCR31_FLAGS_I);

    }

    CYG_REPORT_RETVAL(handled);
    return handled;
} // cyg_hal_mips_process_fpe()

#endif // ifdef CYGHWR_HAL_MIPS_FPU

// EOF mipsfp.c
