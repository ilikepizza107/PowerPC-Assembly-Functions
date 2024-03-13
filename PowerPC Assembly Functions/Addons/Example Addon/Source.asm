####################################
Example Addon [CodeMenu]
####################################
.include "Source/CM_Addons/AddonMacros.asm"

HOOK @ $804E003C
{
	%GetBaseLOC(r11)
	nop
}
