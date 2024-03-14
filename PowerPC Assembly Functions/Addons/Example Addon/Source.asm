#############################################################
[CM_Addons] Example Addon Test: Replace Jumpsquat [QuickLava]
#############################################################
.include "Source/CM_Addons/AddonAliases.asm"

HOOK @ $808734F4
{
	lwz	r4, 0x00D8(r30) # Restore Original Instruction
	# Use Example Addon Integer Line to Replace Jumpsquat!
	lis r12, EXMP_INTTST_LOC_HI
	lwz r12, EXMP_INTTST_LOC_LO(r12)
	lwz r3, 0x8(r12)
}
