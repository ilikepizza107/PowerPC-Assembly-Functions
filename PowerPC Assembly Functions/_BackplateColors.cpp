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


// Shield Color Notes:
// Goes through function: "GetResAnmClr/[nw4r3g3d7ResFileCFUl]/(g3d_resfile.o)" 0x8018dae8
//		Call Stack:
//			0x80064e30 (in make_model/[nw4r3g3d6ScnMdlRP16gfModelAnimatio]/ec_resour)
//			0x80064448 (in mk_model/[ecResource]/ec_resource.o)
// First param is pointer to I think the BRRES?
//	- Yep, pointer to Model Data [13]
//	- Whatever brres the CLR0 is in probs lol
// Second param is index of anim to use (eg. "_0", "_1", "_2", etc)
// Forcing second param to zero forces us to load the p1 shield and death plume
// It looks like after the call to _vc, r3 is the pointer to the CLR0s in the brres
// From there, it looks like it looks (0x10 * (index + 1)) + 0x14 past to get the data
// But if you go back 4 bytes from there, that's the string!
//

// Menu Notes:
// Cursor Color:
// Goes through function: "GetResAnmClr/[nw4r3g3d7ResFileCFPCc]/g3d_resfile.o" 0x8018d9e4
//  SetActionNo?
// Suffix for animation is already applied by the time we get to this function, have to intercept earlier
// Hand Color:
// Goes through function: "updateTeamColor/[muSelCharHand]/mu_selchar_hand.o" 0x8069ca64
// Works a lot like the setFrameCol func, just need to intercept the frame being prescribed and overwrite it
//


void menSelChrElemntChange()
{
	// If CSS Rosters are enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;

		constexpr int stringReg = 4;

		int applyChangeLabel = GetNextLabel();
		int exitLabel = GetNextLabel();

		ASMStart(0x800b5f20, "[CM: _BackplateColors] MenSelChr Element Override " + codeVersion + " [QuickLava]"); // Hooks "setActionNo/[MuObject]/mu_object.o".

		ADDI(stringReg, 1, 88); // Restore Original Instruction

		LWZ(reg1, stringReg, 0x00);
		SetRegister(reg2, "MenS");
		If(reg1, EQUAL_L, reg2);
		{
			LWZ(reg1, stringReg, 0x08);
			SetRegister(reg2, "rCen");
			If(reg1, EQUAL_L, reg2);
			{
				std::size_t indexDestinationOffset = std::string("MenSelchrCentry4_TopN__0").size() - 1;
				ADDI(reg1, stringReg, indexDestinationOffset); // Location to Write Index
				JumpToLabel(applyChangeLabel); // Jump to apply block if they match
			}
			EndIf();
			SetRegister(reg2, "rChu");
			If(reg1, EQUAL_L, reg2);
			{
				std::size_t indexDestinationOffset = std::string("MenSelchrChuman4_TopN__0").size() - 1;
				ADDI(reg1, stringReg, indexDestinationOffset); // Location to Write Index
				JumpToLabel(applyChangeLabel); // Jump to apply block if they match
			}
			EndIf();
			SetRegister(reg2, "rCoi");
			If(reg1, EQUAL_L, reg2);
			{
				LWZ(reg1, stringReg, 0x0C);
				SetRegister(reg2, "n_To");
				If(reg1, EQUAL_L, reg2);
				{
					std::size_t indexDestinationOffset = std::string("MenSelchrCoin_TopN__0").size() - 1;
					ADDI(reg1, stringReg, indexDestinationOffset); // Location to Write Index
					JumpToLabel(applyChangeLabel); // Jump to apply block if they match
				}
				EndIf();
			}
			EndIf();
			SetRegister(reg2, "rCur");
			If(reg1, EQUAL_L, reg2);
			{
				std::size_t indexDestinationOffset = std::string("MenSelchrCursorB_TopN__0").size() - 1;
				ADDI(reg1, stringReg, indexDestinationOffset); // Location to Write Index
				JumpToLabel(applyChangeLabel); // Jump to apply block if they match
			}
			EndIf();
		}
		EndIf();
		JumpToLabel(exitLabel);


		Label(applyChangeLabel);

		// Borrow the stringRegister for a moment, to store the target index
		// We'll put this back later lol
		LBZ(stringReg, reg1, 0x00);
		// Subtract '0' from it so we get the index as a number.
		ADDI(stringReg, stringReg, -1 * (unsigned char)'0');

		// Calculate offset into Backplate Color LOC Entries
		SetRegister(reg2, 0x4);
		MULLW(reg2, reg2, stringReg);
		// Add that to first entry's location
		ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 16);
		ADDI(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0x0000FFFF);
		// Load line INDEX value
		LWZ(reg2, reg2, 0x00);
		// Load the current value at that line.
		LWZ(reg2, reg2, Line::VALUE);
		// Add '0' to it to get the ASCII hex for the index.
		ADDI(reg2, reg2, '0');

		// Store it at the destination location in the string, as calculated before.
		STB(reg2, reg1, 0x00);

		ADDI(stringReg, 1, 88); // Restore stringRegister's Value

		Label(exitLabel);

		ASMEnd();
	}
}

void shieldColorChange()
{
	// If CSS Rosters are enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;
		int reg3 = 4;

		int applyChangeLabel = GetNextLabel();
		int exitLabel = GetNextLabel();

		ASMStart(0x8018db38, "[CM: _BackplateColors] Shield Color + Death Plume Override " + codeVersion + " [QuickLava]"); // Hooks "GetResAnmClr/[nw4r3g3d7ResFileCFUl]/g3d_resfile.o".

		// Get string offset for the first CLR0 in the list
		LWZ(reg1, 3, 0x20);
		// Add that offset to beginning of list to get string pointer
		ADD(reg1, reg1, 3);
		// Check string length >= 12
		LWZ(reg2, reg1, -0x4);
		If(reg2, GREATER_OR_EQUAL_I, 0xC);
		{
			// If so, check for the "ommo" string, which should help ensure we only look at common effects
			LWZ(reg2, reg1, 0x4);
			// And load "ommo" into reg3 to compare it against
			SetRegister(reg3, "ommo");
			If(reg2, EQUAL_L, reg3);
			{
				// If so, grab the 9th - 12th characters to check against our constants
				LWZ(reg2, reg1, 0x8);
				// And load "nShi" into reg3 to compare it against
				SetRegister(reg3, "nShi");
				If(reg2, EQUAL_L, reg3);
				{
					JumpToLabel(applyChangeLabel); // Jump to apply block if they match
				}
				EndIf();
				// If that failed, load "nDea" to compare against next
				SetRegister(reg3, "nDea");
				If(reg2, EQUAL_L, reg3);
				{
					JumpToLabel(applyChangeLabel); // Jump to apply block if they match
				}
				EndIf();
			}
			EndIf();
		}
		EndIf();
		JumpToLabel(exitLabel);


		Label(applyChangeLabel);

		// Calculate offset into Backplate Color LOC Entries
		SetRegister(reg2, 0x4);
		MULLW(reg2, reg2, 31);
		// Add that to first entry's location
		ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 16);
		ADDI(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0x0000FFFF);
		// Load line INDEX value
		LWZ(reg2, reg2, 0x00);
		// Load the current value at that line.
		LWZ(31, reg2, Line::VALUE);

		Label(exitLabel);


		ASMEnd(0x381f0001); // Restore original instruction: addi r0, r31, 1
	}
}

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