#include "_CSSRosterChange.h"

const std::string codeVersion = "v1.0.0";


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
// Coin, Character Outline, Tag Entry Button, and Tag Pane (setActionNo Elements, menSelChrElemntChange):
// As I've learned, these all get their colors set through the "setActionNo" function.
// The param_2 that gets passed in is an index, which gets appended to an object's base name
// to get the name to use when looking up any relevant animations (VISOs, PAT0s, CLR0s, etc.)
// To force a specific one of these to load, you'll wanna intercept the call to the relevant
// "GetResAnm___()" function call (in my case, I'm only interested in CLR0 calls).
// 
// Misc Elements (setFrameMatColor Elements, backplateColorChange):
// I'm gonna need to rewrite this one, it's currently *far* to broad in terms of what it *can* affect.
// The following are functions which call setFrameMatColor which are relevant to player slot colors:
// - setStockMarkColor/[IfPlayer]/(if_player.o): Various In-Battle Things (Franchise Icons)?
//	- Frachise Icon (First Call)
//	- BP Background (Second Call)
//	- Loupe (Third Call)
//	- Arrow (Fourth Call)
// - initMdlData/[ifVsResultTask]/(if_vsresult.o): Various Results Screen Stuff?
// At the point where we hook in this function, r31 is the original r3 value.
// Additionally, if we go *(*(r3 + 0x14) + 0x18), we find ourselves in what looks to be the 
// structure in memory which actually drives the assembled CLR0 data. I'm fairly certain that:
//	- 0x18 is the current frame
//	- 0x1C is the frame advance rate
//	- 0x20 is maybe loop start frame?
//	- 0x24 into this data is the frame count
//	- 0x28 is a pointer to the playback policy (loop, don't loop, etc.)
//	- 0x2C IS THE CLR0 POINTER!
// 
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

	constexpr unsigned short triggerTag = 0xBEEF;

	// If CSS Rosters are enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;

		// The following four codes all hook "setStockMarkColor/[IfPlayer]/if_player.o"
		ASMStart(0x800e21c0, "[CM: _BackplateColors] Set Override Disabled Flag (Franchise Icon) " + codeVersion + " [QuickLava]");
		SetRegister(reg1, triggerTag);
		ASMEnd(0x7fa3eb78); // Restores original instruction: mr	r3, r29

		ASMStart(0x800e21ec, "[CM: _BackplateColors] Set Override Disabled Flag (BP Background) " + codeVersion + " [QuickLava]");
		SetRegister(reg1, triggerTag);
		ASMEnd(0x807f00b8); // Restores original instruction: lwz	r3, 0x00B8 (r31)

		ASMStart(0x800e21fc, "[CM: _BackplateColors] Set Override Disabled Flag (Loupe) " + codeVersion + " [QuickLava]");
		SetRegister(reg1, triggerTag);
		ASMEnd(0x7fa3eb78); // Restores original instruction: mr	r3, r29

		ASMStart(0x800e2228, "[CM: _BackplateColors] Set Override Disabled Flag (Arrow) " + codeVersion + " [QuickLava]");
		SetRegister(reg1, triggerTag);
		ASMEnd(0x7fa3eb78); // Restores original instruction: mr	r3, r29

		ASMStart(0x800e2254, "[CM: _BackplateColors] Set Override Disabled Flag (???) " + codeVersion + " [QuickLava]");
		SetRegister(reg1, triggerTag);
		ASMEnd(0x7fa3eb78); // Restores original instruction: mr	r3, r29


		ASMStart(0x800b7a74, "[CM: _BackplateColors] HUD Color Changer " + codeVersion + " [QuickLava]"); // Hooks "setFrameMatCol/[MuObject]/mu_object.o".

		If(reg1, EQUAL_I_L, triggerTag);
		{
			// Convert target frame to an integer, and copy it into reg1
			SetRegister(reg1, SET_FLOAT_REG_TEMP_MEM);
			FCTIWZ(1, 31);
			STFD(1, reg1, 0x00);
			LWZ(reg1, reg1, 0x04);

			//Shuffle around the requested frames to line up with the Code Menu Lines
			// If we ask for frame 0, point to 1 above Transparent Line
			If(reg1, EQUAL_I, 0x00);
			{
				SetRegister(reg1, 0x06);
			}
			EndIf();
			// If we ask for frame 9, point to 1 above CPU Line
			If(reg1, EQUAL_I, 0x9);
			{
				SetRegister(reg1, 0x05);
			}
			EndIf();
			// Then subtract 1, ultimately correcting everything.
			ADDI(reg1, reg1, -0x1);

			// Calculate which code menu line we should be looking at
			SetRegister(reg2, 0x4);
			MULLW(reg2, reg2, reg1);
			// Add that to first entry's location
			ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 16);
			ADDI(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0x0000FFFF);
			// Load line INDEX value
			LWZ(reg2, reg2, 0x00);
			// Then Look 0x08 past the line's address to get the selected index
			LWZ(reg2, reg2, Line::VALUE);

			// And now, to perform black magic int-to-float conversion, as pilfered from the Brawl game code lol.
			// Set reg1 to our staging location
			SetRegister(reg1, SET_FLOAT_REG_TEMP_MEM);

			// Store the integer to convert at the tail end of our Float staging area
			STW(reg2, reg1, 0x04);
			// Set reg2 equal to 0x4330, then store it at the head of our staging area
			ADDIS(reg2, 0, 0x4330);
			STW(reg2, reg1, 0x00);
			// Load that value into fr31 now
			LFD(31, reg1, 0x00);
			// Load the global constant 0x4330000000000000 float from 0x805A36EA
			ADDIS(reg2, 0, 0x805A);
			LFD(1, reg2, 0x36E8);
			// Subtract that constant float from our constructed float...
			FSUB(31, 31, 1);
			// ... and voila! conversion done. Not entirely sure why this works lol, though my intuition is that
			// it's setting the exponent part of the float such that the mantissa ends up essentially in plain decimal format.
			// Exponent ends up being 2^52, and a double's mantissa is 52 bits long, so seems like that's what's up.
			// Genius stuff lol.

			// Ensure r11 trigger tag is unset.
			SetRegister(reg1, 0x00);
		}
		EndIf();

		ASMEnd(0xfc20f890); // Restore original instruction: fmr	f1, f31
	}

}