#ifndef ADDITIONAL_CODE_H
#define ADDITIONAL_CODE_H

#include "Code Menu.h"
#include <conio.h>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace lava
{
	const std::string version = "v1.0.4";
	extern int CMNUCopyOverride;
	extern int ASMCopyOverride;
	extern int GCTBuildOverride;
	extern int CloseOnFinishBypass;
	enum argumentIDs
	{
		aI_CMNU = 1,
		aI_ASM,
		aI_GCT,
		aI_CLOSE,
		argumentCount
	};

	// General Utility
	int stringToNum(const std::string& stringIn, bool allowNeg = 1, int defaultVal = INT_MAX);
	std::string numToHexStringWithPadding(std::size_t numIn, std::size_t paddingLength);
	std::string numToDecStringWithPadding(std::size_t numIn, std::size_t paddingLength);

	// File System Management
	bool copyFile(std::string sourceFile, std::string targetFile, bool overwriteExistingFile = 0);
	bool backupFile(std::string fileToBackup, std::string backupSuffix = ".bak", bool overwriteExistingBackup = 0);

	// Prompts and User-Interaction
	bool yesNoDecision(char yesKey, char noKey);
	bool offerCopyOverAndBackup(std::string fileToCopy, std::string fileToOverwrite, int decisionOverride = INT_MAX);
	bool offerCopy(std::string fileToCopy, std::string fileToOverwrite, int decisionOverride = INT_MAX);
	bool handleAutoGCTRMProcess(std::ostream& logOutput, int decisionOverride = INT_MAX);

	// EX Character Functions
	std::vector<std::pair<std::string, u16>> collectNameSlotIDPairs(std::string exCharInputFilePath, bool& fileOpened);
}

#endif