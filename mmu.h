#ifndef __MMU_H__
#define __MMU_H__

#include "stdint.h"
#include "sysinfo.h"
#include "task.h"

#define TTB_MASK           0xFFFFC000
#define MMU_ENABLE         0x1
#define ACHECK_ENABLE      0x2
#define ICACHE_ENABLE      0x1000
#define DCACHE_ENABLE      0x4
#define PREDICT_ENABLE     0x800
#define CP15B_ENABLE       0x20

#define SCTLR_MMU_OPTION  \
        (MMU_ENABLE     | \
         ICACHE_ENABLE  | \
         DCACHE_ENABLE  | \
         PREDICT_ENABLE | \
         CP15B_ENABLE)

/* First Level Descriptor Type */
#define FLD_TYPE_MASK      0x3
#define FLD_FAULT_TYPE     0x0
#define FLD_PGTBL_TYPE     0x1
#define FLD_SECTION_TYPE   0x2
#define FLD_SSECTION_TYPE  0x2

/* First Level Descriptor Domain */
#define FLD_DOMAIN_MASK    0x1E0

/* First Level Descriptor Section */
#define FLD_SBA_MASK       0xFFF00000

/* First Level Descriptor Access Policy */
#define FLD_AP_MASK        0x8C00
#define FLD_NO_ACCESS      0x0000
#define FLD_PLV_ACCESS     0x0400
#define FLD_NO_USR_WRITE   0x0800
#define FLD_FULL_ACCESS    0x0C00
#define FLD_RESERVED       0x8000
#define FLD_PLV_READ_ONLY  0x8400
#define FLD_READ_ONLY      0x8C00

#define FLD_DEVICE_MEMORY  0x0002

/* Translation Table Entry size */
#define SUPER_SECTION_SIZE 0x01000000 // 16 * 1024 * 1024 = 16MB
#define SECTION_SIZE       0x00100000 //  1 * 1024 * 1024 =  1MB
#define LARGE_PAGE         0x00010000 // 64        * 1024 = 64KB
#define SMALL_PAGE         0x00001000 //  4        * 1024 =  4KB

/* Translation Table size */
#define SECTION_TBL_SIZE   0x00004000 // 16        * 1024 = 16KB

/* Section Memory Attribute */
#define SHAREABLE_SECTION  0x10000
#define SHAREABLE_DEV      0x00004
#define WT_NWA             0x00008
#define WB_WA              0x0100C
#define WB_NWA             0x0000C

#define invalidate_icache() \
        __asm__ volatile ("mcr p15,0,%0,c7,c5,0" :: "r"(0) : "memory")

#define ISB() __asm__ volatile ("isb" ::: "memory")
#define DSB() __asm__ volatile ("dsb" ::: "memory")
#define DMB() __asm__ volatile ("dmb" ::: "memory")
#define flush_btcache()     \
        __asm__ volatile ("mcr p15,0,%0,c7,c5,6" :: "r"(0) : "memory")

void     init_mmu(void);
uint32_t is_mmu_enabled(void);

uint32_t get_ttbr0(void);
uint32_t get_ttba0(void);
uint32_t get_ttbr1(void);
uint32_t get_ttba1(void);

uint32_t mmu_fault(uint32_t va, uint32_t pa, uint32_t ttba);
uint32_t mmu_section(uint32_t va, uint32_t pa, uint32_t flags, uint32_t ttba);
uint32_t cre_usr_tbl(uint32_t ttba);

void start_mmu(uint32_t tlb_base_address, uint32_t sctlr_flags,
                const uint32_t cpuid, const uint32_t pm);
void start_mmu_no_lock(const uint32_t cpuid, const uint32_t pm);

void stop_mmu(void);

void invalidate_caches(void);

void invalidate_tlbs(uint32_t asid);
void invalidate_all_tlbs(void);

void sync_ttba_change(uint32_t new_ttba, const uint32_t cpuid,
                const uint32_t pm);
void sync_ttba_change_no_lock(uint32_t new_ttba);
void sync_user_ttba_change(TCB *task, const uint32_t cpuid, const uint32_t pm);

void amp_tbl_setup(const uint32_t     ttba,
                   const ram_area_t  *kram_area,
                   const ram_area_t  *uram_area,
                   const heap_area_t *kheap_area,
                   const uint32_t     sp_bottom);

void smp_tbl_setup(const uint32_t     ttba,
                   const ram_area_t  *kram_area,
                   const ram_area_t  *uram_area,
                   const heap_area_t *kheap_area,
                   const uint32_t     sp_bottom,
                   const uint32_t     num_cpu);

void set_asid(const uint32_t asid);

#endif /* __MMU_H__ */
