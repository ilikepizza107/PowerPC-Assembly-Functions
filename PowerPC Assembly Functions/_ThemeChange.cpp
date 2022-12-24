#include "_ThemeChange.h"

void themeChange()
{
	menuMainChange(); selCharChangeV2(); selMapChangeV2(); selEventChangeV2(); titleChangeV2();
}

void menuMainChange()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 31;
		int reg2 = 12;

		constexpr unsigned long menuMainAddr1 = 0x817F62BC;

		const std::string menuDirectory = "menu2/";
		constexpr unsigned long menuMainAddr2 = 0x806FB248;

		ASMStart(0x806cbfa0); // Hooks the fifth instruction of "start/[muMenuMain]/mu_main.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				if (i == 0) // Correction to ensure the default case uses the right prefix for menuMain.
				{
					lava::WriteByteVec("mu", menuMainAddr1, reg1, reg2, 2);
					lava::WriteByteVec("mu", menuDirectory.size() + menuMainAddr2, reg1, reg2, 2);
				}
				else
				{
					lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuMainAddr1, reg1, reg2, 2);
					lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + menuMainAddr2, reg1, reg2, 2);
				}
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.

			}EndIf();
		}

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

		constexpr unsigned long selCharAddr1 = 0x817F6365;
		constexpr unsigned long selChar2Addr1 = 0x817F634D;

		const std::string menuDirectory = "/menu2/";
		constexpr unsigned long selCharAddr2 = 0x806FF2EC;
		constexpr unsigned long selChar2Addr2 = 0x806FF308;

		ASMStart(0x806c8730); // Hooks "process/[scEnding]/sc_ending.o", credit to SammiHusky for the hook!
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selCharAddr1, reg1, reg2, 2);
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selCharAddr2, reg1, reg2, 2);

				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selChar2Addr1, reg1, reg2, 2);
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selChar2Addr2, reg1, reg2, 2);

				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}

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

		constexpr unsigned long selMapAddr1 = 0x817F637C;

		const std::string menuDirectory = "/menu2/";
		constexpr unsigned long selMapAddr2 = 0x806FF3F0;

		ASMStart(0x806c8d88); // Hooks the fifth instruction of "start/[scSelStage]/sc_sel_stage.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selMapAddr1, reg1, reg2, 2);
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selMapAddr2, reg1, reg2, 2);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}

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

		constexpr unsigned long selEventAddr1 = 0x817F638D;

		const std::string menuDirectory = "/menu2/";
		constexpr unsigned long selEventAddr2 = 0x806FD0B8;

		ASMStart(0x806c44b4); // Hooks the fifth instruction of "start/[scSelEvent]/sc_sel_event.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selEventAddr1, reg1, reg2, 2);
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selEventAddr2, reg1, reg2, 2);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}

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

		constexpr unsigned long titleAddr1 = 0x817F63A1;

		const std::string menuDirectory = "/menu2/";
		constexpr unsigned long titleAddr2 = 0x806FF9A0;

		ASMStart(0x806ca14c); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), titleAddr1, reg1, reg2, 2);
				lava::WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + titleAddr2, reg1, reg2, 2);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}

		ASMEnd(0x80df0378); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
	}
}

void interceptMenuFilepathRef(unsigned long pathRegister, std::vector<std::string> replacementPathsList, unsigned long defaultPathAddress)
{
	int reg1 = pathRegister;
	int reg2 = 12;

	// Ensure that there is a path provided for every registered theme.
	assert(THEME_LIST.size() <= replacementPathsList.size());

	SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
	LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

	for (std::size_t i = 0; i < replacementPathsList.size(); i++) // For each Theme...
	{
		If(reg2, EQUAL_I, i); // ... add a case for that index.
		{
			// When we reach the appropriate case, write the corresponding replacement path into the staging area.
			lava::WriteByteVec(replacementPathsList[i], stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
			// Signal that we've reached an appropriate case, ensuring we exit this if block.
			SetRegister(reg2, -1);
		}EndIf();
	}
	// If we didn't reach an appropriate case...
	If(reg2, NOT_EQUAL_I, -1);
	{
		// ... fallback to the default address for this path.
		SetRegister(reg1, defaultPathAddress);
	}EndIf();
}

void selMapChangeV2()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 5;
		int reg2 = 12;

		constexpr unsigned long defaultSelMapAddr = 0x806FF3F0;

		ASMStart(0x806c8e14); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".

		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec("/menu2/" + THEME_SUFFIX_LIST[i].substr(0, 2) + "_selmap.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}
		If(reg2, NOT_EQUAL_I, -1);
		{
			SetRegister(reg1, defaultSelMapAddr);
		}EndIf();

		ASMEnd(); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
	}
}
void selEventChangeV2()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 5;
		int reg2 = 12;

		constexpr unsigned long defaultSelEventAddr = 0x806fd0b8;

		ASMStart(0x806c4574);
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec("/menu2/" + THEME_SUFFIX_LIST[i].substr(0, 2) + "_sel_event.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}
		If(reg2, NOT_EQUAL_I, -1);
		{
			SetRegister(reg1, defaultSelEventAddr);
		}EndIf();

		ASMEnd(); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
	}
}
void selCharChangeV2()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 5;
		int reg2 = 12;

		constexpr unsigned long defaultSelCharAddr = 0x806FF2EC;
		constexpr unsigned long defaultSelChar2Addr = 0x806FF308;

		// sc_selcharacter.pac
		ASMStart(0x806c8728); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec("/menu2/" + THEME_SUFFIX_LIST[i].substr(0, 2) + "_selcharacter.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}
		If(reg2, NOT_EQUAL_I, -1);
		{
			SetRegister(reg1, defaultSelCharAddr); // If no case matched, set r5 to the default location for its string.
		}EndIf();
		ASMEnd();


		// sc_selcharacter2.pac
		ASMStart(0x806c8744); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec("/menu2/" + THEME_SUFFIX_LIST[i].substr(0, 2) + "_selcharacter2.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}
		If(reg2, NOT_EQUAL_I, -1);
		{
			SetRegister(reg1, defaultSelChar2Addr); // If no case matched, set r5 to the default location for its string.
		}EndIf();
		ASMEnd();
	}
}
void titleChangeV2()
{
	// If Themes are enabled
	if (THEME_SETTING_INDEX != -1)
	{
		int reg1 = 5;
		int reg2 = 12;

		constexpr unsigned long defaultTitleAddr = 0x806FF9A0;

		ASMStart(0x806ca150); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				lava::WriteByteVec("/menu2/" + THEME_SUFFIX_LIST[i].substr(0, 2) + "_title.pac", stringStagingLocation, reg1, reg2, SIZE_MAX, 1);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}
		If(reg2, NOT_EQUAL_I, -1);
		{
			SetRegister(reg1, defaultTitleAddr);
		}EndIf();

		ASMEnd(); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
	}
}
