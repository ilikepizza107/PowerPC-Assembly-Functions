#include "stdafx.h"
#include "_ThemeChange.h"

const std::string codeVersion = "v2.0.0";

void themeChange()
{
	menuMainChange(); selCharChange(); selMapChange(); selEventChange(); titleChange();
}

void themeChangerBody(const std::string menuDirectory, themeConstants::themePathIndices fileIndex, unsigned long stringReg)
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;

		// String Embed Setup
		std::string basePathString = menuDirectory + themeConstants::filenames[fileIndex];
		basePathString.resize(lava::padLengthTo<unsigned long>(basePathString.size(), 0x4, 0), 0x00);
		std::string prefixesString("");
		for (std::size_t i = 0; i < THEME_SPEC_LIST.size(); i++)
		{
			prefixesString += "/";
			prefixesString += THEME_SPEC_LIST[i].prefixes[fileIndex];
		}

		// Actual Body
		// Write and skip over the prefixes string...
		BL(1 + (prefixesString.size() / 4));
		WriteTextToFile(prefixesString);
		// ... then put its address in reg1, which we'll use in just a moment.
		MFLR(reg1);

		// Get the selected theme from the Theme Setting Line
		ADDIS(reg2, 0, THEME_SETTING_INDEX >> 0x10);
		LWZ(reg2, reg2, (THEME_SETTING_INDEX & 0xFFFF) + Line::VALUE);

		// Borrow the string register, set it to 4...
		SetRegister(stringReg, 0x04);
		// ... then multiply it by the desired Theme ID to index into the prefix list.
		MULLW(reg2, reg2, stringReg);
		// Then load the relevant prefix string using the calculated offset. reg1 is now our prefix!
		LWZX(reg1, reg1, reg2);

		// Write and skip over base path string...
		BL(1 + (basePathString.size() / 4));
		WriteTextToFile(basePathString);
		// ... then overwrite the string address register with our new target.
		MFLR(stringReg);

		// Now store the prefix in the correct spot, and we're done!
		STW(reg1, stringReg, menuDirectory.size() - 1);
	}
}

void menuMainChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		ASMStart(0x806bf030, "[CM: _ThemeChange] Theme Changer (menumain) " + codeVersion +" [QuickLava]"); // Hooks a nameless function referenced from "start/[muMenuMain]/mu_main.o".
		themeChangerBody("menu2/", themeConstants::tpi_MENUMAIN, 31);
		ASMEnd(); // Don't need to restore overwritten instruction, string reg already set.
	}
}
void selCharChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		// Write changer for Selchar
		ASMStart(0x806c8728, "[CM: _ThemeChange] Theme Changer (selchar) " + codeVersion + " [QuickLava]");  // Hooks "process/[scEnding]/sc_ending.o", credit to SammiHusky for the hook!
		themeChangerBody("/menu2/", themeConstants::tpi_SELCHAR, 5);
		ASMEnd(); // Don't need to restore overwritten instruction, string reg already set.

		// Write changer for Selchar2
		ASMStart(0x806c8744, "[CM: _ThemeChange] Theme Changer (selchar2) " + codeVersion + " [QuickLava]");  // Hooks "process/[scEnding]/sc_ending.o", credit to SammiHusky for the hook!
		themeChangerBody("/menu2/", themeConstants::tpi_SELCHAR2, 5);
		ASMEnd(); // Don't need to restore overwritten instruction, string reg already set.
	}
}
void selMapChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		ASMStart(0x806c8e14, "[CM: _ThemeChange] Theme Changer (selmap) " + codeVersion + " [QuickLava]"); // Hooks "start/[scSelStage]/sc_sel_stage.o".
		themeChangerBody("/menu2/", themeConstants::tpi_SELMAP, 5);
		ASMEnd(); // Don't need to restore overwritten instruction, string reg already set.
	}
}
void selEventChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		ASMStart(0x806c4574, "[CM: _ThemeChange] Theme Changer (selevent) " + codeVersion + " [QuickLava]"); // Hooks "start/[scSelEvent]/sc_sel_event.o".
		themeChangerBody("/menu2/", themeConstants::tpi_SELEVENT, 5);
		ASMEnd(); // Don't need to restore overwritten instruction, string reg already set.
	}
}
void titleChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		ASMStart(0x806ca150, "[CM: _ThemeChange] Theme Changer (title) " + codeVersion + " [QuickLava]"); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		themeChangerBody("menu2/", themeConstants::tpi_TITLE, 5);
		ASMEnd(); // Don't need to restore overwritten instruction, string reg already set.
	}
}
