

.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.section  .text.__udivmoddi4

.extern __udivmoddi4
.type  __udivmoddi4, %function

.align 2
.global __aeabi_uldivmod
.type  __aeabi_uldivmod, %function

__aeabi_uldivmod:
  push	{r11, lr}
  sub	sp, sp, #16
  add	r12, sp, #8
  str	r12, [sp]
  bl	__udivmoddi4
  ldr	r2, [sp, #8]
  ldr	r3, [sp, #12]
  add	sp, sp, #16
  pop	{r11, pc}
