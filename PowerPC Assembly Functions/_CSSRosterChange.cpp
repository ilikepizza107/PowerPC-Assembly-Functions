#include "stdafx.h"
#include "_CSSRosterChange.h"

const std::string codeVersion = "v1.0.0";

void cssRosterChange() // Adapted from CSS Roster Change via Code Menu [QuickLava]
{
	// If CSS Rosters are enabled
	if (CSS_VERSION_SETTING_INDEX != -1) 
	{
		int reg1 = 6;
		int reg2 = 5;

		// Note, the address here is based on where the associated constant in common3.pac(sora_menu_sel_char Section[5]@0xE40) lands.
		// This should remain constant for the foreseeable future I think, though if it ever changes this code will also need to be adjusted!
		constexpr unsigned long CSSRosterPathAddress = 0x806a1f20; // Location of Roster File Path
		constexpr unsigned long CSSRosterFolderPortionLength = 0x9; // Used to move past the "/BrawlEx/" portion of the Roster File Path

		ASMStart(0x80682928, "[CM: _CSSRosterChange] CSS Roster Changer " + codeVersion + " [QuickLava]"); // Hooks the second instruction of "__ct/[muSelCharTask]/mu_selchar.o".
		SetRegister(reg1, CSS_VERSION_SETTING_INDEX); // Load the location of the CSS Roster Line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index of the CSS Roster Line

		// Load the address we'll write the new filename to (the CSSRoster Path's address, plus the length of its folder portion so we don't overwrite it)
		SetRegister(reg1, CSSRosterPathAddress + CSSRosterFolderPortionLength);

		for (std::size_t i = 0; i < ROSTER_FILENAME_LIST.size(); i++) // For each Roster Filename...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index...
			{
				WriteStringToMem(ROSTER_FILENAME_LIST[i], reg1); // ... in which we write that filename to the filepath.
			}EndIf();
		}

		ASMEnd(0x7c0802a6); // Restore the instruction replaced by the branch; mflr	r0.
	}

}