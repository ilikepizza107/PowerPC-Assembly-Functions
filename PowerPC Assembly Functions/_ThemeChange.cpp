#include "stdafx.h"
#include "_ThemeChange.h"

void WriteByteVec(const unsigned char* Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite)
{
	SetRegister(addressReg, Address); // Load destination address into register

	unsigned long fullWordCount = numToWrite / 4; // Get the number of 4 byte words we can make from the given vec.
	unsigned long currWord = 0;
	unsigned long offsetIntoBytes = 0;

	// For each word we can make...
	for (unsigned long i = 0; i < fullWordCount; i++)
	{
		// ... grab it, then store it in our manip register.
		currWord = lava::bytesToFundamental<unsigned long>(Bytes + offsetIntoBytes);
		SetRegister(manipReg, currWord);
		// Then write a command which stores the packed word into the desired location.
		STW(manipReg, addressReg, offsetIntoBytes);
		offsetIntoBytes += 0x04;
	}
	// For the remaining few bytes...
	for (offsetIntoBytes; offsetIntoBytes < numToWrite; offsetIntoBytes++)
	{
		// ... just write them one by one, no other choice really.
		SetRegister(manipReg, Bytes[offsetIntoBytes]);
		STB(manipReg, addressReg, offsetIntoBytes);
	}
}
void WriteByteVec(std::vector<unsigned char> Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite)
{
	WriteByteVec((const unsigned char*)Bytes.data(), Address, addressReg, manipReg, std::min<std::size_t>(Bytes.size(), numToWrite));
}
void WriteByteVec(std::string Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite)
{
	WriteByteVec((const unsigned char*)Bytes.data(), Address, addressReg, manipReg, std::min<std::size_t>(Bytes.size(), numToWrite));
}

void themeChange()
{
	menuMainChange(); selCharChange(); selMapChange(); selEventChange(); titleChange();
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
					WriteByteVec("mu", menuMainAddr1, reg1, reg2, 2);
					WriteByteVec("mu", menuDirectory.size() + menuMainAddr2, reg1, reg2, 2);
				}
				else
				{
					WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuMainAddr1, reg1, reg2, 2);
					WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + menuMainAddr2, reg1, reg2, 2);
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
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selCharAddr1, reg1, reg2, 2);
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selCharAddr2, reg1, reg2, 2);

				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selChar2Addr1, reg1, reg2, 2);
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selChar2Addr2, reg1, reg2, 2);

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
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selMapAddr1, reg1, reg2, 2);
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selMapAddr2, reg1, reg2, 2);
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
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), selEventAddr1, reg1, reg2, 2);
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + selEventAddr2, reg1, reg2, 2);
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

		const std::string menuDirectory = "menu2/";
		constexpr unsigned long titleAddr2 = 0x806FF9A0;

		ASMStart(0x806ca14c); // Hooks " next/[adList<Ul,42>]/sc_fig_get_demo.o".
		SetRegister(reg1, THEME_SETTING_INDEX); // Load the location of the Theme Setting line into our first register.
		LWZ(reg2, reg1, Line::VALUE); // Then Look 0x08 past that address to get the selected index.

		for (std::size_t i = 0; i < THEME_SUFFIX_LIST.size(); i++) // For each Theme...
		{
			If(reg2, EQUAL_I, i); // ... add a case for that index.
			{
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), titleAddr1, reg1, reg2, 2);
				WriteByteVec(THEME_SUFFIX_LIST[i].substr(0, 2), menuDirectory.size() + titleAddr2, reg1, reg2, 2);
				SetRegister(reg2, -1); // Load the location of the Theme Setting line into our first register.
			}EndIf();
		}

		ASMEnd(0x80df0378); // Restore the instruction replaced by the branch; lwz	r6, 0x0378 (r31).
	}
}
