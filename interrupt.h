#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "stdint.h"

#define ARM_IRQS_PER_REG  32

#define IRQ_LINES         (ARM_IRQS_PER_REG * 2 + 8)

#define ARM_IRQ1_BASE     0
#define ARM_IRQ2_BASE     (ARM_IRQ1_BASE + ARM_IRQS_PER_REG)
#define ARM_IRQBASIC_BASE (ARM_IRQ2_BASE + ARM_IRQS_PER_REG)

#define ARM_IC_IRQ_PENDING(irq)  (  (irq) < ARM_IRQ2_BASE       \
                                  ? IRQ_PEND1                   \
                                  : ( (irq) < ARM_IRQBASIC_BASE \
                                    ? IRQ_PEND2                 \
                                    : IRQ_BASIC_PEND))

#define ARM_IC_IRQS_ENABLE(irq)  (  (irq) < ARM_IRQ2_BASE       \
                                  ? ENABLE_IRQ1                 \
                                  : ( (irq) < ARM_IRQBASIC_BASE \
                                    ? ENABLE_IRQ2               \
                                    : ENABLE_BASIC_IRQ))

#define ARM_IC_IRQS_DISABLE(irq) (  (irq) < ARM_IRQ2_BASE       \
                                  ? DISABLE_IRQ1                \
                                  : ( (irq) < ARM_IRQBASIC_BASE \
                                    ? DISABLE_IRQ2              \
                                    : DISABLE_BASIC_IRQ))

#define ARM_IRQ_MASK(irq) (1 << ((irq) & (ARM_IRQS_PER_REG-1)))

void init_irq_handler(void);
void send_irq(uint32_t prid);
void aux_int_handler (void);

#endif /* __INTERRUPT_H__ */
