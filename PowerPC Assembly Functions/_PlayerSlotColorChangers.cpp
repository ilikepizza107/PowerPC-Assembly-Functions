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

	// If all those checks pass, set up our Frame Buffer.
	constexpr unsigned long frameBlendBufferLength = 2;
	BL(frameBlendBufferLength + 1);
	for (unsigned long i = 0; i < frameBlendBufferLength; i++)
	{
		WriteIntToFile(0x0);
	}
	// Quadruple the target frame...
	MULLI(reg2, reg3, 0x4);
	// ... and use it to index into the original frame array and grab the original frame's color value!
	LWZX(reg2, frameBufferReg, reg2);
	// And now that we've got the original value, we can point frameBufferReg at our custom blending buffer...
	MFLR(frameBufferReg);
	// ... and store the original frame in the custom buffers second slot!
	STW(reg2, frameBufferReg, 0x04);

	// Embedded color table!
	BL(colorsTable.size() + 1);
	for (unsigned long color : colorsTable)
	{
		WriteIntToFile(color);
	}
	
	// Load buffered Team Battle Status Offset
	ADDIS(reg1, 0, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC >> 0x10);
	LBZ(reg2, reg1, BACKPLATE_COLOR_TEAM_BATTLE_STORE_LOC & 0xFFFF);
	// Now multiply the target frame by 4 to calculate the offset to the line we want, and insert it into reg1.
	RLWIMI(reg1, reg3, 2, 0x10, 0x1D);
	LWZ(reg1, reg1, (BACKPLATE_COLOR_1_LOC & 0xFFFF) - 0x4); // Minus 0x4 because target frame is 1 higher than the corresponding line.
	// Use it to load the relevant value.
	LWZX(reg2, reg1, reg2);

	// Pull Color Table address into reg2...
	MFLR(reg1);
	// ... multiply the desired color ID by 4 to turn it into an index...
	MULLI(reg2, reg2, 0x4);
	// ... load the RGB associated with that color.
	LWZX(reg2, reg1, reg2);
	// Then, regrab the original frame's color...
	LWZ(reg3, frameBufferReg, 0x04);
	// ... overwrite our custom color's Alpha with that one's...
	RLWIMI(reg2, reg3, 0x00, 0x18, 0x1F);
	// ... and finally store it in the first slot of our blending buffer!
	STW(reg2, frameBufferReg, 0x00);

	// Lastly, force the "currentFrame" to 0.5, to evenly blend the two frames in the blending buffer!
	ADDIS(reg1, 0, FLOAT_CONVERSION_STAGING_LOC >> 0x10);
	ADDIS(reg2, 0, 0x3F00);
	STW(reg2, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);
	LFS(frameFloatReg, reg1, (FLOAT_CONVERSION_STAGING_LOC & 0xFFFF) + 4);

	Label(exitLabel);

	ASMEnd();

	CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS = backupMulliOptSetting;
}