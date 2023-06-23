#ifndef BACKPLATE_COLORS_H
#define BACKPLATE_COLORS_H

#include "stdafx.h"
#include "_AdditionalCode.h"

void playerSlotColorChangers();

void transparentCSSandResultsScreenNames();

void storeTeamBattleStatus();

void randomColorChange();
void menSelChrElemntChange();
void backplateColorChange();

void shieldColorChange();

// These were developed as part of the research for this project, but ultimately not used.
// They do provide potentially useful isolated functionality though, and so are being left here.
// The SelChar one isn't finished, it only acts when you change into and out of Team Mode, and doesn't
//	correctly affect all menu elements (and values are incorrect in certain modes, eg. hands in Classic Mode).
void selcharCLR0ColorChange();
void infoPacCLR0ColorChange();
void resultsCLR0ColorChange();
#endif
