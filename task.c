#include "task.h"
#include "smp_kernel.h"

// call this function in the period that disable interrupts
void create_task(TCB **task_head, void *func, uint32_t pri, uint32_t *lock)
{
        if (task_head == NULL || func == NULL)
                return;

        // create task control block
        TCB       *task    = (TCB *)kmalloc(sizeof(TCB));
        context_t *context = (context_t *)kmalloc(sizeof(context_t));
        context->sp   = 0x00000000;
        context->pc   = (uint32_t)func;
        task->state   = TASK_STATE_READY;
        task->pri     = pri;
        task->mapid   = 0;
        task->ttba    = 0x00000000;
        task->asid    = 0;
        task->prev    = NULL;
        task->next    = NULL;
        task->context = context;

        // set task to ready queue
        spin_req(lock);
        TCB *last = task_head[pri]->prev;
        if (task_head[pri] == last) { // There is no task
                task_head[pri]->prev = task;
                task_head[pri]->next = task;
                task->prev           = task_head[pri];
                task->next           = task_head[pri];
        } else {
                last->next           = task;
                task->prev           = last;
                task->next           = task_head[pri];
                task_head[pri]->prev = task;
        }
        spin_rel(lock);
}

// call this function in the period that disable interrupts
void start_task(TCB *task, uint32_t *lock, task_map_t *tsk_alc_map)
{
        uint32_t i;
        const uint32_t cpuid     = _get_prid();
        const uint32_t pm        = get_pm();
        const SysStructInfo *ssi = get_sys_struct_info(cpuid, pm);
        // search INIT_MAP in the tsk_alc_map
        for (i=0; i < ssi->max_num_task_space; i++) {
                spin_req(lock);
                if (smp_tsk_alc_map[i].state == INIT_MAP) {
                        smp_tsk_alc_map[i].state = RUNNING_MAP;
                        smp_tsk_alc_map[i].task  = task;
                        task->mapid = i;
                        task->ttba =
                                (i * SECTION_SIZE) + ssi->user_ram_area->bottom;
                        cre_usr_tbl(task->ttba);
                }
                spin_rel(lock);
        }
        task->state = TASK_STATE_RUNNING;
}

// call this function in the period that disable interrupts
void end_task(TCB *task, uint32_t *lock, task_map_t *tsk_alc_map)
{
        uint32_t *asid_head            = SYSCommonInfo->asid_head;
        asid_head[task->asid]          = FINISHED_ASID;
        tsk_alc_map[task->mapid].state = WAIT_INIT_MAP;
        task->state                    = TASK_STATE_FINISHED;
}

// call this function in the period that disable interrupts
void destroy_task(TCB *task, uint32_t *lock, task_map_t *tsk_alc_map)
{
        invalidate_tlbs(task->asid);
        SYSCommonInfo->asid_head[task->asid] = UNUSED_ASID;

        uint32_t pa;
        for (pa = task->ttba; pa < task->ttba + SECTION_SIZE; pa += 4) {
                write32(pa, 0x00000000);
        }
        spin_req(&smp_queue_lock);
        tsk_alc_map[task->mapid].state = INIT_MAP;
        tsk_alc_map[task->mapid].task  = NULL;

        task->prev->next = task->next;
        task->next->prev = task->prev;
        spin_rel(&smp_queue_lock);

        kfree(task->context);
        kfree(task);
}

void destroy_core_tasks(void)
{
        const uint32_t cpuid = _get_prid();
        SysStructInfo *ssi = SYSStructTop[SYSSTRUCT_AMP_INDEX(cpuid)];
        const uint32_t mapid_limit =
                ((ssi->user_ram_area->limit - SMP_USER_RAM_BOTTOM) /
                 SECTION_SIZE);
        uint32_t mapid =
                ((ssi->kernel_ram_area->bottom - SMP_USER_RAM_BOTTOM) /
                SECTION_SIZE);
        for (; mapid < mapid_limit; mapid++) {
                if (smp_tsk_alc_map[mapid].state == INIT_MAP)
                        continue;
                destroy_task(smp_tsk_alc_map[mapid].task,
                                &smp_queue_lock, smp_tsk_alc_map);
        }
}

// call this function in the period that disable interrupts
void move_task_spaces(void)
{
        const uint32_t  cpuid       = _get_prid();
        const uint32_t  pm          = get_pm();
        SysStructInfo  *ssi         = get_sys_struct_info(cpuid, pm);

        const uint32_t  map_bottom  = SMP_USER_RAM_BOTTOM;
        const uint32_t  ram_bottom  = ssi->kernel_ram_area->bottom;
        const uint32_t  ram_limit   = ssi->user_ram_area->limit;

        const uint32_t  bottomid    = (ram_bottom - map_bottom) / SECTION_SIZE;
        const uint32_t  limitid     = (ram_limit  - map_bottom) / SECTION_SIZE;

        uint32_t mapid;
        uint32_t id_offset = 0;
        for (mapid = bottomid; mapid < limitid; mapid++) {
                spin_req(&smp_queue_lock);

                if (smp_tsk_alc_map[mapid].state == 0) {
                        spin_rel(&smp_queue_lock);
                        continue;
                }

                move_task_space(&(smp_tsk_alc_map[mapid]));
                spin_rel(&smp_queue_lock);

                destroy_task(smp_tsk_alc_map[mapid].task,
                                &smp_queue_lock, smp_tsk_alc_map);
                smp_tsk_alc_map[mapid].task  = NULL;
                smp_tsk_alc_map[mapid].state = INIT_MAP;
        }
}

void move_task_space(task_map_t *task_map)
{
        const uint32_t  cpuid       = _get_prid();
        const uint32_t  pm          = get_pm();
        SysStructInfo  *ssi         = get_sys_struct_info(cpuid, pm);
        const uint32_t  num_map_id  =
                ((ssi->user_ram_area->limit - ssi->user_ram_area->bottom) /
                 SECTION_SIZE);

        uint32_t mapid;
        uint32_t resource_pa = task_map->task->ttba;
        uint32_t target_pa;
        uint32_t target_bottom;
        for (mapid = 0; mapid < num_map_id; mapid++) {
                spin_req(&smp_queue_lock);

                if (smp_tsk_alc_map[mapid].state != 0) {
                        spin_rel(&smp_queue_lock);
                        continue;
                }

                smp_tsk_alc_map[mapid].state = task_map->state;
                smp_tsk_alc_map[mapid].task  = task_map->task;

                if (task_map->task->asid >= ssi->asid_area->limit) {
                        invalidate_tlbs(task_map->task->asid);
                        SYSCommonInfo->asid_head[task_map->task->asid] =
                                UNUSED_ASID;
                        task_map->task->asid = 0;
                }

                target_bottom = SMP_USER_RAM_BOTTOM + (mapid * SECTION_SIZE);

                for (target_pa = target_bottom;
                     target_pa < target_bottom + SECTION_SIZE;
                     target_pa += 4, resource_pa += 4) {
                        write32(target_pa, read32(resource_pa));
                }

                cre_usr_tbl(target_bottom);
                return;
        }
}
