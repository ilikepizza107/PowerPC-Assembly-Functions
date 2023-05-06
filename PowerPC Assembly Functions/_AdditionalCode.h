#ifndef ADDITIONAL_CODE_H
#define ADDITIONAL_CODE_H

#include "Code Menu.h"
#include "_lavaBytes.h"
#include <conio.h>
#include <iomanip>
#include <type_traits>
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
	struct decConvStream
	{
		std::stringstream buf;
		decConvStream();
	};
	struct hexConvStream
	{
		std::stringstream buf;
		hexConvStream();
	};
	struct fltConvStream
	{
		std::stringstream buf;
		fltConvStream();
	};
	int stringToNum(const std::string& stringIn, bool allowNeg = 1, int defaultVal = INT_MAX);
	template <typename numType>
	std::string numToHexStringWithPadding(numType numIn, unsigned char paddingLength)
	{
		static_assert(std::is_integral<numType>::value, "Type must be an integer primitve.");

		static hexConvStream conv;
		conv.buf.str("");
		conv.buf << std::setw(paddingLength) << numIn;
		return conv.buf.str();
	}
	template <typename numType>
	std::string numToDecStringWithPadding(numType numIn, unsigned char paddingLength)
	{
		static_assert(std::is_integral<numType>::value, "Template type must be a valid integer primitve.");

		static decConvStream conv;
		conv.buf.str("");
		conv.buf << std::setw(paddingLength) << numIn;
		return conv.buf.str();
	}
	std::string doubleToStringWithPadding(double dblIn, unsigned char paddingLength, unsigned long precisionIn = 2);
	std::string floatToStringWithPadding(float fltIn, unsigned char paddingLength, unsigned long precisionIn = 2);

	// File System Management
	bool copyFile(std::string sourceFile, std::string targetFile, bool overwriteExistingFile = 0);
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

	// EX Character Functions
	std::vector<std::pair<std::string, u16>> collectNameSlotIDPairs(std::string exCharInputFilePath, bool& fileOpened);

	// EX Roster Functions
	std::vector<std::pair<std::string, std::string>> collectedRosterNamePathPairs(std::string exRosterInputFilePath, bool& fileOpened);

	// EX Theme Functions
	std::vector<menuTheme> collectThemesFromXML(std::string exThemeInputFilePath, bool& fileOpened);
}

#endif