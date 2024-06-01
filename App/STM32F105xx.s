
  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

.global g_pfnVectors
.global Default_Handler

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/
  .section .text.Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:

/* Copy the data segment initializers from flash to SRAM */
  ldr sp, =_estack
  movs r1, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r3, =_sidata
  ldr r3, [r3, r1]
  str r3, [r0, r1]
  adds r1, r1, #4

LoopCopyDataInit:
  ldr r0, =_sdata
  ldr r3, =_edata
  adds r2, r0, r1
  cmp r2, r3
  bcc CopyDataInit
  ldr r2, =_sbss
  b LoopFillZerobss
/* Zero fill the bss segment. */
FillZerobss:
  movs r3, #0
  str r3, [r2], #4

LoopFillZerobss:
  ldr r3, = _ebss
  cmp r2, r3
  bcc FillZerobss

  // Configure application stack
  ldr r0, =0x20010000
  msr PSP, r0
  // Configure handler stack
  ldr r0, =0x20010000
  msr MSP, r0
  // Call inialization
  bl  systemInit
  // Start application
  bl systemMain
 end_loop:
  b end_loop
.size Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an 
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 * @param  None     
 * @retval None       
*/
.section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M3. Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
* 
*******************************************************************************/
   .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors
myIrqVectors:
g_pfnVectors:
 .word  _estack
 .word   Reset_Handler            // Reset Handler
 .word   NMI_Handler              // NMI Handler
 .word   HardFault_Handler        // Hard Fault Handler
 .word   MemManage_Handler        // MPU Fault Handler
 .word   BusFault_Handler         // Bus Fault Handler
 .word   UsageFault_Handler       // Usage Fault Handler
 .word   0                        // Reserved
 .word   0                        // Reserved
 .word   0                        // Reserved
 .word   0                        // Reserved
 .word   SVC_Handler              // SVCall Handler
 .word   DebugMon_Handler         // Debug Monitor Handler
 .word   0                        // Reserved
 .word   PendSV_Handler           // PendSV Handler
 .word   SysTick_Handler          // SysTick Handler
 							      //
 // External Interrupts
 .word   AnyUserIRQHandler        // Window Watchdog
 .word   AnyUserIRQHandler        // PVD through EXTI Line detect
 .word   AnyUserIRQHandler        // Tamper
 .word   AnyUserIRQHandler        // RTC
 .word   AnyUserIRQHandler        // Flash
 .word   AnyUserIRQHandler        // RCC
 .word   AnyUserIRQHandler        // EXTI Line 0
 .word   AnyUserIRQHandler        // EXTI Line 1
 .word   AnyUserIRQHandler        // EXTI Line 2
 .word   AnyUserIRQHandler        // EXTI Line 3
 .word   AnyUserIRQHandler        // EXTI Line 4
 .word   AnyUserIRQHandler        // DMA1 Channel 1
 .word   AnyUserIRQHandler        // DMA1 Channel 2
 .word   AnyUserIRQHandler        // DMA1 Channel 3
 .word   AnyUserIRQHandler        // DMA1 Channel 4
 .word   AnyUserIRQHandler        // DMA1 Channel 5
 .word   AnyUserIRQHandler        // DMA1 Channel 6
 .word   AnyUserIRQHandler        // DMA1 Channel 7
 .word   AnyUserIRQHandler        // ADC1 and ADC2
 .word   Can1Tx_IRQHandler        // CAN1 TX
 .word   Can1Rx0_IRQHandler       // CAN1 RX0
 .word   Can1Rx1_IRQHandler       // CAN1 RX1
 .word   Can1Sce_IRQHandler       // CAN1 SCE
 .word   AnyUserIRQHandler        // EXTI Line 9..5
 .word   AnyUserIRQHandler        // TIM1 Break
 .word   TIM1_IRQHandler          // TIM1 Update
 .word   AnyUserIRQHandler        // TIM1 Trigger and Commutation
 .word   AnyUserIRQHandler        // TIM1 Capture Compare
 .word   TIM2_IRQHandler          // TIM2
 .word   AnyUserIRQHandler        // TIM3
 .word   AnyUserIRQHandler        // TIM4
 .word   AnyUserIRQHandler        // I2C1 Event
 .word   AnyUserIRQHandler        // I2C1 Error
 .word   AnyUserIRQHandler        // I2C2 Event
 .word   AnyUserIRQHandler        // I2C1 Error
 .word   AnyUserIRQHandler        // SPI1
 .word   AnyUserIRQHandler        // SPI2
 .word   USART1_IRQHandler        // USART1
 .word   AnyUserIRQHandler        // USART2
 .word   AnyUserIRQHandler        // USART3
 .word   AnyUserIRQHandler        // EXTI Line 15..10
 .word   AnyUserIRQHandler        // RTC alarm through EXTI line
 .word   AnyUserIRQHandler     	  // USB OTG FS Wakeup through EXTI line
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // TIM5
 .word   AnyUserIRQHandler        // SPI3
 .word   AnyUserIRQHandler        // UART4
 .word   AnyUserIRQHandler        // UART5
 .word   AnyUserIRQHandler        // TIM6
 .word   AnyUserIRQHandler        // TIM7
 .word   AnyUserIRQHandler        // DMA2 Channel1
 .word   AnyUserIRQHandler        // DMA2 Channel2
 .word   AnyUserIRQHandler        // DMA2 Channel3
 .word   AnyUserIRQHandler        // DMA2 Channel4
 .word   AnyUserIRQHandler    	  // DMA2 Channel5
 .word   AnyUserIRQHandler        // Reserved
 .word   AnyUserIRQHandler        // Reserved
 .word   Can2Tx_IRQHandler        // CAN2 TX
 .word   Can2Rx0_IRQHandler       // CAN2 RX0
 .word   Can2Rx1_IRQHandler       // CAN2 RX1
 .word   Can2Sce_IRQHandler       // CAN2 SCE
 .word   USB_IRQHandler    	  	  // USB OTG FS

                        

                  


.weak NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

  .weak PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak USBWakeUp_IRQHandler
  .thumb_set USBWakeUp_IRQHandler,Default_Handler

  .weak USB_OTG_FS_IRQHandler
  .thumb_set USB_OTG_FS_IRQHandler,Default_Handler

  .weak AnyUserIRQHandler
  .thumb_set AnyUserIRQHandler,Default_Handler


