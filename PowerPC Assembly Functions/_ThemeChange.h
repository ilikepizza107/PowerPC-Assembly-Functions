#ifndef THEME_CHANGE_ASM_H
#define THEME_CHANGE_ASM_H

#include "_AdditionalCode.h"

constexpr unsigned long stringStagingLocation = 0x800002B0;

void themeChange();
void menuMainChange();
void selCharChange();
void selMapChange();
void selEventChange();
void titleChange();

void interceptMenuFilepathRef(unsigned long pathRegister, std::string replacementPath, unsigned long defaultPathAddress);

void selMapChangeV2();
void selEventChangeV2();
void selCharChangeV2();
void titleChangeV2();

#endif
