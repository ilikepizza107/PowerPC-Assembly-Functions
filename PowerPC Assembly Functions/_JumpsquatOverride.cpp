#include "stdafx.h"
#include "_JumpsquatOverride.h"

void jumpsquatOverride(bool codeEnabled)
{
	// If Code Enabled
	if (codeEnabled && JUMPSQUAT_OVERRIDE_TOGGLE_INDEX != -1)
	{
		int frameCountReg = 3;
		int reg1 = 11;
		int reg2 = 12;

		ASMStart(0x808734F4, "[CM: _JumpsquatOverride] Universal Jumpsquat Lengths");
		// Setup top half of Line Addr in reg1...
		ADDIS(reg1, 0, JUMPSQUAT_OVERRIDE_TOGGLE_INDEX >> 0x10);
		// ... and grab the current value of the toggle.
		LWZ(reg2, reg1, (JUMPSQUAT_OVERRIDE_TOGGLE_INDEX & 0xFFFF) + Line::VALUE);

		// If the toggle is disabled (set to 0), then we can skip.
		CMPLI(reg2, 0, 0);
		BC(2, bCACB_EQUAL);

		// Otherwise, load the desired number of frames into the frame count register.
		LWZ(frameCountReg, reg1, (JUMPSQUAT_OVERRIDE_FRAMES_INDEX & 0xFFFF) + Line::VALUE);

		ASMEnd(0x809E00D8); // Restore Original Instruction: lwz r4, 0x00D8 (r30)
	}
}