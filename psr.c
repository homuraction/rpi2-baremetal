#include "psr.h"

uint32_t get_cpsr(void)
{
        uint32_t cpsr;
        __asm__ volatile("mov %0, #0\n"
                         "mrs %0, cpsr\n"
                          : "=r"(cpsr)
                        );
        return cpsr;
}

uint32_t get_cpsr_m(uint32_t cpsr)
{
        return (cpsr & PSR_M);
}
