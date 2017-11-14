
;@-------------------------------------------------------------------------
;@-------------------------------------------------------------------------

.globl _start
_start:
    mov sp,#0x8000
    bl notmain
hang: b hang

.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl PUT16
PUT16:
    strh r1,[r0]
    bx lr

.globl PUT8
PUT8:
    strb r1,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr

.globl GETPC
GETPC:
    mov r0,lr
    bx lr

.globl dummy
dummy:
    bx lr



.globl LDREX
LDREX:
	ldrex r0,[r1]
	bx lr

.globl STREX
STREX:
	strex r0,r1,[r2]
	bx lr

.globl EXTEST
EXTEST:
	ldrex r3,[r2]
	strex r0,r1,[r2]
	bx lr


.globl start_l1cache
start_l1cache:
	mov r0, #0
	mcr p15, 0, r0, c7, c7, 0 ;@ invalidate caches
	mcr p15, 0, r0, c8, c7, 0 ;@ invalidate tlb
	mcr p15, 0, r2, c7,c10, 4 ;@ DSB ??
	mrc p15, 0, r0, c1, c0, 0
	orr r0,r0,#0x1000 ;@ instruction
	orr r0,r0,#0x0004 ;@ data
	mcr p15, 0, r0, c1, c0, 0
	bx lr

.globl stop_l1cache
stop_l1cache:
	mrc p15, 0, r0, c1, c0, 0
	bic r0,r0,#0x1000 ;@ instruction
	bic r0,r0,#0x0004 ;@ data
	mcr p15, 0, r0, c1, c0, 0
	bx lr


.globl start_mmu
start_mmu:
	mov r2,#0
	mcr p15,0,r2,c7,c7,0 ;@ invalidate caches
	mcr p15,0,r2,c8,c7,0 ;@ invalidate tlb

	mvn r2,#0
	mcr p15,0,r2,c3,c0,0 ;@ domain

	mcr p15,0,r0,c2,c0,0 ;@ tlb base
	mcr p15,0,r0,c2,c0,1 ;@ tlb base

	mrc p15,0,r2,c1,c0,0
	orr r2,r2,r1
	mcr p15,0,r2,c1,c0,0

	bx lr

