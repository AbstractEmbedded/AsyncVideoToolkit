section .text ; //make sthis executable
bits 64 ; allow 64 bit register names

global foo ; makes this visible to linker
foo:
	mov rax,7 ; just return a constant
	ret
	