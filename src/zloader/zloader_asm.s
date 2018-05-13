/*
 *      This is the assembly startup and relocation code for the zloader. See
 *      zloader.c for how the zloader works
 */

        .set    MODE_SYS, 0x1F

        .set    I_BIT, 0x80
        .set    F_BIT, 0x40


.section vectors
.code 32
.balign 4

/*
 * System points
 */
 
_start:
        // Setup stack, disable interrupts        
        ldr     r0, =__stack_top__
        msr     CPSR_c, #MODE_SYS | I_BIT | F_BIT
        mov     sp, r0

        // For benchmarking: store SYSTIMER_CLO in r1
        ldr     r0, =0x20003004
        ldr     r1, [r0]
        
        // Code relocation
        ldr     r5, =__reloc_base__        // Actual start of the code in memory
        ldr     r2, =__reloc_target__
        ldr     r3, =__reloc_end__         // Actual end of the code in memory
        
        // r3, __reloc_end__ now points to the end of text + data in memory
        // After text + data, mkzloader appends the length of the compressed data
        // and then the actual compressed data. Adjust r3 to point to the end of
        // the compressed data
        
        ldr     r0, [r3], #4 // Load zip file size, add 4 bytes (size of this word)
        add     r3, r3, r0   // Add zip file size

        // Copy ourselves including compressed data to the relocation area
relocloop:
        cmp     r5, r3
        ldrlo   r4, [r5], #4
        strlo   r4, [r2], #4
        blo     relocloop

        // Jump to the relocation area (using absolute branch)
        
        ldr pc, =relocjump
relocjump:

        // Call main loader (C)
        bl loader

        ldr pc, =__reloc_base__
        


/** @} */
