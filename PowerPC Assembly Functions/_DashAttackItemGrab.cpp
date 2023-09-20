#include "stdafx.h"
#include "_DashAttackItemGrab.h"

const std::string codeVersion = "v1.0.0";

//FighterPac Start = 80F9FC20
//Add 0x80 to PSAC Offset to get .pac relative offset, 0x20 for .dat relative offset

void dashAttackItemGrab(bool codeEnabled)
{
	// If Code Enabled
	if (codeEnabled && DASH_ATTACK_ITEM_GRAB_INDEX != -1)
	{
		// Free to be used, they get overwritten following this hook anyway.
		int reg1 = 30;
		int reg2 = 31;
		// These following three are used to LSWX, they need to be consecutive!
		int reg3 = 10;
		int reg4 = 11;
		int reg5 = 12;

		int loopStartLabel = GetNextLabel();

		// First Element: Signature Location
		// Second Element: Replacement Signature
		// Third Element: Parameter Address
		std::vector<std::array<unsigned long, 3>> instrOverrideTable =
		{
			// Restore Item Grabs via Dash Attack
			{0x80FB240C, 0x00070100, 0x80FB2394}, // Restore vBrawl PSA Instruction:	Sub Routine: 2xDE24
			// Restore Item Grabs via Aerials
			{0x80FC2798, 0x000A0400, 0x80FB2DE4}, // Restore vBrawl PSA Instruction:	If Compare: RA-Basic[1] <= IC-Basic[23127]
			{0x80FC27A0, 0x00070100, 0x80FADAC4}, // Restore vBrawl PSA Instruction:		Sub Routine: 2xDE24
			{0x80FC27A8, 0x000F0000, 0x00000000}, // Restore vBrawl PSA Instruction:	End If:
		};
		const unsigned long instrOverrideTableEntrySize = sizeof(instrOverrideTable[0]);
		const unsigned long instrOverrideTableTableSize = instrOverrideTable.size() * instrOverrideTableEntrySize;

		ASMStart(0x808e0094, "[CM: _DashAttackItemGrab] vBrawl Item Grab Restoration Toggle " + codeVersion + " [QuickLava]"); // Hooks "getPowerMul/[ftLogPatternModule]/ft_pattern_log.o" (same as the Staling Toggle Code).

		// Create Instruction Address and Content Table
		BL(1 + (instrOverrideTableTableSize / 4));
		for (auto i : instrOverrideTable)
		{
			WriteIntToFile(i[0]);
			WriteIntToFile(i[1]);
			WriteIntToFile(i[2]);
		}

		// Load the current state of the toggle...
		ADDIS(reg1, 0, DASH_ATTACK_ITEM_GRAB_INDEX >> 0x10);
		LWZ(reg1, reg1, (DASH_ATTACK_ITEM_GRAB_INDEX & 0xFFFF) + 0x8);
		// ... and check whether or not the Toggle was on, which we'll use in a moment.
		CMPLI(reg1, 0, 0);

		// Setup Predefined Values Here:
		// These are the values for the NOP signature, which we'll preload into reg4 and reg5.
		// They'll be overwritten if the toggle was on, but left alone if not!
		SetRegister(reg4, 0x00020000);
		SetRegister(reg5, 0x00000000);
		// This is the number of bytes we're gonna LSWX with later.
		// By default we'll set it to 0xC, which pulls the full 3 values for each entry from the table, populating reg3, reg4, and reg5.
		ADDI(reg1, 0, 0xC);
		// But if the Toggle was off...
		BC(2, bCACB_NOT_EQUAL);
		// ... then we'll overwrite that if 0x4, so we only pull the target location instead, leaving our preloaded NOP Signature in place!
		ADDI(reg1, 0, 0x4);
		// Either way, we store the result in XER.
		MTXER(reg1);
		// Finally, we'll pull the table address from LR...
		MFLR(reg1);
		// ... and zero reg2, which'll be our offset iterator.
		ADDI(reg2, 0, 0);

		// Beginning of Loop
		Label(loopStartLabel);
		// Load the necessary values from the table based on Toggle State:
		// If Toggle was on, loads all 3 entries from (reg1 + reg2) into registers starting at reg3!
		// If it's off, loads just the signatuer Loc, leaving reg4 and reg5 alone!
		LSWX(reg3, reg1, reg2);
		// Our first value is the address to write the following 0x8 bytes from the table to,
		// so we STSWI to reg3, starting from register reg4, for 0x8 bytes!
		STSWI(reg4, reg3, 0x8);
		// Push offset register forwards by one entry...
		ADDI(reg2, reg2, instrOverrideTableEntrySize);
		// ... and if we're still within our table...
		CMPLI(reg2, instrOverrideTableTableSize, 0);
		// ... continue with our loop!
		JumpToLabel(loopStartLabel, bCACB_LESSER);

		ASMEnd(0x7c7e1b78); // Restore the instruction replaced by the branch; mr r30, r3.
	}
}
