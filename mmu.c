#include "hwio.h"
#include "mmu.h"
#include "sysinfo.h"
#include "uart.h"
#include "boot.h"
#include "memory.h"
#include "interrupt.h"

extern volatile int busy_wait(int32_t count);

void init_mmu(void)
{
        //if (is_mmu_enabled())
        stop_mmu(); // Disable MMU, I-cache and D-cache

        invalidate_caches();
        invalidate_all_tlbs();

        // Set SMP into ACTLR
        uint32_t actlr;
        __asm__ volatile("mrc p15,0,%0,c1,c0,1" : "=r"(actlr));
        actlr |= (1 << 6);
        __asm__ volatile("mcr p15,0,%0,c1,c0,1" :: "r"(actlr) : "memory");

        // set domain (CORE has the right to full access)
        __asm__ volatile("mcr p15,0,%0,c3,c0,0" :: "r"(0xFFFFFFFF) : "memory");

        // Always use TTBR0
        uint32_t ttbcr;
        __asm__ volatile("mcr p15,0,%0,c2,c0,2" :: "r"(0));

        DSB();
        ISB();
}

uint32_t is_mmu_enabled(void)
{
        return (_get_sctlr() & 0x1);
}

uint32_t get_ttbr0(void)
{
        uint32_t ttbr0;
        __asm__ volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttbr0));
        return ttbr0;
}

uint32_t get_ttbr1(void)
{
        uint32_t ttbr1;
        __asm__ volatile("mrc p15, 0, %0, c2, c0, 1" : "=r"(ttbr1));
        return ttbr1;
}

uint32_t get_ttba0(void)
{
        return (get_ttbr0() & TTB_MASK);
}

uint32_t get_ttba1(void)
{
        return (get_ttbr1() & TTB_MASK);
}

uint32_t mmu_section(uint32_t va, uint32_t pa, uint32_t flags, uint32_t ttba)
{
        uint32_t tblid    = va >> 20;
        uint32_t l1descpa = ttba | (tblid << 2);
        uint32_t l1desc   = (pa & FLD_SBA_MASK) |
                            flags               |
                            FLD_SECTION_TYPE;
        *(uint32_t *)l1descpa = l1desc;
        //uart_puts("ms: ");hexstring(l1descpa);uart_puts(": ");hexstring(l1desc);uart_puts("\n");

        return 0;
}

uint32_t mmu_fault(uint32_t va, uint32_t pa, uint32_t ttba)
{
        uint32_t tblid    = va >> 20;
        uint32_t l1descpa = ttba | (tblid << 2);
        uint32_t l1desc   = (pa & FLD_SBA_MASK) |
                            FLD_FAULT_TYPE;
        *(uint32_t *)l1descpa = l1desc;
        //uart_puts("ms: ");hexstring(l1descpa);uart_puts(": ");hexstring(l1desc);uart_puts("\n");

        return 0;
}

uint32_t cre_usr_tbl(uint32_t ttba)
{
        uint32_t va;
        for (va = 0;; va += SECTION_SIZE) {
                if        (va == ttba) {
                        mmu_section(va,
                                    ttba,
                                    SHAREABLE_SECTION | WB_WA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va < ttba) {
                        /* Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WB_WA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= SHARED_BOTTOM && va < SHARED_LIMIT) {
                        /* Shareable O&I Write-Through, NWA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WT_NWA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= PERIPH_BOTTOM && va < PERIPH_LIMIT) {
                        /* Set shareable device memory */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_DEV | FLD_FULL_ACCESS,
                                    ttba);
                } else {
                        mmu_fault(va, va, ttba);
                }
                if (va == 0xFFF00000) break;
        }
}

void start_mmu(uint32_t tlb_base_address, uint32_t sctlr_flags,
                const uint32_t cpuid, const uint32_t pm)
{
        // Set ttbr and enable mmu;
        sync_ttba_change(tlb_base_address, cpuid, pm);
        DSB();

        // Enable MMU
        uint32_t sctlr = _get_sctlr();
        sctlr |= sctlr_flags; // on MMU
        __asm__ volatile("mcr p15,0,%0,c1,c0,0"
                          :: "r"(sctlr) : "memory"
                        );
        DSB();
        ISB();
        uart_puts("Enable MMU\n");
}

void start_mmu_no_lock(const uint32_t cpuid, const uint32_t pm)
{
        if (is_mmu_enabled())
                stop_mmu();

        const SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        // Set ttbr and enable mmu;
        sync_ttba_change_no_lock(ssi->ttba);

        register_value_puts("CPUID: ", cpuid);
        register_value_puts("CPSR: ", _get_cpsr());

        // Enable MMU
        uint32_t sctlr = _get_sctlr();
        sctlr |= SCTLR_MMU_OPTION; // on MMU
        register_value_puts("Updated SCTLR: ", sctlr);
        __asm__ volatile("mcr p15,0,%0,c1,c0,0"
                          :: "r"(sctlr) : "memory"
                        );
        DSB();
        ISB();
        uart_puts("Enable MMU\n");
}

void stop_mmu(void)
{
        __asm__ volatile("push {r2}\n"
                         "mrc p15,0,r2,c1,c0,0\n"
                         "bic r2,%0\n"
                         "mcr p15,0,r2,c1,c0,0\n"
                         "pop {r2}"
                          :: "r"(SCTLR_MMU_OPTION)
                          :  "memory"
                        );
        DSB();
        ISB();
}

void invalidate_caches(void)
{
        invalidate_data_cache_l10_only();
        invalidate_icache();
        flush_btcache();
        DSB();
        ISB();
}

void invalidate_tlbs(uint32_t asid)
{
        // Invalidate unitifed tlbs by asid
        __asm__ volatile("mcr p15,0,%0,c8,c7,2" :: "r"(asid) : "memory");
        DSB();
        ISB();
}

void invalidate_all_tlbs(void)
{
        __asm__ volatile("mcr p15,0,%0,c8,c7,0" :: "r"(0) : "memory");
}


void sync_ttba_change(uint32_t new_ttba, const uint32_t cpuid,
                const uint32_t pm)
{
        DSB();
        uart_puts("dsb finished\n");

        __asm__ volatile("cpsid i");

        // Take and check ASID
        uart_puts("start to take asid\n");
        uint32_t asid = take_kernel_asid(cpuid, pm);
        register_value_puts("ASID: ", asid);
        if (asid < 0) {
                uart_puts("No ASID is available\n");
                __asm__ volatile("cpsie i");
                return;
        }

        // Set ASID RESERVED_ASID_NUM to change ttba and asid
        set_asid(RESERVED_ASID_NUM);

        // Set new TTBA to TTBA0/1
        __asm__ volatile("mcr p15,0,%0,c2,c0,0\n"
                         :: "r"(0x4a|new_ttba)
                        );
        __asm__ volatile("mcr p15,0,%0,c2,c0,1\n"
                         :: "r"(0x4a|new_ttba)
                        );
        ISB();

        // Set ASID
        set_asid(asid);

        // Eable interrupt
        __asm__ volatile("cpsie i");
}

void sync_ttba_change_no_lock(uint32_t new_ttba)
{
        DSB();

        // Disable interrupt
        __asm__ volatile("cpsid i");

        register_value_puts("PRE TTBA0: ", get_ttba0());

        // Take and check ASID
        uint32_t asid = RESERVED_ASID_NUM;
        if (asid < 0) {
                uart_puts("No ASID is available\n");
                __asm__ volatile("cpsie i");
                return;
        }

        // Set ASID RESERVED_ASID to change ttba and asid
        set_asid(RESERVED_ASID_NUM);

        // Set new TTBA to TTBA0/1
        __asm__ volatile("mcr p15,0,%0,c2,c0,0\n"
                         :: "r"(0x4a|new_ttba)
                        );
        __asm__ volatile("mcr p15,0,%0,c2,c0,1\n"
                         :: "r"(0x4a|new_ttba)
                        );
        ISB();

        // Set ASID
        set_asid(asid);

        invalidate_all_tlbs();
        invalidate_caches();
        register_value_puts("CURRENT TTBA0: ", get_ttba0());
        //invalidate_tlbs(asid);
        // Eable interrupt
        __asm__ volatile("cpsie i");
}

void sync_user_ttba_change(TCB *task, const uint32_t cpuid, const uint32_t pm)
{
        DSB();
        uart_puts("dsb finished\n");

        // Disable interrupt
        __asm__ volatile("cpsid i");

        // Take and check ASID
        uart_puts("start to take asid\n");
        uint32_t asid = set_user_asid(task, cpuid, pm);
        register_value_puts("ASID: ", asid);
        if (asid < 0) {
                uart_puts("No ASID is available\n");
                __asm__ volatile("cpsie i");
                return;
        }

        // Set ASID RESERVED_ASID to change ttba and asid
        __asm__ volatile("push {r2}\n"
                         "mrc p15,0,r2,c13,c0,1\n"
                         "and r2, r2, #0xFFFFFF00\n"
                         "mcr p15,0,r2,c13,c0,1\n"
                         "pop {r2}" ::: "memory");
        ISB();

        // Set new TTBA to TTBA0/1
        __asm__ volatile("mcr p15,0,%0,c2,c0,0\n"
                         :: "r"(0x4a|(task->ttba))
                        );
        __asm__ volatile("mcr p15,0,%0,c2,c0,1\n"
                         :: "r"(0x4a|(task->ttba))
                        );
        ISB();

        //  Set ASID
        __asm__ volatile("push {r2}\n"
                         "mrc p15,0,r2,c13,c0,1\n"
                         "and r2, r2, #0xFFFFFF00\n"
                         "orr r2, r2, %0\n"
                         "mcr p15,0,r2,c13,c0,1\n"
                         "pop {r2}"
                          :: "r"(asid) : "memory");
        DSB();
        ISB();

        // Eable interrupt
        __asm__ volatile("cpsie i");
}

void amp_tbl_setup(const uint32_t     ttba,
                   const ram_area_t  *kram_area,
                   const ram_area_t  *uram_area,
                   const heap_area_t *kheap_area,
                   const uint32_t     sp_bottom)
{
        //register_value_puts("amp translation table base address: ", ttba);
        uint32_t va;
        for (va = 0;; va += SECTION_SIZE) {
                if        (va >= 0x00000000       && va < SP_BASE) {
                        /* Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WB_WA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= sp_bottom        &&
                           va < sp_bottom + SECTION_SIZE) {
                        /* Non-Shareable O&I Write-Through, NWA */
                        mmu_section(va,
                                    va,
                                    WT_NWA | FLD_PLV_ACCESS,
                                    ttba);
                } else if (va >= SHARED_BOTTOM    && va < SHARED_LIMIT) {
                        /* Shareable O&I Write-Through, NWA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WT_NWA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= kram_area->bottom && va < kram_area->limit) {
                        /* Non-Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    WB_WA | FLD_PLV_ACCESS,
                                    ttba);
                } else if (va >= kheap_area->bottom && va < kheap_area->limit) {
                        /* Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WB_WA | FLD_PLV_ACCESS,
                                    ttba);
                } else if (va >= uram_area->bottom && va < uram_area->limit) {
                        /* Non-Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    WB_WA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= PERIPH_BOTTOM    && va < PERIPH_LIMIT) {
                        /* Set shareable device memory */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_DEV | FLD_FULL_ACCESS,
                                    ttba);
                } else {
                        mmu_fault(va, va, ttba);
                }
                if (va == 0xFFF00000) break;
        }
}

void smp_tbl_setup(const uint32_t     ttba,
                   const ram_area_t  *kram_area,
                   const ram_area_t  *uram_area,
                   const heap_area_t *kheap_area,
                   const uint32_t     sp_bottom,
                   const uint32_t     num_cpu)
{
        register_value_puts("KHA BOTTOM: ", kheap_area->bottom);
        //register_value_puts("smp translation table base address: ", ttba);
        uint32_t va;
        for (va = 0;; va += SECTION_SIZE) {
                if        (va >= 0x00000000       && va < SP_BASE) {
                        /* Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WB_NWA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= sp_bottom        &&
                           va <  sp_bottom + (SECTION_SIZE*num_cpu)) {
                        /* Non-Shareable O&I Write-Through, NWA */
                        mmu_section(va,
                                    va,
                                    WT_NWA | FLD_PLV_ACCESS,
                                    ttba);
                } else if (va >= SHARED_BOTTOM    && va < SHARED_LIMIT) {
                        /* Shareable O&I Write-Through, NWA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WT_NWA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= kram_area->bottom && va < kram_area->limit) {
                        /* Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WB_NWA | FLD_PLV_ACCESS,
                                    ttba);
                } else if (va >= kheap_area->bottom && va < kheap_area->limit) {
                        /* Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_SECTION | WB_NWA | FLD_PLV_ACCESS,
                                    ttba);
                } else if (va >= uram_area->bottom && va < uram_area->limit) {
                        /* Non-Shareable O&I Write-Back, WA */
                        mmu_section(va,
                                    va,
                                    WB_NWA | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va >= PERIPH_BOTTOM    && va < PERIPH_LIMIT) {
                        /* Set shareable device memory */
                        mmu_section(va,
                                    va,
                                    SHAREABLE_DEV | FLD_FULL_ACCESS,
                                    ttba);
                } else if (va > PERIPH_LIMIT) {
                        mmu_section(va,
                                    va,
                                    SHAREABLE_DEV | FLD_FULL_ACCESS,
                                    ttba);
                        //mmu_fault(va, va, ttba);
                } else {
                        mmu_fault(va, va, ttba);
                }
                if (va == 0xFFF00000) break;
        }
}

void set_asid(const uint32_t asid)
{
        __asm__ volatile("push {r2}\n"
                         "mrc p15,0,r2,c13,c0,1\n"
                         "and r2, r2, #0xFFFFFF00\n"
                         "orr r2, r2, %0\n"
                         "mcr p15,0,r2,c13,c0,1\n"
                         "dsb\n"
                         "isb\n"
                         "pop {r2}"
                          :: "r"(asid) : "memory");
}
