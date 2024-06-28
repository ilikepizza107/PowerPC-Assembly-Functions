#include "stdafx.h"
#include "_PlayerSlotColorChangers.h"

const std::string codePrefix = "[CM: _PlayerSlotColorChangers v3.1.2] ";
const std::string codeSuffix = " [QuickLava]";

// New approach:
// - Verify that incoming CLR0 is asking for override
// - Determine the index of the frame they're asking for
// - Determine that it's in our range
// - Get the index for the color being requested by the relevant code menu line
// - Grab the stored "RGBA" for that frame; not actually RGBA, it's the following
//     0x00 = Unsigned 8 - bit Hue Shift(Fixed Point)
//     0x01 = Unsigned 8 - bit Saturation(Fixed Point)
//     0x02 = Unsigned 8 - bit Luminence(Fixed Point)
//     0x03 = Unsigned 8 - bit Alpha
// - Convert the above to floats; adding the hueshift to our code line's hue
// - Follow the rest of the HSL -> RGB conversion process to get new RGB
// - Write that into r3 to make that the result color.
// - Profit
// Idea: Separate Color Definitions from Scheme Definitions
// Colors: Define the HSL for an actual color itself, collect these into a Color Table
//         Probs get 0x10 bytes each, 0xC for the HSL, 0x4 for config? Or perhaps 0x4 for Callback?
// Palettes: These are what players are actually picking from! Specify some set of Colors to be used by the game!
//           Probs get 0x8 bytes each? 0x4 for 4 Color IDs, 0x4 for any per-scheme configuration stuff.
// Another Idea: Callback System?
// Approach 1: Each color entry has a word to designate the address of a callback function, once per frame we run each func on each color?
//	Pros: Clean, can easily just lwz-mtctr-bctrl to run it
//  Cons: Takes 4 bytes, and I don't want entries to exceed 0x20 bytes per, so if we allow a word for flags or something too we only get 1.
// Approach 2: Each color has a flags word, and each flag corresponds to a request to call or not call a given stock function.
//  Pros: Can potentially chain together long sequences of effects to achieve interesting results, and even 8 function requests is 1 byte.
//  Cons: Makes externally defining new functions to expand functionality more complicated? Every additional function requires +1 bit.
//


const std::string activatorStringBase = "lBC0";
signed short activatorStringHiHalf = lava::bytesToFundamental<signed short>((unsigned char*)activatorStringBase.data());
signed short activatorStringLowHalf = lava::bytesToFundamental<signed short>((unsigned char*)activatorStringBase.data() + 2);
unsigned long safeStackWordOff = 0x10; // Stores version of the code we need to run if signal word found; 0xFFFFFFFF otherwise!

void psccIncrementOnButtonPress()
{
	int reg0 = 0;
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 10;
	int reg4 = 30; // Safe to use, overwritten by the instructions following our hook.

	int padPtrReg = 25;
	int collKindIDReg1 = 3;
	int collKindIDReg2 = 26;
	int controllerIDReg = 28;
	int playerAreaIDReg = 29;

	// Thresholds for registering and unregistering a trigger press.
	constexpr unsigned char analogPressThreshold = 0x60;
	constexpr unsigned char analogUnpressThreshold = 0x30;
	// CR Bit IDs for digital press checks.
	constexpr unsigned char digitalPressCRF = 7;
	constexpr unsigned char digitalZPressBit = crBitInCRF(EQ, digitalPressCRF);
	constexpr unsigned char digitalZPressBitFromRight = 31 - digitalZPressBit;
	constexpr unsigned char digitalLPressBit = crBitInCRF(LT, digitalPressCRF);
	constexpr unsigned char digitalLPressBitFromRight = 31 - digitalLPressBit;
	constexpr unsigned char digitalRPressBit = crBitInCRF(GT, digitalPressCRF);
	constexpr unsigned char digitalRPressBitFromRight = 31 - digitalRPressBit;

	// Setup branch destination labels.
	int decrCheckEndLabel = GetNextLabel();
	int incrCheckEndLabel = GetNextLabel();
	int decrCheckUnpressLabel = GetNextLabel();
	int incrCheckUnpressLabel = GetNextLabel();
	int decrInputDisabledLabel = GetNextLabel();
	int incrInputDisabledLabel = GetNextLabel();
	int applyChangesLabel = GetNextLabel();
	int storeValueLabel = GetNextLabel();
	int exitLabel = GetNextLabel();

	ASMStart(0x8068B168, codePrefix + "Incr and Decr Slot Color with L/R, Reset with Z on Player Kind Button" + codeSuffix);
	// If the A Button was pressed...
	RLWINM(reg2, reg0, bitIndexFromButtonHex(BUTTON_A) + 1, 31, 31, 1);
	// ... that takes priority over anything else, exit!
	JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

	// Check if the Z Button is pressed, store in reg2.
	RLWINM(reg2, reg0, bitIndexFromButtonHex(BUTTON_Z) + 1 + digitalZPressBitFromRight, digitalZPressBit, digitalZPressBit);
	// Check if the L Button is pressed, store in reg2.
	RLWIMI(reg2, reg0, bitIndexFromButtonHex(BUTTON_L) + 1 + digitalLPressBitFromRight, digitalLPressBit, digitalLPressBit);
	// Check if the R Button is pressed, store in reg2.
	RLWIMI(reg2, reg0, bitIndexFromButtonHex(BUTTON_R) + 1 + digitalRPressBitFromRight, digitalRPressBit, digitalRPressBit);
	// And move all the above to the Condition Register!
	MTCRF(0b1 << (7 - digitalPressCRF), reg2);

	// Load the press state bitfield...
	ADDIS(reg1, 0, PSCC_CSS_INPUT_PRESS_STATE_LOC >> 0x10);
	LBZ(reg2, reg1, PSCC_CSS_INPUT_PRESS_STATE_LOC & 0xFFFF);
	// ... zero out reg3...
	ADDI(reg3, 0, 0);
	// ... and set up our probe bit for checking the current port.
	ADDI(reg4, 0x0, 0b0001);
	RLWNM(reg4, reg4, controllerIDReg, 0x18, 0x1F);

	// Grab the L Analog Distance byte...
	LBZ(reg0, padPtrReg, 0x34);
	// If that distance is both lower than the press threshold...
	CMPLI(reg0, analogPressThreshold, 0);
	// ... *and* we didn't detect a digital press...
	CRANDC(crBitInCRF(LT, 0), crBitInCRF(LT, 0), digitalLPressBit);
	// ... just skip to checking the unpress.
	JumpToLabel(decrCheckUnpressLabel, bCACB_LESSER);
	{
		// Check the bitmask for whether or not the input is already active.
		AND(reg0, reg2, reg4, 1);
		// If so...
		JumpToLabel(decrInputDisabledLabel, bCACB_NOT_EQUAL);
		{
			// ... set the pressed bit in the bitfield...
			OR(reg2, reg2, reg4);
			// ... and then set delta register to -1.
			ADDI(reg3, reg3, -1);
		}
		Label(decrInputDisabledLabel);
		// Skip past unpress check.
		JumpToLabel(decrCheckEndLabel);
	}
	Label(decrCheckUnpressLabel);
	// Otherwise, if the analog distance is beneath the unpress threshold... 
	CMPLI(reg0, analogUnpressThreshold, 0);
	JumpToLabel(decrCheckEndLabel, bCACB_GREATER);
	{
		// ... then unset the corresponding bit!
		ANDC(reg2, reg2, reg4);
	}
	Label(decrCheckEndLabel);

	// Shift probe bit up to check the increment input.
	RLWINM(reg4, reg4, 4, 0x18, 0x1F, 1);
	// Grab the R Analog Distance byte...
	LBZ(reg0, padPtrReg, 0x35);
	// If that distance is both lower than the press threshold...
	CMPLI(reg0, analogPressThreshold, 0);
	// ... *and* we didn't detect a digital press...
	CRANDC(crBitInCRF(LT, 0), crBitInCRF(LT, 0), digitalRPressBit);
	// ... just skip to checking the unpress.
	JumpToLabel(incrCheckUnpressLabel, bCACB_LESSER);
	{
		// Check the bitmask for whether or not the input is already active.
		AND(reg0, reg2, reg4, 1);
		// If so...
		JumpToLabel(incrInputDisabledLabel, bCACB_NOT_EQUAL);
		{
			// ... set the pressed bit in the bitfield...
			OR(reg2, reg2, reg4);
			// ... and then set delta register to 1.
			ADDI(reg3, reg3, 1);
		}
		Label(incrInputDisabledLabel);
		// Skip past unpress check.
		JumpToLabel(incrCheckEndLabel);
	}
	Label(incrCheckUnpressLabel);
	// Otherwise, if the analog distance is beneath the unpress threshold... 
	CMPLI(reg0, analogUnpressThreshold, 0);
	JumpToLabel(incrCheckEndLabel, bCACB_GREATER);
	{
		// ... then unset the corresponding bit!
		ANDC(reg2, reg2, reg4);
	}
	Label(incrCheckEndLabel);

	// After all that, we've set up reg3 with our delta value, and updated the bitfield with.
	// Store the updated bitset.
	STB(reg2, reg1, PSCC_CSS_INPUT_PRESS_STATE_LOC & 0xFFFF);

	// Zero reg0...
	ADDI(reg0, 0, 0x00);
	// ... check if the delta register is 0...
	CMPLI(reg3, 0x00, 0);
	// ... AND we didn't press Z before.
	CRANDC(crBitInCRF(EQ, 0), crBitInCRF(EQ, 0), digitalZPressBit);
	// ... and exit if so.
	JumpToLabel(exitLabel, bCACB_EQUAL);

	// If we're not over either the playerkind or nametag buttons...
	ADDI(reg2, collKindIDReg1, -0x1C);
	CMPLI(reg2, 0x2, 0);
	// ... then jump to exit!
	JumpToLabel(exitLabel, bCACB_GREATER_OR_EQ);
	// Otherwise, ensure we set the collkind registers to 0x1D (playerkind button).
	ADDI(collKindIDReg1, 0, 0x1D);
	MR(collKindIDReg2, collKindIDReg1);

	// Disable input if the port kind isn't currently set to Human
	LWZ(reg2, 4, 0x44);
	LWZ(reg2, reg2, 0x1B4);
	CMPLI(reg2, 0x1, 0);
	JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

	// Disable input if we're in team mode (also set up reg1 with top half of Code Menu Addr).
	LBZ(reg2, reg1, PSCC_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	CMPI(reg2, Line::DEFAULT, 0);
	JumpToLabel(exitLabel, bCACB_EQUAL);

	// Multiply slot value by 4, move it into reg1
	RLWIMI(reg1, playerAreaIDReg, 2, 0x10, 0x1D);
	// And use that to grab the relevant line's INDEX Value
	LWZ(reg1, reg1, PSCC_COLOR_1_LOC & 0xFFFF);

	// Load the line's current option into reg3...
	LWZ(reg2, reg1, Line::VALUE);
	// ... and add our modification value to it.
	ADD(reg3, reg2, reg3);

	// If modified value is greater than the max...
	CMPI(reg3, pscc::schemeTable.entries.size() - 1, 0);
	// ... roll its value around to the min.
	BC(2, bCACB_LESSER_OR_EQ);
	ADDI(reg3, 0, 0);

	// If modified value is less than the min...
	CMPI(reg3, 0, 0);
	// ... roll its value around to the max.
	BC(2, bCACB_GREATER_OR_EQ);
	ADDI(reg3, 0, pscc::schemeTable.entries.size() - 1);

	// If we pressed the Z button though, then load the line's default value instead!
	BC(2, BRANCH_IF_FALSE, digitalZPressBit);
	LWZ(reg3, reg1, Line::DEFAULT);

	// Store our modified value back in place.
	STW(reg3, reg1, Line::VALUE);

	// Set the current playerkind to none...
	LWZ(reg1, 4, 0x44);
	ADDI(reg2, 0, 0x0);
	STW(reg2, reg1, 0x1B4);
	// ... then set padReg to A button press to piggyback off the setPlayerKind call to update our colors.
	ADDI(reg0, 0, BUTTON_A);

	Label(exitLabel);
	ASMEnd(0x540005ef); // Restore Original Instruction: rlwinm. r0, r0, 0, 23, 23 (00000100)

	ASMStart(0x806828CC, "", "");
	ADDI(reg2, 0, -1);
	ADDIS(reg1, 0, PSCC_CSS_INPUT_PRESS_STATE_LOC >> 0x10);
	STB(reg2, reg1, PSCC_CSS_INPUT_PRESS_STATE_LOC & 0xFFFF);
	ASMEnd(0x3880002A); // Restore Original Instruction: li r4, 42
}

void psccTransparentCSSandResultsScreenNames()
{
	int reg1 = 11;
	int reg2 = 12;

	CodeRawStart(codePrefix + "Results Screen Player Names are Transparent" + codeSuffix, "");
	// Overwrite a branch we *don't* want by setting up reg1 with the top half of the FontLoc!
	// Since we're ADDISing from r0 as well, we guarantee that the bottom half
	// of reg1 is zeroed out; which we're going to take advantage of in the STBX in the following HOOK!
	WriteIntToFile(0x040ea724); ADDIS(reg1, 0, PSCC_FONT_CONSTS_LOC >> 0x10);
	CodeRawEnd();
	ASMStart(0x800ea73c, ""); // Hooks "initMdlData/[ifVsResultTask]/if_vsresult.o".
	// Load Message Array Addr
	LWZ(5, 3, 0x0C);
	// Get offset to target message...
	MULLI(reg2, 4, 0x48);
	// ... and store the bottom 8 bits of reg1 (zeroed by the above OP@) over the formatting bit, disabling Stroke.
	STBX(reg1, 5, reg2);
	// Then, put the *rest* of the FontLoc in the bottom of reg1...
	ADDI(reg1, reg1, PSCC_FONT_CONSTS_LOC & 0xFFFF);
	// ... and load all 4 values for r5-r8 at once!!
	LSWI(5, reg1, 0x10);
	ASMEnd();

	ASMStart(0x800ea8c0); // Hooks same as above.
	// Load Message Array Addr
	LWZ(5, 3, 0x0C);
	// Get offset to target message...
	MULLI(reg2, 4, 0x48);
	// Same trick as above, zero bottom of reg1 by loading top half of FontLoc...
	ADDIS(reg1, 0, PSCC_FONT_CONSTS_LOC >> 0x10);
	// ... and store the bottom 8 bits of it over the formatting bit.
	STBX(reg1, 5, reg2);
	// Then, put the *rest* of the FontLoc in the bottom of reg1...
	ADDI(reg1, reg1, PSCC_FONT_CONSTS_LOC & 0xFFFF);
	// ... and load all 4 values for r5-r8 at once!!
	LSWI(5, reg1, 0x10);
	ASMEnd();

	ASMStart(0x8069b268, codePrefix + "CSS Player Names are Transparent" + codeSuffix); // Hooks "dispName/[muSelCharPlayerArea]/mu_selchar_player_area_obj".
	ADDIS(reg1, 0, PSCC_FONT_CONSTS_LOC >> 0x10);
	ADDI(reg1, reg1, PSCC_FONT_CONSTS_LOC & 0xFFFF);
	LSWI(5, reg1, 0x10);
	ASMEnd();
}

void psccStoreTeamBattleStatusBody(int statusReg)
{
	int reg1 = 11;
	int reg2 = 12;
	MULLI(reg2, statusReg, Line::DEFAULT - Line::VALUE);
	ADDI(reg2, reg2, Line::VALUE);
	// Store team battle status in our buffer word.
	ADDIS(reg1, 0, PSCC_TEAM_BATTLE_STORE_LOC >> 0x10);
	STB(reg2, reg1, PSCC_TEAM_BATTLE_STORE_LOC & 0xFFFF);
}
void psccStoreTeamBattleStatus()
{
	// Hooks "setMeleeKind/[muSelCharTask]/mu_selchar_obj.o"
	ASMStart(0x8068eda8, codePrefix + "Cache SelChar Team Battle Status" + codeSuffix);
	psccStoreTeamBattleStatusBody(4);
	ASMEnd(0x7c7c1b78); // Restore original instruction: mr	r28, r3

	// Hooks "appear/[IfPlayer]/if_player.o"
	ASMStart(0x800e0a44, codePrefix + "Cache In-game Mode Team Battle Status" + codeSuffix);
	psccStoreTeamBattleStatusBody(0);
	ASMEnd(0x2c000000); // Restore Original Instruction: cmpwi	r0, 0

	// Hooks "setAdventureCondition/[sqSingleBoss]/sq_single_boss.o"
	ASMStart(0x806e5f08, codePrefix + "Only 2P Stadium Boss Battles Are Considered Team Battles" + codeSuffix);
	// Where we're hooking, we're in a loop being used to initialize player slots for Stadium Boss Battles.
	// Here, r23 is the iterator register (starts at 0, increments once per cycle), and r30 is a pointer to the GameModeMelee struct.
	// This specific line we're hooking is in a block which indicates that the given player slot is active, so on the first run
	// through we guarantee set the Team Mode byte to '0', and *if* a second player exists, we'll set it to '1' on the second run through.
	STB(23, 30, 0x13);
	ASMEnd(0x9bf90099); // Restore Original Instruction: stb	r31, 0x0099 (r25)
}

void psccMiscAdjustments()
{
	CodeRaw(codePrefix + "Disable Franchise Icon Color 10-Frame Offset in Results Screen" + codeSuffix, "",
		{
			0xC60ebb98, 0x800ebbb8, // Branch Past Second Mark Color Set
			0xC60ebde4, 0x800ebe00, // Branch Past Third Mark Color Set
		});

	CodeRawStart(codePrefix + "Hand Color Fix" + codeSuffix, 
		"Fixes a conflict with Eon's Roster-Size-Based Hand Resizing code, which could"
		"\nin some cases cause CSS hands to wind up the wrong color."
		);
	WriteIntToFile(0x0469CA2C); LFS(0, 3, 0x1014);
	CodeRawEnd();

	CodeRawStart(codePrefix + "Re-Enable Material Recalc on Certain In-Game Elements" + codeSuffix, 
		"Prevents skipping the material recalc on certain in-game HUD elements, specifically"
		"\nincluding the blastzone magnifying glass (and accompanying arrow) and the nametag arrow elements."
		"\nRe-enabling the recalc ensures that their colors update every frame, allowing animated color support!"
	);
	WriteIntToFile(0x040E083C); NOP();
	CodeRawEnd();

	ASMStart(0x806971C0, codePrefix + "Color Choice Resets on Controller Disconnect" + codeSuffix,
		"Ensures that colors are reset when players unplug their controllers."
	);
	{
		int reg1 = 11;
		int reg2 = 12;
		int colorResetExitLabel = GetNextLabel();

		// If we're turning the target slot off...
		CMPI(4, -1, 0);
		JumpToLabel(colorResetExitLabel, bCACB_NOT_EQUAL);
		// ... grab the current port ID...
		LWZ(reg2, 3, 0x1B0);
		// ... and grab the pointer to the associated Line.
		ADDIS(reg1, 0, PSCC_COLOR_1_LOC >> 0x10);
		RLWIMI(reg1, reg2, 0x2, 0x10, 0x1D);
		LWZ(reg1, reg1, PSCC_COLOR_1_LOC & 0xFFFF);
		// Load its default value...
		LWZ(reg2, reg1, Line::DEFAULT);
		// ... and write that over the current value to reset its color!
		STW(reg2, reg1, Line::VALUE);

		Label(colorResetExitLabel);
		XORI(0, 4, 0x8); // Restore Original Instruction
	}
	ASMEnd();

	CodeRawStart(codePrefix + "Port-Specific Stocks Set CLR0 Frame" + codeSuffix,
		"Has stock icons set their texture using the SetFrame function instead of SetFameTex, which ensures\n"
		"the CLR0 frame is set properly in addition to the texture itself; to make sure CPUs don't activate PSCC."
	);
	WriteIntToFile(0x040E2188); BL(-0xAB13);
	CodeRawEnd();
}

void psccSSSRandomStocks()
{
	int reg1 = 11;
	int reg2 = 12;
	int portIDReg = 23;
	int playerKindReg = 24;
	int isTeamModeReg = 27;
	int currentTeamReg = 28;
	int muObjectPtrReg = 29;
	int constantsPtrReg = 30;

	int teamCaseEndLabel = GetNextLabel();
	int exitLabel = GetNextLabel();

	const std::string clr0Name = "MenSelmapFaceR_\0";
	const unsigned long nameEmbedLineCount = 0x4;

	CodeRawStart(codePrefix + "SSS Random Stocks Use CLR0 Coloring" + codeSuffix, "");
	// Force CPU check to always pass so all players use the same Random Texture.
	WriteIntToFile(0x046B2FB0); CMPL(24, 24, 0);
	CodeRawEnd();

	// Set CLR0 Frame
	ASMStart(0x806B2FE8, "", "");
	// Embed base CLR0 name string.
	BL(1 + nameEmbedLineCount);
	for (unsigned long i = 0; i < clr0Name.size(); i += 0x4)
	{
		WriteIntToFile(lava::bytesToFundamental<unsigned long>((unsigned char*)clr0Name.data() + i));
	}
	// Pull string address into r4 for use in trying to load it to the stock object.
	MFLR(4);
	// And convert Port ID to its ASCII form...
	ADDI(reg2, portIDReg, 0x30);
	// ... and also add 0x4 if Player Kind is CPU, to map those to 4 - 7.
	RLWIMI(reg2, playerKindReg, 2, 0x1D, 0x1D);

	// If, however, we're in Team Mode...
	CMPLI(isTeamModeReg, 0x1, 0);
	JumpToLabel(teamCaseEndLabel, bCACB_NOT_EQUAL);
	// ... then we'll override slot ID with team ID, using same process as the base game to get ID...
	ADDI(reg2, constantsPtrReg, 0x190);
	LBZX(reg2, reg2, currentTeamReg);
	// ... then adding 0x30 again to convert to ASCII.
	ADDI(reg2, reg2, 0x30);
	Label(teamCaseEndLabel);

	// Write the number suffix into the name string...
	STB(reg2, 4, clr0Name.size() - 1);
	// ... copy the object pointer into r3...
	MR(3, muObjectPtrReg);
	// ... and launch!
	CallBrawlFunc(MU_CHANGE_CLR_ANIM_N_IF, 12, 1);

	Label(exitLabel);
	LFS(31, 30, 0x019C); // Restore Original Instruction
	ASMEnd();
}

void psccRandomIcons()
{
	ASMStart(0x80697558, codePrefix + "CSS Random Always Uses P1 CSP" + codeSuffix, "");
	// Restore Original Instruction
	SetRegister(24, 0);
	// Force Number for Portrait Texture Name to 501 (for P1 Random)
	SetRegister(25, 501);
	ASMEnd();

	std::string codeGroupName = "Random Franchise Icon uses Unique CLR0 Frame ";
	int randFranchiseIconExitLabel = GetNextLabel();
	ASMStart(0x80697074, codePrefix + codeGroupName + "(setCharKind)" + codeSuffix, "");
	// Restore Original Instruction
	MR(25, 3);
	// Grab player kind...
	LWZ(12, 30, 0x1B4);
	// ... and if we aren't a Human player...
	CMPLI(12, 0x1, 0x7);
	// ... then skip this code.
	JumpToLabel(randFranchiseIconExitLabel, bCACB_NOT_EQUAL.inConditionRegField(0x7));
	// Grab Team Battle Status...
	ADDIS(11, 0, PSCC_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(12, 11, PSCC_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	// ... and do the comparison to check if we *are* in Team mode.
	CMPLI(12, Line::VALUE, 0x7);
	// Grab player port ID by default, assuming for now we're not in team mode.
	LWZ(12, 30, 0x1B0);
	// If we *are* in team mode though...
	BC(4, bCACB_EQUAL.inConditionRegField(0x7));
	// ... instead grab the Team ID we're on.
	LWZ(12, 30, 0x1C0);
	RLWINM(0, 12, 0x1F, 0x1F, 0x1F, 0);
	ADD(12, 12, 0);
	// Re-use the comparison from before this hook: add 4 to the ID if we're on the Random Icon.
	BC(2, bCACB_NOT_EQUAL);
	ADDI(12, 12, 0x4);
	// Add another 0x1 to account for Frame 0 being clear.
	ADDI(12, 12, 0x1);
	// Convert the ID to float!
	// Store the ID in the bottom half of the staging location...
	STH(12, 11, (FLOAT_CONVERSION_STAGING_LOC + 0x04) & 0xFFFF);
	// ... then load it as a Paired Single (Unscaled Unsigned Short quantization).
	PSQ_L(1, 11, (FLOAT_CONVERSION_STAGING_LOC + 0x04) & 0xFFFF, 1, 3);
	// Get Franchise Icon pointer from Player Area struct.
	LWZ(3, 30, 0xB8);
	// And set the frame color!
	CallBrawlFunc(MU_SET_FRAME_MAT_COL, 12);
	MR(3, 25);
	CMPLI(3, 0x29, 0);
	Label(randFranchiseIconExitLabel);
	ASMEnd();

	int hook2EndLabel = GetNextLabel();
	int hook2SubroutineLabel = GetNextLabel();
	ASMStart(0x80699A2C, codePrefix + codeGroupName + "(incTeamColor)" + codeSuffix, 
		"Note: Serves as a subroutine for the following codes as well, to avoid redundancy."
	);
	// Load char kind...
	LWZ(12, 27, 0x1B8);
	JumpToLabel(hook2SubroutineLabel, bCACB_UNSPECIFIED, 1);
	JumpToLabel(hook2EndLabel);
	// We'll use this as a subroutine to avoid duplicating the code across all these hooks!
	Label(hook2SubroutineLabel);
	// Load 1.0f into fr0, normal frame offset constant.
	LFS(0, 13, 0x18);
	// ... and if charKind is set to random...
	CMPLI(12, 0x29, 0x0);
	BC(2, bCACB_NOT_EQUAL);
	// ... load 5.0f instead, effectively adding 4.0f.
	LFS(0, 13, 0x5F4);
	// Restore Original Instruction
	CMPI(3, 0, 0);
	BLR();
	// Exit point for this hook, to skip the subroutine.
	Label(hook2EndLabel);
	ASMEnd();

	// Hook 3
	ASMStart(0x80699D70, codePrefix + codeGroupName + "(decTeamColor)" + codeSuffix, "");
	// Load char kind...
	LWZ(12, 27, 0x1B8);
	JumpToLabel(hook2SubroutineLabel, bCACB_UNSPECIFIED, 1);
	ASMEnd();

	// Hook 4
	ASMStart(0x80698690, codePrefix + codeGroupName + "(setPlayerKind)" + codeSuffix, "");
	// Load char kind...
	LWZ(12, 28, 0x1B8);
	JumpToLabel(hook2SubroutineLabel, bCACB_UNSPECIFIED, 1);
	ASMEnd();

	// Hook 5, updateMeleeKind
	ASMStart(0x80698F18, codePrefix + codeGroupName + "(updateMeleeKind)" + codeSuffix, "");
	// Load char kind...
	LWZ(12, 26, 0x1B8);
	JumpToLabel(hook2SubroutineLabel, bCACB_UNSPECIFIED, 1);
	ASMEnd();

	CodeRawStart(codePrefix + codeGroupName + "(NOP Writes)" + codeSuffix, "");
	// Hook 2 NOP
	WriteIntToFile(0x04699A40); NOP();
	// Hook 3 NOP
	WriteIntToFile(0x04699D84); NOP();
	// Hook 4 NOPs
	WriteIntToFile(0x046986A4); NOP();
	WriteIntToFile(0x046986CC); NOP();
	// Hook 5 NOPs
	WriteIntToFile(0x04698F2C); NOP();
	WriteIntToFile(0x04698F54); NOP();
	CodeRawEnd();
}

void psccCLR0V4InstallCode()
{
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 8;

	int v4PatchExit = GetNextLabel();
	ASMStart(0x80197dc4, codePrefix + "Bootleg CLR0 v4 Support Patch" + codeSuffix, 
		"Fakes CLR0 v4 support by rearranging the contents of the v4 header such that they match the orientation\n"
		"found in v3 files, just with the UserData pointer stuck to the end. This ensures that we maintain compatibility\n"
		"with the game's assumptions about where the struct's fields should be, while keeping access to UserData!\n"
	);
	LWZ(reg1, 5, 0x00);
	// Check CLR0 Version
	LWZ(reg2, reg1, 0x8);
	CMPLI(reg2, 0x4, 0);
	JumpToLabel(v4PatchExit, bCACB_NOT_EQUAL);

	// If we're looking at one of our CLR0s, we're gonna rotate its header up to match its order to v3's!
	LMW(28, reg1, 0x18);
	LWZ(reg2, reg1, 0x14);
	STMW(28, reg1, 0x14);
	STW(reg2, reg1, 0x24);

	ADDI(reg2, 0, 3);
	STW(reg2, reg1, 0x08);

	Label(v4PatchExit);
	ASMEnd(0x81050000); // Restore Original Instruction: lwz r8, 0 (r5)
}

void psccEmbedFloatTable()
{
	CodeRawStart(codePrefix + "Embed Color and Scheme Tables" + codeSuffix, "");
	GeckoDataEmbedStart();
	for (auto i = pscc::colorTable.cbegin(); i != pscc::colorTable.cend(); i++)
	{
		unsigned long hueHex = unsigned short((std::max(std::min(i->second.hue, 6.0f), 0.0f) / 6.0f) * USHRT_MAX);
		unsigned long satHex = unsigned short(std::max(std::min(i->second.saturation, 1.0f), 0.0f) * USHRT_MAX);
		unsigned long lumHex = unsigned short(std::max(std::min(i->second.luminance, 1.0f), 0.0f) * USHRT_MAX);

		WriteIntToFile((hueHex << 0x10) | satHex);
		WriteIntToFile((lumHex << 0x10) | (unsigned long(i->second.flags) << 0x8) | unsigned long(i->second.callbackFunctionIndex));
	}

	std::vector<unsigned char> schemeVec = pscc::schemeTable.tableToByteVec();
	for (auto i : schemeVec)
	{
		WPtr << lava::numToHexStringWithPadding(i, 2);
	}
	GeckoDataEmbedEnd(PSCC_FLOAT_TABLE_LOC);
	CodeRawEnd();
}

void psccCallbackCodes()
{
	CodeRawStart(codePrefix + "Embed Color Callback Table" + codeSuffix,
		lava::numToDecStringWithPadding(pscc::callbackTableEntryCount, 0) + " Slots Long, First is Reserved for RGB Strobe!");
	GeckoDataEmbedStart();
	WriteIntToFile(0xFFFFFFFF);
	for (std::size_t i = 0; i < (pscc::callbackTableEntryCount - 1); i++)
	{
		WriteIntToFile(0x00000000);
	}
	GeckoDataEmbedEnd(PSCC_CALLBACK_TABLE_LOC);
	CodeRawEnd();

	CodeRawStart(codePrefix + "RGB Strobe Callback" + codeSuffix, "");
	GeckoDataEmbedStart();
	LHZ(12, 3, 0x00);
	ADDI(12, 12, 0x80);
	STH(12, 3, 0x00);
	BLR();
	GeckoDataEmbedEnd(ULONG_MAX, 1);
	LoadIntoGeckoPointer(PSCC_CALLBACK_TABLE_LOC);
	StoreGeckoBaseAddressRelativeTo(0, 1);
	GeckoReset();
	CodeRawEnd();
}

void psccProtectStackCode()
{
	CodeRawStart(codePrefix + "Borrow Stack Space" + codeSuffix, 
		"Consolidates two stack locations the game uses for float conversions into just one,\n"
		"allowing us to use the newly freed one as storage for some of the variables we'll need!"
	);
	WriteIntToFile(0x04193494); STW(3, 1, 0xC);
	WriteIntToFile(0x04193498); STW(0, 1, 0x8);
	WriteIntToFile(0x0419349C); LFD(0, 1, 0x8);
	CodeRawEnd();
}

void psccSetupCode()
{
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 28; // Holds Mode!

	// Mode0 = Port via Name Suffix
	// Mode1 = Port via Frame
	// Mode2 = Mode1 + 1
	// Mode3 = Mode0 - 1

	int frameModeLabel = GetNextLabel();
	int getUserDataLabel = GetNextLabel();
	int badExitLabel = GetNextLabel();
	int exitLabel = GetNextLabel();

	ASMStart(0x80193410, codePrefix + "Prep Code" + codeSuffix);
	// Grab the Data Offset in this CLR0; if it's not 0x28, we're not looking at one of our CLR0s, and we can skip to bad exit!
	LWZ(reg3, 6, 0x10);
	CMPLI(reg3, 0x28, 0);
	JumpToLabel(badExitLabel, bCACB_NOT_EQUAL);

	// Attempt to grab first 4 bytes of the CLR0's "Original Address" value.
	LWZ(reg3, 6, 0x18);
	LWZX(reg3, 6, reg3);
	// If this fails, the register'll be "CLR0", otherwise it should be the specified value.
	// Do some subtractions to check that the activator is there *and* get the code operation version!
	ADDIS(reg3, reg3, -activatorStringHiHalf);
	ADDI(reg3, reg3, -activatorStringLowHalf);

	// If we're at or above 8, then we're looking at an invalid label...
	CMPLI(reg3, 0x8, 0);
	// ... so exit!
	JumpToLabel(badExitLabel, bCACB_GREATER_OR_EQ);

	// Otherwise, if the resulting value is greater than or equal to 4...
	CMPLI(reg3, 0x4, 0);
	// ... then we need to grab the CLR0's frame, jump to the relevant label.
	JumpToLabel(frameModeLabel, bCACB_GREATER_OR_EQ);
	// Otherwise, we're dealing with the absolute cases! Just store the value itself as our port...
	STH(reg3, 1, safeStackWordOff + 0x6);
	// ... and jump down to grabbing the userData.
	JumpToLabel(getUserDataLabel);

	Label(frameModeLabel);
	// The remaining modes deal with the frame float, so make a mutable copy of that...
	FMR(13, 1);
	// round it to single precision, just to be certain we can safely PSQ_ST it...
	FRSP(13, 13);
	// ... then store using GQR3 (store as unsigned short) in the third & fourth half-words in our safe space.
	PSQ_ST(13, 1, safeStackWordOff + 0x4, 0, 3);
	LHZ(reg1, 1, safeStackWordOff + 0x6);
	// If our Mode value's 1s bit is set (ie. Mode 5 or 7)...
	ANDI(reg2, reg3, 0b01);
	BC(2, bCACB_EQUAL);
	// ... then subtract 1 from it!
	ADDI(reg1, reg1, -0x1);
	// If our Mode value's 2s bit is set (ie. Mode 6 or 7)
	ANDI(reg2, reg3, 0b10);
	BC(2, bCACB_EQUAL);
	// Unset the 4s bit, so values 4 through 7 map to 0 through 3, but values 8 and above still register invalid.
	ANDI(reg1, reg1, ~0b100);
	STH(reg1, 1, safeStackWordOff + 0x6);

	// Next, we need to try to get the CLR0's UserData and the accompanying mask data.
	Label(getUserDataLabel);
	// Initialize the Mask slot to 0.
	ADDI(reg2, 0, 0x0);
	STW(reg2, 1, safeStackWordOff);
	// Load the UserData offset from the CLR0
	LWZ(reg1, 6, 0x24);
	// IF that value was 0 (ie. the CLR0 doesn't have any UserData defined) we'll just leave the 0 and exit.
	CMPLI(reg1, 0x0, 0);
	JumpToLabel(exitLabel, bCACB_EQUAL);

	// Otherwise though, we'll grab the relevant material mask!
	// Get the address to the UserData struct...
	ADD(reg1, 6, reg1);
	// ... then get the address for the first UserData entry's Data (which we require is the masks).
	LWZU(reg2, reg1, 0x4);
	ADD(reg1, reg1, reg2);
	// Next, check that there is an entry for our current Material.
	// Grab the number of entries in the array here...
	LWZ(reg2, reg1, 0x8);
	// ... compare it with the current Material's index...
	CMPL(5, reg2, 0);
	// ... and if it's too high, skip grabbing the mask (the 0 already in its place will leave the code enabled!)
	JumpToLabel(exitLabel, bCACB_GREATER_OR_EQ);
	// Otherwise though, if there is a mask to grab, jump forwards to the mask array.
	LWZ(reg2, reg1, 0x4);
	ADD(reg1, reg1, reg2);
	// Then multiply the requested Material ID to index into our list and load the appropiate value...
	MULLI(reg2, 5, 0x4);
	LWZX(reg2, reg1, reg2);
	// ... and finally store the value in our stack space!
	STW(reg2, 1, safeStackWordOff);
	// Then jump down to the exit, skipping the bad exit bit!
	JumpToLabel(exitLabel);

	Label(badExitLabel);
	// If the detected Mode doesn't correspond to a supported case, force the mask mode to 0xFFFFFFFF exit!
	ORC(reg3, reg3, reg3);
	STW(reg3, 1, safeStackWordOff);

	Label(exitLabel);
	ADDI(5, 5, 0x1); // Restore Original Instruction
	ASMEnd();
}

void psccMainCode()
{
	int reg0 = 0;
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 3;
	int GQRBackupReg1 = 9;
	int GQRBackupReg2 = 10;
	int CustomGQRID1 = 917; int CustomGQRIndex1 = CustomGQRID1 - 912; // GQR5
	int CustomGQRID2 = 918; int CustomGQRIndex2 = CustomGQRID2 - 912; // GQR6
	int CLR0ResultStructReg = 28;

	int floatCalcRegisters[2] = { 7, 8 };
	int floatTempRegisters[2] = { 9, 10 };
	int floatHSLRegisters[3] = { 11, 12, 13 };
	int floatCurrFrameReg = 31;

	int exitLabel = GetNextLabel();
	// Hooks "GetAnmResult/[nw4r3g3d9ResAnmClrCFPQ34nw4r3g3d12]/g3d_res".
	ASMStart(0x801934fc, codePrefix + "Main Code" + codeSuffix, "");
	// Restore Original Instruction: Store RGBA result in Result struct.
	// Note, this register is free to use after this point, since the result's been stored now!
	STW(reg3, CLR0ResultStructReg, 0x4);

	// Otherwise, check if the code is disabled for the current material!
	// Get the mask from the safe space...
	LWZ(reg2, 1, safeStackWordOff);
	// ... calculate how many bytes to shift by based on what iteration of the loop we're in...
	SUBFIC(reg3, 26, 0x20);
	// ... and rotate the relevant bit into the bottom of reg0.
	RLWNM(reg3, reg2, reg3, 0x1F, 0x1F, 1);
	// If the bit wasn't 0 (ie. if the code is disabled for this material-target), skip to exit!
	JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

	// Load the port index short...
	LHZ(reg0, 1, safeStackWordOff + 0x6);
	// ... and exit if our value doesn't correspond to a valid port.
	CMPLI(reg0, 0x3, 0);
	JumpToLabel(exitLabel, bCACB_GREATER);

	// Main Algorithm!
	{
		// Set up the top half of the address for Code Menu stuff in reg3 ahead of time so we don't have to redundantly do it.
		ADDIS(reg3, 0, START_OF_CODE_MENU_HEADER >> 0x10);

		// Backup GQRs in preparation for our Paired Single float reads.
		MFSPR(GQRBackupReg1, CustomGQRID1);
		MFSPR(GQRBackupReg2, CustomGQRID2);
		// Setup new Quantization Register for our reads
		// First GQR: For reading/writing Unsigned RGBA Bytes, and quantizing/dequantizing them to 2^7 and 2^8 respectively!
		ADDIS(reg2, 0, 0x0704);
		ORI(reg2, reg2, 0x0804);
		MTSPR(reg2, CustomGQRID1);
		//Second GQR: For reading Unsigned Color Table Shorts (quantized to 2^15)!
		ADDIS(reg2, 0, 0x1005);
		MTSPR(reg2, CustomGQRID2);
		// Load Hue Float to Hue FReg
		PSQ_L(floatHSLRegisters[0], CLR0ResultStructReg, 0x04, 1, CustomGQRIndex1);
		// Load Saturation and Luminance Floats to Sat FReg.
		PSQ_L(floatHSLRegisters[1], CLR0ResultStructReg, 0x05, 0, CustomGQRIndex1);
		// Load 3.0f into TempReg0...
		LFS(floatTempRegisters[0], 2, -0x6168);
		// ... and use it to multiply the Hue (to ensure it ranges from 0.0 to 6.0).
		FMULS(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[0]);

		// Set up the top half of reg1 with 0x804E to simplify accessing code menu stuff.
		// Note: Not using reg3 for this since we're gonna modify the register through the RLWIMI trick below.
		ADDIS(reg1, 0, PSCC_TEAM_BATTLE_STORE_LOC >> 0x10);
		// Load buffered Team Battle Status Offset
		LBZ(reg2, reg1, PSCC_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Now multiply the target port by 4 to calculate the offset to the line we want, and insert it into reg1.
		RLWIMI(reg1, reg0, 2, 0x10, 0x1D);
		LWZ(reg1, reg1, PSCC_COLOR_1_LOC & 0xFFFF);
		// Use it to load the targetIndex...
		LWZX(reg2, reg1, reg2);

		// Multiply that by 4 to use it as an index...
		RLWINM(reg2, reg2, 2, 0, 0x1D);
		// ... add the length of the color table to it, so we're indexing now into the schemes table.
		ADDI(reg2, reg2, pscc::getColorTableSizeInBytes());
		// Load the scheme slot ID this Material-Target uses...
		LBZ(reg0, 1, safeStackWordOff + 1);
		// ... and add it to reg2, finalizing the offset to the Color ID we want!
		ADD(reg2, reg2, reg0);

		// Grab the pointer to our Float Hue Table
		LWZ(reg1, reg3, PSCC_FLOAT_TABLE_LOC & 0xFFFF);

		// Grab the appropriate Color Index
		LBZX(reg2, reg1, reg2);
		MULLI(reg0, reg2, pscc::colorTableEntrySizeInBytes);

		// Load and quantize the Hue short (and update reg1 to point to our floatTriple)...
		PSQ_LUX(floatCalcRegisters[0], reg1, reg0, 1, CustomGQRIndex2);
		// ... and load the color's flags while we're at it.
		LBZ(reg2, reg1, 0x06);
		// Load 6.0f into TempReg1...
		LFS(floatTempRegisters[1], 2, -0x62FC);
		// ... and use it to scale up our Hue float!
		FMULS(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[1]);

		// Apply Hue Color Flags!
		// If the color requests its Hue modifiers be inverted...
		ANDI(reg0, reg2, pscc::color::fb_INVERT_HUE_MOD);
		BC(2, bCACB_EQUAL);
		// ... subtract the incoming modifier from 6.0f to flip it.
		FSUBS(floatHSLRegisters[0], floatTempRegisters[1], floatHSLRegisters[0]);
		// If the color requests that Hue modifiers be outright disabled though...
		ANDI(reg0, reg2, pscc::color::fb_DISABLE_HUE_MOD);
		BC(2, bCACB_EQUAL);
		// ... then zero it out.
		FSUBS(floatHSLRegisters[0], floatHSLRegisters[0], floatHSLRegisters[0]);

		// Then add our Hue modifier to the incoming Hue value!
		FADDS(floatHSLRegisters[0], floatHSLRegisters[0], floatCalcRegisters[0]);

		// Ensure that our Hue remains in the 0.0f to 6.0 range: 
		// Subtract 6.0f from Hue value...
		FSUBS(floatCalcRegisters[0], floatHSLRegisters[0], floatTempRegisters[1]);
		// ... and if the result is >= 0.0f, take it. Otherwise, retain the original value.
		// Note: Don't need a full modulo loop: the Hue mathematically *can't* be >= 12.0f, so never requires >1 subtraction.
		FSEL(floatHSLRegisters[0], floatCalcRegisters[0], floatCalcRegisters[0], floatHSLRegisters[0]);

		// Load 1.0f into TempReg0, and 2.0f into TempReg1.
		LFS(floatTempRegisters[0], 2, -0x6170);
		FADDS(floatTempRegisters[1], floatTempRegisters[0], floatTempRegisters[0]);

		// Apply Saturation and Luminance Color Flags!
		// Prepare our minimum (0.0f) and maximum (2.0f) values in CalcRegs 0 and 1 respectively!
		FSUBS(floatCalcRegisters[0], floatTempRegisters[0], floatTempRegisters[0]);
		PS_MERGE01(floatCalcRegisters[1], floatTempRegisters[1], floatTempRegisters[1]);
		// If downwards saturation mods are disabled...
		ANDI(reg0, reg2, pscc::color::fb_DISABLE_SAT_MOD_DOWN);
		BC(2, bCACB_EQUAL);
		// ... then overwrite PS0 of the Min reg with 1.0f
		PS_MERGE01(floatCalcRegisters[0], floatTempRegisters[0], floatCalcRegisters[0]);
		// If downwards luminance mods are disabled...
		ANDI(reg0, reg2, pscc::color::fb_DISABLE_LUM_MOD_DOWN);
		BC(2, bCACB_EQUAL);
		// ... then overwrite PS1 of the Min reg with 1.0f
		PS_MERGE01(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[0]);
		// If upwards saturation mods are disabled...
		ANDI(reg0, reg2, pscc::color::fb_DISABLE_SAT_MOD_UP);
		BC(2, bCACB_EQUAL);
		// ... then overwrite PS0 of the Max reg with 1.0f
		PS_MERGE01(floatCalcRegisters[1], floatTempRegisters[0], floatCalcRegisters[1]);
		// If upwards saturation mods are disabled...
		ANDI(reg0, reg2, pscc::color::fb_DISABLE_LUM_MOD_UP);
		BC(2, bCACB_EQUAL);
		// ... then overwrite PS1 of the Max reg with 1.0f
		PS_MERGE01(floatCalcRegisters[1], floatCalcRegisters[1], floatTempRegisters[0]);
		// Then Clamp the multiplier values!
		// Apply the min...
		PS_SUB(floatHSLRegisters[2], floatHSLRegisters[1], floatCalcRegisters[0]);
		PS_SEL(floatHSLRegisters[1], floatHSLRegisters[2], floatHSLRegisters[1], floatCalcRegisters[0]);
		// Apply the max...
		PS_SUB(floatHSLRegisters[2], floatCalcRegisters[1], floatHSLRegisters[1]);
		PS_SEL(floatHSLRegisters[1], floatHSLRegisters[2], floatHSLRegisters[1], floatCalcRegisters[1]);

		// Apply Saturation and Luminance Multipliers!
		// Zero out the dedicated Lum Mul reg, since we'll need a 0.0f in a second and aren't actually using the reg yet.
		FSUBS(floatHSLRegisters[2], floatTempRegisters[0], floatTempRegisters[0]);
		// Load the Absolute Saturation and Luminance values into CalcReg0 PS1 and 2
		PSQ_L(floatCalcRegisters[0], reg1, 0x2, 0, CustomGQRIndex2);
		// Subtract 1.0 from each multiplier
		PS_SUB(floatCalcRegisters[1], floatHSLRegisters[1], floatTempRegisters[0]);
		// If (Mul - 1) >= 0.0f, then FinalMul = Mul - 1, Else FinalMul = Mul
		PS_SEL(floatHSLRegisters[1], floatCalcRegisters[1], floatCalcRegisters[1], floatHSLRegisters[1]);
		// If (Mul - 1) >= 0.0f, then FinalAdd = AbsVal, Else FinalAdd = 0.0
		PS_SEL(floatHSLRegisters[2], floatCalcRegisters[1], floatCalcRegisters[0], floatHSLRegisters[2]);
		// Get 1 - AbsVal
		PS_SUB(floatTempRegisters[1], floatTempRegisters[0], floatCalcRegisters[0]);
		// If (Mul - 1) >= 0.0f, then FinalMul2 = 1 - AbsVal, Else FinalMul2 = AbsVal
		PS_SEL(floatCalcRegisters[1], floatCalcRegisters[1], floatTempRegisters[1], floatCalcRegisters[0]);
		// Final Result = (FinalMul1 * FinalMul2) + FinalAdd
		PS_MADD(floatHSLRegisters[1], floatHSLRegisters[1], floatCalcRegisters[1], floatHSLRegisters[2]);
		// Copy Lum result back into Lum Register!
		PS_MERGE11(floatHSLRegisters[2], floatHSLRegisters[1], floatHSLRegisters[1]);
		// Put 2.0f back in TempReg1
		FADDS(floatTempRegisters[1], floatTempRegisters[0], floatTempRegisters[0]);

		// Calculate Chroma: C = Sat(1 - Abs(2Lum - 1.0f))
		FMSUBS(floatCalcRegisters[0], floatHSLRegisters[2], floatTempRegisters[1], floatTempRegisters[0]); // C = (Lum * 2.0f) - 1.0f
		FABS(floatCalcRegisters[0], floatCalcRegisters[0]);								// C = Abs(C)
		FNMSUBS(floatCalcRegisters[0], floatHSLRegisters[1], floatCalcRegisters[0], floatHSLRegisters[1]); // C = -(Sat*C - Sat) == Sat(1.0f - C)

		// Note: using GQR3 here for Non-quantized Unsigned Short conversion).
		PSQ_ST(floatHSLRegisters[0], reg3, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4, 1, 3);
		LHZ(reg2, reg3, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);

		// We don't need the original Hue value anymore, but do need (Hue % 2.0f) - 1.0f, so we'll do that in HSLReg0!
		B(2);																			// HSLReg0 = Hue mod 2.0f
		FSUBS(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[1]);		//
		FCMPU(floatHSLRegisters[0], floatTempRegisters[1], 1);							//
		BC(-2, bCACB_GREATER_OR_EQ.inConditionRegField(1));								//
		FSUBS(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[0]);		// HSLReg0 = X - 1.0f

		// Calculate X: X = C(1.0f - Abs((H % 2.0f) - 1.0f))
		FABS(floatCalcRegisters[1], floatHSLRegisters[0]);								// X = Abs(X)
		FNMSUBS(floatCalcRegisters[1], floatCalcRegisters[0], floatCalcRegisters[1], floatCalcRegisters[0]); // X = -(C*X - C) == C(1.0f - X)

		// Calculate M (we'll write this into TempReg1, since we'll no longer need 2.0f after this!
		FDIVS(floatTempRegisters[1], floatCalcRegisters[0], floatTempRegisters[1]);		// M = C / 2.0f
		FSUBS(floatTempRegisters[1], floatHSLRegisters[2], floatTempRegisters[1]);		// M = Luminence - M

		// Now that C, X, and M are all calculated, finish up the conversion!
		int addMLabel = GetNextLabel();
		// Remember, we have (Hue % 2.0f) - 1.0f) in HSLReg0; we're gonna FSEL off it!
		// Then, do each case of the piecewise.
		// Cases 0 and 1
		CMPLI(reg2, 0x2, 0);
		BC(5, bCACB_GREATER_OR_EQ);
		FSEL(floatHSLRegisters[1], floatHSLRegisters[0], floatCalcRegisters[0], floatCalcRegisters[1]);	// HSL[1] = (Hue % 2.0f) ? C : X
		FSEL(floatHSLRegisters[0], floatHSLRegisters[0], floatCalcRegisters[1], floatCalcRegisters[0]);	// HSL[0] = (Hue % 2.0f) ? X : C
		FSUBS(floatHSLRegisters[2], floatHSLRegisters[2], floatHSLRegisters[2]);						// HSL[2] = 0
		JumpToLabel(addMLabel);
		// Cases 2 and 3
		CMPLI(reg2, 0x4, 0);
		BC(5, bCACB_GREATER_OR_EQ);
		FSEL(floatHSLRegisters[1], floatHSLRegisters[0], floatCalcRegisters[1], floatCalcRegisters[0]);	// HSL[1] = (Hue % 2.0f) ? X : C
		FSEL(floatHSLRegisters[2], floatHSLRegisters[0], floatCalcRegisters[0], floatCalcRegisters[1]);	// HSL[2] = (Hue % 2.0f) ? C : X
		FSUBS(floatHSLRegisters[0], floatHSLRegisters[2], floatHSLRegisters[2]);						// HSL[0] = 0
		JumpToLabel(addMLabel);
		// Cases 4 and 5
		FSEL(floatHSLRegisters[2], floatHSLRegisters[0], floatCalcRegisters[1], floatCalcRegisters[0]);	// HSL[2] = (Hue % 2.0f) ? X : C
		FSEL(floatHSLRegisters[0], floatHSLRegisters[0], floatCalcRegisters[0], floatCalcRegisters[1]);	// HSL[0] = (Hue % 2.0f) ? C : X
		FSUBS(floatHSLRegisters[1], floatHSLRegisters[2], floatHSLRegisters[2]);						// HSL[1] = 0

		// Add M to our newly sorted registers.
		Label(addMLabel);
		// Add M to Hue Reg
		FADDS(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[1]);
		// Pull Luminance into PS1 of Saturation Register, then add M to both at once.
		PS_MERGE00(floatHSLRegisters[1], floatHSLRegisters[1], floatHSLRegisters[2]);
		PS_ADD(floatHSLRegisters[1], floatHSLRegisters[1], floatTempRegisters[1]);

		// Store R component
		PSQ_ST(floatHSLRegisters[0], CLR0ResultStructReg, 0x04, 1, CustomGQRIndex1);
		// Store G and B components
		PSQ_ST(floatHSLRegisters[1], CLR0ResultStructReg, 0x05, 0, CustomGQRIndex1);

		// Restore backed up GQR0 values!
		MTSPR(GQRBackupReg1, CustomGQRID1);
		MTSPR(GQRBackupReg2, CustomGQRID2);
	}

	Label(exitLabel);
	ASMEnd();
}

void playerSlotColorChangersV3(bool enabled)
{
	if (enabled)
	{
		bool backupMulliOptSetting = CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS;
		CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = 1;

		psccIncrementOnButtonPress();
		psccTransparentCSSandResultsScreenNames();
		psccStoreTeamBattleStatus();
		psccMiscAdjustments();
		psccRandomIcons();
		psccSSSRandomStocks();
		psccCLR0V4InstallCode();
		psccEmbedFloatTable();
		psccCallbackCodes();
		psccProtectStackCode();
		psccSetupCode();
		psccMainCode();

		CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
	}
}
