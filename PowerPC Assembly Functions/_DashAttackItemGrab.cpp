#include "stdafx.h"
#include "_DashAttackItemGrab.h"

const std::string codeVersion = "v1.0.0";

void dashAttackItemGrab()
{
	// If Toggle Enabled
	if (DASH_ATTACK_ITEM_GRAB_INDEX != -1)
	{
		int reg1 = 30;
		int reg2 = 12;

		constexpr unsigned long dashAttackItemGrabRamWriteLoc = 0x80FB240C;


		ASMStart(0x808e0094, "[CM: _DashAttackItemGrab] Dash Attack Item Grab Toggle " + codeVersion + " [QuickLava]"); // Hooks "getPowerMul/[ftLogPatternModule]/ft_pattern_log.o" (same as the Staling Toggle Code).
		SetRegister(reg1, DASH_ATTACK_ITEM_GRAB_INDEX); // Load the location of the toggle's line.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index of the CSS Roster Line

		// Load the address we'll be writing to.
		SetRegister(reg1, dashAttackItemGrabRamWriteLoc);

		// If the toggle is set to on (ie. item grabbing is on)...
		If(reg2, EQUAL_I, 1);
		{
			SetRegister(reg2, 0x00070100); // ... write in the Subroutine PSA Signature...
			STW(reg2, reg1, 0x00);
			SetRegister(reg2, 0x80FB2394); // ... and the address of the PSA Subroutine.
			STW(reg2, reg1, 0x04);
		}
		// Otherwise...
		Else();
		{
			SetRegister(reg2, 0x00020000); // ... NOP out the signature...
			STW(reg2, reg1, 0x00);
			SetRegister(reg2, 0x00000000); // ... and zero out the pointer.
			STW(reg2, reg1, 0x04);
		}
		EndIf();

		ASMEnd(0x7c7e1b78); // Restore the instruction replaced by the branch; mr r30, r3.
	}
}
