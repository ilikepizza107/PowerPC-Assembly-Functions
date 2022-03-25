#ifndef ADDITIONAL_CODE_H
#define ADDITIONAL_CODE_H

#include "Code Menu.h"
#include <conio.h>
#include <iomanip>
#include <sstream>

namespace lava
{
	int stringToNum(const std::string& stringIn, bool allowNeg = 1, int defaultVal = INT_MAX);
	std::string numToHexStringWithPadding(std::size_t numIn, std::size_t paddingLength);
	std::string numToDecStringWithPadding(std::size_t numIn, std::size_t paddingLength);

	bool fileExists(std::string filepathIn);
	bool folderExists(std::string folderpathIn);
	bool copyFile(std::string sourceFile, std::string targetFile, bool overwriteExistingFile = 0);
	bool backupFile(std::string fileToBackup, std::string backupSuffix = ".bak", bool overwriteExistingBackup = 0);

	bool yesNoDecision(char yesKey, char noKey);
	bool offerCopyOverAndBackup(std::string fileToCopy, std::string fileToOverwrite);
	bool handleAutoGCTRMProcess(std::ostream& logOutput);
}

#endif