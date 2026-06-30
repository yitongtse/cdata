
.extern InterruptHandler	# void InterruptHandler(unsigned long exceptNumber)
.extern __reset

.globl gInterruptVectorTable, gInterruptVectorTableEnd, reset

.section	.reset,4,1,6		# put this code in .init section


##############################################################################
#
#	isr_prologue
#
#	Saves the necessary registers for an interrupt service routine
#
##############################################################################

isr_prologue: 	.macro
	
				stwu     rsp,-80(rsp) 
				stw      r0,8(rsp) 
				mfctr    r0 
				stw      r0,12(rsp) 
				mfxer    r0 
				stw      r0,16(rsp) 
				mfcr     r0 
				stw      r0,20(rsp) 
				mflr     r0 
				stw      r0,24(rsp) 
				stw      r3,40(rsp) 
				stw      r4,44(rsp) 
				stw      r5,48(rsp) 
				stw      r6,52(rsp) 
				stw      r7,56(rsp) 
				stw      r8,60(rsp) 
				stw      r9,64(rsp) 
				stw      r10,68(rsp) 
				stw      r11,72(rsp) 
				stw      r12,76(rsp) 
				mfsrr0   r0 
				stw      r0,28(rsp) 
				mfsrr1   r0
				stw      r0,32(rsp)
	
				.endm


##############################################################################
#
#	isr_epilogue
#
#	Restores the necessary registers for an interrupt service routine
#
##############################################################################

isr_epilogue: 	.macro

				lwz      r0,32(rsp)
				mtsrr1   r0
				lwz      r0,28(rsp)
				mtsrr0   r0
				lwz      r3,40(rsp)
				lwz      r4,44(rsp)
				lwz      r5,48(rsp)
				lwz      r6,52(rsp)
				lwz      r7,56(rsp)
				lwz      r8,60(rsp)
				lwz      r9,64(rsp)
				lwz      r10,68(rsp)
				lwz      r11,72(rsp)
				lwz      r12,76(rsp)
				lwz      r0,24(rsp)
				mtlr     r0
				lwz      r0,20(rsp)
				mtcrf    0xff,r0
				lwz      r0,16(rsp)
				mtxer    r0
				lwz      r0,12(rsp)
				mtctr    r0
				lwz      r0,8(rsp)
				addi     rsp,rsp,80
				rfi

				.endm
				

gInterruptVectorTable:

##############################################################################
#
#	0x0100 System Reset
#
##############################################################################
		.org	0x100
reset:

		lis		r4,__reset@h
		ori		r4,r4,__reset@l
		mtlr	r4 
		blr

##############################################################################
#
#	0x0200 Machine Check
#
##############################################################################
		.org 	0x200

		isr_prologue
		
		li		r3,0x0200
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue
		
##############################################################################
#
#	0x0300 Data Storage
#
##############################################################################
		.org	0x300		

		isr_prologue
		
		li		r3,0x0300
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0400 Instruction Storage
#
##############################################################################
		.org	0x400

		isr_prologue
		
		li		r3,0x0400
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0500 External Interrupt
#
##############################################################################
		.org	0x500

		isr_prologue
		
		li		r3,0x0500
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0600 Alignment
#
##############################################################################
		.org	0x600

		isr_prologue
		
		li		r3,0x0600
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0700 Program
#
##############################################################################
		.org	0x700

		isr_prologue
		
		li		r3,0x0700
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0800 Floating Point Unavailable
#
##############################################################################
		.org	0x800

		isr_prologue
		
		li		r3,0x0800
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0900 Decrementer
#
##############################################################################
		.org	0x900

		isr_prologue
		
		li		r3,0x0900
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0C00 System Call
#
##############################################################################
		.org	0xC00

		isr_prologue
		
		li		r3,0x0C00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0D00 Trace
#
##############################################################################
		.org	0xD00

		isr_prologue
		
		li		r3,0x0D00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0E00 FP Assist
#
##############################################################################
		.org	0xE00

		isr_prologue
		
		li		r3,0x0E00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x0F00 PPC740 and PPC750: Performance Monitor
#
##############################################################################
		.org	0xF00

		isr_prologue
		
		li		r3,0x0F00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x1000 	MPC8xx and MPC505:	Software Emulation
#			PPC603e/82xx:		Instruction TLB Miss
#
##############################################################################
		.org	0x1000

		isr_prologue
		
		li		r3,0x1000
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x1100 	MPC8xx:				Instruction TLB Miss
#			PPC603e/82xx:		Data Load TLB Miss
#
##############################################################################
		.org	0x1100

		isr_prologue
		
		li		r3,0x1100
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x1200	MPC8xx:				Data TLB Miss
#			PPC603e/82xx:		Data Store TLB Miss
#
##############################################################################
		.org	0x1200

		isr_prologue
		
		li		r3,0x1200
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x1300	MPC8xx:						Instruction TLB Error
#			PPC7xx and PPC603e/82xx:	Instruction address breakpoint
#
##############################################################################
		.org	0x1300

		isr_prologue
		
		li		r3,0x1300
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

##############################################################################
#
#	0x1400	MPC8xx:						Data TLB Error
#			PPC7xx and PPC603e/82xx:	System management
#
##############################################################################
		.org	0x1400

		isr_prologue
		
		li		r3,0x1400
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue
		
##############################################################################
#
#	0x1700	PPC7xx:				Thermal Management
#
##############################################################################
		.org	0x1700

		isr_prologue
		
		li		r3,0x1700
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue
		
##############################################################################
#
#	0x1C00 MPC8xx and MPC505:	Data breakpoint
#
##############################################################################
		.org	0x1C00

		isr_prologue
		
		li		r3,0x1C00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue
		
##############################################################################
#
#	0x1D00 MPC8xx and MPC505:	Instruction breakpoint
#
##############################################################################
		.org	0x1D00

		isr_prologue
		
		li		r3,0x1D00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue
		
##############################################################################
#
#	0x1E00 MPC8xx and MPC505:	Peripheral breakpoint
#
##############################################################################
		.org	0x1E00

		isr_prologue
		
		li		r3,0x1E00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue
		
##############################################################################
#
#	0x1F00 MPC8xx and MPC505:	Non-maskable development port
#
##############################################################################
		.org	0x1F00

		isr_prologue
		
		li		r3,0x1F00
		lis		r4,InterruptHandler@h
		ori		r4,r4,InterruptHandler@l
		mtlr	r4 
		blrl
		
		isr_epilogue

gInterruptVectorTableEnd:
