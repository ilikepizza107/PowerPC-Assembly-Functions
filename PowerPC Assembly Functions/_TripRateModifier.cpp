#include "stdafx.h"
#include "_TripRateModifier.h"

const std::string codeVersion = "v1.1.0";

void tripRateModifier()
{
	if (TRIP_TOGGLE_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;
		ASMStart(0x8089E910, "[CM: _TripRateModifier] Tripping Toggle " + codeVersion + " [QuickLava]"); // Hooks "isSlip/[ftUtil]/ft_util.o".
		// Look 0x08 past the address of the trip toggle's line to get whether it's on or not.
		ADDIS(reg1, 0, TRIP_TOGGLE_INDEX >> 0x10);
		LWZ(reg1, reg1, (TRIP_TOGGLE_INDEX & 0xFFFF) + Line::VALUE);
		// By default, do as the no-trip code does, disabling tripping.
		FMR(31, 30);
		// But if (and *only* if) the toggle is set to on though...
		If(reg1, EQUAL_I, 1);
		{
			// ... do as vBrawl does, *enabling* tripping.
			FMR(31, 1);
		}
		EndIf();
		ASMEnd();
		

		const unsigned long dashTurnSlipMultiplierAddress = 0x80b883dc;
		ASMStart(0x8089e868, "[CM: _TripRateModifier] Tripping Rate Modifier " + codeVersion + " [QuickLava]"); // Hooks "isSlip/[ftUtil]/ft_util.o".
		// Look 0x08 past the address of the trip toggle's line to get whether it's on or not.
		ADDIS(reg1, 0, TRIP_TOGGLE_INDEX >> 0x10);
		LWZ(reg2, reg1, (TRIP_TOGGLE_INDEX & 0xFFFF) + Line::VALUE);
		// If the toggle is set to on...
		If(reg2, EQUAL_I, 1);
		{
			// ... load the location of the rate multiplier's line.
			LWZ(reg2, reg1, (TRIP_RATE_MULTIPLIER_INDEX & 0xFFFF) + Line::VALUE);
			// Write it over the normal dash slip chance multiplier.
			SetRegister(reg1, dashTurnSlipMultiplierAddress); // Note, can't do the more optimal ADDIS then STW with offset cuz our offset >= 0x8000.
			STW(reg2, reg1, 0x00);
		}
		EndIf();
		ASMEnd();


		ASMStart(0x8081cb68, "[CM: _TripRateModifier] Tripping Interval Modifier " + codeVersion + " [QuickLava]"); // Hooks "isSlipInterval/[ftOwner]/ft_owner.o".
		// Look 0x08 past the address of the trip cooldown toggle's line to get its value.
		ADDIS(reg1, 0, TRIP_INTERVAL_INDEX >> 0x10);
		LWZ(reg1, reg1, (TRIP_INTERVAL_INDEX & 0xFFFF) + Line::VALUE);
		// Importantly, the trip cooldown toggle is a boolean, and so is the r3 value coming out of this function,
		// so the only thing we actually need to do here is invert the toggle's state into r3, so we can just NOR for the inverse.
		NOR(3, reg1, reg1);
		ASMEnd(0x4e800020); // Restore original instruction; blr
	}
}
