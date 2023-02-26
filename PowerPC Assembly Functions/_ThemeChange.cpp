#include "_ThemeChange.h"

void themeChange()
{
	menuMainChange(); selCharChange(); selMapChange(); selEventChange(); titleChange();
}
void editFilepathConstant(std::string fileDirectory, std::string baseFilename, unsigned long pathRegister, unsigned long defaultPathAddress1, unsigned long defaultPathAddress2, themeConstants::themePathIndices relevantFile)
{
	int reg1 = pathRegister;
	int reg2 = 12;

	// Pre-write the default versions of the filename and filepath into their respective locations.
	// This should ensure that, even if we somehow don't activate one of the cases below, a valid value is written in place.
	// Additionally, since we don't respecify the instructions for writing the entire paths in every case, the code should be shorter overall.

	menuTheme* defaultTheme = &THEME_SPEC_LIST[0];

	lava::WriteByteVec(defaultTheme->prefixes[relevantFile] + baseFilename, defaultPathAddress1, reg1, reg2, SIZE_MAX, 1);
	lava::WriteByteVec(fileDirectory + defaultTheme->prefixes[relevantFile] + baseFilename, defaultPathAddress2, reg1, reg2, SIZE_MAX, 1);

	SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
	LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

	// Note, we can start at index 1 because we've already written index 0 as the default above!
	for (std::size_t i = 1; i < THEME_SPEC_LIST.size(); i++) // For each additional Theme...
	{
		If(reg2, EQUAL_I, i); // ... add a case for that index.
		{
			// Initialize our full prefix string.
			// We're gonna write what'll be the first 4 bytes of the patched filename into this variable, which we'll store in reg1.
			// That way, all we're doing in this loop is finding the appropriate case and getting the associated prefix.
			// Then we can just STW a single time once we're out of the loop, minimizing the instruction count of each case, which should
			// help keep the size of the entire code down as more themes are added.
			std::string temporaryPrefixString = "";

			menuTheme* currTheme = &THEME_SPEC_LIST[i];

			// If the current theme's prefix for this file is validly sized, store it in the temporary prefix string.
			if (currTheme->prefixes[relevantFile].size() == themeConstants::prefixLength)
			{
				temporaryPrefixString = currTheme->prefixes[relevantFile];
			}
			// Otherwise, use the default theme's value instead.
			else
			{
				temporaryPrefixString = defaultTheme->prefixes[relevantFile];
			}
			// If our temporary prefix string is less than 4 characters long, fill the remainder with chars from the base filename.
			if (temporaryPrefixString.size() < 4)
			{
				temporaryPrefixString += baseFilename.substr(0, 4 - temporaryPrefixString.size());
			}
			// Store the prefix string in reg1 so we can STW it later.
			SetRegister(reg1, temporaryPrefixString.substr(0, 4));
			SetRegister(reg2, -1); // Null out reg2 to signal we reached an appropriate case.
		}EndIf();
	}
	// If we ran into a valid case earlier...
	If(reg2, EQUAL_I, -1);
	{
		// .., STW the recorded prefix string into the appropriate locations for each address.
		SetRegister(reg2, defaultPathAddress1);
		STW(reg1, reg2, 0);
		SetRegister(reg2, defaultPathAddress2);
		STW(reg1, reg2, fileDirectory.size());
	}EndIf();
}
void menuMainChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 31;
		int reg2 = 12;

		const std::string menuDirectory = "/menu2/";
		const std::string filenameBase = getThemeFileBaseName(themeConstants::tpi_MENUMAIN);
		constexpr unsigned long menuMainAddr1 = 0x817F62BC;
		constexpr unsigned long menuMainAddr2 = 0x806FB248;

		ASMStart(0x806cbfa0, "Theme Changer (menumain) " + version +" [QuickLava]"); // Hooks the fifth instruction of "start/[muMenuMain]/mu_main.o".
		editFilepathConstant(menuDirectory, filenameBase, reg1, menuMainAddr1, menuMainAddr2, themeConstants::tpi_MENUMAIN);
		ASMEnd(0x7c7f1b78); // Restore the instruction replaced by the branch; mr	r31, r3.
	}
}
void selCharChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 4;
		int reg2 = 12;

		const std::string menuDirectory = "/menu2/";
		const std::string filenameBase1 = getThemeFileBaseName(themeConstants::tpi_SELCHAR);
		const std::string filenameBase2 = getThemeFileBaseName(themeConstants::tpi_SELCHAR2);
		constexpr unsigned long selCharAddr1 = 0x817F6365;
		constexpr unsigned long selCharAddr2 = 0x806FF2EC;
		constexpr unsigned long selChar2Addr1 = 0x817F634D;
		constexpr unsigned long selChar2Addr2 = 0x806FF308;

		ASMStart(0x806c8730, "Theme Changer (selchar) " + version + " [QuickLava]"); // Hooks "process/[scEnding]/sc_ending.o", credit to SammiHusky for the hook!
		editFilepathConstant(menuDirectory, filenameBase1, reg1, selCharAddr1, selCharAddr2, themeConstants::tpi_SELCHAR);
		editFilepathConstant(menuDirectory, filenameBase2, reg1, selChar2Addr1, selChar2Addr2, themeConstants::tpi_SELCHAR2);
		ASMEnd(0x3880002b); // Restore the instruction replaced by the branch; li	r4, 43.
	}
}
void selMapChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 31;
		int reg2 = 12;

		const std::string menuDirectory = "/menu2/";
		const std::string filenameBase = getThemeFileBaseName(themeConstants::tpi_SELMAP);
		constexpr unsigned long selMapAddr1 = 0x817F637C;
		constexpr unsigned long selMapAddr2 = 0x806FF3F0;

		ASMStart(0x806c8d88, "Theme Changer (selmap) " + version + " [QuickLava]"); // Hooks the fifth instruction of "start/[scSelStage]/sc_sel_stage.o".
		editFilepathConstant(menuDirectory, filenameBase, reg1, selMapAddr1, selMapAddr2, themeConstants::tpi_SELMAP);
		ASMEnd(0x7c7f1b78); // Restore the instruction replaced by the branch; mr	r31, r3.
	}
}
void selEventChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 30;
		int reg2 = 12;

		const std::string menuDirectory = "/menu2/";
		const std::string filenameBase = getThemeFileBaseName(themeConstants::tpi_SELEVENT);
		constexpr unsigned long selEventAddr1 = 0x817F638D;
		constexpr unsigned long selEventAddr2 = 0x806FD0B8;

		ASMStart(0x806c44b4, "Theme Changer (selevent) " + version + " [QuickLava]"); // Hooks the fifth instruction of "start/[scSelEvent]/sc_sel_event.o".
		editFilepathConstant(menuDirectory, filenameBase, reg1, selEventAddr1, selEventAddr2, themeConstants::tpi_SELEVENT);
		ASMEnd(0x7c7e1b78); // Restore the instruction replaced by the branch; mr	r30, r3.
	}
}
void titleChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 6;
		int reg2 = 12;

		const std::string menuDirectory = "/menu2/";
		const std::string filenameBase = getThemeFileBaseName(themeConstants::tpi_TITLE);
		constexpr unsigned long titleAddr1 = 0x817F63A1;
		constexpr unsigned long titleAddr2 = 0x806FF9A0;

		ASMStart(0x806ca14c, "Theme Changer (title) " + version + " [QuickLava]"); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		editFilepathConstant(menuDirectory, filenameBase, reg1, titleAddr1, titleAddr2, themeConstants::tpi_TITLE);
		ASMEnd(0x80df0378); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
	}
}

//void themeChangeV2()
//{
//	menuMainChange(); selCharChangeV2(); selMapChangeV2(); selEventChangeV2(); titleChangeV2();
//}
//void interceptMenuFilepathRef(unsigned long pathRegister, std::vector<std::string> replacementPathsList, unsigned long defaultPathAddress)
//{
//	int reg1 = pathRegister;
//	int reg2 = 12;
//
//	// Ensure that there is a path provided for every registered theme.
//	assert(THEME_LIST.size() <= replacementPathsList.size());
//
//	SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
//	LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.
//
//	for (std::size_t i = 0; i < replacementPathsList.size(); i++) // For each Theme...
//	{
//		If(reg2, EQUAL_I, i); // ... add a case for that index.
//		{
//			// When we reach the appropriate case, write the corresponding replacement path into the staging area.
//			lava::WriteByteVec(replacementPathsList[i], stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
//			// Signal that we've reached an appropriate case, ensuring we exit this if block.
//			SetRegister(reg2, -1);
//		}EndIf();
//	}
//	// If we didn't reach an appropriate case...
//	If(reg2, NOT_EQUAL_I, -1);
//	{
//		// ... fallback to the default address for this path.
//		SetRegister(reg1, defaultPathAddress);
//	}EndIf();
//}
//void selMapChangeV2()
//{
//	// If Themes are enabled
//	if (THEME_SETTING_INDEX != -1)
//	{
//		int reg1 = 5;
//		int reg2 = 12;
//
//		constexpr unsigned long defaultSelMapAddr = 0x806FF3F0;
//
//		ASMStart(0x806c8e14); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
//
//		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
//		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.
//
//		for (std::size_t i = 0; i < THEME_PREFIX_LIST.size(); i++) // For each Theme...
//		{
//			If(reg2, EQUAL_I, i); // ... add a case for that index.
//			{
//				lava::WriteByteVec("/menu2/" + THEME_PREFIX_LIST[i].substr(0, 2) + "_selmap.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
//				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
//			}EndIf();
//		}
//		If(reg2, NOT_EQUAL_I, -1);
//		{
//			SetRegister(reg1, defaultSelMapAddr);
//		}EndIf();
//
//		ASMEnd(); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
//	}
//}
//void selEventChangeV2()
//{
//	// If Themes are enabled
//	if (THEME_SETTING_INDEX != -1)
//	{
//		int reg1 = 5;
//		int reg2 = 12;
//
//		constexpr unsigned long defaultSelEventAddr = 0x806fd0b8;
//
//		ASMStart(0x806c4574);
//		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
//		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.
//
//		for (std::size_t i = 0; i < THEME_PREFIX_LIST.size(); i++) // For each Theme...
//		{
//			If(reg2, EQUAL_I, i); // ... add a case for that index.
//			{
//				lava::WriteByteVec("/menu2/" + THEME_PREFIX_LIST[i].substr(0, 2) + "_sel_event.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
//				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
//			}EndIf();
//		}
//		If(reg2, NOT_EQUAL_I, -1);
//		{
//			SetRegister(reg1, defaultSelEventAddr);
//		}EndIf();
//
//		ASMEnd(); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
//	}
//}
//void selCharChangeV2()
//{
//	// If Themes are enabled
//	if (THEME_SETTING_INDEX != -1)
//	{
//		int reg1 = 5;
//		int reg2 = 12;
//
//		constexpr unsigned long defaultSelCharAddr = 0x806FF2EC;
//		constexpr unsigned long defaultSelChar2Addr = 0x806FF308;
//
//		// sc_selcharacter.pac
//		ASMStart(0x806c8728); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
//		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
//		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.
//
//		for (std::size_t i = 0; i < THEME_PREFIX_LIST.size(); i++) // For each Theme...
//		{
//			If(reg2, EQUAL_I, i); // ... add a case for that index.
//			{
//				lava::WriteByteVec("/menu2/" + THEME_PREFIX_LIST[i].substr(0, 2) + "_selcharacter.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
//				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
//			}EndIf();
//		}
//		If(reg2, NOT_EQUAL_I, -1);
//		{
//			SetRegister(reg1, defaultSelCharAddr); // If no case matched, set r5 to the default location for its string.
//		}EndIf();
//		ASMEnd();
//
//
//		// sc_selcharacter2.pac
//		ASMStart(0x806c8744); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
//		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
//		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.
//
//		for (std::size_t i = 0; i < THEME_PREFIX_LIST.size(); i++) // For each Theme...
//		{
//			If(reg2, EQUAL_I, i); // ... add a case for that index.
//			{
//				lava::WriteByteVec("/menu2/" + THEME_PREFIX_LIST[i].substr(0, 2) + "_selcharacter2.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
//				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
//			}EndIf();
//		}
//		If(reg2, NOT_EQUAL_I, -1);
//		{
//			SetRegister(reg1, defaultSelChar2Addr); // If no case matched, set r5 to the default location for its string.
//		}EndIf();
//		ASMEnd();
//	}
//}
//void titleChangeV2()
//{
//	// If Themes are enabled
//	if (THEME_SETTING_INDEX != -1)
//	{
//		int reg1 = 5;
//		int reg2 = 12;
//
//		constexpr unsigned long defaultTitleAddr = 0x806FF9A0;
//
//		ASMStart(0x806ca150); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
//		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
//		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.
//
//		for (std::size_t i = 0; i < THEME_PREFIX_LIST.size(); i++) // For each Theme...
//		{
//			If(reg2, EQUAL_I, i); // ... add a case for that index.
//			{
//				lava::WriteByteVec("/menu2/" + THEME_PREFIX_LIST[i].substr(0, 2) + "_title.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
//				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
//			}EndIf();
//		}
//		If(reg2, NOT_EQUAL_I, -1);
//		{
//			SetRegister(reg1, defaultTitleAddr);
//		}EndIf();
//
//		ASMEnd(); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
//	}
//}
