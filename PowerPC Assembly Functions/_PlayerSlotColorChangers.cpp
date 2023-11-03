#include "stdafx.h"
#include "_PlayerSlotColorChangers.h"

const std::string codePrefix = "[CM: _PlayerSlotColorChangers v3.0.0] ";
const std::string codeSuffix = " [QuickLava]";

void playerSlotColorChangersV3(unsigned char codeLevel)
{
	int CLR0PtrReg = 6;
	int frameBufferReg = 3;
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 10; // Gets overwritten by original instruction

	int frameFloatReg = 1;
	int safeFloatReg = 13;

	// New approach:
	// - Verify that incoming CLR0 is asking for override
	// - Determine the index of the frame they're asking for
	// - Determine that it's in our range
	// - Get the index for the color being requested by the relevant code menu line
	// - Grab the RGB for that color
	// - Write that over the requested frame in the CLR0
	// - Profit


	const std::string activatorString = "lBC1";
	bool backupMulliOptSetting = CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS;
	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = 1;
	// Hooks "GetAnmResult/[nw4r3g3d9ResAnmClrCFPQ34nw4r3g3d12]/g3d_res".
	ASMStart(0x801934f4, codePrefix + "CSS, In-game, and Results HUD Color Changer" + codeSuffix,
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

	int exitLabel = GetNextLabel();

	// Restore original instruction; pointing r3 to frame RGB array!
	ADDI(frameBufferReg, frameBufferReg, 0x4);

	ADDIS(reg1, 0, 0x8000);
	CMPL(CLR0PtrReg, reg1, 0);
	BC(2, bCACB_GREATER);
	NOP();
	JumpToLabel(exitLabel, bCACB_LESSER_OR_EQ);

	// Grab the pointer to the CLR0 from r6.
	MR(reg1, CLR0PtrReg);

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

	// Convert target frame to an integer, and copy it into reg3
	FCTIWZ(safeFloatReg, frameFloatReg);
	ADDIS(reg1, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
	STFD(safeFloatReg, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x4);
	LWZ(reg3, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x8);

	// If the target frame doesn't correspond to one of the code menu lines, we'll skip execution.
	CMPLI(reg3, 1, 0);
	JumpToLabel(exitLabel, bCACB_LESSER);
	CMPLI(reg3, 5, 0);
	JumpToLabel(exitLabel, bCACB_GREATER);

	std::vector<unsigned long> colorsTable =
	{
		0x000000FF,
		0xFF0000FF, // Red
		0xFFFF00FF, // Yellow
		0x00FF00FF, // Green
		0x00FFFFFF, // Cyan
		0x0000FFFF, // Blue
		0xFF00FFFF, // Magenta
		0xFF0000FF, // Red (Again)
	};
	BL(colorsTable.size() + 1);
	for (unsigned long color : colorsTable)
	{
		WriteIntToFile(color);
	}
	MFLR(frameBufferReg);
	
	// Load buffered Team Battle Status Offset
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	// Now multiply the target frame by 4 to calculate the offset to the line we want, and insert it into reg1.
	RLWIMI(reg1, reg3, 2, 0x10, 0x1D);
	LWZ(reg1, reg1, (BACKPLATE_COLOR_1_LOC & 0xFFFF) - 0x4); // Minus 0x4 because target frame is 1 higher than the corresponding line.
	// Use it to load the targetIndex...
	LWZX(reg2, reg1, reg2);
	// ... and multiply it by 4 to turn it into the offset to our target value.
	MULLI(reg2, reg2, 0x4);
	// Get source index for line.
	LWZ(reg1, reg1, Line::SELECTION_LINE_SOURCE_SELECTION_INDEX);
	// Move forwards to the offsets section of the line.
	ADDI(reg1, reg1, Line::SELECTION_LINE_OFFSETS_START + 2);
	// And pull the top-half of the float associated with this line!
	LHZX(reg2, reg1, reg2);
	// Stage the associated float in our staging spot...
	ADDIS(reg1, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
	STH(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
	// And load it! We've now got the relevant float loaded in f1!
	LFS(frameFloatReg, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
	// Then load the constant float for 1.
	LFS(safeFloatReg, 2, -0x6138);
	FADD(frameFloatReg, frameFloatReg, safeFloatReg);

	Label(exitLabel);

	ASMEnd();

	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
}