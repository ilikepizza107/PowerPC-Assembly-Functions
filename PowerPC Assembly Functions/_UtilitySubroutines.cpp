#include "stdafx.h"
#include "_UtilitySubroutines.h"

const std::string codePrefix = "[CM: _UtilitySubroutines v1.0.0] ";
const std::string codeSuffix = " [QuickLava]";

template <std::size_t size>
struct labelArr : std::array<int, size>
{
	labelArr()
	{
		for (std::size_t i = 0; i < this->size(); i++)
		{
			(*this)[i] = GetNextLabel();
		}
	}
};

/*
unsigned char highestSaveGPRLabelUsed = UCHAR_MAX;
labelArr<32> saveGPRDownLabels;
int getSaveGPRDownLabel(unsigned char startingFromRegister)
{
	assert(startingFromRegister < 32, "Starting Reg must be less than 32!");
	if (highestSaveGPRLabelUsed == UCHAR_MAX)
	{
		highestSaveGPRLabelUsed = 0;
	}
	highestSaveGPRLabelUsed = std::max(highestSaveGPRLabelUsed, startingFromRegister);
	return saveGPRDownLabels[startingFromRegister];
}
void saveGPRDown()
{
	if (highestSaveGPRLabelUsed > 31) return;
	for (int i = highestSaveGPRLabelUsed; i >= 0; i--)
	{
		Label(saveGPRDownLabels[i]);
		STW(i, 11, 0x8 + (i * 0x4));
	}
	BLR();
}
unsigned char highestRestoreGPRLabelUsed = UCHAR_MAX;
labelArr<32> restoreGPRDownLabels;
int getRestoreGPRDownLabel(unsigned char startingFromRegister)
{
	assert(startingFromRegister < 32, "Starting Reg must be less than 32!");
	if (highestRestoreGPRLabelUsed == UCHAR_MAX)
	{
		highestRestoreGPRLabelUsed = 0;
	}
	highestRestoreGPRLabelUsed = std::max(highestRestoreGPRLabelUsed, startingFromRegister);
	return restoreGPRDownLabels[startingFromRegister];
}
void restoreGPRDown()
{
	if (highestRestoreGPRLabelUsed > 31) return;
	for (int i = highestRestoreGPRLabelUsed; i >= 0; i--)
	{
		Label(restoreGPRDownLabels[i]);
		LWZ(i, 11, 0x8 + (i * 0x4));
	}
	BLR();
}
*/

unsigned char highestSaveFPRLabelUsed = UCHAR_MAX;
labelArr<32> saveFPRLabels;
int getSaveFPRsDownLabel(unsigned char highestFPR)
{
	assert(highestFPR < 32, "Highest register must be below 32!");
	if (highestSaveFPRLabelUsed == UCHAR_MAX)
	{
		highestSaveFPRLabelUsed = 0;
	}
	highestSaveFPRLabelUsed = std::max(highestFPR, highestSaveFPRLabelUsed);
	return saveFPRLabels[highestFPR];
}
void saveFPRsDown()
{
	if (highestSaveFPRLabelUsed == UCHAR_MAX) return;
	int stackOff = -(0x10 + (highestSaveFPRLabelUsed * 0x8));
	for (int i = highestSaveFPRLabelUsed; i >= 0; i--)
	{
		Label(saveFPRLabels[i]);
		STFD(i, 1, stackOff);
		stackOff += 0x8;
	}
	BLR();
}

unsigned char highestRestoreFPRLabelUsed = UCHAR_MAX;
labelArr<32> restoreFPRLabels;
int getRestoreFPRsDownLabel(unsigned char highestFPR)
{
	assert(highestFPR < 32, "Highest register must be below 32!");
	if (highestRestoreFPRLabelUsed == UCHAR_MAX)
	{
		highestRestoreFPRLabelUsed = 0;
	}
	highestRestoreFPRLabelUsed = std::max(highestFPR, highestRestoreFPRLabelUsed);
	return restoreFPRLabels[highestFPR];
}
void restoreFPRsDown()
{
	if (highestRestoreFPRLabelUsed == UCHAR_MAX) return;
	int stackOff = -(0x10 + (highestRestoreFPRLabelUsed * 0x8));
	for (int i = highestRestoreFPRLabelUsed; i >= 0; i--)
	{
		Label(restoreFPRLabels[i]);
		LFD(i, 1, stackOff);
		stackOff += 0x8;
	}
	BLR();
}


void utilitySubroutines()
{
	ASMStart(UTILITY_SUBROUTINES_HOOK_LOC, codePrefix + "" + codeSuffix, "");
	saveFPRsDown();
	restoreFPRsDown();
	ASMEnd();
}
