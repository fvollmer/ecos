#ifndef CYGONCE_DEVS_FLASH_AMD_AM29XXXXX_INL
#define CYGONCE_DEVS_FLASH_AMD_AM29XXXXX_INL
//==========================================================================
//
//      am29xxxxx.inl
//
//      AMD AM29xxxxx series flash driver
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000, 2001 Red Hat, Inc.
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas, jskov
// Date:         2001-02-21
// Purpose:      
// Description:  AMD AM29xxxxx series flash device driver
// Notes:        While the parts support sector locking, some only do so
//               via crufty magic and the use of programmer hardware
//               (specifically by applying 12V to one of the address
//               pins) so the driver does not support write protection.
//
// FIXME:        Should support SW locking on the newer devices.
//
// FIXME:        Figure out how to do proper error checking when there are
//               devices in parallel. Presently the driver will return
//               driver timeout error on device errors which is not very
//               helpful.
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>
#include <pkgconf/devs_flash_amd_am29xxxxx.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_cache.h>
#include CYGHWR_MEMORY_LAYOUT_H

#define  _FLASH_PRIVATE_
#include <cyg/io/flash.h>


//----------------------------------------------------------------------------
// Common device details.
#define FLASH_Read_ID                   FLASHWORD( 0x90 )
#define FLASH_WP_State                  FLASHWORD( 0x90 )
#define FLASH_Reset                     FLASHWORD( 0xFF )
#define FLASH_Program                   FLASHWORD( 0xA0 )
#define FLASH_Block_Erase               FLASHWORD( 0x30 )

#define FLASH_Data                      FLASHWORD( 0x80 ) // Data complement
#define FLASH_Busy                      FLASHWORD( 0x40 ) // "Toggle" bit
#define FLASH_Err                       FLASHWORD( 0x20 )
#define FLASH_Sector_Erase_Timer        FLASHWORD( 0x08 )

#define FLASH_unlocked                  FLASHWORD( 0x00 )

#define FLASH_Setup_Addr1               (0x555)
#define FLASH_Setup_Addr2               (0x2AA)
#define FLASH_Setup_Code1               FLASHWORD( 0xAA )
#define FLASH_Setup_Code2               FLASHWORD( 0x55 )
#define FLASH_Setup_Erase               FLASHWORD( 0x80 )

// Platform code must define the below
// #define CYGNUM_FLASH_INTERLEAVE      : Number of interleaved devices (in parallel)
// #define CYGNUM_FLASH_SERIES          : Number of devices in series
// #define CYGNUM_FLASH_WIDTH           : Width of devices on platform
// #define CYGNUM_FLASH_BASE            : Address of first device

#define CYGNUM_FLASH_BLANK              (1)

#ifndef FLASH_P2V
# define FLASH_P2V( _a_ ) ((volatile flash_data_t *)((CYG_ADDRWORD)(_a_)))
#endif
#ifndef CYGHWR_FLASH_AM29XXXXX_PLF_INIT
# define CYGHWR_FLASH_AM29XXXXX_PLF_INIT()
#endif

//----------------------------------------------------------------------------
// Now that device properties are defined, include magic for defining
// accessor type and constants.
#include <cyg/io/flash_dev.h>

//----------------------------------------------------------------------------
// Information about supported devices
typedef struct flash_dev_info {
    flash_data_t device_id;
    cyg_uint32   block_size;
    cyg_int32    block_count;
    cyg_uint32   base_mask;
    cyg_uint32   device_size;
    cyg_bool     bootblock;
    cyg_uint32   bootblocks[12];         // 0 is bootblock offset, 1-11 sub-sector sizes (or 0)
} flash_dev_info_t;

static const flash_dev_info_t* flash_dev_info;
static const flash_dev_info_t supported_devices[] = {
#include <cyg/io/flash_am29xxxxx_parts.inl>
};
#define NUM_DEVICES (sizeof(supported_devices)/sizeof(flash_dev_info_t))

//----------------------------------------------------------------------------
// Functions that put the flash device into non-read mode must reside
// in RAM.
void flash_query(void* data) __attribute__ ((section (".2ram.flash_query")));
int  flash_erase_block(void* block, unsigned int size) 
    __attribute__ ((section (".2ram.flash_erase_block")));
int  flash_program_buf(void* addr, void* data, int len)
    __attribute__ ((section (".2ram.flash_program_buf")));

//----------------------------------------------------------------------------
// Flash Query
//
// Only reads the manufacturer and part number codes for the first
// device(s) in series. It is assumed that any devices in series
// will be of the same type.

void
flash_query(void* data)
{
    volatile flash_data_t *ROM;
    volatile flash_data_t *f_s1, *f_s2;
    flash_data_t* id = (flash_data_t*) data;
    flash_data_t w;

    ROM = (flash_data_t*) CYGNUM_FLASH_BASE;
    f_s1 = FLASH_P2V(ROM+FLASH_Setup_Addr1);
    f_s2 = FLASH_P2V(ROM+FLASH_Setup_Addr2);

    w = *(FLASH_P2V(ROM));

    *f_s1 = FLASH_Setup_Code1;
    *f_s2 = FLASH_Setup_Code2;
    *f_s1 = FLASH_Read_ID;

    // Manufacturers' code
    id[0] = *(FLASH_P2V(ROM));
    // Part number
    id[1] = *(FLASH_P2V(ROM+1));

    *(FLASH_P2V(ROM)) = FLASH_Reset;

    // Stall, waiting for flash to return to read mode.
    while (w != *(FLASH_P2V(ROM)));
}

//----------------------------------------------------------------------------
// Initialize driver details
int
flash_hwr_init(void)
{
    flash_data_t id[2];
    int i;

    CYGHWR_FLASH_AM29XXXXX_PLF_INIT();

    flash_dev_query(id);

    // Look through table for device data
    flash_dev_info = supported_devices;
    for (i = 0; i < NUM_DEVICES; i++) {
        if (flash_dev_info->device_id == id[1])
            break;
        flash_dev_info++;
    }

    // Did we find the device? If not, return error.
    if (NUM_DEVICES == i)
        return FLASH_ERR_DRV_WRONG_PART;

    // Hard wired for now
    flash_info.block_size = flash_dev_info->block_size;
    flash_info.blocks = flash_dev_info->block_count;
    flash_info.start = (void *)CYGNUM_FLASH_BASE;
    flash_info.end = (void *)(CYGNUM_FLASH_BASE+ (flash_dev_info->device_size * CYGNUM_FLASH_SERIES));
    return FLASH_ERR_OK;
}

//----------------------------------------------------------------------------
// Map a hardware status to a package error
int
flash_hwr_map_error(int e)
{
    return e;
}


//----------------------------------------------------------------------------
// See if a range of FLASH addresses overlaps currently running code
bool
flash_code_overlaps(void *start, void *end)
{
    extern unsigned char _stext[], _etext[];

    return ((((unsigned long)&_stext >= (unsigned long)start) &&
             ((unsigned long)&_stext < (unsigned long)end)) ||
            (((unsigned long)&_etext >= (unsigned long)start) &&
             ((unsigned long)&_etext < (unsigned long)end)));
}

//----------------------------------------------------------------------------
// Erase Block

int
flash_erase_block(void* block, unsigned int size)
{
    volatile flash_data_t* ROM;
    volatile flash_data_t* b_p = (flash_data_t*) block;
    volatile flash_data_t *b_v;
    volatile flash_data_t *f_s1, *f_s2;
    int timeout = 50000;
    int len, len_ix = 1;
    int res = FLASH_ERR_OK;
    flash_data_t state;
    cyg_bool bootblock;

    ROM = (volatile flash_data_t*)((unsigned long)block & flash_dev_info->base_mask);
    f_s1 = FLASH_P2V(ROM+FLASH_Setup_Addr1);
    f_s2 = FLASH_P2V(ROM+FLASH_Setup_Addr2);

    // Is this the boot sector?
    bootblock = (flash_dev_info->bootblock &&
                 (flash_dev_info->bootblocks[0] == ((unsigned long)block - (unsigned long)ROM)));
    if (bootblock) {
        len = flash_dev_info->bootblocks[len_ix++];
    } else {
        len = flash_dev_info->block_size;
    }

    while (len > 0) {
        // First check whether the block is protected
        *f_s1 = FLASH_Setup_Code1;
        *f_s2 = FLASH_Setup_Code2;
        *f_s1 = FLASH_WP_State;
        state = *FLASH_P2V(b_p+2);
        *FLASH_P2V(ROM) = FLASH_Reset;

        if (FLASH_unlocked != state)
            return FLASH_ERR_PROTECT;

        b_v = FLASH_P2V(b_p);
    
        // Send erase block command - six step sequence
        *f_s1 = FLASH_Setup_Code1;
        *f_s2 = FLASH_Setup_Code2;
        *f_s1 = FLASH_Setup_Erase;
        *f_s1 = FLASH_Setup_Code1;
        *f_s2 = FLASH_Setup_Code2;
        *b_v = FLASH_Block_Erase;

        // Now poll for the completion of the sector erase timer (50us)
        timeout = 5000000;              // how many retries?
        while (true) {
            state = *b_v;
            if ((state & FLASH_Sector_Erase_Timer) == 0) break;

            if (--timeout == 0) {
                res = FLASH_ERR_DRV_TIMEOUT;
                break;
            }
        }

        // Then wait for erase completion.
        if (FLASH_ERR_OK == res) {
            timeout = 5000000;
            while (true) {
                state = *b_v;
                if (FLASH_BlankValue == state) {
                    break;
                }

                // Don't check for FLASH_Err here since it will fail
                // with devices in parallel because these may finish
                // at different times.

                if (--timeout == 0) {
                    res = FLASH_ERR_DRV_TIMEOUT;
                    break;
                }
            }
        }

        if (FLASH_ERR_OK != res)
            *FLASH_P2V(ROM) = FLASH_Reset;

        // Verify erase operation
        while (len > 0) {
            b_v = FLASH_P2V(b_p++);
            if (*b_v != FLASH_BlankValue) {
                // Only update return value if erase operation was OK
                if (FLASH_ERR_OK == res) res = FLASH_ERR_DRV_VERIFY;
                return res;
            }
            len -= sizeof(*b_p);
        }

        if (bootblock)
            len = flash_dev_info->bootblocks[len_ix++];
    }
    return res;
}

//----------------------------------------------------------------------------
// Program Buffer

int
flash_program_buf(void* addr, void* data, int len)
{
    volatile flash_data_t* ROM;
    volatile flash_data_t* data_ptr = (volatile flash_data_t*) data;
    volatile flash_data_t* addr_v;
    volatile flash_data_t* addr_p = (flash_data_t*) addr;
    volatile flash_data_t *f_s1, *f_s2;

    int timeout;
    int res = FLASH_ERR_OK;

    while (len > 0) {
        int state;

        // Note: Should really only do the address calculations on
        // section/device boundaries - but then, we're likely to be
        // spinning for a while down below anyway, so no point in
        // making the loop more complicated.

        // Base address of device(s) being programmed.
        ROM = (volatile flash_data_t*)((unsigned long)addr_p & flash_dev_info->base_mask);
        f_s1 = FLASH_P2V(ROM+FLASH_Setup_Addr1);
        f_s2 = FLASH_P2V(ROM+FLASH_Setup_Addr2);
        addr_v = FLASH_P2V(addr_p++);

        // Program data [byte] - 4 step sequence
        *f_s1 = FLASH_Setup_Code1;
        *f_s2 = FLASH_Setup_Code2;
        *f_s1 = FLASH_Program;
        *addr_v = *data_ptr;

        timeout = 5000000;
        while (true) {
            state = *addr_v;
            if (*data_ptr == state) {
                break;
            }

            // Can't check for FLASH_Err since it'll fail in parallel
            // configurations.

            if (--timeout == 0) {
                res = FLASH_ERR_DRV_TIMEOUT;
                break;
            }
        }

        if (FLASH_ERR_OK != res)
            *FLASH_P2V(ROM) = FLASH_Reset;

        if (*addr_v != *data_ptr++) {
            // Only update return value if erase operation was OK
            if (FLASH_ERR_OK == res) res = FLASH_ERR_DRV_VERIFY;
            break;
        }
        len -= sizeof(*data_ptr);
    }

    // Ideally, we'd want to return not only the failure code, but also
    // the address/device that reported the error.
    return res;
}
#endif // CYGONCE_DEVS_FLASH_AMD_AM29XXXXX_INL
