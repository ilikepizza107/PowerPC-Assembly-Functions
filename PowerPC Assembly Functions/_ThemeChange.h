#ifndef THEME_CHANGE_ASM_H
#define THEME_CHANGE_ASM_H

#include "stdafx.h"
#include "_AdditionalCode.h"

//constexpr unsigned long stringStagingLocation = 0x800002B0;

void themeChange();
void editFilepathConstant(std::string fileDirectory, std::string baseFilename, unsigned long pathRegister, unsigned long defaultPathAddress1, unsigned long defaultPathAddress2, themeConstants::themePathIndices relevantFile);
void menuMainChange();
void selCharChange();
void selMapChange();
void selEventChange();
void titleChange();


//void themeChangev2();
//void interceptMenuFilepathRef(unsigned long pathRegister, std::string replacementPath, unsigned long defaultPathAddress);
//void selMapChangeV2();
//void selEventChangeV2();
//void selCharChangeV2();
//void titleChangeV2();

#endif
