.code
	intel_invept_all proc
		invept rcx, oword ptr [rdx]
		ret
	intel_invept_all endp
end
