#ifndef CYGONCE_HAL_PLATFORM_EXTRAS_H
#define CYGONCE_HAL_PLATFORM_EXTRAS_H

/*=============================================================================
//
//      hal_platform_extras.h
//
//      Platform specific support for HAL (assembly code)
//
//=============================================================================
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas, msalter
// Date:         1999-12-02
// Purpose:      Cogent CMA222 platform extras, in particular MMU tables
// Description: 
// Usage:       #include <cyg/hal/hal_platform_extras.h>
//
//####DESCRIPTIONEND####
//
//===========================================================================*/

        .section ".mmu_tables","aw"

#define NEXT_PAGE                \
        .long   PTE                     ; \
        .set    PTE,PTE+MMU_PAGE_SIZE

_PT_0x0:
        .set    PTE,DRAM_PA_START|MMU_L2_TYPE_Small\
                    |MMU_AP_Any|MMU_Bufferable|MMU_Cacheable
        .rept   (0x2000000)/0x1000
        NEXT_PAGE
        .endr
        .balign 0x0400

// Too bad these macros don't work ['as' bug?] :-(

#define NEXT_SECTION                                            \
        .long   PTE+OFF                                        ;\
        .set    OFF,OFF+(MMU_SECTION_SIZE/MMU_PAGE_SIZE)*4

#define FILL_SEGMENT_(base)                     \
        .set    PTE,base                        \
        .set    OFF,0x00000000                  \
        .rept   0x10000000/MMU_SECTION_SIZE     \
            NEXT_SECTION                        \
        .endr

#define OFF     ((MMU_SECTION_SIZE/MMU_PAGE_SIZE)*4)

#define FILL_8M_SEGMENT(base,seg)               \
        .long   base+(OFF*(seg+0x00));          \
        .long   base+(OFF*(seg+0x01));          \
        .long   base+(OFF*(seg+0x02));          \
        .long   base+(OFF*(seg+0x03));          \
        .long   base+(OFF*(seg+0x04));          \
        .long   base+(OFF*(seg+0x05));          \
        .long   base+(OFF*(seg+0x06));          \
        .long   base+(OFF*(seg+0x07));          \
        .long   base+(OFF*(seg+0x08));

#define FILL_16M_SEGMENT(base,seg)              \
        .long   base+(OFF*(seg+0x00));          \
        .long   base+(OFF*(seg+0x01));          \
        .long   base+(OFF*(seg+0x02));          \
        .long   base+(OFF*(seg+0x03));          \
        .long   base+(OFF*(seg+0x04));          \
        .long   base+(OFF*(seg+0x05));          \
        .long   base+(OFF*(seg+0x06));          \
        .long   base+(OFF*(seg+0x07));          \
        .long   base+(OFF*(seg+0x08));          \
        .long   base+(OFF*(seg+0x09));          \
        .long   base+(OFF*(seg+0x0A));          \
        .long   base+(OFF*(seg+0x0B));          \
        .long   base+(OFF*(seg+0x0C));          \
        .long   base+(OFF*(seg+0x0D));          \
        .long   base+(OFF*(seg+0x0E));          \
        .long   base+(OFF*(seg+0x0F));

#define FILL_32M_SEGMENT(base,seg)              \
        FILL_16M_SEGMENT(base,0x00);            \
        FILL_16M_SEGMENT(base,0x10);

#define FILL_256M_SEGMENT(base)                 \
        FILL_16M_SEGMENT(base,0x00);            \
        FILL_16M_SEGMENT(base,0x10);            \
        FILL_16M_SEGMENT(base,0x20);            \
        FILL_16M_SEGMENT(base,0x30);            \
        FILL_16M_SEGMENT(base,0x40);            \
        FILL_16M_SEGMENT(base,0x50);            \
        FILL_16M_SEGMENT(base,0x60);            \
        FILL_16M_SEGMENT(base,0x70);            \
        FILL_16M_SEGMENT(base,0x80);            \
        FILL_16M_SEGMENT(base,0x90);            \
        FILL_16M_SEGMENT(base,0xA0);            \
        FILL_16M_SEGMENT(base,0xB0);            \
        FILL_16M_SEGMENT(base,0xC0);            \
        FILL_16M_SEGMENT(base,0xD0);            \
        FILL_16M_SEGMENT(base,0xE0);            \
        FILL_16M_SEGMENT(base,0xF0);

        .balign 0x4000
_MMU_table:     
        FILL_32M_SEGMENT(_PT_0x0+MMU_L1_TYPE_Page,0x00)
        FILL_32M_SEGMENT(MMU_L1_TYPE_Fault,0x00)
        FILL_32M_SEGMENT(PCIMEM_PA|MMU_L1_TYPE_Section|MMU_AP_Any)
        FILL_32M_SEGMENT(MMU_L1_TYPE_Fault,0x00)
        FILL_32M_SEGMENT(MMU_L1_TYPE_Fault,0x00)
        FILL_32M_SEGMENT(MMU_L1_TYPE_Fault,0x00)
        FILL_32M_SEGMENT(MMU_L1_TYPE_Fault,0x00)
        FILL_8M_SEGMENT(ROM_PA|MMU_L1_TYPE_Section|MMU_AP_Any)
        FILL_16M_SEGMENT(MBIO_PA|MMU_L1_TYPE_Section|MMU_AP_Any)
        FILL_8M_SEGMENT(FLASH_PA|MMU_L1_TYPE_Section|MMU_AP_Any)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)
        FILL_256M_SEGMENT(MMU_L1_TYPE_Fault)

#endif // CYGONCE_HAL_PLATFORM_EXTRAS_H
