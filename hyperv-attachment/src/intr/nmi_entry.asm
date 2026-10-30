extern ?process@nmi@intr@@YAXXZ : proc
extern original_nmi_handler : qword

.code
	save_gp_regs macro
		mov [rsp], rax
		mov [rsp+8h], rcx
		mov [rsp+10h], rdx
		mov [rsp+18h], rbx
		mov [rsp+20h], rbp
		mov [rsp+28h], rsi
		mov [rsp+30h], rdi
		mov [rsp+38h], r8
		mov [rsp+40h], r9
		mov [rsp+48h], r10
		mov [rsp+50h], r11
		mov [rsp+58h], r12
		mov [rsp+60h], r13
		mov [rsp+68h], r14
		mov [rsp+70h], r15
	endm

	restore_gp_regs macro
		mov rax, [rsp]
		mov rcx, [rsp+8h]
		mov rdx, [rsp+10h]
		mov rbx, [rsp+18h]
		mov rbp, [rsp+20h]
		mov rsi, [rsp+28h]
		mov rdi, [rsp+30h]
		mov r8,  [rsp+38h]
		mov r9,  [rsp+40h]
		mov r10, [rsp+48h]
		mov r11, [rsp+50h]
		mov r12, [rsp+58h]
		mov r13, [rsp+60h]
		mov r14, [rsp+68h]
		mov r15, [rsp+70h]
	endm

	save_xmm_regs macro
		movdqu [rsp],      xmm0
		movdqu [rsp+10h],  xmm1
		movdqu [rsp+20h],  xmm2
		movdqu [rsp+30h],  xmm3
		movdqu [rsp+40h],  xmm4
		movdqu [rsp+50h],  xmm5
		movdqu [rsp+60h],  xmm6
		movdqu [rsp+70h],  xmm7
		movdqu [rsp+80h],  xmm8
		movdqu [rsp+90h],  xmm9
		movdqu [rsp+0A0h], xmm10
		movdqu [rsp+0B0h], xmm11
		movdqu [rsp+0C0h], xmm12
		movdqu [rsp+0D0h], xmm13
		movdqu [rsp+0E0h], xmm14
		movdqu [rsp+0F0h], xmm15
	endm

	restore_xmm_regs macro
		movdqu xmm15, [rsp+0F0h]
		movdqu xmm14, [rsp+0E0h]
		movdqu xmm13, [rsp+0D0h]
		movdqu xmm12, [rsp+0C0h]
		movdqu xmm11, [rsp+0B0h]
		movdqu xmm10, [rsp+0A0h]
		movdqu xmm9,  [rsp+90h]
		movdqu xmm8,  [rsp+80h]
		movdqu xmm7,  [rsp+70h]
		movdqu xmm6,  [rsp+60h]
		movdqu xmm5,  [rsp+50h]
		movdqu xmm4,  [rsp+40h]
		movdqu xmm3,  [rsp+30h]
		movdqu xmm2,  [rsp+20h]
		movdqu xmm1,  [rsp+10h]
		movdqu xmm0,  [rsp]
	endm

	handle_nmi macro
		sub rsp, 78h
		save_gp_regs

		sub rsp, 100h
		save_xmm_regs

		sub rsp, 20h
		call ?process@nmi@intr@@YAXXZ
		add rsp, 20h

		restore_xmm_regs
		add rsp, 100h

		restore_gp_regs
		add rsp, 78h
	endm

	nmi_standalone_entry proc
		handle_nmi
		iretq
	nmi_standalone_entry endp

	nmi_entry proc
		handle_nmi
		jmp original_nmi_handler
	nmi_entry endp
END
