#include "stdafx.h"
#include "_JumpsquatOverride.h"

void jumpsquatOverride(bool codeEnabled)
{
	// If Code Enabled
	if (codeEnabled && JUMPSQUAT_OVERRIDE_TOGGLE_INDEX != -1)
	{
		int frameReg = 3;
		int reg1 = 11;
		int reg2 = 12;
		int modificationResultReg = 0; // Used to temporarily hold our modified frame count

		int applyChangesLabel = GetNextLabel();
		int exitLabel = GetNextLabel();

		ASMStart(0x808734F8, "[CM: _JumpsquatOverride] Jumpsquat Length Modifier");
		// Setup top half of Line Addr in reg1...
		ADDIS(reg1, 0, JUMPSQUAT_OVERRIDE_TOGGLE_INDEX >> 0x10);
		// ... and grab the current value of the toggle.
		LWZ(reg2, reg1, (JUMPSQUAT_OVERRIDE_TOGGLE_INDEX & 0xFFFF) + Line::VALUE);

		// If the toggle is disabled (set to 0), then we can skip.
		CMPLI(reg2, 0, 0);
		JumpToLabel(exitLabel, bCACB_EQUAL);

		// Otherwise, load the modifier's value into reg1, since we won't need its value anymore.
		LWZ(reg1, reg1, (JUMPSQUAT_OVERRIDE_FRAMES_INDEX & 0xFFFF) + Line::VALUE);

		// Replace Case:
		MR(modificationResultReg, reg1);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Add Case:
		ADD(modificationResultReg, reg1, frameReg);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Subtract Case:
		SUBF(modificationResultReg, frameReg, reg1);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Multiply Case:
		MULLW(modificationResultReg, reg1, frameReg);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Divide Case:
		DIVWU(modificationResultReg, frameReg, reg1);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Distance Case (New Value = ABS(MOD_VAL - FRAME_COUNT)):
		SUBF(modificationResultReg, reg1, frameReg);
		CMPI(modificationResultReg, 0, 0);
		BC(2, bCACB_GREATER_OR_EQ);
		NEG(modificationResultReg, modificationResultReg);
		ADDIC(reg2, reg2, -1, 1);
		JumpToLabel(applyChangesLabel, bCACB_EQUAL);

		// Random Case:
		Randi(modificationResultReg, reg1, frameReg); // We can just use the canned Randi implementation for this!	
		// And we don't need to do the normal ADDIC, Jump idiom here since it's the last case in the list.

		Label(applyChangesLabel);

		// Check if our result ended up less than 1...
		CMPI(modificationResultReg, 1, 0);
		BC(2, bCACB_GREATER_OR_EQ);
		// ... and hard set it to 1 if it did.
		ADDI(modificationResultReg, 0, 1);

		// And finally, copy the modified value into the frame register!
		MR(frameReg, modificationResultReg);

		Label(exitLabel);

		ASMEnd(0x6C638000); // Restore Original Instruction: xoris	r3, r3, 0x8000
	}
}