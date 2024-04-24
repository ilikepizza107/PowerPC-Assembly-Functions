#ifndef UTILITY_SUBROUTINES_V1_H
#define UTILITY_SUBROUTINES_V1_H

#include "stdafx.h"
#include "PowerPC Assembly Functions.h"

int getSaveFPRsDownLabel(unsigned char highestFPR);
int getRestoreFPRsDownLabel(unsigned char highestFPR);

// Note: Must load r4 with SoundInfoIndex prior to calling this subroutine!! Function call will also overwrite r3 - r12!
enum SoundInfoIndex
{
	sii_SND_SE_SYSTEM_CURSOR = 0x00,
	sii_SND_SE_SYSTEM_INFOWINDOW_OPEN = 0x05,
	sii_SND_SE_SYSTEM_PLATE_CATCH = 0x08,
	sii_SND_SE_SYSTEM_COUNTER = 0x25,
};
int getPlaySELabel();

void utilitySubroutines();

#endif