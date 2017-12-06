
        
.global FiqHandler
FiqHandler:
    //stmfd   sp!, {r0-r6, r12, lr} // TODO: optimize
    
    
    // [_set] = bit
    
    ldr r10, =#0
    //DMB
    mcr p15, 0, r10, c7, c10, 5
    
    ldr r8, _set
    ldr r9, _bit
    str r9, [r8]
    
    //DMB
    mcr p15, 0, r10, c7, c10, 5
    
    //bl fiq
    
    // [_cmp1] = (nexttime+=10)
    ldr r8, _cmp1
    ldr r9, _nexttime
    add r9, #10
    str r9, _nexttime
    str r9, [r8]

    //DMB
    mcr p15, 0, r10, c7, c10, 5
   
    
    ldr r8, _cs
    ldr r9, =#2
    str r9, [r8]
    
    //DMB
    mcr p15, 0, r10, c7, c10, 5
    
    ldr r11, =#10
lp:
    subs r11, #1
    bne lp

    // [_clear] = bit
    ldr r8, _clear
    ldr r9, _bit
    str r9, [r8]

    //DMB
    mcr p15, 0, r10, c7, c10, 5

    //ldmfd   sp!, {r0-r6, r12, lr}
    subs    pc, lr, #4


_set:   .word 0x2020001C
_clear: .word 0x20200028
_bit:   .word 0x40000
_nexttime: .word 50000000
_cmp1:  .word 0x20003010
_lo:    .word 0x20003004
_cs:    .word 0x20003000


.global FiqHandlerInit
FiqHandlerInit:
    // _nexttime = SYSTIMER_CMP1 = SYSTIMER_LO+100
    ldr r0, _lo
    ldr r1, [r0]
    add r1, #100
    ldr r0, _cmp1
    str r1, [r0]
    str r1, _nexttime
    
    bx lr
    

 