#include "_CSSRosterChange.h"

const std::string codeVersion = "v1.0.0";

enum colorConstants
{
	cc_CLEAR = 0x0000,
	cc_RED = 0x3ff0,
	cc_BLUE = 0x4000,
	cc_YELLOW = 0x4008,
	cc_GREEN = 0x4010,
	cc_PINK = 0x4014,
	cc_PURPLE = 0x4018,
	cc_ORANGE = 0x401c,
	cc_TEAL = 0x4020,
	cc_GRAY = 0x4022,
	cc_COLOR10 = 0x4024,
	cc_COLOR11 = 0x4026,
	cc_COLOR12 = 0x4028,
	cc_COLOR13 = 0x402a,
	cc_COLOR14 = 0x402c,
	cc_COLOR15 = 0x402e,
};


void backplateColorChange()
{
	// 00 = Clear
	// 01 = P1
	// 02 = P2
	// 03 = P3
	// 04 = P4
	// 05 = Pink
	// 06 = Purple
	// 07 = Orange
	// 08 = Teal
	// 09 = CPU Grey

	// If CSS Rosters are enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;

		ASMStart(0x800e1900, "[CM: _BackplateColors] Set Override Disabled Flag " + codeVersion + " [QuickLava]"); // Hooks "setDamageNumColor/[IfPlayer]/if_player.o"

		SetRegister(reg1, 0x1);

		ASMEnd(0x7fc3f378); // Restores original instruction: mr	r3, r30

		ASMStart(0x800b7a74, "[CM: _BackplateColors] HUD Color Changer " + codeVersion + " [QuickLava]"); // Hooks "setFrameMatCol/[MuObject]/mu_object.o".

		If(reg1, NOT_EQUAL_I, 0x1);
		{
			// Select Appropriate Line
			SetRegister(reg1, SET_FLOAT_REG_TEMP_MEM);
			STFD(31, reg1, 0x00);
			LHZ(reg1, reg1, 0x00);

			SetRegister(reg2, 0xFFFF);
			// Red
			If(reg1, EQUAL_I, colorConstants::cc_RED);
			{
				SetRegister(reg2, BACKPLATE_COLOR_1_INDEX);
			}EndIf();
			// Blue
			If(reg1, EQUAL_I, colorConstants::cc_BLUE);
			{
				SetRegister(reg2, BACKPLATE_COLOR_2_INDEX);
			}EndIf();
			// Yellow
			If(reg1, EQUAL_I, colorConstants::cc_YELLOW);
			{
				SetRegister(reg2, BACKPLATE_COLOR_3_INDEX);
			}EndIf();
			// Green
			If(reg1, EQUAL_I, colorConstants::cc_GREEN);
			{
				SetRegister(reg2, BACKPLATE_COLOR_4_INDEX);
			}EndIf();
			// CPU
			If(reg1, EQUAL_I, colorConstants::cc_GRAY);
			{
				SetRegister(reg2, BACKPLATE_COLOR_C_INDEX);
			}EndIf();
			// CPU
			If(reg1, EQUAL_I, colorConstants::cc_CLEAR);
			{
				SetRegister(reg2, BACKPLATE_COLOR_T_INDEX);
			}EndIf();

			If(reg2, NOT_EQUAL_I, 0xFFFF);
			{
				LWZ(reg2, reg2, Line::VALUE); // Then Look 0x08 past the line's address to get the selected index
				for (std::size_t i = 0; i < 16; i++) // For each color...
				{
					double tempDouble = i;
					std::vector<unsigned char> doubleHex = lava::fundamentalToBytes(tempDouble);

					If(reg2, EQUAL_I, i); // ... add a case for that index...
					{
						SetRegister(reg1, lava::bytesToFundamental<short>(doubleHex.data()));
					}EndIf();
				}

				SetRegister(reg2, SET_FLOAT_REG_TEMP_MEM);
				STH(reg1, reg2, 0x00);
				LFD(31, reg2, 0x00);
			}
			EndIf();
		}
		Else();
		{
			SetRegister(reg1, 0x00);
		}
		EndIf();

		ASMEnd(0xfc20f890); // Restore original instruction: fmr	f1, f31
	}

}