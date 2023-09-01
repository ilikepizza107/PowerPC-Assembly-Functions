#include "stdafx.h"
#include "_JumpsquatOverride.h"

void jumpsquatOverride(bool codeEnabled)
{
	// If Code Enabled
	if (codeEnabled && JUMPSQUAT_OVERRIDE_TOGGLE_INDEX != -1)
	{
		int modifierValueFloatReg = 13;
		int modificationResultFloatReg = 12;
		int frameFloatReg = 31;
		int reg1 = 11;
		int reg2 = 12;

		int applyChangesLabel = GetNextLabel();
		int exitLabel = GetNextLabel();

		ASMStart(0x80873528, "[CM: _JumpsquatOverride] Jumpsquat Length Modifier");
		// Setup top half of Line Addr in reg1...
		ADDIS(reg1, 0, JUMPSQUAT_OVERRIDE_TOGGLE_INDEX >> 0x10);
		// ... and grab the current value of the toggle.
		LWZ(reg2, reg1, (JUMPSQUAT_OVERRIDE_TOGGLE_INDEX & 0xFFFF) + Line::VALUE);

		// If the toggle is disabled (set to 0), then we can skip.
		CMPLI(reg2, 0, 0);
		JumpToLabel(exitLabel, bCACB_EQUAL);

		// Otherwise, load the modifier's value into our safe register.
		LFS(modifierValueFloatReg, reg1, (JUMPSQUAT_OVERRIDE_FRAMES_INDEX & 0xFFFF) + Line::VALUE);

		// Replace Case:
		FMR(modificationResultFloatReg, modifierValueFloatReg, 0);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Add Case:
		FADD(modificationResultFloatReg, frameFloatReg, modifierValueFloatReg, 0);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Subtract Case:
		FSUB(modificationResultFloatReg, frameFloatReg, modifierValueFloatReg, 0);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Multiply Case:
		FMUL(modificationResultFloatReg, frameFloatReg, modifierValueFloatReg, 0);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Divide Case (Note: don't need to subtract and branch on final case!):
		FDIV(modificationResultFloatReg, frameFloatReg, modifierValueFloatReg, 0);

		Label(applyChangesLabel);

		// Finally, copy modified value into frame register!
		FMR(frameFloatReg, modificationResultFloatReg);

		Label(exitLabel);

		ASMEnd(0x4E800421); // Restore Original Instruction: bctrl
	}
}