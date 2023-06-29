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

		// Prefix String Embed Setup
		std::string prefixesString("");
		for (std::size_t i = 0; i < THEME_SPEC_LIST.size(); i++)
		{
			// Use the end of the menuDirectory string to pad the prefix to 4 bytes, if it's shorter than that.
			prefixesString += menuDirectory.substr(menuDirectory.size() - (4 - themeConstants::prefixLength));
			prefixesString += THEME_SPEC_LIST[i].prefixes[fileIndex];
		}

		// Actual Body
		// Get the selected theme from the Theme Setting Line
		ADDIS(reg2, 0, THEME_SETTING_INDEX >> 0x10);
		LWZ(reg2, reg2, (THEME_SETTING_INDEX & 0xFFFF) + Line::VALUE);
		// Multiply the desired Theme ID by 4 to calculate our index into the prefix list. reg2 is now our offset!
		MULLI(reg2, reg2, 0x04);

		// Write and skip over the prefixes string...
		BL(1 + (prefixesString.size() / 4));
		WriteTextToFile(prefixesString);
		// ... then put its address in reg1, which we'll use to...
		MFLR(reg1);
		// ... load the relevant prefix string using the calculated offset. reg1 is now our prefix!
		LWZX(reg1, reg1, reg2);

		// Finally store the prefix in the correct spot (accounting for any padding from the menuDirectory string), and we're done!
		STW(reg1, stringReg, menuDirectory.size() - (4 - themeConstants::prefixLength));
	}
}

void menuMainChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1 && THEME_FILE_GOT_UNIQUE_PREFIX[themeConstants::tpi_MENUMAIN])
	{
		ASMStart(0x806bf030, "[CM: _ThemeChange] Theme Changer (menumain) " + codeVersion +" [QuickLava]"); // Hooks a nameless function referenced from "start/[muMenuMain]/mu_main.o".
		WriteIntToFile(0x3bffb248); // Restore original instruction
		themeChangerBody("menu2/", themeConstants::tpi_MENUMAIN, 31);
		ASMEnd();
	}
}
void selCharChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		if (THEME_FILE_GOT_UNIQUE_PREFIX[themeConstants::tpi_SELCHAR])
		{
			// Write changer for Selchar
			ASMStart(0x806c8728, "[CM: _ThemeChange] Theme Changer (selchar) " + codeVersion + " [QuickLava]");  // Hooks "process/[scEnding]/sc_ending.o", credit to SammiHusky for the hook!
			WriteIntToFile(0x38a5f2ec); // Restore Original Instruction
			themeChangerBody("/menu2/", themeConstants::tpi_SELCHAR, 5);
			ASMEnd();
		}
		
		if (THEME_FILE_GOT_UNIQUE_PREFIX[themeConstants::tpi_SELCHAR2])
		{
			// Write changer for Selchar2
			ASMStart(0x806c8744, "[CM: _ThemeChange] Theme Changer (selchar2) " + codeVersion + " [QuickLava]");  // Hooks "process/[scEnding]/sc_ending.o", credit to SammiHusky for the hook!
			WriteIntToFile(0x38a5f308); // Restore Original Instruction
			themeChangerBody("/menu2/", themeConstants::tpi_SELCHAR2, 5);
			ASMEnd();
		}
	}
}
void selMapChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1 && THEME_FILE_GOT_UNIQUE_PREFIX[themeConstants::tpi_SELMAP])
	{
		ASMStart(0x806c8e14, "[CM: _ThemeChange] Theme Changer (selmap) " + codeVersion + " [QuickLava]"); // Hooks "start/[scSelStage]/sc_sel_stage.o".
		WriteIntToFile(0x38a5f3f0); // Restore Original Instruction
		themeChangerBody("/menu2/", themeConstants::tpi_SELMAP, 5);
		ASMEnd();
	}
}
void selEventChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1 && THEME_FILE_GOT_UNIQUE_PREFIX[themeConstants::tpi_SELEVENT])
	{
		ASMStart(0x806c4574, "[CM: _ThemeChange] Theme Changer (selevent) " + codeVersion + " [QuickLava]"); // Hooks "start/[scSelEvent]/sc_sel_event.o".
		WriteIntToFile(0x38a5d0b8); // Restore Original Instruction
		themeChangerBody("/menu2/", themeConstants::tpi_SELEVENT, 5);
		ASMEnd();
	}
}
void titleChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1 && THEME_FILE_GOT_UNIQUE_PREFIX[themeConstants::tpi_TITLE])
	{
		ASMStart(0x806ca150, "[CM: _ThemeChange] Theme Changer (title) " + codeVersion + " [QuickLava]"); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		WriteIntToFile(0x38a5f9a0); // Restore Original Instruction
		themeChangerBody("menu2/", themeConstants::tpi_TITLE, 5);
		ASMEnd();
	}
}
