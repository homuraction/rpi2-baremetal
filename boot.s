.section ".text.boot"

.equ PSR_USR,          0x10
.equ PSR_FIQ,          0x11
.equ PSR_IRQ,          0x12
.equ PSR_SVC,          0x13
.equ PSR_MON,          0x16
.equ PSR_ABT,          0x17
.equ PSR_I,            0x80
.equ PSR_F,            0x40
.equ LEVEL1_BASE_ADDR, 0x00100000
.equ VECTOR_BASE_ADDR, 0x00000000
.equ STACK_BASE_ADDR,  0x00100000
.equ STACK_SIZE,       0x00040000

.text
.align  4

// save all registers and SPSR on the SVC stack, (LR corrected by offset)
.macro saveall, num, offset
        // corrcet lr
        sub lr, #\offset
        // save SPSR and LR onto the SVC stack
        srsdb #0x13!
        // switch to SVC mode, interrupts disabled
        cpsid i, #0x13
        // save user space SP/LR
        sub sp, #8
        stmia sp, {sp, lr}^
        // save all registers on the stack (+ LR for alignment)
        push {r0-r12, lr}
        // move pointer to registers into first argument
        mov r0, sp
        // move number for exception into second argument
        mov r1, #\num

        // align stack to 8 byte and save original SP
        // 8 byte alignment is required for ldrd but the exception may
        // have happened while the stack was only 4 byte aligned.

        // align stack to the next lower multiple of 8
        and sp, sp, #~7
        // push saved SP and dummy register so it again is 8 byte aligned
        push {r0, r1}
.endm

// restore all registers and return from exception
.macro restoreall
        // restore saved unaligend stack
.endm

.macro safe_svcmode_maskall reg:req
        /* Change mode HYP to SVC */
        mrs \reg, cpsr         /* Get current PSR */
        eor \reg, \reg, #0x1a  /* XOR with r0 and 0b11111 */
        tst \reg, #0x1f        /* If r0 is #0x1a(HYP), r0 is all zero. */
        bic \reg, \reg, #0x1f  /* Clear the Mode-bits */
        orr \reg, \reg, #0xd3  /* r0 = 0b1101 0011 */
        bne 1f                 /* If r0 does not equal to 0b11111, goto label 1 */
        orr \reg, \reg, #0x100 /* r0 = 0x1d3 = 0b1 1101 0011 */
        adr lr, 2f             /* lr = address of label 2 */
        msr spsr_cxsf, \reg    /* Write r0 into the all bits of spsr */
        .word 0xe12ef30e       /* msr elr_hyp, lr */ /* When eret is called, processor start from lr(label 2) */
        .word 0xe160006e       /* eret */
1:      msr cpsr_c, \reg
2:      nop
.endm

.global _start
_start:
        ldr pc, _reset_h
        ldr pc, _undefined_instruction_vector_h
        ldr pc, _software_interrupt_vector_h
        ldr pc, _prefetch_abort_vector_h
        ldr pc, _data_abort_vector_h
        ldr pc, _unused_handler_h
        ldr pc, _interrupt_vector_h
        ldr pc, _fast_interrupt_vector_h

_reset_h:                        .word _reset_
_undefined_instruction_vector_h: .word undefined_instruction_vector
_software_interrupt_vector_h:    .word _svc_handler
_prefetch_abort_vector_h:        .word prefetch_abort_vector
_data_abort_vector_h:            .word _dabort
_unused_handler_h:               .word _reset_
_interrupt_vector_h:             .word _irq
_fast_interrupt_vector_h:        .word fast_interrupt_vector

_reset_:
        safe_svcmode_maskall r0

        // Return CPU ID (0..3) of the CPU executed on
        mrc p15,0,r0,c0,c0,5         // R0 = Multiprocessor Affinity Register (MPIDR)
        ands r0, r0, #0x3            // r9 indicates CPU ID
        bne _secondary_start

        // Move Vector Table from 0x8000 to 0x0000
        mov   r0,     #0x8000
        mov   r1,     #0x0000
        ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
        stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}
        ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
        stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}

        // Set stack base address to r1 register
        mov r1, #STACK_BASE_ADDR
        add r1, r1, #STACK_SIZE

        // Set FIQ stack pointer
        mov   r0,     #(PSR_FIQ | PSR_I | PSR_F)
        msr   cpsr_c, r0
        mov   sp, r1
        add   r1, r1, #STACK_SIZE

        // We're going to use interrupt mode, so setup the interrupt mode
        // stack pointer which differs to the application stack pointer:
        mov   r0,     #(PSR_IRQ | PSR_I | PSR_F)
        msr   cpsr_c, r0
        mov   sp, r1
        add   r1, r1, #STACK_SIZE

        // Set ABT stack pointer
        mov   r0,     #(PSR_ABT | PSR_I | PSR_F)
        msr   cpsr_c, r0
        mov   sp, r1
        add   r1, r1, #STACK_SIZE
        sub   r1, r1, #0x4

        // Switch back to supervisor mode (our application mode) and
        // set the stack pointer towards the end of RAM. Remember that the
        // stack works its way down memory, our heap will work it's way
        // up memory toward the application stack.
        mov   r0,     #(PSR_SVC | PSR_I | PSR_F)
        msr   cpsr_c, r0
        mov   sp, r1

        bl    sys_init_primary

        bl    main

        b     _inf_loop

.global _secondary_start
_secondary_start:
        safe_svcmode_maskall r0

        // Return CPU ID (0..3) of the CPU executed on
        mrc p15,0,r1,c0,c0,5         // R0 = Multiprocessor Affinity Register (MPIDR)
        and r1, r1, #0x3             // r9 indicates CPU ID

        // Set stack base address to r1 register
        mov r0, #STACK_BASE_ADDR
        mov r3, #STACK_BASE_ADDR
        mul r4, r1, r3
        add r0, r0, r4
        add r0, r0, #STACK_SIZE

        // Set FIQ stack pointer
        mov   r2,     #(PSR_FIQ | PSR_I | PSR_F)
        msr   cpsr_c, r2
        mov   sp, r0
        add   r0, r0, #STACK_SIZE

        //mov r0, #STACK_BASE_ADDR
        //mul r0, r0, r1
        //add r0, r0, #0x80000

        // We're going to use interrupt mode, so setup the interrupt mode
        // stack pointer which differs to the application stack pointer:
        mov   r2,     #(PSR_IRQ | PSR_I | PSR_F)
        msr   cpsr_c, r2
        mov   sp, r0
        add   r0, r0, #STACK_SIZE

        //mov r0, #STACK_BASE_ADDR
        //mul r0, r0, r1
        //add r0, r0, #0xC0000

        // Set ABT stack pointer
        mov   r2,     #(PSR_ABT | PSR_I | PSR_F)
        msr   cpsr_c, r2
        mov   sp, r0
        add   r0, r0, #STACK_SIZE
        sub   r0, r0, #0x4

        //mov r0, #STACK_BASE_ADDR
        //mul r0, r0, r1
        //add r0, r0, #0xFFFFC

        // Switch back to supervisor mode (our application mode) and
        // set the stack pointer towards the end of RAM. Remember that the
        // stack works its way down memory, our heap will work it's way
        // up memory toward the application stack.
        mov   r2,     #(PSR_SVC | PSR_I | PSR_F)
        msr   cpsr_c, r2
        mov   sp, r0

        bl    sys_init_secondary

        //bl    other

        // If main does return for some reason, just catch it and stay here
_inf_loop:
        b     _inf_loop

.global _irq
_irq:
        sub   lr,  lr, #4
        stmfd sp!, {r0-r12, lr}
        bl    interrupt_handler
        cpsie i
        ldmfd sp!, {r0-r12, pc}^

.global _dabort
_dabort:
        sub   lr,  lr, #-4
        stmfd sp!, {r0-r12, lr}
        mrc p15,0,r0,c6,c0,0
        mrc p15,0,r1,c5,c0,0
        bl data_abort_vector
        cpsie i
        ldmfd sp!, {r0-r12, pc}^

.global _svc_handler
_svc_handler:
        push {r1-r12, lr}
        ldr  r0, [lr,#-4]
        bic  r0,r0,#0xFF000000
        bl   software_interrupt_handler
        ldm  sp!, {r1-r12,pc}^

.global _get_hvbadr
_get_hvbadr:
        mrc p15, 4, r0, c12, c0, 0 // Get Hyp Vector Base Address Register
        and r0, r0, #0xFFFFFFE0
        mov pc, lr

.global _get_vbadr
_get_vbadr:
        mrc p15, 0, r0, c12, c0, 0 // Get Vector Base Address Register
        and r0, r0, #0xFFFFFFE0    // Get Vector Base Address
        mov pc, lr

.global _get_stack_pointer
_get_stack_pointer:
        // Return the stack pointer value
        str   sp,     [sp]
        ldr   r0,     [sp]

        // Return from the function
        mov   pc,     lr

.global _get_cpsr
_get_cpsr:
        mov   r0,     #0
        mrs   r0,     cpsr
        mov   pc,     lr

.global _get_prid
_get_prid:
        mrc p15, 0, r0, c0, c0, 5   //  Get Processor ID
        and r0, r0, #0x3
        mov pc, lr

.global _get_mpidr
_get_mpidr:
        mrc p15, 0, r0, c0, c0, 5
        mov pc, lr

.global _get_hsctlr
_get_hsctlr:
        mrc p15, 4, r0, c1, c0, 0    // Read Hyp System Control Register
        mov pc, lr

.global _get_sctlr
_get_sctlr:
        mrc p15, 0, r0, c1, c0, 0
        mov pc, lr

.global _get_num_cpus
_get_num_cpus:
        mrc p15, 1, r0, c9, c0, 2    // Read L2 Contro Register
        and r0, r0, #0x003000000     // [25:24] bits indicate a number of cpus
        lsr r0, r0, #24              // Shift num cpus bits to [1:0]
        mov pc, lr                   // return

.global _is_smp
_is_smp:
        mrc p15, 0, r0, c1, c0, 1
        and r0, r0, #0x40
        mov pc, lr

.global _set_smp
_set_smp:
        mrc p15, 0, r0, c1, c0, 1
        orr r0, r0, #0x40
        mcr p15, 0, r0, c1, c0, 1
        mov pc, lr

.global _enable_interrupts
_enable_interrupts:
        mrs   r0,     cpsr
        bic   r0,     r0,                               #(0x80)
        msr   cpsr_c, r0
        mov   pc,     lr

.global DataMemoryBarrierAll
DataMemoryBarrierAll:
	mov r0, #0                   //  The read register Should Be Zero before the call
	mcr p15, 0, r0, C7, C6, 0    //  Invalidate Entire Data Cache
	mcr p15, 0, r0, c7, c10, 0   //  Clean Entire Data Cache
	mcr p15, 0, r0, c7, c14, 0   //  Clean and Invalidate Entire Data Cache
	mcr p15, 0, r0, c7, c10, 4   //  Data Synchronization Barrier
	mcr p15, 0, r0, c7, c10, 5   //  Data Memory Barrier
	mov pc, lr


.global MemoryBarrier
MemoryBarrier:
	mcr p15, #0, ip, c7, c5,  0 // invalidate I cache
	mcr p15, #0, ip, c7, c5,  6 // invalidate BTB
	mcr p15, #0, ip, c7, c10, 4 // drain write buffer
	mcr p15, #0, ip, c7, c5,  4 // prefetch flush
	mov pc, lr

.global dummy
dummy:
        bx lr

.global _start1
_start1:
        ldr r0, =_start
        mov r1,#0x40000000
        str r0,[r1,#0x9C]
        bx lr

.global _start2
_start2:
        ldr r0, =_start
        mov r1,#0x40000000
        str r0,[r1,#0xAC]
        bx lr

.global _start3
_start3:
        ldr r0, =_start
        mov r1,#0x40000000
        str r0,[r1,#0xBC]
        bx lr

.global read32
read32:
        ldr r0, [r0]
        bx lr

.global write32
write32:
        str r1, [r0]
        bx lr

.global _inc_cores
_inc_cores:
        mov     r3, #0x01
_try:
        ldrex   r2, [r0]
        cmp     r2, #0x00
        strexeq r2, r3, [r0]
        cmpeq   r2, #0x00
        bne     _try

        ldr r4, [r1]
        add r4, #0x1
        str r4, [r1]
        mov r3, #0x00
_rel:
        ldrex   r2, [r0]
        cmp     r2, #0x01
        strexeq r2, r3, [r0]
        cmpeq   r2, #0x00
        bne     _rel
        bx lr

.global _spin_req
_spin_req:
        mov     r1, #0x1
_lock_try:
        ldrex   r2, [r0]
        cmp     r2, #0
        strexeq r2, r1, [r0]
        cmpeq   r2, #0
        bne     _lock_try
        dsb
        bx lr

.global _spin_rel
_spin_rel:
        mov     r1, #0x0
_rel_try:
        ldrex   r2, [r0]
        cmp     r2, #1
        dmb
        strexeq r2, r1, [r0]
        cmpeq   r2, #0
        bne     _rel_try
        dsb
        bx lr

.global _get_tlbtr
_get_tlbtr:
        mrc p15, 0, r0, c0, c0, 3
        bx lr

.global _get_mmfr0
_get_mmfr0:
        mrc p15, 0, r0, c0, c1, 4
        bx lr

.global _get_mmfr1
_get_mmfr1:
        mrc p15, 0, r0, c0, c1, 5
        bx lr

.global _get_mmfr2
_get_mmfr2:
        mrc p15, 0, r0, c0, c1, 6
        bx lr

.global _get_mmfr3
_get_mmfr3:
        mrc p15, 0, r0, c0, c1, 7
        bx lr

.global _get_isar4
_get_isar4:
        mrc p15, 0, r0, c0, c2, 4
        bx lr

.global _change_pc
_change_pc:
        mov pc, r1

.global _get_dacr
_get_dacr:
        mrc p15, 0, r0, c3, c0, 0
        bx lr

.global _get_pfr1
_get_pfr1:
        mrc p15,0,r0,c0,c1,1
        bx lr

.global _get_contextidr
_get_contextidr:
        mrc p15,0,r0,c13,c0,1
        bx lr

.global invalidate_data_cache_l10_only
invalidate_data_cache_l10_only:
        push {r0-r6}
        mov  r0, #0
        mcr  p15,2,r0,c0,c0,0
        mrc  p15,1,r0,c0,c0,0

        movw r1,#0x7FFF
        and  r2,r1,r0,lsr #13

        movw r1, #0x3FF

        and  r3,r1,r0,lsr #3   // NumWays - 1
        add  r2,r2,#1          // NumSets

        and  r0,r0,#0x7
        add  r0,r0,#4          // SetShift

        clz  r1,r3             // WayShift
        add  r4,r3,#1          // NumWays
1:      sub  r2,r2,#1          // NumSets--
        mov  r3,r4             // Temp = NumWays
2:      subs r3,r3,#1          // Temp--
        mov  r5,r3,lsl r1
        mov  r6,r2,lsl r0
        orr  r5,r5,r6          // Reg = (Temp << WayShift)|(NumSets << SetShift)
        mcr  p15,0,r5,c7,c6,2
        bgt  2b
        cmp  r2,#0
        bgt  1b
        dsb  st
        isb
        pop  {r0-r6}
        bx   lr
