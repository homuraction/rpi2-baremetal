#include "stdint.h"
#include "util.h"
#include "task.h"

#ifndef __SYSINFO_H__
#define __SYSINFO_H__

#define SYSInfo                    ((volatile SysInfo *)(0x40000))
#define SYSStructTop               (SYSInfo->sys_struct_info_top->struct_info)
#define SYSCommonInfo              (SYSInfo->sys_common_info)

#define SMP_PROC_NUM(pm)           (((pm & 0xF) == 0xC)? 2:    \
                                    ((pm & 0xF) == 0x8)? 3:    \
                                    ((pm & 0xF) == 0x0)? 4: 0)
#define NUM_CPU                    4
#define SMP_FLG                    0
#define AMP_FLG                    1
#define SYSINFO_SMP_OFFSET         NUM_CPU
#define PCore                      0
#define ALL_SMP_MODE               0
#define ALL_AMP_MODE               ((1 << NUM_CPU) - 1)
#define SYSSTRUCT_SMP_INDEX(pm)    (SYSINFO_SMP_OFFSET +         \
                                    (NUM_CPU - SMP_PROC_NUM(pm)) \
                                   )
#define SYSSTRUCT_AMP_INDEX(cpuid) (cpuid)

#define SMP_KERNEL_RAM_BOTTOM      0x1F000000
#define SMP_KERNEL_RAM_LIMIT       0x21FFFFFF
#define SMP_KERNEL_HEAP_BOTTOM     0x22000000
#define SMP_KERNEL_HEAP_LIMIT      0x23FFFFFF
#define SMP_USER_RAM_BOTTOM        0x24000000

#define TTBA_BASE                  0x00020000

#define SP_BASE                    0x00100000

#define SHARED_BOTTOM              0x00500000
#define SHARED_LIMIT               0x1F000000

#define NUM_ASID                   256
#define ASID_SIZE                  (NUM_ASID / NUM_CPU)
#define RESERVED_ASID_NUM          0  // Used to change TTBA and ASID
#define ERROR_ASID_NUM             -1 // No ASID can be available

/* ASID status */
#define UNUSED_ASID                0 // No TTBA using this. You can allocate it
#define ACTIVE_ASID                1 // A TTBA using this. You cant allocate it
#define FINISHED_ASID              2 // Waiting to invalidate tlbs and release

typedef struct ram_area {
        uint32_t bottom;
        uint32_t limit;
} ram_area_t;

typedef struct heap_area {
        uint32_t bottom;
        uint32_t limit;
} heap_area_t;

typedef struct asid_area {
        uint32_t bottom;
        uint32_t limit;
} asid_area_t;

typedef struct {
        uint32_t lock;
        uint32_t pm;
        uint32_t prev_pm;
        uint32_t base_ttba;
        uint32_t waking_cores;
        uint32_t *asid_head;
} SysCommonInfo;

typedef struct {
        uint32_t     lock;
        uint32_t     ttba;
        ram_area_t  *kernel_ram_area;
        ram_area_t  *user_ram_area;
        heap_area_t *kernel_heap_area;
        HEADER      *kernel_heap_freep;
        asid_area_t *asid_area;
        uint32_t     max_num_task_space;
} SysStructInfo;

typedef struct {
        SysStructInfo *struct_info[2 * NUM_CPU - 1];
} SysStructInfoTop;

typedef struct {
        SysCommonInfo    *sys_common_info;
        SysStructInfoTop *sys_struct_info_top;
} SysInfo;

void init_sysinfos(void);

uint32_t is_valid_pm(uint32_t pm);
uint32_t is_valid_cpuid(uint32_t cpuid);

uint32_t is_smp(uint32_t cpuid, uint32_t pm);
uint32_t is_amp(uint32_t cpuid, uint32_t pm);

const static uint32_t  get_sys_struct_info_index(uint32_t cpuid, uint32_t pm);
SysStructInfo         *get_sys_struct_info(uint32_t cpuid, uint32_t pm);

const  uint32_t get_pm(void);
static uint32_t set_pm(uint32_t pm);

const  uint32_t get_prev_pm(void);
static uint32_t set_prev_pm(uint32_t pm);

const  uint32_t get_ttba(uint32_t cpuid, uint32_t pm);
static uint32_t set_ttba(uint32_t cpuid, uint32_t pm, uint32_t ttba);

const  uint32_t    get_kernel_ram_bottom(uint32_t cpuid, uint32_t pm);
const  uint32_t    get_kernel_ram_limit (uint32_t cpuid, uint32_t pm);
const  ram_area_t *get_kernel_ram_area  (uint32_t cpuid, uint32_t pm);


const  uint32_t    get_user_ram_bottom(uint32_t cpuid, uint32_t pm);
const  uint32_t    get_user_ram_limit (uint32_t cpuid, uint32_t pm);
const  ram_area_t *get_user_ram_area  (uint32_t cpuid, uint32_t pm);

static uint32_t set_user_ram_area  (
                ram_area_t *ram_area, uint32_t bottom, uint32_t limit);


const  uint32_t     get_kernel_heap_bottom(uint32_t cpuid, uint32_t pm);
const  uint32_t     get_kernel_heap_limit (uint32_t cpuid, uint32_t pm);
const  heap_area_t *get_kernel_heap_area(uint32_t cpuid, uint32_t pm);
HEADER             *get_kernel_heap_free_head(uint32_t cpuid, uint32_t pm);

static uint32_t set_kernel_heap_area  (
                heap_area_t *heap_area, uint32_t bottom, uint32_t limit);

const  uint32_t get_user_heap_bottom(uint32_t cpuid, uint32_t pm);
const  uint32_t get_user_heap_limit (uint32_t cpuid, uint32_t pm);
HEADER         *get_user_heap_free_head(uint32_t cpuid, uint32_t pm);

static uint32_t set_user_heap_area  (
                heap_area_t *heap_area, uint32_t bottom, uint32_t limit);

uint32_t take_kernel_asid(uint32_t cpuid, uint32_t pm);
uint32_t set_user_asid(TCB *task, uint32_t cpuid, uint32_t pm);

uint32_t change_pm(const uint32_t pm);

#endif /* __SYSINFO_H__ */
