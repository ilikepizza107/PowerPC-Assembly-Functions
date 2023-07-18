#ifndef BACKPLATE_COLORS_H
#define BACKPLATE_COLORS_H

#include "stdafx.h"
#include "_AdditionalCode.h"

// Writes the code for the Player Slot Color Changer to the .asm output!
// The codeLevel argument specifies what elements the code should be configured to affect.
void playerSlotColorChangers(unsigned char codeLevel = backplateColorConstants::pSCL_MENUS_AND_IN_GAME_WITH_CSS_INPUT);

void incrementOnButtonPress();

void transparentCSSandResultsScreenNames();

void storeTeamBattleStatus();

void disableCPUTeamColors();

void randomColorChange();
void menSelChrElemntChange();
void backplateColorChange();

void shieldColorChange();

void infoPacCLR0ColorChange();

// These were developed as part of the research for this project, but ultimately not used.
// They do provide potentially useful isolated functionality though, and so are being left here.
// The SelChar one isn't finished, it only acts when you change into and out of Team Mode, and doesn't
//	correctly affect all menu elements (and values are incorrect in certain modes, eg. hands in Classic Mode).
void selcharCLR0ColorChange();
void resultsCLR0ColorChange();
#endif
