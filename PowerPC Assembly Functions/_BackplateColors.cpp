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
//	- 0x14 is flags, potentially?
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

// This code is usable between both updateTeamColor and updateMeleeKind for the hand, so
// I've moved the bulk of it out into this function here to ensure things are consistent
// between the two.
void handColorChangeBody(int reg1, int reg2, int reg3)
{
	// Convert target frame to an integer, and copy it into reg1
	SetRegister(reg3, SET_FLOAT_REG_TEMP_MEM);
	FCTIWZ(1, 1);
	STFD(1, reg3, 0x00);
	LWZ(reg1, reg3, 0x04);
	// We'll be using this to index into our Code Menu lines, so
	// if it's greater than 0, we'll subtract 1 (Red is usually called via frame 1).
	If(reg1, GREATER_I_L, 0x00);
	{
		ADDI(reg1, reg1, -0x01);
	}
	// Use that to calculate which code menu line we should be looking at
	SetRegister(reg2, 0x4);
	MULLW(reg2, reg2, reg1);
	// Add that to first entry's location
	ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 16);
	ADDI(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0x0000FFFF);
	// Load line INDEX value
	LWZ(reg2, reg2, 0x00);
	// Then Look 0x08 past the line's address to get the selected index
	LWZ(reg2, reg2, Line::VALUE);
	// Convert the prescribed frame back into a double:
	// Store the prescribed frame at the end of the double staging area.
	STW(reg2, reg3, 0x04);
	// Write the conversion double's head to the front of the area, and load it into a register.
	SetRegister(reg2, 0x43300000);
	STW(reg2, reg3, 0x00);
	LFD(1, reg3, 0x00);
	// Load the 0x4330 double to subtract with.
	ADDIS(reg1, 0, 0x805A);
	LFD(0, reg1, 0x36E8);
	// Subtract to finish the conversion.
	FSUB(1, 1, 0);
}
void handColorChange()
{

	// If Color Changer is enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		// In team color case, the frame gets one added to it while it's converted to Double
		// Frames:
		//	- 0x00 Red2?
		//	- 0x01 Red Main
		//	- 0x02 Blue
		//	- 0x03 Yellow
		//	- 0x04 Green Main
		//	- 0x05 Green2?
		//
		
		int reg1 = 11;
		int reg2 = 12;
		int reg3 = 3;

		ASMStart(0x8069ca3c, "[CM: _BackplateColors] CSS Hand Color Change (meleeKind) " + codeVersion + " [QuickLava]"); // Hooks "setFrameMatCol/[MuObject]/mu_object.o".

		handColorChangeBody(reg1, reg2, reg3);
		
		ASMEnd(0x807f004c); // Restore original instruction: lwz	r3, 0x004C (r31)

		ASMStart(0x8069caf0, "[CM: _BackplateColors] CSS Hand Color Change (teamColor) " + codeVersion + " [QuickLava]"); // Hooks "setFrameMatCol/[MuObject]/mu_object.o".

		handColorChangeBody(reg1, reg2, reg3);

		ASMEnd(0x807f004c); // Restore original instruction: lwz	r3, 0x004C (r31)
	}
}

void randomColorChange()
{
	int reg1 = 11;
	int reg2 = 12;
	int	playerKindReg = 23;
	int	fighterKindReg = 31;
	int	costumeIDReg = 24;

	ASMStart(0x80697554, "[CM: _BackplateColors] MenSelChr Random Color Override " + codeVersion + " [QuickLava]"); // Hooks "setCharPic/[muSelCharPlayerArea]/mu_selchar_player_area_o".

	// Where we're hooking, we guarantee that we're dealing with the Random portrait.
	// The costumeIDReg at this moment is guaranteed to range from 0 (Red) to 4 (CPU).
	// Calculate offset into Backplate Color LOC Entries
	SetRegister(reg2, 0x4);
	MULLW(reg2, reg2, costumeIDReg);
	// Add that to first entry's location
	ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 16);
	ADDI(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0x0000FFFF);
	// Load line INDEX value
	LWZ(reg2, reg2, 0x00);
	// Load the current value at that line into the costume register.
	LWZ(costumeIDReg, reg2, Line::VALUE);

	ASMEnd(0x3b5b0004); // Restore Original Instruction: addi	r26, r27, 4
}

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
		int reg3 = 3;

		int exitLabel = GetNextLabel();
		int applyChangesLabel = GetNextLabel();

		ASMStart(0x800b7a70, "[CM: _BackplateColors] HUD Color Changer " + codeVersion + " [QuickLava]"); // Hooks "setFrameMatCol/[MuObject]/mu_object.o".

		// r31 is the original r3 value. Grab the address from 0x14 past that...
		LWZ(reg1, 31, 0x14);
		// ... then the address 0x18 past that, putting us at I think CLR0 Anm Obj.
		LWZ(reg1, reg1, 0x18);
		// Now grab 0x2C past that, and we've got the original CLR0 data now.
		LWZ(reg1, reg1, 0x2C);
		// Grab the number of frames in the CLR0.
		LHZ(reg2, reg1, 0x1C);
		CMPLI(reg2, 10, 0);
		JumpToLabel(exitLabel, bCACB_LESSER);

		// Now grab the string offset, from 0x14 past the CLR0 pointer...
		LWZ(reg2, reg1, 0x14);
		// ... and add it to the CLR0 pointer to get the string.
		ADD(reg2, reg1, reg2);

		// From here, we're going to do some checks to identify whether the CLR0 we're looking at is one we want to override.
		// Each area will specify strings to try to catch, as well as a starting position, and will iterate through them all
		// until we've found one.
		std::vector<std::string> stringsToCatch;
		std::size_t startingPos = SIZE_MAX;

		// Load the first 4 bytes of the string.
		LWZ(reg1, reg2, 0x00);

		// Char Select CLR0s
		SetRegister(reg3, "MenS");
		If(reg1, EQUAL_L, reg3);
		{
			startingPos = 0x08;
			stringsToCatch = {
				"MenSelchrCbase4_TopN__0",
				"MenSelchrCmark4_TopN__0",
			};

			LWZ(reg1, reg2, startingPos);
			for (std::size_t i = 0; i < stringsToCatch.size(); i++)
			{
				SetRegister(reg3, stringsToCatch[i].substr(startingPos, 0x04));
				CMPL(reg3, reg1, 0);
				JumpToLabel(applyChangesLabel, bCACB_EQUAL);
			}
			JumpToLabel(exitLabel);
		}
		EndIf();
		// In-Game, Results Creen CLR0s
		SetRegister(reg3, "Inf");
		// Set the bottom char of the string to 0.
		// Note: because we're changing the reg holding the loaded part of the string, keep
		// this group last, or your comparisons won't work as you expect.
		RLWINM(reg1, reg1, 0x0, 0x00, 0x17);
		If(reg1, EQUAL_L, reg3);
		{
			stringsToCatch = {
				"InfArrow_TopN__0",
				"InfFace_TopN__0",
				"InfLoupe0_TopN__0",
				"InfMark_TopN__0",
				"InfPlynm_TopN__0",
				"InfResultRank#_TopN__0",
				"InfResultMark##_TopN",
			};
			startingPos = 0x06;

			LWZ(reg1, reg2, startingPos);
			for (std::size_t i = 0; i < stringsToCatch.size(); i++)
			{
				SetRegister(reg3, stringsToCatch[i].substr(startingPos, 0x04));
				CMPL(reg3, reg1, 0);
				JumpToLabel(applyChangesLabel, bCACB_EQUAL);
			}
			JumpToLabel(exitLabel);
		}
		EndIf();

		Label(applyChangesLabel);
		
		// Convert target frame to an integer, and copy it into reg1
		SetRegister(reg1, SET_FLOAT_REG_TEMP_MEM);
		FCTIWZ(1, 31);
		STFD(1, reg1, 0x00);
		LWZ(reg1, reg1, 0x04);

		// This is only really here for the Results Screen Franchise icons.
		// Those ask for normal frame + 10 as you click through the stats.
		// This prevents that from happening.
		If(reg1, GREATER_OR_EQUAL_I_L, 10);
		{
			ADDI(reg1, reg1, -10);
		}
		EndIf();

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

		Label(exitLabel);

		ASMEnd(0x807f0014); // Restore original instruction: lwz	r3, 0x0014 (r31)
	}

}