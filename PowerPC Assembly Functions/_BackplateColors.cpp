#include "stdafx.h"
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


void overrideSetFontColorRGBA(int red, int green, int blue, int alpha)
{
	SetRegister(5, red);
	SetRegister(6, green);
	SetRegister(7, blue);
	SetRegister(8, alpha);
}
void resultsScreenNameFix(int reg1, int reg2)
{
	overrideSetFontColorRGBA(0xFF, 0xFF, 0xFF, 0xA0);
	// Index to relevant message
	LWZ(reg1, 3, 0x0C);
	MULLI(reg2, 4, 0x48);
	ADD(reg1, reg1, reg2);
	// Load message's format byte
	LBZ(reg2, reg1, 0x00);
	// Disable text stroke (zero out 2's bit)
	ANDI(reg2, reg2, 0b11111101);
	// Store message's format byte
	STB(reg2, reg1, 0x00);
}
void transparentCSSandResultsScreenNames()
{
	// If Color Changer is enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;

		ASMStart(0x800ea73c, "[CM: _BackplateColors] Results Screen Player Names are Transparent " + codeVersion + " [QuickLava]"); // Hooks "initMdlData/[ifVsResultTask]/if_vsresult.o".
		resultsScreenNameFix(reg1, reg2);
		ASMEnd();

		ASMStart(0x800ea8c0); // Hooks same as above.
		resultsScreenNameFix(reg1, reg2);
		ASMEnd();

		ASMStart(0x8069b268, "[CM: _BackplateColors] CSS Player Names are Transparent " + codeVersion + " [QuickLava]"); // Hooks "dispName/[muSelCharPlayerArea]/mu_selchar_player_area_obj".
		overrideSetFontColorRGBA(0xFF, 0xFF, 0xFF, 0xB0);
		ASMEnd();
	}
}

void storeTeamBattleStatus()
{
	int reg1 = 11;
	int reg2 = 12;

	ASMStart(0x8068eda8, "[CM: _BackplateColors] Cache SelChar Team Battle Status");
	// Set up store val (optimization to allow future uses of it to just lbz the val, then lwzx with it)
	MULLI(reg2, 4, Line::DEFAULT - Line::VALUE);
	ADDI(reg2, reg2, Line::VALUE);
	// Store team battle status in our buffer word.
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	STB(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	ASMEnd(0x7c7c1b78); // Restore original instruction: mr	r28, r3

	ASMStart(0x806e10dc, "[CM: _BackplateColors] Cache Classic Mode Team Battle Status");
	// Set up store val (optimization to allow future uses of it to just lbz the val, then lwzx with it)
	MULLI(reg2, 0, Line::DEFAULT - Line::VALUE);
	ADDI(reg2, reg2, Line::VALUE);
	// Store team battle status in our buffer word.
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	STB(0, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	ASMEnd(0x88170009); // Restore Original Instruction: lbz	r0, 0x0009 (r23)

	// 806dcf00
	// 806e0aec setSimpleSetting/[sqSingleSimple]/sq_single_simple.o
	// 800500a0 gmInitMeleeDataDefault/[gmMeleeInitData]/gm_lib.o
	// 806e0c30 setStageSetting/[sqSingleSimple]/sq_single_simple.o - triggers after match end in classic, r0 is 0 tho
	// 806e10cc setStageSetting/[sqSingleSimple]/sq_single_simple.o 
	// 806e10d8 setStageSetting/[sqSingleSimple]/sq_single_simple.o - triggers after match end, r0 is 1!
	//// 806e0bc8
	//ASMStart(0x806e10e0, "[CM: _BackplateColors] Cache globalModeMelee Team Battle Status in Code Menu");

	//// Store team battle status in our buffer word.
	//// Note, it's fine we overwrite the existing value here, as we always want the most up to date status.
	//ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	//STB(0, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);

	//ASMEnd(0x3b000001); // Restore original instruction: stb	r0, 0x0013 (r31)
}

void randomColorChange()
{
	// If Color Changer is enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
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
		MULLI(reg2, costumeIDReg, 0x04);
		// Add that to first entry's location and Load line INDEX value
		ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 0x10);
		LWZ(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0xFFFF);
		// Load buffered Team Battle Status Offset
		ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
		LBZ(reg1, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Use it to load the relevant value.
		LWZX(costumeIDReg, reg2, reg1);

		ASMEnd(0x3b5b0004); // Restore Original Instruction: addi	r26, r27, 4
	}
}

void menSelChrElemntChange()
{
	// If Color Changer is enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;
		int reg3 = 10;

		const std::string activatorString = "lBC3";
		int applyChangeLabel = GetNextLabel();
		int exitLabel = GetNextLabel();

		// Hooks "GetResAnmClr/[nw4r3g3d7ResFileCFPCc]/g3d_resfile.o".
		ASMStart(0x8018da3c, "[CM: _BackplateColors] MenSelChr Element Override " + codeVersion + " [QuickLava]",
			"\nIntercepts calls to certain player-slot-specific Menu CLR0s, and redirects them according"
			"\nto the appropriate Code Menu line. Intended for use with:"
			"\n\t- MenSelchrCentry4_TopN__#\n\t- MenSelchrChuman4_TopN__#\n\t- MenSelchrCoin_TopN__#\n\t- MenSelchrCursorB_TopN__#"
			"\nTo trigger this code on a given CLR0, set its \"OriginalPath\" field to \"" + activatorString + "\" in BrawlBox!"
		);

		// If the previous search for the targeted CLR0 was successful...
		If(3, NOT_EQUAL_I_L, 0x00);
		{
			// ... check if returned CLR0 has the activator string set.
			LWZ(reg2, 3, 0x18);
			LWZX(reg2, reg2, 3);
			SetRegister(reg1, activatorString);

			// If the activator string isn't set, then we can exit.
			CMPL(reg2, reg1, 0);
			JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

			// Otherwise, we need to overwrite the target string's requested index.
			// First, we need to use strlen to get the end of the string.
			// That'll require overwriting the current r3 though, so we'll store that in reg1.
			MR(reg3, 3);

			// Then, load the string address into r3...
			MR(3, 31);
			// ... then load the strlen's address...
			SetRegister(reg2, 0x803F0640);
			// ... and run it.
			MTCTR(reg2);
			BCTRL();

			// Now that r3 is the length of the string, we can Subtract one to get the index we'll actually be overwriting.
			ADDI(3, 3, -1);
			// Load the current index byte into r3...
			LBZX(4, 3, 31);
			// ... and subtract '0' from it so we get the index as a number. Keep this!
			ADDI(4, 4, -1 * (unsigned char)'0');

			// Multiply the index by 4 to calculate offset into Backplate Color LOC Entries.
			MULLI(reg2, 4, 0x04);
			// Add that to first entry's location and Load line INDEX value
			ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 0x10);
			LWZ(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0xFFFF);
			// Load buffered Team Battle Status Offset
			ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
			LBZ(reg1, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
			// Use it to load the relevant value.
			LWZX(reg2, reg2, reg1);

			// If the retrieved line value isn't the same as the original value...
			If(reg2, NOT_EQUAL_L, 4);
			{
				// ... add '0' to it to get the ASCII hex for the index...
				ADDI(reg2, reg2, '0');
				// ... and store it at the destination location in the string, as calculated before.
				STBX(reg2, 3, 31);

				// Lastly, prepare to run _vc.
				// Restore the params to what they were when we last ran _vC...
				ADDI(3, 1, 0x08);
				MR(4, 31);
				// ... load _vc's address...
				SetRegister(reg2, 0x8018CF30);
				// ... and run it.
				MTCTR(reg2);
				BCTRL();
			}
			Else();
			{
				MR(3, reg3);
			}
			EndIf();
		}
		EndIf();

		Label(exitLabel);

		ASMEnd(0x80010024); // Restore Original Instruction: lwz	r0, 0x0024 (sp)
	}
}

void shieldColorChange()
{
	// If Color Changer is enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;
		int reg3 = 3;

		CodeRaw("[CM: _BackplateColors] Shield Color + Death Plume Override " + codeVersion + " [QuickLava]",
			"Overrides IC-Basic[21029], which is only used by Shield and Death Plume to determine their colors, at least as far as I can tell,"
			"\nto instead report the selected value in the Code Menu line associated with the color that would've been requested."
			, 
			{
				0xC6855a9c, 0x80855ab0,	// Force CPU Case to branch to our hook below.
			}
		);

		// Hooks "getVariableIntCore/[ftValueAccesser]/ft_value_accesser.o", more specifically intercepting calls to IC-Basic[21029] (used by Shield and Death Plume).
		ASMStart(0X80855ab0, "", "");
		// Calculate offset into Backplate Color LOC Entries and Load the Relevant Index
		MULLI(reg2, reg3, 0x04); // reg3 is desired color frame.
		// Add that to first entry's location
		ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 0x10);
		LWZ(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0xFFFF);
		// Load buffered Team Battle Status Offset
		ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
		LBZ(reg1, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Use it to load the relevant value.
		LWZX(reg3, reg2, reg1);

		// Restore original instruction, we need to branch to 0x80855ab8
		SetRegister(reg1, 0x80855ab8);
		MTCTR(reg1);
		BCTR();

		ASMEnd();
	}
}
void infoPacCLR0ColorChange()
{
	int reg1 = 11;
	int reg2 = 12;
	int targetColorReg = 0;

	CodeRaw("[CM: _BackplateColors] Disable CPU Team Colors", "",
		{
			0x040e0a88, 0x38600000	// Overwrite op "rlwinm	r3, r0, 1, 31, 31 (80000000)" with li r3, 0
		});

	int skipLabel = GetNextLabel();

	CodeRaw("[CM: _BackplateColors] In-Game HUD Color Changer (Info.pac CLR0s) " + codeVersion + " [QuickLava]",
		"Overrides the color parameter passed into the \"setStockMarkColor\" to redirect to the desired color."
		,
		{
			0xC60e0a68, 0x800e0a5c,	// Force CPU Case to branch to our hook below.
		}
	);

	// Note, we know specifically that we *aren't* in team mode in this case, so we don't have to check!
	// Multiply target color by 4 to calc offset to relevant code menu line.
	ASMStart(0x800e0a5c, "", "");

	CMPLI(targetColorReg, 4, 0);
	JumpToLabel(skipLabel, bCACB_GREATER);
	MULLI(reg2, targetColorReg, 0x04);
	// Add that to first entry's location to get offset to target line.
	ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 0x10);
	LWZ(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0xFFFF);
	// ... and load the line's value.
	LWZ(reg2, reg2, Line::VALUE);
	// Then subtract 1, since the game'll add 1 later anyway, and put it in the color register.
	ADDI(targetColorReg, reg2, -1);

	Label(skipLabel);
	ASMEnd(0x901e0024); // Restore Original Instruction: stw	r0, 0x0024 (r30)
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

	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;

		int exitLabel = GetNextLabel();

		const std::string activatorString = "lBC1";

		CodeRaw("[CM: _BackplateColors] Hand Color Fix " + codeVersion + " [QuickLava]",
			"Fixes a conflict with Eon's Roster-Size-Based Hand Resizing code, which could"
			"\nin some cases cause CSS hands to wind up the wrong color."
, 
			{ 
				0x0469CA2C, 0xC0031014, // op	lfs	f0, 0x1014 (r3) @ 8069CA2C
				0x0469CAE0, 0xC0031014	// op	lfs	f0, 0x1014 (r3) @ 8069CAE0
			}
		);

		// Hooks "SetFrame/[nw4r3g3d15AnmObjMatClrResFf]/g3d_anmclr.o".
		ASMStart(0x80197fac, "[CM: _BackplateColors] CSS + Results HUD Color Changer " + codeVersion + " [QuickLava]",
			"\nIntercepts the setFrameMatCol calls used to color certain Menu elements by player slot, and redirects them according"
			"\nto the appropriate Code Menu lines. Intended for use with:"
			"\n\tIn sc_selcharacter.pac:"
			"\n\t\t- MenSelchrCbase4_TopN__0\n\t\t- MenSelchrCursorA_TopN__0\n\t\t- MenSelchrCmark4_TopN__0"
			"\n\tIn stgresult.pac:"
			"\n\t\t- InfResultRank#_TopN__0\n\t\t- InfResultMark##_TopN"
			"\nTo trigger this code on a given CLR0, set its \"OriginalPath\" field to \"" + activatorString + "\" in BrawlBox!"
		);

		// Grab 0x2C past the pointer in r3, and we've got the original CLR0 data now.
		LWZ(reg1, 3, 0x2C);

		// Grab the offset for the "Original Path" Value
		LWZ(reg2, reg1, 0x18);

		// Grab the first 4 bytes of the Orig Path
		LWZX(reg2, reg2, reg1);
		// Set reg1 to our Activation String
		SetRegister(reg1, activatorString);
		// Compare the 4 bytes we loaded to the target string...
		CMPL(reg2, reg1, 0);
		// ... and exit if they're not equal.
		JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

		// Convert target frame to an integer, and copy it into reg1
		SetRegister(reg1, SET_FLOAT_REG_TEMP_MEM);
		FCTIWZ(3, 1);
		STFD(3, reg1, 0x00);
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
		// Now multiply by 4 to calculate the offset to the line we want.
		MULLI(reg2, reg1, 0x04);
		// Add that to first entry's location and Load line INDEX value
		ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 0x10);
		LWZ(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0xFFFF);
		// Load buffered Team Battle Status Offset
		ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
		LBZ(reg1, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Use it to load the relevant value.
		LWZX(reg2, reg2, reg1);

		// And now, to perform black magic int-to-float conversion, as pilfered from the Brawl game code lol.
		// Set reg1 to our staging location
		SetRegister(reg1, SET_FLOAT_REG_TEMP_MEM);

		// Store the integer to convert at the tail end of our Float staging area
		STW(reg2, reg1, 0x04);
		// Set reg2 equal to 0x4330, then store it at the head of our staging area
		ADDIS(reg2, 0, 0x4330);
		STW(reg2, reg1, 0x00);
		// Load that value into fr3 now
		LFD(3, reg1, 0x00);
		// Load the global constant 0x4330000000000000 float from 0x805A36EA
		ADDIS(reg2, 0, 0x805A);
		LFD(1, reg2, 0x36E8);
		// Subtract that constant float from our constructed float...
		FSUB(1, 3, 1);

		// ... and voila! conversion done. Not entirely sure why this works lol, though my intuition is that
		// it's setting the exponent part of the float such that the mantissa ends up essentially in plain decimal format.
		// Exponent ends up being 2^52, and a double's mantissa is 52 bits long, so seems like that's what's up.
		// Genius stuff lol.

		Label(exitLabel);

		ASMEnd(0xfc600890); // Restore original instruction: fmr	f3, f1
	}

}



void selCharColorOverrideBody(int colorReg)
{
	int reg1 = 11;
	int reg2 = 12;
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(colorReg, reg1, (BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF) + 1);
}

void selCharColorFrameOverrideBody(int colorReg)
{
	int reg1 = 11;
	int reg2 = 12;
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(reg1, reg1, (BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF) + 1);
	// This is the one for the setFrameMatCol calls, so we need to subtract 1 from the target value.
	ADDI(colorReg, reg1, -1);
}

void selcharCLR0ColorChange()
{
	const std::string codeGroupName = "[CM: _BackplateColors] Selchar HUD Color Changer";

	int reg1 = 11;
	int reg2 = 12;
	int r3ValueReg = 26;

	ASMStart(0x80698a68, codeGroupName + " (Setup) "  + codeVersion + " [QuickLava]",
		"Pulls r3 + 0x1B0 (player slot, used for picking colors within this function), determines the"
		"\ncode menu line for that color, and caches it for use in overriding color calls in this function!"
	);
	// Pull the original Slot Value from the r3 Object...
	LWZ(reg2, r3ValueReg, 0x1B0);
	// ... and multiply it by 4...
	MULLI(reg2, reg2, 0x04);
	// ... and use to get the relevant code menu line loc.
	ORIS(reg2, reg2, BACKPLATE_COLOR_1_LOC >> 0x10);
	LWZ(reg2, reg2, BACKPLATE_COLOR_1_LOC & 0x0000FFFF);
	// Load the line's value...
	LWZ(reg2, reg2, Line::VALUE);
	// ... and store it 1 byte into the Team Battle Store Loc. We'll reuse this for the rest of this function!
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	STB(reg2, reg1, (BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF) + 1);
	ASMEnd(0x90a10008); // Restore Original Instruction: stw	r5, 0x0008 (sp)

	ASMStart(0x80698ea8, codeGroupName + " (0) " + codeVersion + " [QuickLava]", "");
	selCharColorFrameOverrideBody(0);
	ASMEnd();

	ASMStart(0x80698f48, codeGroupName + " (1) " + codeVersion + " [QuickLava]", "");
	selCharColorFrameOverrideBody(0);
	ASMEnd();

	ASMStart(0x80698f48, codeGroupName + " (2) " + codeVersion + " [QuickLava]", "");
	selCharColorFrameOverrideBody(0);
	ASMEnd();

	ASMStart(0x80699010, codeGroupName + " (3) " + codeVersion + " [QuickLava]", "");
	selCharColorOverrideBody(29);
	ASMEnd();

	ASMStart(0x80699050, codeGroupName + " (4) " + codeVersion + " [QuickLava]", "");
	selCharColorOverrideBody(4);
	ASMEnd();

	ASMStart(0x8069ca18, "[CM: _BackplateColors] Override UpdateMeleeKind for Hand in Non-Team Case " + codeVersion + " [QuickLava]", "");
	selCharColorFrameOverrideBody(5);
	ASMEnd();

	ASMStart(0x80696f58, "[CM: _BackplateColors] Override getColorNo in Non-Team Case " + codeVersion + " [QuickLava]", "");
	// Multiply target color by 4, as part of getting address for relevant line.
	MULLI(reg1, 3, 0x4);
	ORIS(reg1, 0, BACKPLATE_COLOR_1_LOC >> 0x10);
	LWZ(reg1, reg1, BACKPLATE_COLOR_1_LOC & 0xFFFF);
	LWZ(3, reg1, Line::VALUE);
	ADDI(3, 3, 1);
	ASMEnd();


	// Not called but present, probs not needed, can comment these out.
	//
	//ASMStart(0x80698c58, codeGroupName + " (5) " + codeVersion + " [QuickLava]", "");
	//selCharColorFrameOverrideBody(0);
	//ASMEnd();
	//
	//ASMStart(0x80698cf4, codeGroupName + " (6) " + codeVersion + " [QuickLava]", "");
	//selCharColorFrameOverrideBody(0);
	//ASMEnd();
	//
	//ASMStart(0x80698db0, codeGroupName + " (7) " + codeVersion + " [QuickLava]", "");
	//selCharColorOverrideBody(31);
	//ASMEnd();
	//
	//ASMStart(0x80698dec, codeGroupName + " (8) " + codeVersion + " [QuickLava]", "");
	//selCharColorOverrideBody(4);
	//ASMEnd();
}
