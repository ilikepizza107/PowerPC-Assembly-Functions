#ifndef LAVA_ASM_HEX_CONVERT_V1_H
#define LAVA_ASM_HEX_CONVERT_V1_H

#include <functional>
#include <string>
#include <sstream>
#include <vector>
#include <map>

namespace lava
{
	enum asmPrimaryOpCodes
	{
		aPOC_NULL = -1,
		aPOC_ADDI = 14,
		aPOC_ADDIS = 15,
		aPOC_31 = 31,
		// Load Instructions
		aPOC_LWZ = 32,
		aPOC_LWZU = 33,
		aPOC_LBZ = 34,
		aPOC_LBZU = 35,
		aPOC_LHZ = 40,
		aPOC_LHZU = 41,
		aPOC_LFS = 48,
		aPOC_LFSU = 49,
		aPOC_LFD = 50,
		aPOC_LFDU = 51,
		// Store Instructions
		aPOC_STW = 36,
		aPOC_STWU = 37,
		aPOC_STB = 38,
		aPOC_STBU = 39,
		aPOC_STH = 44,
		aPOC_STHU = 45,
		aPOC_STFS = 52,
		aPOC_STFSU = 53,
		aPOC_STFD = 54,
		aPOC_STFDU = 55,
		// Logical Integer Instructions
		aPOC_ORI = 24,
		aPOC_ORIS = 25,
		aPOC_ANDI = 28,
		aPOC_ANDIS = 29,
	};

	unsigned long extractInstructionArg(unsigned long hexIn, unsigned char startBitIndex, unsigned char length);
	unsigned long getInstructionOpCode(unsigned long hexIn);

	struct asmInstruction;

	// Default function, just returns mneumonic.
	std::string defaultAsmInstrToStrFunc(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn);

	struct asmInstruction
	{
		asmPrimaryOpCodes primaryOpCode = aPOC_NULL;
		std::string name = "";
		std::string mneumonic = "";
		unsigned long canonForm = ULONG_MAX;
		unsigned short secondaryOpCode = USHRT_MAX;

		asmInstruction() {};
		asmInstruction(asmPrimaryOpCodes prOpIn, std::string nameIn, std::string mneumIn, unsigned short secOpIn, unsigned long canonIn) :
			primaryOpCode(prOpIn), name(nameIn), mneumonic(mneumIn), secondaryOpCode(secOpIn), canonForm(canonIn) {};

		std::string(*convertInstructionArgumentsToString)(asmInstruction*, std::vector<unsigned long>) = defaultAsmInstrToStrFunc;
	};
	struct asmPrOpCodeGroup
	{
		asmPrimaryOpCodes primaryOpCode = aPOC_NULL;
		std::vector<unsigned char> argumentStartBits{};
		std::map<unsigned short, asmInstruction> secondaryOpCodeToInstructions{};
		unsigned char secondaryOpCodeArgumentIndex = UCHAR_MAX;

		asmPrOpCodeGroup() {};
		asmPrOpCodeGroup(std::vector<unsigned char> argStartsIn, std::map<unsigned short, asmInstruction> secOpToInsIn) :
			argumentStartBits(argStartsIn), secondaryOpCodeToInstructions(secOpToInsIn) {};

		std::vector<unsigned long> splitHexIntoArguments(unsigned long instructionHexIn);

		asmInstruction* pushInstruction(std::string nameIn, unsigned short secOpIn);
	};

	extern std::map<unsigned short, asmPrOpCodeGroup> instructionDictionary;
	asmPrOpCodeGroup* pushOpCodeGroupToDict(asmPrimaryOpCodes opCodeIn, std::vector<unsigned char> argStartsIn, unsigned char secOpCodeArgIndex = UCHAR_MAX);
	void buildInstructionDictionary();

	std::string convertOperationHexToString(unsigned long hexIn);
}

#endif