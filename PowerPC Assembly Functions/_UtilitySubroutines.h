#ifndef UTILITY_SUBROUTINES_V1_H
#define UTILITY_SBUROUTINES_V1_H

#include "stdafx.h"
#include "PowerPC Assembly Functions.h"

int getSaveFPRsDownLabel(unsigned char highestFPR);
int getRestoreFPRsDownLabel(unsigned char highestFPR);

void utilitySubroutines();

#endif