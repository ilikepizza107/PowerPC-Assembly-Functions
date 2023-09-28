#ifndef ADDITIONAL_CODE_H
#define ADDITIONAL_CODE_H

#include "stdafx.h"
#include "Code Menu.h"
#include "_lavaBytes.h"
#include "_lavaOutputSplitter.h"
#include <conio.h>
#include <filesystem>

namespace lava
{
	const std::string version = "v1.3.1";
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

	// File System Management
	bool copyFile(std::filesystem::path sourceFile, std::filesystem::path targetFile, bool overwriteExistingFile = 0);
	bool backupFile(std::string fileToBackup, std::string backupSuffix = ".bak", bool overwriteExistingBackup = 0);

	// Prompts and User-Interaction
	bool yesNoDecision(char yesKey, char noKey);
	bool offerCopyOverAndBackup(std::string fileToCopy, std::string fileToOverwrite, int decisionOverride = INT_MAX);
	bool offerCopy(std::string fileToCopy, std::string fileToOverwrite, int decisionOverride = INT_MAX);
	bool handleAutoGCTRMProcess(std::ostream& logOutput, int decisionOverride = INT_MAX);

	// Assembly Utility Functions
	void WriteByteVec(const unsigned char* Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite, bool appendNullTerminator = 0);
	void WriteByteVec(std::vector<unsigned char> Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite, bool appendNullTerminator = 0);
	void WriteByteVec(std::string Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite, bool appendNullTerminator = 0);

	// ==================== Menu Config Parsing and Constants ====================

	// Parses the specified Configuration XML and applies any changes it requests to the menu.
	bool parseAndApplyConfigXML(std::string configFilePath, lava::outputSplitter& logOutput);
}

#endif