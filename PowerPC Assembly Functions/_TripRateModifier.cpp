#include "stdafx.h"
#include "_TripRateModifier.h"

const std::string codeVersion = "v1.0.0";

void tripRateModifier()
{
	if (TRIP_TOGGLE_INDEX != -1)
	{
		int reg1 = 5;
		int reg2 = 12;
		ASMStart(0x8089E910, "[CM: _TripRateModifier] Tripping Toggle " + codeVersion + " [QuickLava]"); // Hooks "isSlip/[ftUtil]/ft_util.o".
		SetRegister(reg1, TRIP_TOGGLE_INDEX); // Load the location of the toggle's line.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the current state of the toggle.
		// If the toggle is set to on...
		If(reg2, EQUAL_I, 1);
		{
			// ... use the original instruction, allowing tripping.
			FMR(31, 1);
		}
		// Otherwise...
		Else();
		{
			// ... use the instruction from the no-trip code, disabling tripping.
			FMR(31, 30);
		}
		EndIf();
		ASMEnd();

		

		reg1 = 5;
		reg2 = 12;
		const unsigned long dashTurnSlipMultiplierAddress = 0x80b883dc;
		ASMStart(0x8089e868, "[CM: _TripRateModifier] Tripping Rate Modifier " + codeVersion + " [QuickLava]"); // Hooks "isSlip/[ftUtil]/ft_util.o".
		SetRegister(reg1, TRIP_TOGGLE_INDEX); // Load the location of the toggle's line.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the current state of the toggle.
		// If the toggle is set to on...
		If(reg2, EQUAL_I, 1);
		{
			SetRegister(reg1, TRIP_RATE_MULTIPLIER_INDEX); // ... load the location of the rate multiplier's line.
			LWZ(reg1, reg1, Line::VALUE); // Then look 0x08 past that address to get the stored float.
			SetRegister(reg2, dashTurnSlipMultiplierAddress); // Write it over the normal dash slip chance multiplier.
			STW(reg1, reg2, 0x00);
		}
		EndIf();
		ASMEnd();



		reg1 = 12;
		ASMStart(0x8081cb68, "[CM: _TripRateModifier] Tripping Interval Modifier " + codeVersion + " [QuickLava]"); // Hooks "isSlipInterval/[ftOwner]/ft_owner.o".
		SetRegister(reg1, TRIP_INTERVAL_INDEX); // Load the location of the cooldown toggle's line.
		LWZ(reg1, reg1, Line::VALUE); // Then look 0x08 past that address to get the value.
		If(reg1, EQUAL_I, 0);
		{
			SetRegister(reg1, 0x1); // Load 1 into reg1.
		}
		Else();
		{
			SetRegister(reg1, 0x0); // Load 0 into reg1.
		}
		EndIf();
		MR(3, reg1);
		ASMEnd(0x4e800020); // Restore original instruction; blr
	}
}
