#include "stdafx.h"
#include "_PlayerSlotColorChangers.h"

const std::string codePrefix = "[CM: _PlayerSlotColorChangers v3.0.0] ";
const std::string codeSuffix = " [QuickLava]";

void playerSlotColorChangersV3(unsigned char codeLevel)
{
	int reg1 = 11;
	int reg2 = 12;
	int reg3 = 6; // Gets overwritten by original instruction

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
	ASMStart(0x801933e8, codePrefix + "CSS, In-game, and Results HUD Color Changer" + codeSuffix,
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

	std::vector<unsigned long> colorsTable =
	{
		(unsigned long)BLACK,
		(unsigned long)RED,
		(unsigned long)BLUE,
		(unsigned long)YELLOW,
		(unsigned long)GREEN,
		(unsigned long)LINE_COLOR_TABLE.COLORS[LINE_COLOR_TABLE.COLOR_PINK],
		(unsigned long)PURPLE,
		(unsigned long)ORANGE,
		(unsigned long)TEAL,
	};

	// Grab 0x2C past the pointer in r3, and we've got the original CLR0 data now.
	LWZ(reg1, 3, 0x00);

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

	BL(colorsTable.size() + 1);
	for (unsigned long color : colorsTable)
	{
		WriteIntToFile(color);
	}
	
	// Convert target frame to an integer, and copy it into reg1
	FCTIWZ(safeFloatReg, frameFloatReg);
	ADDIS(reg1, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
	STFD(safeFloatReg, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x4);
	LWZ(reg3, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x8);

	// If the target frame doesn't correspond to one of the code menu lines, we'll skip execution.
	CMPLI(reg3, 1, 0);
	JumpToLabel(exitLabel, bCACB_LESSER);
	CMPLI(reg3, 5, 0);
	JumpToLabel(exitLabel, bCACB_GREATER);

	// Load buffered Team Battle Status Offset
	//ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	// Now multiply the target frame by 4 to calculate the offset to the line we want, and insert it into reg1.
	RLWIMI(reg1, reg3, 2, 0x10, 0x1D);
	LWZ(reg1, reg1, (BACKPLATE_COLOR_1_LOC & 0xFFFF) - 0x4); // Minus 0x4 because target frame is 1 higher than the corresponding line.
	// Use it to load the relevant value.
	LWZX(reg2, reg1, reg2);
	
	// Get CLR0 Pointer Back
	LWZ(reg1, 3, 0x00);

	// Get Length of ResourceGroup, and scoot reg1 up to beginning of Group
	LWZU(reg3, reg1, 0x24);
	// Add the length of the ResourceGroup to reg1 to get address of first CLR0Mat
	ADD(reg1, reg1, reg3);
	// Look forwards into the first MatEntry in the Mat to get its Data Offset...
	LWZU(reg3, reg1, 0xC);
	// ... and add that to reg1 to get to its frames!
	ADD(reg1, reg1, reg3);

	// Quadruple our desired color index by 4 to index into color table
	MULLI(reg3, reg2, 0x4);

	// Reacquire original target frame.
	ADDIS(reg2, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
	LWZ(reg2, reg2, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 0x8);
	// Multiply it by 4 and add it to reg1 to point to the target frame!
	MULLI(reg2, reg2, 0x4);
	ADD(reg1, reg1, reg2);

	// Get Color Table Pointer
	MFLR(reg2);
	// Get RGB for target color
	LWZUX(reg3, reg2, reg3);
	// Load the frame's current color...
	LWZ(reg2, reg1, 0x00);
	// ... and write its alpha over ours.
	RLWIMI(reg3, reg2, 0x00, 0x18, 0x1F);
	// Then store that over the relevant frame in the CLR0!
	STW(reg3, reg1, 0x00);

	Label(exitLabel);

	ASMEnd(0x80c30000); // Restore original instruction: lwz r6, 0 (r3)

	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
}