#include "stdafx.h"
#include "_PlayerSlotColorChangers.h"

const std::string codePrefix = "[CM: _PlayerSlotColorChangers v3.0.0] ";
const std::string codeSuffix = " [QuickLava]";

// New approach:
// - Verify that incoming CLR0 is asking for override
// - Determine the index of the frame they're asking for
// - Determine that it's in our range
// - Get the index for the color being requested by the relevant code menu line
// - Grab the stored "RGBA" for that frame; not actually RGBA, it's the following
//     0x00 = Signed 8 - bit Hue Shift(Fixed Point)
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

	// Disable input if the port kind isn't currently set to Human
	LWZ(reg2, 4, 0x44);
	LWZ(reg2, reg2, 0x1B4);
	CMPLI(reg2, 0x1, 0);
	JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

	// Disable input if we're in team mode (also set up reg1 with top half of Code Menu Addr).
	ADDIS(reg1, 0, PSCC_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(reg2, reg1, PSCC_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	CMPI(reg2, Line::DEFAULT, 0);
	JumpToLabel(exitLabel, bCACB_EQUAL);

	// Multiply slot value by 4, move it into reg1
	RLWIMI(reg1, 29, 2, 0x10, 0x1D);
	// And use that to grab the relevant line's INDEX Value
	LWZ(reg1, reg1, PSCC_COLOR_1_LOC & 0xFFFF);

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
	CMPI(reg3, pscc::schemeTable.entries.size() - 1, 0);
	// ... roll its value around to the min.
	BC(2, bCACB_LESSER_OR_EQ);
	ADDI(reg3, 0, 0);

	// If modified value is less than the min...
	CMPI(reg3, 0, 0);
	// ... roll its value around to the max.
	BC(2, bCACB_GREATER_OR_EQ);
	ADDI(reg3, 0, pscc::schemeTable.entries.size() - 1);

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

	ASMStart(0x80697558, codePrefix + "CSS Random Always Uses P1 CSP" + codeSuffix, "");
	// Restore Original Instruction
	SetRegister(24, 0);
	// Force Number for Portrait Texture Name to 501 (for P1 Random)
	SetRegister(25, 501);
	ASMEnd();

	int randFranchiseIconExitLabel = GetNextLabel();
	ASMStart(0x80697074, codePrefix + "Random Franchise Icon uses Unique CLR0 Frame" + codeSuffix, "");
	// Restore Original Instruction
	MR(25, 3);
	// Grab Team Battle Status...
	ADDIS(11, 0, PSCC_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(12, 11, PSCC_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	// ... and if we *are* in Team mode...
	CMPLI(12, Line::VALUE, 0x7);
	// ... then skip this code.
	JumpToLabel(randFranchiseIconExitLabel, bCACB_NOT_EQUAL.inConditionRegField(0x7));
	// Grab player kind...
	LWZ(12, 30, 0x1B4);
	// ... and if we aren't a Human player...
	CMPLI(12, 0x1, 0x7);
	// ... then skip this code.
	JumpToLabel(randFranchiseIconExitLabel, bCACB_NOT_EQUAL.inConditionRegField(0x7));
	// Grab player port ID.
	LWZ(12, 30, 0x1B0);
	// Re-use the comparison from before this hook: add 4 to the ID if we're on the Random Icon.
	BC(2, bCACB_NOT_EQUAL);
	ADDI(12, 12, 0x4);
	// Add another 0x1 to account for Frame 0 being clear.
	ADDI(12, 12, 0x1);
	// Convert the ID to float!
	// Store the ID in the bottom half of the staging location
	STW(12, 11, (FLOAT_CONVERSION_STAGING_LOC + 0x4) & 0xFFFF);
	// Load the staged ID float into f12
	LFD(1, 11, FLOAT_CONVERSION_STAGING_LOC & 0xFFFF);
	// Load the float conversion constant from its location
	LFD(13, 11, FLOAT_CONVERSION_CONST_LOC & 0xFFFF);
	// Subtract the constant from the ID float to finish conversion!
	FSUBS(1, 1, 13);
	// Get Franchise Icon pointer from Player Area struct.
	LWZ(3, 30, 0xB8);
	// And set the frame color!
	CallBrawlFunc(MU_SET_FRAME_MAT_COL, 12);
	MR(3, 25);
	CMPLI(3, 0x29, 0);
	Label(randFranchiseIconExitLabel);
	ASMEnd();

	ASMStart(0x806986C0, "", "");
	LWZ(0, 28, 0x1B0);
	LWZ(12, 28, 0x1B8);
	CMPLI(12, 0x29, 0x0);
	BC(2, bCACB_NOT_EQUAL);
	ADDIC(0, 0, 0x4);
	ASMEnd();

	int colorResetExitLabel = GetNextLabel();
	ASMStart(0x8068BE94, codePrefix + "Color Choice Resets on Setting PlayerKind to None" + codeSuffix, 
		"Ensures that colors are reset when players unplug their controllers,\n"
		"while also providing an easy way of resetting without the use of the added controls."
	);
	// If the PlayerKind we're switching to isn't "None" (ie. 0)...
	CMPLI(5, 0, 0);
	// ... then we'll skip this code, exit early.
	JumpToLabel(colorResetExitLabel, bCACB_NOT_EQUAL);
	// Otherwise, get the pointer to the relevant line (note: r0 already holds port# * 4)...
	ORIS(11, 0, PSCC_COLOR_1_LOC >> 0x10);
	LWZ(11, 11, PSCC_COLOR_1_LOC & 0xFFFF);
	// ... load the line's default value...
	LWZ(12, 11, Line::DEFAULT);
	// ... and write that over the current value.
	STW(12, 11, Line::VALUE);
	Label(colorResetExitLabel);
	MR(27, 5); // Restore Original Instruction
	ASMEnd();
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

	// Attempt to Load the "Original Path" Value
	LWZ(reg3, reg1, 0x1C);
	LWZUX(reg2, reg3, reg1);
	// If this fails, the register'll be "CLR0", otherwise it should be the specified value.
	// Do some subtractions to check that the activator is there *and* get the code operation version!
	ADDIS(reg2, reg2, -activatorStringHiHalf);
	ADDI(reg2, reg2, -activatorStringLowHalf);
	// If the activator was present, the above subtractions should have reduced it down to just the number corresponding to its mode!
	// If it failed to (ie. the reg > 9), we'll skip.
	CMPLI(reg2, 0x9, 0);
	JumpToLabel(v4PatchExit, bCACB_GREATER);

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
	// Mode1 = Mode0 - 1
	// Mode2 = Port via Frame
	// Mode3 = Mode2 + 1

	int mode0Label = GetNextLabel();
	int mode1Label = GetNextLabel();
	int mode2Label = GetNextLabel();
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
	// If the activator was present, the above subtractions should have reduced it down to just the number corresponding to its mode!
	// If we're still above 3, then we're not looking at a valid CLR0; skip to the exit!
	CMPLI(reg3, 0x3, 7);
	JumpToLabel(badExitLabel, bCACB_GREATER.inConditionRegField(0x7));

	// Reuse the check from the last line to branch for Mode 3 (which is handled in the Mode 1 logic, this isn't a typo lol)
	// Note, we did this check in CR7 so that we can use whether we're in it again later to toggle the final port number subtraction!
	JumpToLabel(mode0Label, bCACB_EQUAL.inConditionRegField(0x7));
	// If Mode 0...
	CMPLI(reg3, 0x0, 0);
	JumpToLabel(mode0Label, bCACB_EQUAL);
	// The remaining two modes deal with the frame float, so make a mutable copy of that...
	FMR(13, 1);
	// If Mode 1...
	CMPLI(reg3, 0x1, 0);
	JumpToLabel(mode1Label, bCACB_EQUAL);
	// Mode2 is the only option left, and it's the same as Mode1, only we add 1 to the frame first; so we can just make it an add-on for Mode1.

	Label(mode2Label);
	LFS(12, 2, -0x6170);
	FADD(13, 13, 12);
	// Store frame as an integer, and store it as the second word of our safe space.
	Label(mode1Label);
	FCTIWZ(13, 13);
	STFD(13, 1, safeStackWordOff);
	JumpToLabel(getUserDataLabel);

	Label(mode0Label);
	// Get the address of the CLR0's name string...
	LWZ(reg1, 6, 0x14);
	ADD(reg1, reg1, 6);
	// ... and load its length (also pushing reg1 backwards by 4 bytes).
	LWZU(reg2, reg1, -0x4);
	// Now LWZ using the length as an offset (causing us to load the last 4 bytes of the name, since we moved back 4 before).
	LWZX(reg2, reg1, reg2);
	// Isolate just the final nibble of those bytes, which'll (for our purposes) convert the number at the end to an integer.
	RLWINM(reg2, reg2, 0, 0x1C, 0x1F);
	BC(2, bCACB_EQUAL.inConditionRegField(0x7));
	// Add 1 to that number...
	ADDI(reg2, reg2, 1);
	// ... and store it to reference as our target port!
	STW(reg2, 1, safeStackWordOff + 0x4);

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
	// If the detected Mode doesn't correspond to a supported case, force the mode to 0xFFFFFFFF and exit!
	ORC(reg3, reg3, reg3);

	Label(exitLabel);
	// And store our Code Mode as the top half of the second st!
	STH(reg3, 1, safeStackWordOff + 0x4);
	ADDI(5, 5, 0x1); // Restore Original Instruction
	ASMEnd();
}

void psccEmbedFloatTable()
{
	CodeRawStart(codePrefix + "Embed Color and Scheme Tables" + codeSuffix, "");
	GeckoDataEmbedStart();
	for (auto i = pscc::colorTable.cbegin(); i != pscc::colorTable.cend(); i++)
	{
		WriteIntToFile(lava::bytesToFundamental<unsigned long>(lava::fundamentalToBytes<float>(i->second.hue).data()));
		WriteIntToFile(lava::bytesToFundamental<unsigned long>(lava::fundamentalToBytes<float>(i->second.saturation).data()));
		WriteIntToFile(lava::bytesToFundamental<unsigned long>(lava::fundamentalToBytes<float>(i->second.luminance).data()));
	}

	std::vector<unsigned char> schemeVec = pscc::schemeTable.tableToByteVec();
	for (auto i : schemeVec)
	{
		WPtr << lava::numToHexStringWithPadding(i, 2);
	}
	GeckoDataEmbedEnd(PSCC_FLOAT_TABLE_LOC);
	CodeRawEnd();
}

void psccMainCode()
{
	int reg0 = 0;
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 10;
	int GQRBackupReg = 9;
	int RGBAResultReg = 3;

	int floatCalcRegisters[2] = { 7, 8 };
	int floatTempRegisters[2] = { 9, 10 };
	int floatHSLRegisters[3] = { 11, 12, 13 };
	int floatCurrFrameReg = 31;

	int endOfSubroutines = GetNextLabel();
	int skipMode1 = GetNextLabel();
	int exitLabel = GetNextLabel();
	// Hooks "GetAnmResult/[nw4r3g3d9ResAnmClrCFPQ34nw4r3g3d12]/g3d_res".
	ASMStart(0x801934fc, codePrefix + "Main Code" + codeSuffix, "");
	// Load the code mode short...
	LHZ(reg2, 1, safeStackWordOff + 0x4);
	// ... and if it's null...
	CMPLI(reg2, 0xFFFF, 0);
	// ... exit the code!
	JumpToLabel(exitLabel, bCACB_EQUAL);

	// Otherwise, check if the code is disabled for the current material!
	// Get the mask from the safe space...
	LWZ(reg2, 1, safeStackWordOff);
	// ... calculate how many bytes to shift by based on what iteration of the loop we're in...
	SUBFIC(reg0, 26, 0x20);
	// ... and rotate the relevant bit into the bottom of reg0.
	RLWNM(reg0, reg2, reg0, 0x1F, 0x1F, 1);
	// If the bit wasn't 0 (ie. if the code is disabled for this material-target), skip to exit!
	JumpToLabel(exitLabel, bCACB_NOT_EQUAL);

	// Otherwise, jump past our subroutines to the code body!
	JumpToLabel(endOfSubroutines);

	// Apply Multiplier Subroutine
	// Arguments:
	//		CalcReg0 = Multiplier
	//		CalcReg1 = Value
	// Requires:
	//		TempReg0 = 1.0f
	//		TempReg1 = 2.0f
	// Returns:
	//		Scaled Value via CalcReg0
	// Description:
	// Takes in a Multiplier between 0.0f and 2.0f, and a Value between 0.0f and 1.0f, and:
	// - For Multiplier values below or equal to 1.0f, scales Value down towards 0.0f
	// - For Multiplier values above 1.0f, scales Value up towards 1.0f
	int applyMultiplierSubroutineLabel = GetNextLabel();
	{
		Label(applyMultiplierSubroutineLabel);
		// Argument
		int satModUpLabel = GetNextLabel();
		int satModEndLabel = GetNextLabel();
		FCMPU(floatCalcRegisters[0], floatTempRegisters[1], 1);							// Compare Multiplier with 2.0f...
		JumpToLabel(satModEndLabel, bCACB_GREATER.inConditionRegField(1));				// ... and skip our multiplier if it's too large!
		FCMPU(floatCalcRegisters[0], floatTempRegisters[0], 1);							// Compare Multiplier with 1.0f
		JumpToLabel(satModUpLabel, bCACB_GREATER.inConditionRegField(1));
		// Modifier <= 1.0f Case:
		FMUL(floatCalcRegisters[0], floatCalcRegisters[0], floatCalcRegisters[1]);		// Simply scale Value by our Multiplier...
		JumpToLabel(satModEndLabel);													// ... and jump to end!
		// Modifier > 1.0f case!
		Label(satModUpLabel);															// Otherwise, we'll scale the *rest* of the distance!
		FSUBS(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[0]);		// Mul = Mul - 1.0f  
		FSUBS(floatTempRegisters[1], floatTempRegisters[0], floatCalcRegisters[1]);		// Temp = 1.0f - Value
		FMADDS(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[1], floatCalcRegisters[1]);		// Value = (Mul * Temp) + Value
		FADD(floatTempRegisters[1], floatTempRegisters[0], floatTempRegisters[0]);		// Restore the 2.0f float, since we overwrote it earlier!
		Label(satModEndLabel);
		BLR();
	}
	Label(endOfSubroutines);

	// Main Algorithm!
	{
		// Load the target port from safe space!
		LHZ(reg0, 1, safeStackWordOff + 0x6);
		// Subtract 1 so P1 is now 0, and use the Condition Reg to also implicitly compare against 0!
		ADDIC(reg0, reg0, -1, 1);
		// If the target port doesn't correspond to one of the code menu lines, we'll skip execution.
		JumpToLabel(exitLabel, bCACB_LESSER);
		CMPLI(reg0, 7, 0);
		JumpToLabel(exitLabel, bCACB_GREATER);
		ANDI(reg0, reg0, 0b11);

		// Set up the top half of reg1 with 0x804E to simplify accessing code menu stuff.
		ADDIS(reg1, 0, FLOAT_CONVERSION_CONST_LOC >> 0x10);
		// Store our RGBA value so we can load it Paired-Single style!
		STW(RGBAResultReg, reg1, (FLOAT_CONVERSION_STAGING_LOC + 0x4) & 0xFFFF);
		// Backup GQR0 in preparation for our Paired Single float reads.
		MFSPR(GQRBackupReg, 912);
		STW(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC + 0x8) & 0xFFFF);
		// Setup a new Quantization Register for our reads
		// Specifically, we're reading/writing Unsigned Bytes, and quantizing them to 2^7 = 128!
		ADDIS(reg2, 0, 0x0704);
		ORI(reg2, reg2, 0x0804);
		MTSPR(reg2, 912);
		// Load Hue Float to Hue FReg
		PSQ_L(floatHSLRegisters[0], reg1, (FLOAT_CONVERSION_STAGING_LOC + 0x4) & 0xFFFF, 1, 0);
		// Load Saturation and Luminance Floats to Sat FReg...
		PSQ_L(floatHSLRegisters[1], reg1, (FLOAT_CONVERSION_STAGING_LOC + 0x5) & 0xFFFF, 0, 0);
		// ... and move Luminance Float to Lum FReg.
		PS_MERGE11(floatHSLRegisters[2], floatHSLRegisters[1], floatHSLRegisters[1]);
		// Load 3.0f into TempReg0...
		LFS(floatTempRegisters[0], 2, -0x6168);
		// ... and use it to multiply the Hue (to ensure it ranges from 0.0 to 6.0).
		FMULS(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[0]);

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
		ADDIS(reg1, 0, PSCC_FLOAT_TABLE_LOC >> 0x10);
		LWZ(reg1, reg1, PSCC_FLOAT_TABLE_LOC & 0xFFFF);

		// Grab the appropriate Color Index
		LBZX(reg2, reg1, reg2);
		MULLI(reg0, reg2, 0xC);

		// Load the associated float (and point reg1 to our floatTriple)...
		LFSUX(floatTempRegisters[0], reg1, reg0);
		// ... and add it to our Hue float!
		FADDS(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[0]);

		// Load 1.0f into TempReg0, and 2.0f into TempReg1.
		LFS(floatTempRegisters[0], 2, -0x6170);
		FADDS(floatTempRegisters[1], floatTempRegisters[0], floatTempRegisters[0]);

		// Ensure that our Hue remains in the 0.0f to 6.0 range by doing mod 6.0f!
		FADDS(floatCalcRegisters[1], floatTempRegisters[0], floatTempRegisters[1]);		// Hue = Hue mod 6.0f
		FADDS(floatCalcRegisters[1], floatCalcRegisters[1], floatCalcRegisters[1]);		//
		B(2);																			//
		FSUBS(floatHSLRegisters[0], floatHSLRegisters[0], floatCalcRegisters[1]);		//
		FCMPU(floatHSLRegisters[0], floatCalcRegisters[1], 1);							//
		BC(-2, bCACB_GREATER_OR_EQ.inConditionRegField(1));								//

		// Apply Saturation Multiplier
		FMR(floatCalcRegisters[0], floatHSLRegisters[1]);								// Copy Saturation Multiplier into CalcReg0...
		LFS(floatCalcRegisters[1], reg1, 0x4);											// ... and load Absolute Saturation into CalcReg1 from the triple.
		JumpToLabel(applyMultiplierSubroutineLabel, bCACB_UNSPECIFIED, 1);
		FMR(floatHSLRegisters[1], floatCalcRegisters[0]);

		// Apply Luminence Multiplier
		FMR(floatCalcRegisters[0], floatHSLRegisters[2]);								// Copy Luminence Multiplier into CalcReg0...
		LFS(floatCalcRegisters[1], reg1, 0x8);											// ... and load Absolute Luminence into CalcReg1 from the triple.
		JumpToLabel(applyMultiplierSubroutineLabel, bCACB_UNSPECIFIED, 1);
		FMR(floatHSLRegisters[2], floatCalcRegisters[0]);

		// Calculate Chroma
		FADDS(floatCalcRegisters[0], floatHSLRegisters[2], floatHSLRegisters[2]);		// C = Luminence * 2.0f
		FSUBS(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[0]);		// C = C - 1.0f
		FABS(floatCalcRegisters[0], floatCalcRegisters[0]);								// C = Abs(X)
		FSUBS(floatCalcRegisters[0], floatTempRegisters[0], floatCalcRegisters[0]);		// C = 1.0f - C
		FMULS(floatCalcRegisters[0], floatCalcRegisters[0], floatHSLRegisters[1]);		// C = C * Saturation

		// Calculate X
		FMR(floatCalcRegisters[1], floatHSLRegisters[0]);								// X = Hue
		B(2);																			// X = X mod 2.0f
		FSUB(floatCalcRegisters[1], floatCalcRegisters[1], floatTempRegisters[1]);		//
		FCMPU(floatCalcRegisters[1], floatTempRegisters[1], 1);							//
		BC(-2, bCACB_GREATER_OR_EQ.inConditionRegField(1));								//
		FSUB(floatCalcRegisters[1], floatCalcRegisters[1], floatTempRegisters[0]);		// X = X - 1.0f
		FABS(floatCalcRegisters[1], floatCalcRegisters[1]);								// X = Abs(X)
		FSUB(floatCalcRegisters[1], floatTempRegisters[0], floatCalcRegisters[1]);		// X = 1.0f - X
		FMUL(floatCalcRegisters[1], floatCalcRegisters[1], floatCalcRegisters[0]);		// X = X * C

		// Calculate M (we'll write this into TempReg1, since we'll no longer need 2.0f after this!
		FDIVS(floatTempRegisters[1], floatCalcRegisters[0], floatTempRegisters[1]);		// M = C / 2.0f
		FSUBS(floatTempRegisters[1], floatHSLRegisters[2], floatTempRegisters[1]);		// M = Luminence - M

		// Get integer-converted Hue value in reg0!
		FCTIWZ(floatTempRegisters[0], floatHSLRegisters[0]);
		ADDIS(reg1, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
		STFD(floatTempRegisters[0], reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
		LWZ(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 8);

		int addMLabel = GetNextLabel();
		// Write our floats into their appropriate slots!
		// Move the target case into CTR in case we need it for BDZ'ing in a moment.
		MTCTR(reg2);
		// Additionally, compare it with 0; since our BDZ won't catch it if we decr past 0 then check.
		CMPLI(reg2, 0x00, 0);

		FSUB(floatHSLRegisters[2], floatHSLRegisters[2], floatHSLRegisters[2]);			// B = 0
		// 0 Case:
		FMR(floatHSLRegisters[0], floatCalcRegisters[0]);								// R = Chroma
		FMR(floatHSLRegisters[1], floatCalcRegisters[1]);								// G = X
		JumpToLabel(addMLabel, bCACB_EQUAL);
		// 1 Case:
		FMR(floatHSLRegisters[0], floatCalcRegisters[1]);								// R = X
		FMR(floatHSLRegisters[1], floatCalcRegisters[0]);								// G = Chroma
		JumpToLabel(addMLabel, bCACB_DZ);

		FSUB(floatHSLRegisters[0], floatHSLRegisters[0], floatHSLRegisters[0]);			// R = 0
		// 2 Case:
		FMR(floatHSLRegisters[1], floatCalcRegisters[0]);								// G = Chroma
		FMR(floatHSLRegisters[2], floatCalcRegisters[1]);								// B = X
		JumpToLabel(addMLabel, bCACB_DZ);
		// 3 Case:
		FMR(floatHSLRegisters[1], floatCalcRegisters[1]);								// G = X
		FMR(floatHSLRegisters[2], floatCalcRegisters[0]);								// B = Chroma
		JumpToLabel(addMLabel, bCACB_DZ);

		FSUB(floatHSLRegisters[1], floatHSLRegisters[1], floatHSLRegisters[1]);			// G = 0
		// 4 Case:
		FMR(floatHSLRegisters[2], floatCalcRegisters[0]);								// B = Chroma
		FMR(floatHSLRegisters[0], floatCalcRegisters[1]);								// R = X
		JumpToLabel(addMLabel, bCACB_DZ);
		// 5 Case:
		FMR(floatHSLRegisters[2], floatCalcRegisters[1]);								// B = X
		FMR(floatHSLRegisters[0], floatCalcRegisters[0]);								// R = Chroma

		// Add M to our newly sorted registers.
		Label(addMLabel);
		// Add M to Hue Reg
		FADDS(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[1]);
		// Pull Luminance into PS1 of Saturation Register, then add M to both at once.
		PS_MERGE00(floatHSLRegisters[1], floatHSLRegisters[1], floatHSLRegisters[2]);
		PS_ADD(floatHSLRegisters[1], floatHSLRegisters[1], floatTempRegisters[1]);

		// Store R component
		PSQ_ST(floatHSLRegisters[0], reg1, (FLOAT_CONVERSION_STAGING_LOC + 4) & 0xFFFF, 1, 0);
		// Store G and B components
		PSQ_ST(floatHSLRegisters[1], reg1, (FLOAT_CONVERSION_STAGING_LOC + 5) & 0xFFFF, 0, 0);
		// Store A component
		STB(RGBAResultReg, reg1, (FLOAT_CONVERSION_STAGING_LOC + 7) & 0xFFFF);
		// Re-load final RGBA hex!
		LWZ(RGBAResultReg, reg1, (FLOAT_CONVERSION_STAGING_LOC + 4) & 0xFFFF);

		// Restore backed up GQR0 value!
		MTSPR(GQRBackupReg, 912);
	}
	Label(skipMode1);

	Label(exitLabel);
	ASMEnd(0x907c0004); // Restore Original Instruction: stw r3, 0x0004 (r28)
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
		psccCLR0V4InstallCode();
		psccProtectStackCode();
		psccEmbedFloatTable();
		psccSetupCode();
		psccMainCode();

		CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
	}
}
