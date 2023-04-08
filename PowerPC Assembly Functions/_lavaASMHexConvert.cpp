#include "_lavaASMHexConvert.h"

namespace lava
{
	constexpr unsigned long overflowSecondaryOpcodeFlag = 0b1000000000;
	const std::string opName_WithOverflowString = " w/ Overflow";
	const std::string opName_WithUpdateString = " w/ Update";
	const std::string opName_WithComplString = " w/ Complement";
	const std::string opName_DoublePrecision = " (Double-Precision)";
	const std::string opName_SinglePrecision = " Single";

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
	std::string integerAndOrImmInstrToString(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 4)
		{
			if (instructionIn->mneumonic == "ori" && argumentsIn[1] == 0 && argumentsIn[2] == 0 && argumentsIn[3] == 0)
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

	std::string integerThreeRegABSwapWithRc(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mneumonic;
			if (argumentsIn[5])
			{
				result << '.';
			}
			result << " r" << argumentsIn[2];
			result << ", r" << argumentsIn[1];
			result << ", r" << argumentsIn[3];
		}

		return result.str();
	}
	std::string integerThreeRegWithRc(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mneumonic;
			if (argumentsIn[5])
			{
				result << '.';
			}
			result << " r" << argumentsIn[1];
			result << ", r" << argumentsIn[2];
			result << ", r" << argumentsIn[3];
		}

		return result.str();
	}

	std::string floatTwoRegOmitAWithRc(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 7)
		{
			result << instructionIn->mneumonic;
			if (argumentsIn[6])
			{
				result << '.';
			}
			result << " f" << argumentsIn[1];
			result << ", f" << argumentsIn[3];
		}

		return result.str();
	}
	std::string floatThreeRegOmitCWithRc(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 7)
		{
			result << instructionIn->mneumonic;
			if (argumentsIn[6])
			{
				result << '.';
			}
			result << " f" << argumentsIn[1];
			result << ", f" << argumentsIn[2];
			result << ", f" << argumentsIn[3];
		}

		return result.str();
	}
	std::string floatThreeRegOmitBWithRc(asmInstruction* instructionIn, std::vector<unsigned long> argumentsIn)
	{
		std::stringstream result;

		if (argumentsIn.size() >= 7)
		{
			result << instructionIn->mneumonic;
			if (argumentsIn[6])
			{
				result << '.';
			}
			result << " f" << argumentsIn[1];
			result << ", f" << argumentsIn[2];
			result << ", f" << argumentsIn[4];
		}

		return result.str();
	}



	std::vector<unsigned long> asmPrOpCodeGroup::splitHexIntoArguments(unsigned long instructionHexIn)
	{
		std::vector<unsigned long> result{};

		unsigned long instructionLength = 0;
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
			result->canonForm = result->primaryOpCode << (32 - 6);
			if (this->secondaryOpCodeArgumentIndex != UCHAR_MAX)
			{
				int secOpShiftAmount = 0;
				if ((this->secondaryOpCodeArgumentIndex + 1) < this->argumentStartBits.size())
				{
					secOpShiftAmount = 32 - this->argumentStartBits[this->secondaryOpCodeArgumentIndex + 1];
				}

				result->canonForm |= result->secondaryOpCode << secOpShiftAmount;
			}
		}

		return result;
	}
	asmInstruction* asmPrOpCodeGroup::pushOverflowVersionOfInstruction(asmInstruction* originalInstrIn)
	{
		asmInstruction* result = nullptr;

		if (originalInstrIn != nullptr)
		{
			result = pushInstruction(originalInstrIn->name + opName_WithOverflowString, originalInstrIn->secondaryOpCode | overflowSecondaryOpcodeFlag);
			if (result != nullptr)
			{
				result->mneumonic = originalInstrIn->mneumonic + "o";
				result->convertInstructionArgumentsToString = integerThreeRegWithRc;
			}
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

		// Arithmetic Instructions

		// Op Code 7
		currentOpGroup = pushOpCodeGroupToDict(aPOC_MULLI, { 0, 6, 11, 16 });
		{
			// Operation: MULLI
			currentInstruction = currentOpGroup->pushInstruction("Multiply Low Immediate", USHRT_MAX);
			currentInstruction->mneumonic = "mulli";
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}

		// Op Code 8
		currentOpGroup = pushOpCodeGroupToDict(aPOC_SUBFIC, { 0, 6, 11, 16 });
		{
			// Operation: SUBFIC
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Immediate Carrying", USHRT_MAX);
			currentInstruction->mneumonic = "subfic";
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}

		// Op Code 12
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDIC, { 0, 6, 11, 16 });
		{
			// Operation: ADDIC
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Carrying", USHRT_MAX);
			currentInstruction->mneumonic = "addic";
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}
		// Op Code 13
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDIC_DOT, { 0, 6, 11, 16 });
		{
			// Operation: ADDIC.
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Carrying and Record", USHRT_MAX);
			currentInstruction->mneumonic = "addic.";
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}
		// Op Code 14
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDI, { 0, 6, 11, 16});
		{
			// Operation: ADDI
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate", USHRT_MAX);
			currentInstruction->mneumonic = "addi";
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}
		// Op Code 15
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDIS, { 0, 6, 11, 16 });
		{
			// Operation: ADDI
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Shifted", USHRT_MAX);
			currentInstruction->mneumonic = "addis";
			currentInstruction->convertInstructionArgumentsToString = integerAddSubImmInstrToString;
		}

		// Op Code 63
		currentOpGroup = pushOpCodeGroupToDict(aPOC_FLOAT_D_ARTH, { 0, 6, 11, 16, 21, 26, 31 }, 5);
		{
			// Operation: FADD
			currentInstruction = currentOpGroup->pushInstruction("Floating Add" + opName_DoublePrecision, 21);
			currentInstruction->mneumonic = "fadd";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitCWithRc;
			// Operation: FDIV
			currentInstruction = currentOpGroup->pushInstruction("Floating Divide" + opName_DoublePrecision, 18);
			currentInstruction->mneumonic = "fdiv";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitCWithRc;
			// Operation: FMUL
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply" + opName_DoublePrecision, 25);
			currentInstruction->mneumonic = "fmul";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitBWithRc;
			// Operation: FSUB
			currentInstruction = currentOpGroup->pushInstruction("Floating Subtract" + opName_DoublePrecision, 20);
			currentInstruction->mneumonic = "fsub";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitCWithRc;

			// Operation: FCTIW
			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word", 14);
			currentInstruction->mneumonic = "fctiw";
			currentInstruction->convertInstructionArgumentsToString = floatTwoRegOmitAWithRc;
			// Operation: FCTIWZ
			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word with Round toward Zero", 15);
			currentInstruction->mneumonic = "fctiwz";
			currentInstruction->convertInstructionArgumentsToString = floatTwoRegOmitAWithRc;
			// Operation: FRSP
			currentInstruction = currentOpGroup->pushInstruction("Floating Round to Single", 12);
			currentInstruction->mneumonic = "frsp";
			currentInstruction->convertInstructionArgumentsToString = floatTwoRegOmitAWithRc;
		}
		// Op Code 59
		currentOpGroup = pushOpCodeGroupToDict(aPOC_FLOAT_S_ARTH, { 0, 6, 11, 16, 21, 26, 31 }, 5);
		{
			// Operation: FADDS
			currentInstruction = currentOpGroup->pushInstruction("Floating Add" + opName_SinglePrecision, 21);
			currentInstruction->mneumonic = "fadds";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitCWithRc;
			// Operation: FDIVS
			currentInstruction = currentOpGroup->pushInstruction("Floating Divide" + opName_SinglePrecision, 18);
			currentInstruction->mneumonic = "fdivs";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitCWithRc;
			// Operation: FMULS
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply" + opName_SinglePrecision, 25);
			currentInstruction->mneumonic = "fmuls";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitBWithRc;
			// Operation: FSUBS
			currentInstruction = currentOpGroup->pushInstruction("Floating Subtract" + opName_SinglePrecision, 20);
			currentInstruction->mneumonic = "fsubs";
			currentInstruction->convertInstructionArgumentsToString = floatThreeRegOmitCWithRc;
		}

		// Op Code 31
		currentOpGroup = pushOpCodeGroupToDict(aPOC_31, { 0, 6, 11, 16, 21, 31 }, 4);
		{
			// Operation: ADD
			currentInstruction = currentOpGroup->pushInstruction("Add", 266);
			currentInstruction->mneumonic = "add";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: ADDO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDC
			currentInstruction = currentOpGroup->pushInstruction("Add Carrying", 10);
			currentInstruction->mneumonic = "addc";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: ADDCO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDE
			currentInstruction = currentOpGroup->pushInstruction("Add Extended", 138);
			currentInstruction->mneumonic = "adde";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: ADDEO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: DIVW
			currentInstruction = currentOpGroup->pushInstruction("Divide Word", 491);
			currentInstruction->mneumonic = "divw";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: DIVWO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: DIVWU
			currentInstruction = currentOpGroup->pushInstruction("Divide Word Unsigned", 459);
			currentInstruction->mneumonic = "divwu";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: DIVWUO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: MULHW
			currentInstruction = currentOpGroup->pushInstruction("Multiply High Word", 75);
			currentInstruction->mneumonic = "mulhw";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: MULHWO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: MULHWU
			currentInstruction = currentOpGroup->pushInstruction("Multiply High Word Unsigned", 11);
			currentInstruction->mneumonic = "mulhwu";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: MULHWUO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: MULLW
			currentInstruction = currentOpGroup->pushInstruction("Multiply Low Word", 235);
			currentInstruction->mneumonic = "mullw";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: MULLWO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: SUBF
			currentInstruction = currentOpGroup->pushInstruction("Subtract From", 40);
			currentInstruction->mneumonic = "subf";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: SUBFO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: SUBFC
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Carrying", 8);
			currentInstruction->mneumonic = "subfc";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: SUBFCO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: SUBFE
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Extended", 136);
			currentInstruction->mneumonic = "subfe";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: SUBFEO
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: AND
			currentInstruction = currentOpGroup->pushInstruction("AND", 28);
			currentInstruction->mneumonic = "and";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: ANDC
			currentInstruction = currentOpGroup->pushInstruction("AND" + opName_WithComplString, 60);
			currentInstruction->mneumonic = "andc";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;

			// Operation: OR
			currentInstruction = currentOpGroup->pushInstruction("OR", 444);
			currentInstruction->mneumonic = "or";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;
			// Operation: ORC
			currentInstruction = currentOpGroup->pushInstruction("OR" + opName_WithComplString, 412);
			currentInstruction->mneumonic = "orc";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;

			// Operation: NOR
			currentInstruction = currentOpGroup->pushInstruction("NOR", 124);
			currentInstruction->mneumonic = "nor";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegWithRc;

			// Operation: SLW
			currentInstruction = currentOpGroup->pushInstruction("Shift Left Word", 24);
			currentInstruction->mneumonic = "slw";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegABSwapWithRc;
			// Operation: SRAW
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word", 792);
			currentInstruction->mneumonic = "sraw";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegABSwapWithRc;
			// Operation: SRAWI
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word Immediate", 824);
			currentInstruction->mneumonic = "srawi";
			currentInstruction->convertInstructionArgumentsToString = integerThreeRegABSwapWithRc;
		}

		// Load Instructions

		// Op Code 32
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LWZ, { 0, 6, 11, 16 });
		{
			// Operation: LWZ
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero", USHRT_MAX);
			currentInstruction->mneumonic = "lwz";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 33
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LWZU, { 0, 6, 11, 16 });
		{
			// Operation: LWZU
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lwzu";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 34
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LBZ, { 0, 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero", USHRT_MAX);
			currentInstruction->mneumonic = "lbz";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 35
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LBZU, { 0, 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lbzu";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 40
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LHZ, { 0, 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero", USHRT_MAX);
			currentInstruction->mneumonic = "lhz";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 41
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LHZU, { 0, 6, 11, 16 });
		{
			// Operation: LBZ
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lhzu";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 48
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFS, { 0, 6, 11, 16 });
		{
			// Operation: LFS
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single", USHRT_MAX);
			currentInstruction->mneumonic = "lfs";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 49
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFSU, { 0, 6, 11, 16 });
		{
			// Operation: LFSU
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lfsu";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 50
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFD, { 0, 6, 11, 16 });
		{
			// Operation: LFD
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double", USHRT_MAX);
			currentInstruction->mneumonic = "lfd";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 51
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFDU, { 0, 6, 11, 16 });
		{
			// Operation: LFDU
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "lfdu";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}



		// Store Instructions

		// Op Code 36
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STW, { 0, 6, 11, 16 });
		{
			// Operation: STW
			currentInstruction = currentOpGroup->pushInstruction("Store Word", USHRT_MAX);
			currentInstruction->mneumonic = "stw";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 37
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STWU, { 0, 6, 11, 16 });
		{
			// Operation: STWU
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stwu";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 38
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STB, { 0, 6, 11, 16 });
		{
			// Operation: STB
			currentInstruction = currentOpGroup->pushInstruction("Store Byte", USHRT_MAX);
			currentInstruction->mneumonic = "stb";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 39
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STBU, { 0, 6, 11, 16 });
		{
			// Operation: STBU
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stbu";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 44
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STH, { 0, 6, 11, 16 });
		{
			// Operation: STH
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word", USHRT_MAX);
			currentInstruction->mneumonic = "sth";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 45
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STHU, { 0, 6, 11, 16 });
		{
			// Operation: STHU
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "sthu";
			currentInstruction->convertInstructionArgumentsToString = integerLoadStoreInstrToString;
		}
		// Op Code 52
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFS, { 0, 6, 11, 16 });
		{
			// Operation: STFS
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single", USHRT_MAX);
			currentInstruction->mneumonic = "stfs";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 53
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFSU, { 0, 6, 11, 16 });
		{
			// Operation: STFSU
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stfsu";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 54
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFD, { 0, 6, 11, 16 });
		{
			// Operation: STFD
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double", USHRT_MAX);
			currentInstruction->mneumonic = "stfd";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}
		// Op Code 55
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFDU, { 0, 6, 11, 16 });
		{
			// Operation: STFDU
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "stfdu";
			currentInstruction->convertInstructionArgumentsToString = floatLoadStoreInstrToString;
		}

		// Logical Integer Instructions

		// Op Code 24
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ORI, { 0, 6, 11, 16});
		{
			// Operation: ORI
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "ori";
			currentInstruction->convertInstructionArgumentsToString = integerAndOrImmInstrToString;
		}
		// Op Code 25
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ORIS, { 0, 6, 11, 16 });
		{
			// Operation: ORI
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate Shifted" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "oris";
			currentInstruction->convertInstructionArgumentsToString = integerAndOrImmInstrToString;
		}
		// Op Code 28
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ANDI, { 0, 6, 11, 16 });
		{
			// Operation: ANDI
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "andi.";
			currentInstruction->convertInstructionArgumentsToString = integerAndOrImmInstrToString;
		}
		// Op Code 28
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ANDIS, { 0, 6, 11, 16 });
		{
			// Operation: ORI
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate Shifted" + opName_WithUpdateString, USHRT_MAX);
			currentInstruction->mneumonic = "andis.";
			currentInstruction->convertInstructionArgumentsToString = integerAndOrImmInstrToString;
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