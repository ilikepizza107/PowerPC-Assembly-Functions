#include "_lavaASMHexConvert.h"

namespace lava
{
	const std::string opName_WithUpdateString = " w/ Update";

	unsigned long extractInstructionArg(unsigned long hexIn, unsigned char startBitIndex, unsigned char length)
	{
		unsigned long result = ULONG_MAX;

		if (startBitIndex < 32)
		{
			if ((startBitIndex + length) > 32)
			{
				length = 32 - startBitIndex;
			}

			result = hexIn & ((unsigned long long(2) << (31 - startBitIndex)) - 1);
			result = result >> ((32 - startBitIndex) - length);
		}

		return result;
	}
	unsigned long getInstructionOpCode(unsigned long hexIn)
	{
		return extractInstructionArg(hexIn, 0, 6);
	}

	std::string unsignedImmArgToSignedString(unsigned long argIn, unsigned char argLengthInBitsIn, bool hexMode = 1)
	{
		std::stringstream result;

		unsigned long adjustedArg = argIn;

		unsigned long signMask = 1 << (argLengthInBitsIn - 1);
		bool signBit = argIn & signMask;
		if (signBit)
		{
			result << "-";
			adjustedArg = (~argIn & (signMask - 1)) + 1;
		}
		if (hexMode)
		{
			result << std::hex << "0x";
		}
		result << adjustedArg;

		return result.str();
	}

	// Instruction to String Functions
	std::string defaultAsmInstrToStrFunc(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		return instructionIn->mneumonic;
	}
	std::string integerLoadStoreInstrToString(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 4)
		{
			result << instructionIn->mneumonic;
			result << " r" << argumentsIn[1] << ", ";
			result << unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
			result << "(r" << argumentsIn[2] << ")";
		}

		return result.str();
	}
	std::string floatLoadStoreInstrToString(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 4)
		{
			result << instructionIn->mneumonic;
			result << " f" << argumentsIn[1] << ", ";
			result << unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
			result << "(r" << argumentsIn[2] << ")";
		}

		return result.str();
	}

	std::string integerAddSubImmInstrToString(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 4)
		{
			// Activate "li"/"lis" mneumonic if rA == 0
			if (argumentsIn[2] == 0)
			{
				if (instructionIn->mneumonic.back() == 's')
				{
					result << "lis";
				}
				else
				{
					result << "li";
				}
				result << " r" << argumentsIn[1];
				result << ", " << std::hex << "0x" << argumentsIn[3];
			}
			else
			{
				result << instructionIn->mneumonic;
				result << " r" << argumentsIn[1];
				result << ", r" << argumentsIn[2] << ", ";
				result << unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
			}
		}

		return result.str();
	}
	std::string ORImmInstrToString(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 4)
		{
			if (argumentsIn[1] == 0 && argumentsIn[2] == 0 && argumentsIn[3] == 0)
			{
				result << "nop";
			}
			else
			{
				result << instructionIn->mneumonic;
				result << " r" << argumentsIn[1];
				result << ", r" << argumentsIn[2];
				result << ", " << std::hex << "0x" << argumentsIn[3];
			}
		}

		return result.str();
	}


	std::vector<unsigned long> asmPrOpCodeGroup::splitHexIntoArguments(unsigned long instructionHexIn)
	{
		std::vector<unsigned long> result{};

		unsigned long instructionLength = 0;
		result.push_back(getInstructionOpCode(instructionHexIn));
		for (unsigned long i = 0; i < argumentStartBits.size(); i++)
		{
			if (i < (argumentStartBits.size() - 1))
			{
				instructionLength = argumentStartBits[i + 1] - argumentStartBits[i];
			}
			else
			{
				instructionLength = 32 - argumentStartBits[i];
			}
			result.push_back(extractInstructionArg(instructionHexIn, argumentStartBits[i], instructionLength));
		}

		return result;
	}
	asmInstruction* asmPrOpCodeGroup::pushInstruction(std::string nameIn, unsigned short secOpIn)
	{
		asmInstruction* result = nullptr;

		if (secondaryOpCodeToInstructions.find(secOpIn) == secondaryOpCodeToInstructions.end())
		{
			result = &secondaryOpCodeToInstructions[secOpIn];
			result->primaryOpCode = this->primaryOpCode;
			result->name = nameIn;
			result->secondaryOpCode = secOpIn;
		}

		return result;
	}

	std::map<unsigned short, asmPrOpCodeGroup> instructionDictionary{};
	asmPrOpCodeGroup* pushOpCodeGroupToDict(asmPrimaryOpCodes opCodeIn, std::vector<unsigned char> argStartsIn, unsigned char secOpCodeArgIndex)
	{
		asmPrOpCodeGroup* result = nullptr;

		if (instructionDictionary.find(opCodeIn) == instructionDictionary.end())
		{
			result = &instructionDictionary[opCodeIn];
			result->primaryOpCode = opCodeIn;
			result->argumentStartBits = argStartsIn;
			result->secondaryOpCodeArgumentIndex = secOpCodeArgIndex;
		}

		return result;
	}
	void buildInstructionDictionary()
	{
		asmPrOpCodeGroup* currentOpGroup = nullptr;
		asmInstruction* currentInstruction = nullptr;

		// Op Code 14
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDI, { 6, 11, 16});
		{
			// Operation: ADDI
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate", USHRT_MAX);
			currentInstruction->mneumonic = "addi";
			currentInstruction->canonForm = 0x38000000;
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}
		// Op Code 15
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDIS, { 6, 11, 16 });
		{
			// Operation: ADDI
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Shifted", USHRT_MAX);
			currentInstruction->mneumonic = "addis";
			currentInstruction->canonForm = 0x3C000000;
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}

		// Op Code 31
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADD, { 6, 11, 16, 21, 22, 31 }, 3);
		{
			// Operation: ADD
			currentInstruction = currentOpGroup->pushInstruction("Add", 266);
			currentInstruction->mneumonic = "add";
			currentInstruction->canonForm = 0x7C000214;
		}

		// Load Instructions
		// Op Code 32
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LWZ, { 6, 11, 16 });
		{
			// Operation: LWZ
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero", USHRT_MAX);
			currentInstruction->mneumonic = "lwz";
			currentInstruction->canonForm = 0x80000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 33
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LWZU, { 6, 11, 16 });
		{
			// Operation: LWZU
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lwzu";
			currentInstruction->canonForm = 0x84000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 34
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LBZ, { 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero", USHRT_MAX);
			currentInstruction->mneumonic = "lbz";
			currentInstruction->canonForm = 0x88000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 35
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LBZU, { 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lbzu";
			currentInstruction->canonForm = 0x8C000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 40
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LHZ, { 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero", USHRT_MAX);
			currentInstruction->mneumonic = "lhz";
			currentInstruction->canonForm = 0xA0000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 41
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LHZU, { 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lhzu";
			currentInstruction->canonForm = 0xA4000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 48
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFS, { 6, 11, 16 });
		{
			// Operation: LFS
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single", USHRT_MAX);
			currentInstruction->mneumonic = "lfs";
			currentInstruction->canonForm = 0xC0000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 49
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFSU, { 6, 11, 16 });
		{
			// Operation: LFSU
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lfsu";
			currentInstruction->canonForm = 0xC4000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 50
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFD, { 6, 11, 16 });
		{
			// Operation: LFD
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double", USHRT_MAX);
			currentInstruction->mneumonic = "lfd";
			currentInstruction->canonForm = 0xC8000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 51
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFDU, { 6, 11, 16 });
		{
			// Operation: LFDU
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lfdu";
			currentInstruction->canonForm = 0xCC000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}



		// Store Instructions
		// Op Code 36
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STW, { 6, 11, 16 });
		{
			// Operation: STW
			currentInstruction = currentOpGroup->pushInstruction("Store Word", USHRT_MAX);
			currentInstruction->mneumonic = "stw";
			currentInstruction->canonForm = 0x90000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 37
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STWU, { 6, 11, 16 });
		{
			// Operation: STWU
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stwu";
			currentInstruction->canonForm = 0x94000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 38
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STB, { 6, 11, 16 });
		{
			// Operation: STB
			currentInstruction = currentOpGroup->pushInstruction("Store Byte", USHRT_MAX);
			currentInstruction->mneumonic = "stb";
			currentInstruction->canonForm = 0x98000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 39
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STBU, { 6, 11, 16 });
		{
			// Operation: STBU
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stbu";
			currentInstruction->canonForm = 0x9C000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 44
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STH, { 6, 11, 16 });
		{
			// Operation: STH
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word", USHRT_MAX);
			currentInstruction->mneumonic = "sth";
			currentInstruction->canonForm = 0xB0000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 45
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STHU, { 6, 11, 16 });
		{
			// Operation: STHU
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "sthu";
			currentInstruction->canonForm = 0xB4000000;
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 52
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFS, { 6, 11, 16 });
		{
			// Operation: STFS
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single", USHRT_MAX);
			currentInstruction->mneumonic = "stfs";
			currentInstruction->canonForm = 0xD0000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 53
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFSU, { 6, 11, 16 });
		{
			// Operation: STFSU
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stfsu";
			currentInstruction->canonForm = 0xD4000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 54
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFD, { 6, 11, 16 });
		{
			// Operation: STFD
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double", USHRT_MAX);
			currentInstruction->mneumonic = "stfd";
			currentInstruction->canonForm = 0xD8000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 55
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFDU, { 6, 11, 16 });
		{
			// Operation: STFDU
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stfdu";
			currentInstruction->canonForm = 0xDC000000;
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}

		// Logical Integer Instructions
		// Op Code 24
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ORI, {6, 11, 16});
		{
			// Operation: ORI
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "ori";
			currentInstruction->canonForm = 0x60000000;
			currentInstruction->convertInstructionArgumentsToString = ORImmInstrToString;
		}
		// Op Code 25
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ORIS, { 6, 11, 16 });
		{
			// Operation: ORI
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate Shifted" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "oris";
			currentInstruction->canonForm = 0x64000000;
			currentInstruction->convertInstructionArgumentsToString = ORImmInstrToString;
		}
	}

	std::string convertOperationHexToString(unsigned long hexIn)
	{
		std::stringstream result;

		unsigned long opCode = extractInstructionArg(hexIn, 0, 6);
		if (instructionDictionary.find(opCode) != instructionDictionary.end())
		{
			asmPrOpCodeGroup* opCodeGroup = &instructionDictionary[opCode];
			std::vector<unsigned long> deconstructedArguments = opCodeGroup->splitHexIntoArguments(hexIn);
			
			unsigned short secondaryOpCode = USHRT_MAX;
			if (opCodeGroup->secondaryOpCodeArgumentIndex != UCHAR_MAX)
			{
				secondaryOpCode = deconstructedArguments[opCodeGroup->secondaryOpCodeArgumentIndex];
			}

			if (opCodeGroup->secondaryOpCodeToInstructions.find(secondaryOpCode) != opCodeGroup->secondaryOpCodeToInstructions.end())
			{
				asmInstruction* targetInstruction = &opCodeGroup->secondaryOpCodeToInstructions[secondaryOpCode];
				result << targetInstruction->convertInstructionArgumentsToString(targetInstruction, deconstructedArguments);
			}
		}

		return result.str();
	}
}