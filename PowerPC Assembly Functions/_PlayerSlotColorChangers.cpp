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
const std::string activatorStringBase = "lBC0";
signed short activatorStringHiHalf = lava::bytesToFundamental<signed short>((unsigned char*)activatorStringBase.data());
signed short activatorStringLowHalf = lava::bytesToFundamental<signed short>((unsigned char*)activatorStringBase.data() + 2);
//signed short externalActivatorStringHiHalf = (unsigned short)activatorStringHiHalf + 0x2000;
unsigned long safeStackWordOff = 0x10; // Stores version of the code we need to run if signal word found; 0xFFFFFFFF otherwise!

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

	// LHZ(reg2, reg3, 0x02);
	// ADDIS(reg2, reg2, activatorStringHiHalf);
	// STW(reg2, reg3, 0x00);

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
	int exitLabel = GetNextLabel();

	ASMStart(0x80193410, codePrefix + "Prep Code" + codeSuffix);
	// Attempt to grab first 4 bytes of the CLR0's "Original Address" value.
	LWZ(reg3, 6, 0x18);
	LWZX(reg3, 6, reg3);
	// If this fails, the register'll be "CLR0", otherwise it should be the specified value.
	// Do some subtractions to check that the activator is there *and* get the code operation version!
	ADDIS(reg3, reg3, -activatorStringHiHalf);
	ADDI(reg3, reg3, -activatorStringLowHalf);
	// If the activator was present, the above subtractions should have reduced it down to just the number corresponding to its mode!
	// Do the check for Mode 3 in CF7, so that we can use whether we're in it again later to toggle the final port number subtraction!
	CMPLI(reg3, 0x3, 0x7);
	JumpToLabel(mode0Label, bCACB_EQUAL.inConditionRegField(0x7));
	// If Mode 0...
	CMPLI(reg3, 0x0, 0);
	JumpToLabel(mode0Label, bCACB_EQUAL);
	// The remaining two modes deal with the frame float, so make a mutable copy of that...
	FMR(13, 1);
	// If Mode 1...
	CMPLI(reg3, 0x1, 0);
	JumpToLabel(mode1Label, bCACB_EQUAL);
	// If Mode 2...
	CMPLI(reg3, 0x2, 0);
	JumpToLabel(mode2Label, bCACB_EQUAL);
	// If the detected Mode doesn't correspond to a supported case, force the mode to 0xFFFFFFFF and exit!
	ORC(reg3, reg3, reg3);
	JumpToLabel(exitLabel);

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
	JumpToLabel(getUserDataLabel);

	// This is the same as Mode1, only we add 1 to the frame first; so we can just make it an add-on for Mode1.
	Label(mode2Label);
	LFS(12, 2, -0x6170); 
	FADD(13, 13, 12);
	// Store frame as an integer, and store it as the second word of our safe space.
	Label(mode1Label);
	FCTIWZ(13, 13);
	STFD(13, 1, safeStackWordOff);

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

	Label(exitLabel);
	// And store our Code Mode as the top half of the second st!
	STH(reg3, 1, safeStackWordOff + 0x4);
	ADDI(5, 5, 0x1); // Restore Original Instruction
	ASMEnd();
}

void psccEmbedFloatTable()
{
	// Setup Color Float Triple Table
	std::vector<std::array<float, 3>> colorFloats =
	{
		{0.0f,	1.0f,	1.0f},	// Color 0
		{0.0f,	1.0f,	1.0f},	// Color 1
		{4.0f,	0.85f,	1.0f},	// Color 2
		{0.85f,	1.1f,	1.0f},	// Color 3
		{2.15f,	0.85f,	0.95f},	// Color 4
		{5.2f,	1.35f,	1.0f},	// Color 5
		{4.67f,	1.35f,	1.0f},	// Color 6
		{0.5f,	1.35f,	1.0f},	// Color 7
		{3.0f,	1.35f,	1.0f},	// Color 8
		{0.0f,	0.0f,	1.0f},	// Color 9
	};
	// Initialize Converted Float Table (Ensuring it's aligned to 0x10 bytes)
	std::vector<unsigned long> convertedTable(colorFloats.size() * 3, 0x00);
	std::size_t idx = 0;
	for (std::size_t i = 0; i < colorFloats.size(); i++)
	{
		auto currTriple = &colorFloats[i];
		for (std::size_t u = 0; u < 3; u++)
		{
			convertedTable[idx++] = lava::bytesToFundamental<unsigned long>(lava::fundamentalToBytes<float>((*currTriple)[u]).data());
		}
	}
	CodeRawStart(codePrefix + "Embed Color Float Table" + codeSuffix, "");
	GeckoDataEmbed(convertedTable, PLAYER_SLOT_COLOR_CHANGER_FLOAT_TABLE_LOC);
	CodeRawEnd();
}

void psccMainCode(unsigned char codeLevel)
{
	int reg0 = 0;
	int reg1 = 11;
	int reg2 = 12;
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
	// ... and write its complement to r0. If the mode was invalid reg0 should now be 0, and since we have Rc enabled...
	CMPLI(reg2, 0xFFFF, 0);
	// ... this'll branch us to the exit if the mode was invalid!
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
		FSUB(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[0]);		// Mul = Mul - 1.0f  
		FSUB(floatTempRegisters[1], floatTempRegisters[0], floatCalcRegisters[1]);		// Temp = 1.0f - Value
		FMADD(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[1], floatCalcRegisters[1]);		// Value = (Mul * Temp) + Value
		FADD(floatTempRegisters[1], floatTempRegisters[0], floatTempRegisters[0]);		// Restore the 2.0f float, since we overwrote it earlier!
		Label(satModEndLabel);
		BLR();
	}
	Label(endOfSubroutines);

	// Mode 1
	{
		// Load the target port from safe space!
		LHZ(reg0, 1, safeStackWordOff + 0x6);
		// If the target port doesn't correspond to one of the code menu lines, we'll skip execution.
		CMPLI(reg0, 1, 0);
		JumpToLabel(exitLabel, bCACB_LESSER);
		CMPLI(reg0, 5, 0);
		JumpToLabel(exitLabel, bCACB_GREATER);

		// Set up our conversion float in TempReg1
		ADDIS(reg1, 0, FLOAT_CONVERSION_CONST_LOC >> 0x10);
		LFD(floatTempRegisters[1], reg1, (FLOAT_CONVERSION_CONST_LOC & 0xFFFF));

		// Load TempReg0 with 255.0f
		ADDI(reg2, 0, 0xFF);
		STW(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x4);
		LFD(floatTempRegisters[0], reg1, FLOAT_CONVERSION_STAGING_LOC & 0xFFFF);
		FSUB(floatTempRegisters[0], floatTempRegisters[0], floatTempRegisters[1]);

		// Load our 3 HSL values into their float registers!
		for (unsigned long i = 0; i < 3; i++)
		{
			RLWINM(reg2, RGBAResultReg, (i + 1) * 0x8, 0x18, 0x1F);
			if (i == 0)
			{
				// Multiply hue specifically by 6, since hue rotation spans 0.0f -> 6.0f.
				MULLI(reg2, reg2, 0x6);
			}
			STW(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x4);
			LFD(floatHSLRegisters[i], reg1, FLOAT_CONVERSION_STAGING_LOC & 0xFFFF);
			FSUB(floatHSLRegisters[i], floatHSLRegisters[i], floatTempRegisters[1]);
			FDIV(floatHSLRegisters[i], floatHSLRegisters[i], floatTempRegisters[0]);
		}

		// Load buffered Team Battle Status Offset
		LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
		// Now multiply the target port by 4 to calculate the offset to the line we want, and insert it into reg1.
		RLWIMI(reg1, reg0, 2, 0x10, 0x1D);
		LWZ(reg1, reg1, (BACKPLATE_COLOR_1_LOC & 0xFFFF) - 0x4); // Minus 0x4 because target frame is 1 higher than the corresponding line.
		// Use it to load the targetIndex...
		LWZX(reg2, reg1, reg2);
		// ... and multiply it by 0xC to turn it into the offset to our target float triple.
		MULLI(reg0, reg2, 0xC);
		// Grab the pointer to our Float Hue Table
		ADDIS(reg1, 0, PLAYER_SLOT_COLOR_CHANGER_FLOAT_TABLE_LOC >> 0x10);
		LWZ(reg1, reg1, PLAYER_SLOT_COLOR_CHANGER_FLOAT_TABLE_LOC & 0xFFFF);
		// Load the associated float (and point reg1 to our floatTriple)...
		LFSUX(floatTempRegisters[0], reg1, reg0);
		// ... and add it to our Hue float!
		FADD(floatHSLRegisters[0], floatHSLRegisters[0], floatTempRegisters[0]);

		// Load 1.0f into TempReg0, and 2.0f into TempReg1.
		// We'll grab 255.0f again when we need it, and we won't be needing the conversion constant again.
		LFS(floatTempRegisters[0], 2, -0x6170);
		FADD(floatTempRegisters[1], floatTempRegisters[0], floatTempRegisters[0]);

		// Ensure that our Hue remains in the 0.0f to 6.0 range by doing mod 6.0f!
		FADD(floatCalcRegisters[1], floatTempRegisters[0], floatTempRegisters[1]);		// Hue = Hue mod 6.0f
		FADD(floatCalcRegisters[1], floatCalcRegisters[1], floatCalcRegisters[1]);		//
		B(2);																			//
		FSUB(floatHSLRegisters[0], floatHSLRegisters[0], floatCalcRegisters[1]);		//
		FCMPU(floatHSLRegisters[0], floatCalcRegisters[1], 1);							//
		BC(-2, bCACB_GREATER_OR_EQ.inConditionRegField(1));								//

		// Apply Saturation Multiplier
		LFS(floatCalcRegisters[0], reg1, 0x4);											// Load Absolute Saturation Mul to CalcReg0 from the triple...
		FABS(floatCalcRegisters[0], floatCalcRegisters[0]);								// ... and ensure its value is positive.
		FMR(floatCalcRegisters[1], floatHSLRegisters[1]);								// Copy Saturation into CalcReg1
		JumpToLabel(applyMultiplierSubroutineLabel, bCACB_UNSPECIFIED, 1);
		FMR(floatHSLRegisters[1], floatCalcRegisters[0]);

		// Apply Luminence Multiplier
		LFS(floatCalcRegisters[0], reg1, 0x8);											// Load Absolute Luminence Mul to CalcReg0 from the triple...
		FABS(floatCalcRegisters[0], floatCalcRegisters[0]);								// ... and ensure its value is positive.
		FMR(floatCalcRegisters[1], floatHSLRegisters[2]);								// Copy Luminences into CalcReg1
		JumpToLabel(applyMultiplierSubroutineLabel, bCACB_UNSPECIFIED, 1);
		FMR(floatHSLRegisters[2], floatCalcRegisters[0]);

		// Calculate Chroma
		FADD(floatCalcRegisters[0], floatHSLRegisters[2], floatHSLRegisters[2]);		// C = Luminence * 2.0f
		FSUB(floatCalcRegisters[0], floatCalcRegisters[0], floatTempRegisters[0]);		// C = C - 1.0f
		FABS(floatCalcRegisters[0], floatCalcRegisters[0]);								// C = Abs(X)
		FSUB(floatCalcRegisters[0], floatTempRegisters[0], floatCalcRegisters[0]);		// C = 1.0f - C
		FMUL(floatCalcRegisters[0], floatCalcRegisters[0], floatHSLRegisters[1]);		// C = C * Saturation

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
		FDIV(floatTempRegisters[1], floatCalcRegisters[0], floatTempRegisters[1]);		// M = C / 2.0f
		FSUB(floatTempRegisters[1], floatHSLRegisters[2], floatTempRegisters[1]);		// M = Luminence - M

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

		Label(addMLabel);

		// Load Int-Float-Conversion Value into CalcReg0
		LFD(floatCalcRegisters[0], reg1, FLOAT_CONVERSION_CONST_LOC & 0xFFFF);
		// Load 255.0f into CalcReg1
		ADDI(reg2, 0, 0xFF);
		STW(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
		LFD(floatCalcRegisters[1], reg1, FLOAT_CONVERSION_STAGING_LOC & 0xFFFF);
		FSUB(floatCalcRegisters[1], floatCalcRegisters[1], floatCalcRegisters[0]);

		// Finally, for each of our RGB values...
		for (unsigned long i = 0; i < 3; i++)
		{
			// ... add M to it...
			FADD(floatHSLRegisters[i], floatHSLRegisters[i], floatTempRegisters[1]);
			// ... and multiply it by 255.0f;
			FMUL(floatHSLRegisters[i], floatHSLRegisters[i], floatCalcRegisters[1]);
			// From there, convert it back to an integer...
			FCTIWZ(floatHSLRegisters[i], floatHSLRegisters[i]);
			STFD(floatHSLRegisters[i], reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
			LWZ(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 8);
			// ... and store it in the appropriate spot in r3; and we're finally done!
			RLWIMI(RGBAResultReg, reg2, 0x18 - (i * 0x8), 0x00 + (i * 0x08), 0x07 + (i * 0x8));
		}
	}
	Label(skipMode1);

	Label(exitLabel);
	ASMEnd(0x907c0004); // Restore Original Instruction: stw r3, 0x0004 (r28)
}

void playerSlotColorChangersV3(unsigned char codeLevel)
{
	if (codeLevel > 0)
	{
		bool backupMulliOptSetting = CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS;
		CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = 1;

		psccCLR0V4InstallCode();
		psccProtectStackCode();
		psccEmbedFloatTable();
		psccSetupCode();
		psccMainCode(codeLevel);

		CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
	}
}
