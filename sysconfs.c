#include "sysinfo.h"

SysCommonInfo sys_common_info =
        { 0, ALL_SMP_MODE, ALL_SMP_MODE, TTBA_BASE, 0, NULL };

ram_area_t  kram_area0  = { 0x1F000000, 0x1FFFFFFF };
heap_area_t kheap_area0 = { 0x20000000, 0x20FFFFFF };
ram_area_t  uram_area0  = { 0x21000000, 0x26FFFFFF };

ram_area_t  kram_area1  = { 0x27000000, 0x27FFFFFF };
heap_area_t kheap_area1 = { 0x28000000, 0x28FFFFFF };
ram_area_t  uram_area1  = { 0x29000000, 0x2EFFFFFF };

ram_area_t  kram_area2  = { 0x2F000000, 0x2FFFFFFF };
heap_area_t kheap_area2 = { 0x30000000, 0x30FFFFFF };
ram_area_t  uram_area2  = { 0x31000000, 0x36FFFFFF };

ram_area_t  kram_area3  = { 0x37000000, 0x37FFFFFF };
heap_area_t kheap_area3 = { 0x38000000, 0x38FFFFFF };
ram_area_t  uram_area3  = { 0x39000000, 0x3EFFFFFF };

ram_area_t  kram_area4  = { 0x1F000000, 0x21FFFFFF };
heap_area_t kheap_area4 = { 0x22000000, 0x23FFFFFF };
ram_area_t  uram_area4  = { 0x24000000, 0x3EFFFFFF };

ram_area_t  kram_area5  = { 0x1F000000, 0x21FFFFFF };
heap_area_t kheap_area5 = { 0x22000000, 0x23FFFFFF };
ram_area_t  uram_area5  = { 0x24000000, 0x36FFFFFF };

ram_area_t  kram_area6  = { 0x1F000000, 0x21FFFFFF };
heap_area_t kheap_area6 = { 0x22000000, 0x23FFFFFF };
ram_area_t  uram_area6  = { 0x24000000, 0x2EFFFFFF };

ram_area_t *kram_areas[2*NUM_CPU-1] = {
        &kram_area0,
        &kram_area1,
        &kram_area2,
        &kram_area3,
        &kram_area4,
        &kram_area5,
        &kram_area6,
};

ram_area_t *uram_areas[2*NUM_CPU-1] = {
        &uram_area0,
        &uram_area1,
        &uram_area2,
        &uram_area3,
        &uram_area4,
        &uram_area5,
        &uram_area6,
};

heap_area_t *kheap_areas[2*NUM_CPU-1] = {
        &kheap_area0,
        &kheap_area1,
        &kheap_area2,
        &kheap_area3,
        &kheap_area4,
        &kheap_area5,
        &kheap_area6,
};

asid_area_t asid_area0 = { ASID_SIZE*0, ASID_SIZE*1 };
asid_area_t asid_area1 = { ASID_SIZE*1, ASID_SIZE*2 };
asid_area_t asid_area2 = { ASID_SIZE*2, ASID_SIZE*3 };
asid_area_t asid_area3 = { ASID_SIZE*3, ASID_SIZE*4 };
asid_area_t asid_area4 = { ASID_SIZE*0, ASID_SIZE*4 };
asid_area_t asid_area5 = { ASID_SIZE*0, ASID_SIZE*3 };
asid_area_t asid_area6 = { ASID_SIZE*0, ASID_SIZE*2 };

asid_area_t *asid_areas[2*NUM_CPU-1] = {
        &asid_area0,
        &asid_area1,
        &asid_area2,
        &asid_area3,
        &asid_area4,
        &asid_area5,
        &asid_area6
};

uint32_t asid_head[NUM_ASID] = {0};

SysStructInfo sys_struct_info0;
SysStructInfo sys_struct_info1;
SysStructInfo sys_struct_info2;
SysStructInfo sys_struct_info3;
SysStructInfo sys_struct_info4;
SysStructInfo sys_struct_info5;
SysStructInfo sys_struct_info6;

SysStructInfoTop sys_struct_info_top = {
        {
                &sys_struct_info0,
                &sys_struct_info1,
                &sys_struct_info2,
                &sys_struct_info3,
                &sys_struct_info4,
                &sys_struct_info5,
                &sys_struct_info6
        }
};
