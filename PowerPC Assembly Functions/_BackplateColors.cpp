#include "stdafx.h"
#include "_BackplateColors.h"

const std::string codePrefix = "[CM: _BackplateColors] ";
const std::string codeVersion = "v2.0.1";
const std::string codeSuffix = " " + codeVersion + " [QuickLava]";

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

void playerSlotColorChangers(unsigned char codeLevel)
{
	// If Color Changer is enabled, and the code isn't disabled by the codeLevel arg, write code.
	if ((BACKPLATE_COLOR_1_INDEX == -1) || (codeLevel <= backplateColorConstants::pSCL_NONE) || (codeLevel >= backplateColorConstants::pSCL__COUNT)) return;

	// These are always needed, do these first.
	storeTeamBattleStatus();
	shieldColorChange();

	// If you've got more than 10 colors defined, and are using a mode that affects the HUD...
	if ((BACKPLATE_COLOR_TOTAL_COLOR_COUNT > 10) && (codeLevel > backplateColorConstants::pSCL_SHIELDS_AND_PLUMES_ONLY))
	{
		// ... we need to disable CPU Team Colors.
		// Necessary because the game stores CPU Team Colors 10 frames above the originals, so you'd be overwriting those colors.
		disableCPUTeamColors();
	}

	// Then address the specific needs of the other cases.
	switch (codeLevel)
	{
		// This case only additionally needs the infoPac code, include that and break.
	case backplateColorConstants::pSCL_SHIELDS_PLUMES_AND_IN_GAME_HUD:
	{
		infoPacCLR0ColorChange();
		break;
	}
	// This case needs the increment input code, in addition to the stuff in the following case.
	case backplateColorConstants::pSCL_MENUS_AND_IN_GAME_WITH_CSS_INPUT:
	{
		incrementOnButtonPress();
	}
	// And this case includes the code to drive the changer on the CSS + Results Screen.
	case backplateColorConstants::pSCL_MENUS_AND_IN_GAME_WITHOUT_CSS_INPUT:
	{
		backplateColorChange();
		menSelChrElemntChange();
		transparentCSSandResultsScreenNames();
		randomColorChange();
		break;
	}
	default:
	{
		break;
	}
	}
}

void incrementOnButtonPress()
{
	// If Color Changer is enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;
		int reg3 = 30; // Safe to use, overwritten by the instruction following our hook.
		int padReg = 0; // Note, we can use this reg after using the pad data from it
		int padPtrReg = 25;

		int applyChangesLabel = GetNextLabel();
		int exitLabel = GetNextLabel();

		ASMStart(0x8068b168, codePrefix + "Incr and Decr Slot Color with L/R, Reset with Z on Player Kind Button" + codeSuffix);

		// If we're hovering over the player status button.
		CMPI(26, 0x1D, 0);
		JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

		// Disable input if we're in team mode (also set up reg1 with top half of Code Menu Addr).
		ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
		LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		CMPI(reg2, Line::DEFAULT, 0);
		JumpToLabel(exitLabel, bCACB_EQUAL);

		// Multiply slot value by 4, move it into reg1
		RLWIMI(reg1, 29, 2, 0x10, 0x1D);
		// And use that to grab the relevant line's INDEX Value
		LWZ(reg1, reg1, BACKPLATE_COLOR_1_LOC & 0xFFFF);

		// If Z Button is pressed...
		RLWINM(reg2, padReg, bitIndexFromButtonHex(BUTTON_Z) + 1, 31, 31, 1);
		// ... reset the slot's color value.
		LWZ(reg3, reg1, Line::DEFAULT);
		JumpToLabel(applyChangesLabel, bCACB_NOT_EQUAL);

		// Setup incr/decrement value
		// Shift down BUTTON_R bit to use it as a bool, reg2 is 1 if set, 0 if not
		RLWINM(reg2, padReg, bitIndexFromButtonHex(BUTTON_R) + 1, 31, 31);
		// Shift down BUTTON_L bit to use it as a bool, reg3 is 1 if set, 0 if not
		RLWINM(reg3, padReg, bitIndexFromButtonHex(BUTTON_L) + 1, 31, 31);
		// Subtract reg3 from reg2! So if L was pressed, and R was not, reg2 = -1. L not pressed, R pressed, reg2 = 1.
		// Additionally, set the condition bit, and if the result of this subtraction was 0 (ie. either both pressed or neither pressed) we skip.
		SUBF(reg2, reg2, reg3, 1);
		JumpToLabel(exitLabel, bCACB_EQUAL);

		// Load the line's current option into reg3...
		LWZ(reg3, reg1, Line::VALUE);
		// ... and add our modification value to it.
		ADD(reg3, reg3, reg2);

		// If modified value is greater than the max...
		CMPI(reg3, BACKPLATE_COLOR_TOTAL_COLOR_COUNT - 1, 0);
		// ... roll its value around to the min.
		BC(2, bCACB_LESSER_OR_EQ);
		ADDI(reg3, 0, 0);

		// If modified value is less than the min...
		CMPI(reg3, 0, 0);
		// ... roll its value around to the max.
		BC(2, bCACB_GREATER_OR_EQ);
		ADDI(reg3, 0, BACKPLATE_COLOR_TOTAL_COLOR_COUNT - 1);

		// Store our modified value back in place.
		Label(applyChangesLabel);
		STW(reg3, reg1, Line::VALUE);

		// Use value in r4 to grab current player status
		LWZ(reg1, 4, 0x44);
		LWZ(reg2, reg1, 0x1B4);
		// Subtract 1 from it.
		ADDI(reg2, reg2, -1);
		// Put it back.
		STW(reg2, reg1, 0x1B4);

		// Set padReg to A button press to piggyback off the setPlayerKind call to update our colors.
		ADDI(padReg, 0, BUTTON_A);

		Label(exitLabel);

		ASMEnd(0x540005ef); // Restore Original Instruction: rlwinm.	r0, r0, 0, 23, 23 (00000100)
	}
}

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

		ASMStart(0x800ea73c, codePrefix + "Results Screen Player Names are Transparent" + codeSuffix); // Hooks "initMdlData/[ifVsResultTask]/if_vsresult.o".
		resultsScreenNameFix(reg1, reg2);
		ASMEnd();

		ASMStart(0x800ea8c0); // Hooks same as above.
		resultsScreenNameFix(reg1, reg2);
		ASMEnd();

		CodeRaw("", "", 
			{
				0x040ea724, 0x60000000, // Disable Team Battle Player Case
			});

		ASMStart(0x8069b268, codePrefix + "CSS Player Names are Transparent" + codeSuffix); // Hooks "dispName/[muSelCharPlayerArea]/mu_selchar_player_area_obj".
		overrideSetFontColorRGBA(0xFF, 0xFF, 0xFF, 0xB0);
		ASMEnd();
	}
}

void storeTeamBattleStatusBody(int statusReg)
{
	int reg1 = 11;
	int reg2 = 12;
	MULLI(reg2, statusReg, Line::DEFAULT - Line::VALUE);
	ADDI(reg2, reg2, Line::VALUE);
	// Store team battle status in our buffer word.
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	STB(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
}
void storeTeamBattleStatus()
{
	int reg1 = 11;
	int reg2 = 12;

	// Hooks "initDispSelect/[muSelCharPlayerArea]/mu_selchar_player_ar"
	ASMStart(0x806945e8, codePrefix + "Reset Cached SelChar Team Battle Status on Init" + codeSuffix);
	storeTeamBattleStatusBody(3);
	ASMEnd(0x7c651b78); // Restore original instruction: mr	r5, r3

	// Hooks "setMeleeKind/[muSelCharTask]/mu_selchar_obj.o"
	ASMStart(0x8068eda8, codePrefix + "Cache SelChar Team Battle Status" + codeSuffix);
	storeTeamBattleStatusBody(4);
	ASMEnd(0x7c7c1b78); // Restore original instruction: mr	r28, r3

	// Hooks "appear/[IfPlayer]/if_player.o"
	ASMStart(0x800e0a44, codePrefix + "Cache In-game Mode Team Battle Status" + codeSuffix);
	storeTeamBattleStatusBody(0);
	ASMEnd(0x2c000000); // Restore Original Instruction: cmpwi	r0, 0

	// Hooks "setAdventureCondition/[sqSingleBoss]/sq_single_boss.o"
	ASMStart(0x806e5f08, codePrefix + "Only 2P Stadium Boss Battles Are Considered Team Battles" + codeSuffix);
	// Where we're hooking, we're in a loop being used to initialize player slots for Stadium Boss Battles.
	// Here, r23 is the iterator register (starts at 0, increments once per cycle), and r30 is a pointer to the GameModeMelee struct.
	// This specific line we're hooking is in a block which indicates that the given player slot is active, so on the first run
	// through we guarantee set the Team Mode byte to '0', and *if* a second player exists, we'll set it to '1' on the second run through.
	STB(23, 30, 0x13);
	ASMEnd(0x9bf90099); // Restore Original Instruction: stb	r31, 0x0099 (r25)

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

void disableCPUTeamColors()
{
	CodeRaw(codePrefix + "Disable CPU Team Colors" + codeSuffix, "",
		{
			0x040e0a88, 0x38600000,	// Overwrite op "rlwinm	r3, r0, 1, 31, 31 (80000000)" with "li r3, 0"
			0x040e6cc0, 0xc3829164, // Overwrite op "lfs	f28, -0x6E94 (rtoc)" with "lfs	f28, -0x6E9C (rtoc)"
			0x040e7120, 0xc3c29164, // Overwrite op "lfs	f30, -0x6E94 (rtoc)" with "lfs	f30, -0x6E9C (rtoc)"
			0x040ea290, 0xc3829164, // Overwrite op "lfs	f28, -0x6E94 (rtoc)" with "lfs	f28, -0x6E9C (rtoc)"
		});
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

		ASMStart(0x80697554, codePrefix + "MenSelChr Random Color Override" + codeSuffix); // Hooks "setCharPic/[muSelCharPlayerArea]/mu_selchar_player_area_o".

		// Where we're hooking, we guarantee that we're dealing with the Random portrait.
		// The costumeIDReg at this moment is guaranteed to range from 0 (Red) to 4 (CPU).

		// Load buffered Team Battle Status Offset
		ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
		LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Now multiply the target color by 4 to calculate the offset to the line we want, and insert it into reg1.
		RLWIMI(reg1, costumeIDReg, 2, 0x10, 0x1D);
		LWZ(reg1, reg1, BACKPLATE_COLOR_1_LOC & 0xFFFF);
		// Use it to load the relevant value.
		LWZX(costumeIDReg, reg1, reg2);

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

		const std::string activatorString = "lBC3";
		int exitLabel = GetNextLabel();

		// Hooks "GetResAnmClr/[nw4r3g3d7ResFileCFPCc]/g3d_resfile.o".
		ASMStart(0x8018da3c, codePrefix + "MenSelChr Element Override" + codeSuffix,
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

			// Then, load the string address into r3...
			MR(3, 31);
			// ... then load strlen's address...
			SetRegister(reg2, 0x803F0640);
			// ... and run it.
			MTCTR(reg2);
			BCTRL();

			// Now that r3 is the length of the string, we can subtract one to get the index we'll actually be overwriting.
			ADDI(3, 3, -1);
			// Load the current index byte into r4, and store the location we'll be writing back to in r3 (will be our destination ptr for our sprintf later!)
			LBZUX(4, 3, 31);

			// Load buffered Team Battle Status Offset
			ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
			LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
			// Now, simultaneously chop off the 5th and 6th bits of the ASCII byte value to subtract 0x30,
			// and multiply the target color by 4 by shifting left by 2, to calculate the offset to the line we want, and insert it into reg1.
			RLWIMI(reg1, 4, 2, 0x1A, 0x1D);
			// Use that to load our target line's index value...
			LWZ(reg1, reg1, BACKPLATE_COLOR_1_LOC & 0xFFFF);
			// ... then use that to load the relevant value into r5 (our value argument for our upcoming sprintf call!)
			LWZX(5, reg1, reg2);

			// Load sprintf ptr into CTR in preparation for running it.
			SetRegister(reg1, 0x803f89fc);
			MTCTR(reg1);

			// Write and skip over our formatting string...
			BL(2);
			WriteIntToFile(0x25580000); // "%X\0\0"
			// ... and move the pointer to it into r4, as our formatting string pointer.
			MFLR(4);

			// Launch sprintf, which will write our suffix into place!
			BCTRL();

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
		EndIf();

		Label(exitLabel);

		ASMEnd(0x80010024); // Restore Original Instruction: lwz	r0, 0x0024 (sp)
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

	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;
		int reg3 = 0; // Was used previously to store LR, gets overwritten later anyway

		int exitLabel = GetNextLabel();

		const std::string activatorString = "lBC1";

		CodeRaw(codePrefix + "Hand Color Fix" + codeSuffix,
			"Fixes a conflict with Eon's Roster-Size-Based Hand Resizing code, which could"
			"\nin some cases cause CSS hands to wind up the wrong color."
, 
			{ 
				0x0469CA2C, 0xC0031014, // op	lfs	f0, 0x1014 (r3) @ 8069CA2C
				0x0469CAE0, 0xC0031014	// op	lfs	f0, 0x1014 (r3) @ 8069CAE0
			}
		);

		ASMStart(0x8068b5d4, codePrefix + "Updating Player Kind (Player->CPU->None) Updates Hand Color" + codeSuffix);
		// Pull Hand Ptr back from r31...
		MR(3, 31);
		// ... and call "updateColorNo/[muSelCharHand]/mu_selchar_hand.o"!
		SetRegister(reg2, 0x8069c698);
		MTCTR(reg2);
		BCTRL();
		ASMEnd(0x807e01a8); // Restore Original Instruction: lwz	r3, 0x01A8 (r30)

		CodeRaw(codePrefix + "Disable Franchise Icon Color 10-Frame Offset in Results Screen" + codeSuffix, "",
			{
				0xC60ebb98, 0x800ebbb8, // Branch Past Second Mark Color Set
				0xC60ebde4, 0x800ebe00, // Branch Past Third Mark Color Set
			});

		// Hooks "SetFrame/[nw4r3g3d15AnmObjMatClrResFf]/g3d_anmclr.o".
		ASMStart(0x80197fac, codePrefix + "CSS, In-game, and Results HUD Color Changer" + codeSuffix,
			"\nIntercepts the setFrameMatCol calls used to color certain Menu elements by player slot, and"
			"\nredirects them according to the appropriate Code Menu lines. Intended for use with:"
			"\n\tIn sc_selcharacter.pac:"
			"\n\t\t- MenSelchrCbase4_TopN__0\n\t\t- MenSelchrCmark4_TopN__0\n\t\t- MenSelchrCursorA_TopN__0"
			"\n\tIn info.pac (and its variants, eg. info_training.pac):"
			"\n\t\t- InfArrow_TopN__0\n\t\t- InfFace_TopN__0\n\t\t- InfLoupe0_TopN__0\n\t\t- InfMark_TopN__0\n\t\t- InfPlynm_TopN__0"
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
		LWZ(reg3, reg1, 0x04);

		// If the target frame corresponds to one of the code menu lines, we'll skip execution.
		CMPLI(reg3, 1, 0);
		JumpToLabel(exitLabel, bCACB_LESSER);
		CMPLI(reg3, 5, 0);
		JumpToLabel(exitLabel, bCACB_GREATER);

		// Load buffered Team Battle Status Offset
		ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
		LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Now multiply the target frame by 4 to calculate the offset to the line we want, and insert it into reg1.
		RLWIMI(reg1, reg3, 2, 0x10, 0x1D);
		LWZ(reg1, reg1, (BACKPLATE_COLOR_1_LOC & 0xFFFF) - 0x4); // Minus 0x4 because target frame is 1 higher than the corresponding line.
		// Use it to load the relevant value.
		LWZX(reg2, reg1, reg2);

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

void shieldColorChange()
{
	// If Color Changer is enabled
	if (BACKPLATE_COLOR_1_INDEX != -1)
	{
		int reg1 = 11;
		int reg2 = 12;
		int reg3 = 3;

		CodeRaw(codePrefix + "Shield Color + Death Plume Override" + codeSuffix,
			"Overrides IC-Basic[21029], which is only used by Shield and Death Plume to"
			"\ndetermine their colors (at least as far as I can tell) to instead report the selected"
			"\nvalue in the Code Menu line associated with the color that would've been requested."
			,
			{
				0xC6855a9c, 0x80855ab0,	// Force CPU Case to branch to our hook below.
			}
		);

		// Hooks "getVariableIntCore/[ftValueAccesser]/ft_value_accesser.o", more specifically intercepting calls to IC-Basic[21029] (used by Shield and Death Plume).
		ASMStart(0X80855ab0, "", "");

		// Load buffered Team Battle Status Offset
		ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
		LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Now multiply the target color by 4 to calculate the offset to the line we want, and insert it into reg1.
		RLWIMI(reg1, reg3, 2, 0x10, 0x1D);
		LWZ(reg1, reg1, BACKPLATE_COLOR_1_LOC & 0xFFFF);
		// Use it to load the relevant value.
		LWZX(reg3, reg1, reg2);

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

	int skipLabel = GetNextLabel();

	// If CPU Team Colors don't need to be disabled
	if (BACKPLATE_COLOR_TOTAL_COLOR_COUNT <= 10)
	{
		ASMStart(0x800e0a94, codePrefix + "CPU Team Color Fix (Info.pac CLR0s)" + codeSuffix);
		MULLI(reg1, 3, 4);
		ADD(0, 0, reg1);
		ASMEnd(0x901e0024); // Restore Original Instruction: stw	r0, 0x0024 (r30)
	}

	CodeRaw(codePrefix + "In-Game HUD Color Changer (Info.pac CLR0s)" + codeSuffix,
		"Overrides the color parameter passed into the \"setStockMarkColor\" to redirect to the desired color."
		,
		{
			0xC60e0a68, 0x800e0a5c,	// Force CPU Case to branch to our hook below.
			0x040e2108, 0x60000000, // Disable >4 Case in SetStockMarkColor
		}
	);

	// Hooks "appear/[IfPlayer]/if_player.o"
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


// Unused Code
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
	const std::string codeGroupName = codePrefix + "Selchar HUD Color Changer";

	int reg1 = 11;
	int reg2 = 12;
	int r3ValueReg = 26;

	// This code (and the following, unless otherwise noted) hook "updateMeleeKind/[muSelCharPlayerArea]/mu_selchar_player_a"
	ASMStart(0x80698a68, codeGroupName + " (Setup)"  + codeSuffix,
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

	ASMStart(0x80698ea8, codeGroupName + " (0)" + codeSuffix, "");
	selCharColorFrameOverrideBody(0);
	ASMEnd();

	ASMStart(0x80698f48, codeGroupName + " (1)" + codeSuffix, "");
	selCharColorFrameOverrideBody(0);
	ASMEnd();

	ASMStart(0x80698f48, codeGroupName + " (2)" + codeSuffix, "");
	selCharColorFrameOverrideBody(0);
	ASMEnd();

	ASMStart(0x80699010, codeGroupName + " (3)" + codeSuffix, "");
	selCharColorOverrideBody(29);
	ASMEnd();

	ASMStart(0x80699050, codeGroupName + " (4)" + codeSuffix, "");
	selCharColorOverrideBody(4);
	ASMEnd();

	// Not called but present, probs not needed, can comment these out.
	//
	//ASMStart(0x80698c58, codeGroupName + " (5)" + codeSuffix, "");
	//selCharColorFrameOverrideBody(0);
	//ASMEnd();
	//
	//ASMStart(0x80698cf4, codeGroupName + " (6)" + codeSuffix, "");
	//selCharColorFrameOverrideBody(0);
	//ASMEnd();
	//
	//ASMStart(0x80698db0, codeGroupName + " (7)" + codeSuffix, "");
	//selCharColorOverrideBody(31);
	//ASMEnd();
	//
	//ASMStart(0x80698dec, codeGroupName + " (8)" + codeSuffix, "");
	//selCharColorOverrideBody(4);
	//ASMEnd();

	// Hooks "updateMeleeKind/[muSelCharHand]/mu_selchar_hand.o"
	ASMStart(0x8069ca18, codePrefix + "Override UpdateMeleeKind for Hand in Non-Team Case" + codeSuffix, "");
	selCharColorFrameOverrideBody(5);
	ASMEnd();

	// Hooks "getColorNo/[muSelCharPlayerArea]/mu_selchar_player_area_o"
	ASMStart(0x80696f58, codePrefix + "Override getColorNo in Non-Team Case" + codeSuffix, "");
	// Multiply target color by 4, as part of getting address for relevant line.
	MULLI(reg1, 3, 0x4);
	ORIS(reg1, 0, BACKPLATE_COLOR_1_LOC >> 0x10);
	LWZ(reg1, reg1, BACKPLATE_COLOR_1_LOC & 0xFFFF);
	LWZ(3, reg1, Line::VALUE);
	ADDI(3, 3, 1);
	ASMEnd();
}
void resultsColorFrameOverrideBody(int workingReg, int colorReg)
{
	// Multiply the slot index by 4 to get offset into LOC entries for relevant line...
	MULLI(workingReg, colorReg, 0x04);
	// ... and use that ot load the relevant line's INDEX value.
	ADDIS(workingReg, 0, BACKPLATE_COLOR_1_LOC >> 0x10);
	LWZ(workingReg, workingReg, BACKPLATE_COLOR_1_LOC & 0xFFFF);
	// Load the line's value into reg1.
	LWZ(workingReg, workingReg, Line::VALUE);
}
void resultsCLR0ColorChange()
{
	const std::string codeGroupName = codePrefix + "Results HUD Color Changer";

	int reg1 = 11;
	int reg2 = 12;

	// Do Initial Backplate Color Override, Hooks "processAnim/[ifVsResultTask]/if_vsresult.o"
	ASMStart(0x800e6d58, codeGroupName + " (Backplate Fix)" + codeSuffix,
		"The hook does the initial color override, and the C6 code prevents the color changing after that.");
	resultsColorFrameOverrideBody(reg1, 20);
	STW(reg1, 1, 0x01D4); // Replace Original Instruction: stw	reg1, 0x01D4 (sp)
	ASMEnd(); 
	CodeRaw("", "", 
		{
			0xC60e71d0, 0x800e7228, // Branch Past Second Backplate Color Set
			0x040e6d64, 0xFC200090, // Replaces "fadds	f1,f29,f0" with "fmr f1, f0"
		});

	// Do Initial Mark Color Override, Hooks "getMarkColAnimFrame/[ifVsResultTask]/if_vsresult.o"
	ASMStart(0x800e8ea0, codeGroupName + " (Mark Fix)" + codeSuffix,
		"The Hook does the initial color override, and the C6 codes prevent the color changing after that.");
	resultsColorFrameOverrideBody(4, 4);
	ASMEnd(0x9081000c); // Restore Original Instruction: stw	r4, 0x000C (sp)
	CodeRaw("", "",
		{
			0xC60ebb98, 0x800ebbb8, // Branch Past Second Mark Color Set
			0xC60ebde4, 0x800ebe00, // Branch Past Third Mark Color Set
			0x040e8eb8, 0x60000000, // Replaces "fadds	f1,f0,f1" with "nop"
		});
}
