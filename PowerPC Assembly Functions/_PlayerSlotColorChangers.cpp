#include "stdafx.h"
#include "_PlayerSlotColorChangers.h"

const std::string codePrefix = "[CM: _PlayerSlotColorChangers v3.0.0] ";
const std::string codeSuffix = " [QuickLava]";

//void playerSlotColorChangersV3(unsigned char codeLevel)
//{
//	int CLR0PtrReg = 6;
//	int frameBufferReg = 3;
//	int reg1 = 11;
//	int reg2 = 12;
//	int reg3 = 10; // Gets overwritten by original instruction
//
//	int frameFloatReg = 1;
//	int safeFloatReg = 13;
//
//	// New approach:
//	// - Verify that incoming CLR0 is asking for override
//	// - Determine the index of the frame they're asking for
//	// - Determine that it's in our range
//	// - Get the index for the color being requested by the relevant code menu line
//	// - Grab the RGB for that color
//	// - Write that over the requested frame in the CLR0
//	// - Profit
//
//
//	const std::string activatorString = "lBC1";
//	bool backupMulliOptSetting = CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS;
//	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = 1;
//	// Hooks "GetAnmResult/[nw4r3g3d9ResAnmClrCFPQ34nw4r3g3d12]/g3d_res".
//	ASMStart(0x801934f4, codePrefix + "CSS, In-game, and Results HUD Color Changer" + codeSuffix,
//		"\nIntercepts the setFrameMatCol calls used to color certain Menu elements by player slot, and"
//		"\nredirects them according to the appropriate Code Menu lines. Intended for use with:"
//		"\n\tIn sc_selcharacter.pac:"
//		"\n\t\t- MenSelchrCbase4_TopN__0\n\t\t- MenSelchrCmark4_TopN__0\n\t\t- MenSelchrCursorA_TopN__0"
//		"\n\tIn info.pac (and its variants, eg. info_training.pac):"
//		"\n\t\t- InfArrow_TopN__0\n\t\t- InfFace_TopN__0\n\t\t- InfLoupe0_TopN__0\n\t\t- InfMark_TopN__0\n\t\t- InfPlynm_TopN__0"
//		"\n\tIn stgresult.pac:"
//		"\n\t\t- InfResultRank#_TopN__0\n\t\t- InfResultMark##_TopN"
//		"\nTo trigger this code on a given CLR0, set its \"OriginalPath\" field to \"" + activatorString + "\" in BrawlBox!"
//	);
//
//	int exitLabel = GetNextLabel();
//
//	// Restore original instruction; pointing r3 to frame RGB array!
//	ADDI(frameBufferReg, frameBufferReg, 0x4);
//
//	ADDIS(reg1, 0, 0x8000);
//	CMPL(CLR0PtrReg, reg1, 0);
//	BC(2, bCACB_GREATER);
//	NOP();
//	JumpToLabel(exitLabel, bCACB_LESSER_OR_EQ);
//
//	// Grab the pointer to the CLR0 from r6.
//	MR(reg1, CLR0PtrReg);
//
//	// Grab the offset for the "Original Path" Value
//	LWZ(reg2, reg1, 0x18);
//
//	// Grab the first 4 bytes of the Orig Path
//	LWZX(reg2, reg2, reg1);
//	// Set reg1 to our Activation String
//	SetRegister(reg1, activatorString);
//	// Compare the 4 bytes we loaded to the target string...
//	CMPL(reg2, reg1, 0);
//	// ... and exit if they're not equal.
//	JumpToLabel(exitLabel, bCACB_NOT_EQUAL);
//
//	// Convert target frame to an integer, and copy it into reg3
//	FCTIWZ(safeFloatReg, frameFloatReg);
//	ADDIS(reg1, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
//	STFD(safeFloatReg, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x4);
//	LWZ(reg3, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x8);
//
//	// If the target frame doesn't correspond to one of the code menu lines, we'll skip execution.
//	CMPLI(reg3, 1, 0);
//	JumpToLabel(exitLabel, bCACB_LESSER);
//	CMPLI(reg3, 5, 0);
//	JumpToLabel(exitLabel, bCACB_GREATER);
//
//	std::vector<unsigned long> colorsTable =
//	{
//		0x000000FF,
//		0xFF0000FF, // Red
//		0xFFFF00FF, // Yellow
//		0x00FF00FF, // Green
//		0x00FFFFFF, // Cyan
//		0x0000FFFF, // Blue
//		0xFF00FFFF, // Magenta
//		0xFF0000FF, // Red (Again)
//	};
//	BL(colorsTable.size() + 1);
//	for (unsigned long color : colorsTable)
//	{
//		WriteIntToFile(color);
//	}
//	MFLR(frameBufferReg);
//	
//	// Load buffered Team Battle Status Offset
//	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
//	LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
//	// Now multiply the target frame by 4 to calculate the offset to the line we want, and insert it into reg1.
//	RLWIMI(reg1, reg3, 2, 0x10, 0x1D);
//	LWZ(reg1, reg1, (BACKPLATE_COLOR_1_LOC & 0xFFFF) - 0x4); // Minus 0x4 because target frame is 1 higher than the corresponding line.
//	// Use it to load the targetIndex...
//	LWZX(reg2, reg1, reg2);
//	// ... and multiply it by 4 to turn it into the offset to our target value.
//	MULLI(reg2, reg2, 0x4);
//	// Get source index for line.
//	LWZ(reg1, reg1, Line::SELECTION_LINE_SOURCE_SELECTION_INDEX);
//	// Move forwards to the offsets section of the line.
//	ADDI(reg1, reg1, Line::SELECTION_LINE_OFFSETS_START + 2);
//	// And pull the top-half of the float associated with this line!
//	LHZX(reg2, reg1, reg2);
//	// Stage the associated float in our staging spot...
//	ADDIS(reg1, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
//	STH(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
//	// And load it! We've now got the relevant float loaded in f1!
//	LFS(frameFloatReg, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
//	// Then load the constant float for 1.
//	LFS(safeFloatReg, 2, -0x6138);
//	FADD(frameFloatReg, frameFloatReg, safeFloatReg);
//
//	Label(exitLabel);
//
//	ASMEnd();
//
//	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
//}

void playerSlotColorChangersV3(unsigned char codeLevel)
{
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

	bool backupMulliOptSetting = CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS;
	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = 1;

	int reg0 = 0;
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 10;

	// Under normal circumstances, these are used for float conversions.
	// At the points we're hooking though; they're done being used, so we're free to use them!
	int safeStackWordOff1 = 0x10; // Stores version of the code we need to run if signal word found; 0xFFFFFFFF otherwise!
	int safeStackWordOff2 = 0x14; // Stores pointer to the CLR0!
	int stackFltConvOffset = 0x08; // This is a second float conversion spot, which we can mostly guarantee will be set up (unlike the previous one).

	const std::string activatorStringBase = "lBC0";
	signed short activatorStringHiHalf = lava::bytesToFundamental<signed short>((unsigned char*)activatorStringBase.data());
	signed short activatorStringLowHalf = lava::bytesToFundamental<signed short>((unsigned char*)activatorStringBase.data() + 2);

	ASMStart(0x801934ac, codePrefix + "Prep Code" + codeSuffix);
	MR(28, 30); // Restore Original Instruction!
	LWZ(reg2, 6, 0x18);
	LWZX(reg2, 6, reg2);
	ADDIS(reg2, reg2, -activatorStringHiHalf); // Used to check that the activator is there *and* get the code operation version!
	ADDI(reg2, reg2, -activatorStringLowHalf);
	CMPLI(reg2, 0x9, 0);
	BC(2, bCACB_LESSER_OR_EQ);
	ORC(reg2, reg2, reg2);
	// Get current frame as an integer, and store it as the second word of our safe space.
	FCTIWZ(13, 31);
	STFD(13, 1, safeStackWordOff1);
	// And store our Code Mode as the first word (overwriting the junk word from the above STFD)!
	STW(reg2, 1, safeStackWordOff1);
	ASMEnd();

	int RGBAResultReg = 3;

	int floatCalcRegisters[2] = {7, 8};
	int floatTempRegisters[2] = {9, 10};
	int floatHSLRegisters[3] = {11, 12, 13};
	int floatCurrFrameReg = 31;


	int endOfSubroutines = GetNextLabel();
	int skipMode1 = GetNextLabel();
	int exitLabel = GetNextLabel();
	// Hooks "GetAnmResult/[nw4r3g3d9ResAnmClrCFPQ34nw4r3g3d12]/g3d_res".
	ASMStart(0x801934fc, codePrefix + "Main Code" + codeSuffix, "");
	// Load the code mode word...
	LWZ(reg2, 1, safeStackWordOff1);
	// ... and write its complement to r0. If the mode was invalid reg0 should now be 0, and since we have Rc enabled...
	NOR(reg0, reg2, reg2, 1);
	// ... this'll branch us to the exit if the mode was invalid!
	JumpToLabel(exitLabel, bCACB_EQUAL);
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
	CMPLI(reg2, 0x1, 0);
	JumpToLabel(skipMode1, bCACB_NOT_EQUAL);
	{
		// Load current frame from safe space!
		LWZ(reg0, 1, safeStackWordOff1 + 0x4);
		// If the target frame doesn't correspond to one of the code menu lines, we'll skip execution.
		CMPLI(reg0, 1, 0);
		JumpToLabel(exitLabel, bCACB_LESSER);
		CMPLI(reg0, 5, 0);
		JumpToLabel(exitLabel, bCACB_GREATER);

		// Setup Color Float Triple Table
		std::vector<std::array<float, 3>> colorFloats =
		{
			{0.0f, 0.0f, 0.0f}, // Color 0
			{0.0f, 1.0f, 1.0f}, // Color 1
			{4.0f, 1.0f, 0.9f}, // Color 2
			{1.0f, 1.0f, 1.0f}, // Color 3
			{2.0f, 1.0f, 1.0f}, // Color 4
			{5.2f, 1.0f, 1.0f}, // Color 5
			{4.666f, 1.5f, 1.0f}, // Color 6
			{0.5f, 1.5f, 1.0f}, // Color 7
			{3.0f, 1.5f, 1.0f}, // Color 8
			{0.0f, 0.0f, 2.0f}, // Color 9
		};
		BL(1 + (colorFloats.size() * 0x3));
		for (std::size_t i = 0; i < colorFloats.size(); i++)
		{
			auto currTriple = &colorFloats[i];
			for (std::size_t u = 0; u < 3; u++)
			{
				unsigned long floatConv = lava::bytesToFundamental<unsigned long>(lava::fundamentalToBytes<float>((*currTriple)[u]).data());
				WriteIntToFile(floatConv);
			}
		}

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
		// Now multiply the target frame by 4 to calculate the offset to the line we want, and insert it into reg1.
		RLWIMI(reg1, reg0, 2, 0x10, 0x1D);
		LWZ(reg1, reg1, (BACKPLATE_COLOR_1_LOC & 0xFFFF) - 0x4); // Minus 0x4 because target frame is 1 higher than the corresponding line.
		// Use it to load the targetIndex...
		LWZX(reg2, reg1, reg2);
		// ... and multiply it by 0xC to turn it into the offset to our target float triple.
		MULLI(reg0, reg2, 0xC);
		// Grab the pointer to our Float Hue Table
		MFLR(reg1);
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

	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
}
