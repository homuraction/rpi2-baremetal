#include "sysinfo.h"
#include "cpu.h"
#include "stdint.h"
#include "memory.h"
#include "mmu.h"
#include "uart.h"

extern SysCommonInfo sys_common_info;
extern SysStructInfoTop sys_struct_info_top;

extern ram_area_t  *kram_area0;
extern ram_area_t  *uram_area0;
extern heap_area_t  kheap_area0;

extern ram_area_t  *kram_area1;
extern ram_area_t  *uram_area1;
extern heap_area_t  kheap_area1;

extern ram_area_t  *kram_area2;
extern ram_area_t  *uram_area2;
extern heap_area_t  kheap_area2;

extern ram_area_t  *kram_area3;
extern ram_area_t  *uram_area3;
extern heap_area_t  kheap_area3;

extern ram_area_t  *kram_area4;
extern ram_area_t  *uram_area4;
extern heap_area_t  kheap_area4;

extern ram_area_t  *kram_area5;
extern ram_area_t  *uram_area5;
extern heap_area_t  kheap_area5;

extern ram_area_t  *kram_area6;
extern ram_area_t  *uram_area6;
extern heap_area_t  kheap_area6;

extern ram_area_t  *kram_areas[2*NUM_CPU-1];
extern ram_area_t  *uram_areas[2*NUM_CPU-1];
extern heap_area_t *kheap_areas[2*NUM_CPU-1];

extern uint32_t asid_head[NUM_ASID];
extern asid_area_t *asid_areas[2*NUM_CPU-1];

SysInfo sysinfo __attribute__((section (".sysinfo"))) = {
        &sys_common_info,
        &sys_struct_info_top
};

void init_sysinfos(void)
{
        SYSCommonInfo->asid_head = (uint32_t *)asid_head;
        uint32_t i, num_max_task_space;
        SysStructInfo *ssi;
        const uint32_t base_ttba = SYSCommonInfo->base_ttba;
        for (i=0; i < 2*NUM_CPU-1; i++) {
                ssi = SYSStructTop[i];
                ssi->lock               = 0;
                ssi->ttba               = base_ttba + (i * SECTION_TBL_SIZE);
                ssi->kernel_ram_area    = kram_areas[i];
                ssi->user_ram_area      = uram_areas[i];
                ssi->kernel_heap_area   = kheap_areas[i];
                ssi->kernel_heap_freep  = NULL;
                ssi->asid_area          = asid_areas[i];
                ssi->max_num_task_space =
                        ((ssi->user_ram_area->limit - ssi->user_ram_area->bottom)
                         / SECTION_SIZE);
        }
}

uint32_t is_valid_pm(uint32_t pm)
{
        uint32_t cpuid;
        uint32_t smp_continuous_flg;
        if (is_amp(PCore, pm)) {
                for (cpuid = (PCore+1); cpuid < NUM_CPU; cpuid++) {
                        if (is_smp(cpuid, pm)) {
                                uart_puts("invalid pm, PCore is amp but other core is smp\n");
                                return 0;
                        }
                }
                return 1;
        }

        smp_continuous_flg = 1;

        for (cpuid = (PCore+1); cpuid < NUM_CPU; cpuid++) {
                if (is_smp(cpuid, pm) && smp_continuous_flg == 0) {
                        uart_puts("entire smp cores are continuous\n");
                        return 0; // entire smp cores are continuous
                } else if (cpuid == (PCore+1) && is_amp(cpuid, pm)) {
                        uart_puts("smp is not single core\n");
                        return 0; // smp is not single core
                } else if (is_amp(cpuid, pm) && smp_continuous_flg == 1) {
                        smp_continuous_flg = 0;
                }
        }

        return 1;
}

uint32_t is_valid_cpuid(uint32_t cpuid)
{
        return (cpuid >= 0 && cpuid < NUM_CPU);
}

uint32_t is_smp(uint32_t cpuid, uint32_t pm)
{
        if (pm & (1 << cpuid)) return 0; // cpuid is AMP mode
        return 1;                        // cpuid is SMP mode
}

uint32_t is_amp(uint32_t cpuid, uint32_t pm)
{
        if (pm & (1 << cpuid)) return 1; // cpuid is AMP mode
        return 0;                        // cpuid is SMP mode
}

const static uint32_t get_sys_struct_info_index(uint32_t cpuid, uint32_t pm)
{
        if (!is_valid_cpuid(cpuid) || !is_valid_pm(pm))
                return -1;

        uint32_t index;

        if (is_amp(cpuid, pm)) {
                return SYSSTRUCT_AMP_INDEX(cpuid);
        } else {
                return SYSSTRUCT_SMP_INDEX(pm);
        }
}

SysStructInfo *get_sys_struct_info(uint32_t cpuid, uint32_t pm)
{
        const uint32_t index = get_sys_struct_info_index(cpuid, pm);

        if (index < 0)
                return NULL;

        return SYSStructTop[index];
}

const uint32_t get_pm(void)
{
        return SYSCommonInfo->pm;
}

static uint32_t set_pm(uint32_t pm)
{
        if (!is_valid_pm(pm))
                return -1;

        spin_req(&(SYSCommonInfo->lock));
        SYSCommonInfo->pm = pm;
        DSB();
        spin_rel(&(SYSCommonInfo->lock));

        return pm;
}

const uint32_t get_prev_pm(void)
{
        return SYSCommonInfo->prev_pm;
}

static uint32_t set_prev_pm(uint32_t pm)
{
        if (!is_valid_pm(pm))
                return -1;

        spin_req(&(SYSCommonInfo->lock));
        SYSCommonInfo->prev_pm = pm;
        DSB();
        spin_rel(&(SYSCommonInfo->lock));

        return pm;
}

const uint32_t get_ttba(uint32_t cpuid, uint32_t pm)
{
        uint32_t index = get_sys_struct_info_index(cpuid, pm);

        if (index < 0)
                return -1;

        return SYSStructTop[index]->ttba;
}

static uint32_t set_ttba(uint32_t cpuid, uint32_t pm, uint32_t ttba)
{
        uint32_t index = get_sys_struct_info_index(cpuid, pm);

        if (index < 0)
                return -1;

        spin_req(&(SYSStructTop[index]->lock));
        SYSStructTop[index]->ttba = ttba;
        DSB();
        spin_rel(&(SYSStructTop[index]->lock));

        return ttba;
}

const uint32_t get_kernel_ram_bottom(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return -1;

        return ssi->kernel_ram_area->bottom;
}

const uint32_t get_kernel_ram_limit(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return -1;

        return ssi->kernel_ram_area->limit;
}

const ram_area_t *get_kernel_ram_area(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return NULL;

        return ssi->kernel_ram_area;
}

const uint32_t get_user_ram_bottom(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return -1;

        return ssi->user_ram_area->bottom;
}

const uint32_t get_user_ram_limit(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return -1;

        return ssi->user_ram_area->limit;
}

const ram_area_t *get_user_ram_area(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return NULL;

        return ssi->user_ram_area;
}

const uint32_t get_kernel_heap_bottom(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return -1;

        return ssi->kernel_heap_area->bottom;
}

const uint32_t get_kernel_heap_limit(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return -1;

        return ssi->kernel_heap_area->limit;
}

const  heap_area_t *get_kernel_heap_area(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return NULL;

        return ssi->kernel_heap_area;
}

HEADER *get_kernel_heap_free_head(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);

        if (ssi == NULL)
                return NULL;

        return ssi->kernel_heap_freep;
}
// You must call this in the period that interrupts are disabled
//   This function update asid statuses array
uint32_t take_kernel_asid(uint32_t cpuid, uint32_t pm)
{
        SysStructInfo  *ssi         = get_sys_struct_info(cpuid, pm);
        uint32_t       *asid_head   = SYSCommonInfo->asid_head;
        const uint32_t  asid_bottom = ssi->asid_area->bottom + 1;

        if (ssi == NULL)
                return ERROR_ASID_NUM;

        if (asid_bottom == RESERVED_ASID_NUM) {
                asid_head[asid_bottom+1] = 1;
                return (asid_bottom + 1);
        }
        asid_head[asid_bottom] = 1;
        return asid_bottom;
}

// You must call this in the period that interrupts are disabled
//   This function update asid statuses array
uint32_t set_user_asid(TCB *task, uint32_t cpuid, uint32_t pm)
{
        if (task->asid != 0)
                return task->asid;

        SysStructInfo  *ssi         = get_sys_struct_info(cpuid, pm);
        uint32_t       *asid_head   = SYSCommonInfo->asid_head;
        const uint32_t  asid_bottom = ssi->asid_area->bottom;
        const uint32_t  asid_limit  = ssi->asid_area->limit;

        register_value_puts("asid bottom: ", asid_bottom);
        register_value_puts("asid limit: ", asid_limit);

        uart_puts("setup init values in take_asid\n");

        uint32_t asid;
        if (asid_bottom == RESERVED_ASID_NUM)
                asid = asid_bottom + 2;
        else
                asid = asid_bottom + 1;
        for (; asid < asid_limit; asid++) {
                register_value_puts("ASID: ", asid);
                register_value_puts("ASID value: ", asid_head[asid]);
                register_value_puts("SSI LOCK: ", ssi->lock);
                spin_req(&(ssi->lock));
                if (asid == RESERVED_ASID_NUM || asid_head[asid] == ACTIVE_ASID) {
                        uart_puts("RESERVED OR ACTIVE ASID\n");
                        spin_rel(&(ssi->lock));
                        continue;
                }

                if (asid_head[asid] == UNUSED_ASID) {
                        uart_puts("UNUSED\n");
                        asid_head[asid] = ACTIVE_ASID;
                } else if (asid_head[asid] == FINISHED_ASID) {
                        uart_puts("FINISHED\n");
                        asid_head[asid] = ACTIVE_ASID;
                        invalidate_tlbs(asid);
                }
                spin_rel(&(ssi->lock));

                task->asid = asid;

                return asid;
        }

        return ERROR_ASID_NUM;
}

uint32_t change_pm(const uint32_t pm)
{
        __asm__ volatile("cpsid i");

        spin_req(&(SYSCommonInfo->lock));
        SYSCommonInfo->prev_pm = SYSCommonInfo->pm;
        SYSCommonInfo->pm      = pm;
        spin_rel(&(SYSCommonInfo->lock));
        register_value_puts("SCI PM: ", SYSCommonInfo->pm);
        __asm__ volatile("cpsie i");

        DSB();

        return 1;
}
