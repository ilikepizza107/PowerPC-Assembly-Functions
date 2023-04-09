#include "_lavaASMHexConvert.h"

namespace lava
{
	constexpr unsigned long overflowSecondaryOpcodeFlag = 0b1000000000;
	const std::string opName_WithOverflowString = " w/ Overflow";
	const std::string opName_WithUpdateString = " w/ Update";
	const std::string opName_WithComplString = " w/ Complement";
	const std::string opName_DoublePrecision = " (Double-Precision)";
	const std::string opName_SinglePrecision = " Single";

	// Utility
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
	std::string parseBOAndBIToBranchMneum(unsigned char BOIn, unsigned char BIIn, std::string suffixIn)
	{
		std::stringstream result;
		
		char crField = BIIn >> 2;
		char crBit = BIIn & 0b1111;

		bool aBit = 0;
		bool tBit = 0;

		if (crField == 0)
		{
			if (BOIn & 0b00100)
			{
				// This is the always branch condition, mostly used for blr and bctr
				if (BOIn & 0b10000)
				{
					result << "b";
				}
				else
				{
					// This is the CR Bit True condition
					if (BOIn & 0b01000)
					{
						switch (crBit)
						{
						case 0:
						{
							result << "blt";
							break;
						}
						case 1:
						{
							result << "bgt";
							break;
						}
						case 2:
						{
							result << "beq";
							break;
						}
						case 3:
						{
							result << "bso";
						}
						default:
						{
							break;
						}
						}
					}
					// This is the CR Bit False condition
					else
					{
						switch (crBit)
						{
						case 0:
						{
							result << "bge";
							break;
						}
						case 1:
						{
							result << "ble";
							break;
						}
						case 2:
						{
							result << "bne";
							break;
						}
						case 3:
						{
							result << "bns";
							break;
						}
						default:
						{
							break;
						}
						}

					}
					aBit = BOIn & 0b10;
					tBit = BOIn & 0b01;
				}

				// If we've written to result / if a mneumonic was found.
				if (result.tellp() != 0)
				{
					result << suffixIn;
					if (aBit != 0)
					{
						if (tBit != 0)
						{
							result << "+";
						}
						else
						{
							result << "-";
						}
					}
				}
			}
		}

		return result.str();
	}

	// argumentLayout
	std::array<argumentLayout, aIAL_LAYOUT_COUNT> layoutDictionary{};
	std::vector<unsigned long> argumentLayout::splitHexIntoArguments(unsigned long instructionHexIn)
	{
		std::vector<unsigned long> result{};

		unsigned long argumentLength = ULONG_MAX;
		for (unsigned long i = 0; i < argumentStartBits.size(); i++)
		{
			if (i < (argumentStartBits.size() - 1))
			{
				argumentLength = argumentStartBits[i + 1] - argumentStartBits[i];
			}
			else
			{
				argumentLength = 32 - argumentStartBits[i];
			}
			result.push_back(extractInstructionArg(instructionHexIn, argumentStartBits[i], argumentLength));
		}

		return result;
	}
	argumentLayout* defineArgLayout(asmInstructionArgLayout IDIn, std::vector<unsigned char> argStartsIn,
		std::string(*convFuncIn)(asmInstruction*, unsigned long))
	{
		layoutDictionary[IDIn].layoutID = IDIn;
		layoutDictionary[IDIn].argumentStartBits = argStartsIn;
		layoutDictionary[IDIn].conversionFunc = convFuncIn;
		return &layoutDictionary[IDIn];
	}

	// Instruction to String Conversion Predicates
	std::string defaultAsmInstrToStrFunc(asmInstruction* instructionIn, unsigned long hexIn)
	{
		return instructionIn->mneumonic;
	}
	std::string bcConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			bool useAbsolute = 0;
			unsigned char BO = argumentsIn[1];
			unsigned char BI = argumentsIn[2];

			std::string simpleMneum = parseBOAndBIToBranchMneum(BO, BI, "");
			if (!simpleMneum.empty())
			{
				result << simpleMneum;
				if (argumentsIn[4] != 0)
				{
					result << "a";
				}
				if (argumentsIn[5] != 0)
				{
					result << "l";
				}
			}
			else
			{
				result << instructionIn->mneumonic;
				if (argumentsIn[5] != 0)
				{
					result << "l";
				}
				if (argumentsIn[4] != 0)
				{
					result << "a";
				}

				result << " " << (unsigned long)BO << ", " << (unsigned long)BI;
			}
			
			if (argumentsIn[4] != 0)
			{
				result << " 0x" << std::hex << (argumentsIn[3] << 2);
			}
			else
			{
				result << " " << unsignedImmArgToSignedString((argumentsIn[3] << 2), 14, 1);
			}
		}

		return result.str();
	}
	std::string bclrConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 7)
		{
			bool useAbsolute = 0;
			unsigned char BO = argumentsIn[1];
			unsigned char BI = argumentsIn[2];
			unsigned char BH = argumentsIn[4];

			std::string simpleMneum = (BH == 0) ? parseBOAndBIToBranchMneum(BO, BI, "lr") : "";
			if (!simpleMneum.empty())
			{
				result << simpleMneum;
			}
			else
			{
				result << instructionIn->mneumonic;
				if (argumentsIn[6] != 0)
				{
					result << "l";
				}

				result << " " << BO << ", " << BI << ", " << BH;
			}
		}

		return result.str();
	}
	std::string bcctrConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 7)
		{
			bool useAbsolute = 0;
			unsigned char BO = argumentsIn[1];
			unsigned char BI = argumentsIn[2];
			unsigned char BH = argumentsIn[4];

			std::string simpleMneum = (BH == 0) ? parseBOAndBIToBranchMneum(BO, BI, "ctr") : "";
			if (!simpleMneum.empty())
			{
				result << simpleMneum;
			}
			else
			{
				result << instructionIn->mneumonic;
				if (argumentsIn[6] != 0)
				{
					result << "l";
				}

				result << " " << BO << ", " << BI << ", " << BH;
			}
		}

		return result.str();
	}
	std::string bConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 4)
		{
			bool useAbsolute = 0;
			result << instructionIn->mneumonic;
			if (argumentsIn[3] != 0)
			{
				result << "l";
			}
			if (argumentsIn[2] != 0)
			{
				result << "a";
				useAbsolute = 1;
			}
			result << " ";
			if (useAbsolute)
			{
				result << "0x" << std::hex << (argumentsIn[1] << 2);
			}
			else
			{
				result << unsignedImmArgToSignedString((argumentsIn[1] << 2), 24, 1);
			}
		}

		return result.str();
	}
	std::string cmpwConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 8)
		{
			result << instructionIn->mneumonic << " ";
			if (argumentsIn[1] != 0)
			{
				result << "cr" << argumentsIn[1] << ", ";
			}
			result << "r" << argumentsIn[4];
			result << ", r" << argumentsIn[5];
		}

		return result.str();
	}
	std::string cmpwiConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mneumonic << " ";
			if (argumentsIn[1] != 0)
			{
				result << "cr" << argumentsIn[1] << ", ";
			}
			result << "r" << argumentsIn[4];
			result << ", " << unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
		}

		return result.str();
	}
	std::string cmplwiConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mneumonic << " ";
			if (argumentsIn[1] != 0)
			{
				result << "cr" << argumentsIn[1] << ", ";
			}
			result << "r" << argumentsIn[4];
			result << ", 0x" << std::hex << argumentsIn[5];
		}

		return result.str();
	}
	std::string integerAddImmConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
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
				std::string immediateString = unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
				std::size_t minusLoc = immediateString.find('-');
				if (minusLoc != std::string::npos)
				{
					if (instructionIn->mneumonic.back() == 's')
					{
						result << "subis";
					}
					else
					{
						result << "subi";
					}
					immediateString.erase(minusLoc, 1);
				}
				else
				{
					result << instructionIn->mneumonic;
				}
				result << " r" << argumentsIn[1];
				result << ", r" << argumentsIn[2] << ", ";
				result << immediateString;
			}
		}

		return result.str();
	}
	std::string integerORImmConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
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
	std::string integerLoadStoreConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 4)
		{
			result << instructionIn->mneumonic;
			result << " r" << argumentsIn[1] << ", ";
			result << unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
			result << "(r" << argumentsIn[2] << ")";
		}

		return result.str();
	}
	std::string integer2RegWithSIMMConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 4)
		{
			result << instructionIn->mneumonic;
			result << " r" << argumentsIn[1];
			result << ", r" << argumentsIn[2] << ", ";
			result << unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
		}

		return result.str();
	}
	std::string integer2RegWithUIMMConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 4)
		{
			result << instructionIn->mneumonic;
			result << " r" << argumentsIn[1];
			result << ", r" << argumentsIn[2] << ", ";
			result << std::hex << "0x" << argumentsIn[3];
		}

		return result.str();
	}
	std::string integer2RegWithRc(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mneumonic;
			if (argumentsIn[5])
			{
				result << '.';
			}
			result << " r" << argumentsIn[1];
			result << ", r" << argumentsIn[2];
		}

		return result.str();
	}
	std::string integer3RegWithRc(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
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
	std::string integer3RegSASwapWithRc(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
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
	std::string integer2RegSASwapWithSHAndRc(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mneumonic;
			if (argumentsIn[5])
			{
				result << '.';
			}
			result << " r" << argumentsIn[2];
			result << ", r" << argumentsIn[1];
			result << ", " << argumentsIn[3];
		}

		return result.str();
	}
	std::string floatLoadStoreConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 4)
		{
			result << instructionIn->mneumonic;
			result << " f" << argumentsIn[1] << ", ";
			result << unsignedImmArgToSignedString(argumentsIn[3], 16, 1);
			result << "(r" << argumentsIn[2] << ")";
		}

		return result.str();
	}
	std::string float2RegOmitAWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
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
	std::string float3RegOmitCWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
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
	std::string float3RegOmitBWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
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

	// asmInstruction
	argumentLayout* asmInstruction::getArgLayoutPtr()
	{
		argumentLayout* result = nullptr;

		if (layoutID != aIAL_NULL)
		{
			result = &layoutDictionary[layoutID];
		}

		return result;
	}
	bool asmInstruction::isRightInstruction(unsigned long hexIn)
	{
		return (hexIn & canonForm) == hexIn;
	}

	// asmPrOpCodeGroup
	asmInstruction* asmPrOpCodeGroup::pushInstruction(std::string nameIn, std::string mneumIn, asmInstructionArgLayout layoutIDIn, unsigned short secOpIn)
	{
		asmInstruction* result = nullptr;

		if (secondaryOpCodeToInstructions.find(secOpIn) == secondaryOpCodeToInstructions.end())
		{
			result = &secondaryOpCodeToInstructions[secOpIn];
			result->primaryOpCode = this->primaryOpCode;
			result->name = nameIn;
			result->mneumonic = mneumIn;
			result->secondaryOpCode = secOpIn;
			result->canonForm = result->primaryOpCode << (32 - 6);
			result->layoutID = layoutIDIn;

			argumentLayout* layoutPtr = result->getArgLayoutPtr();

			if (secondaryOpCodeStartBit != UCHAR_MAX && secondaryOpCodeLength != UCHAR_MAX)
			{
				int secOpShiftAmount = 32 - (secondaryOpCodeStartBit + secondaryOpCodeLength);
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
			result = pushInstruction(originalInstrIn->name + opName_WithOverflowString, originalInstrIn->mneumonic + "o",
				originalInstrIn->layoutID, originalInstrIn->secondaryOpCode | overflowSecondaryOpcodeFlag);
		}

		return result;
	}

	// instructionDictionary
	std::map<unsigned short, asmPrOpCodeGroup> instructionDictionary{};
	asmPrOpCodeGroup* pushOpCodeGroupToDict(asmPrimaryOpCodes opCodeIn, unsigned char secOpCodeStart, unsigned char secOpCodeLength)
	{
		asmPrOpCodeGroup* result = nullptr;

		if (instructionDictionary.find(opCodeIn) == instructionDictionary.end())
		{
			result = &instructionDictionary[opCodeIn];
			result->primaryOpCode = opCodeIn;
			result->secondaryOpCodeStartBit = secOpCodeStart;
			result->secondaryOpCodeLength = secOpCodeLength;
		}

		return result;
	}
	void buildInstructionDictionary()
	{
		asmPrOpCodeGroup* currentOpGroup = nullptr;
		asmInstruction* currentInstruction = nullptr;

		// Setup Instruction Argument Layouts
		defineArgLayout(aIAL_B, { 0, 6, 30, 31 }, bConv);
		defineArgLayout(aIAL_BC, { 0, 6, 11, 16, 30, 31 }, bcConv);
		defineArgLayout(aIAL_BCLR, { 0, 6, 11, 16, 19, 21, 31 }, bclrConv);
		defineArgLayout(aIAL_BCCTR, { 0, 6, 11, 16, 19, 21, 31 }, bcctrConv);
		defineArgLayout(aIAL_CMPW, { 0, 6, 9, 10, 11, 16, 21, 30 }, cmpwConv);
		defineArgLayout(aIAL_CMPWI, { 0, 6, 9, 10, 11, 16 }, cmpwiConv);
		defineArgLayout(aIAL_CMPLWI, { 0, 6, 9, 10, 11, 16 }, cmplwiConv);
		defineArgLayout(aIAL_IntADDI, { 0, 6, 11, 16 }, integerAddImmConv);
		defineArgLayout(aIAL_IntORI, { 0, 6, 11, 16 }, integerORImmConv);
		defineArgLayout(aIAL_IntLogical, { 0, 6, 11, 16 }, integer2RegWithUIMMConv);
		defineArgLayout(aIAL_IntLoadStore, { 0, 6, 11, 16 }, integerLoadStoreConv);
		defineArgLayout(aIAL_Int2RegWithSIMM, { 0, 6, 11, 16 }, integer2RegWithSIMMConv);
		defineArgLayout(aIAL_Int2RegWithUIMM, { 0, 6, 11, 16 }, integer2RegWithUIMMConv);
		defineArgLayout(aIAL_Int2RegWithRC, {0, 6, 11, 16, 21, 31}, integer2RegWithRc);
		defineArgLayout(aIAL_Int3RegWithRC, { 0, 6, 11, 16, 21, 31 }, integer3RegWithRc);
		defineArgLayout(aIAL_Int3RegSASwapWithRC, { 0, 6, 11, 16, 21, 31 }, integer3RegSASwapWithRc);
		defineArgLayout(aIAL_Int2RegSASwapWithSHAndRC, { 0, 6, 11, 16, 21, 31 }, integer2RegSASwapWithSHAndRc);
		defineArgLayout(aIAL_FltLoadStore, { 0, 6, 11, 16 }, floatLoadStoreConv);
		defineArgLayout(aIAL_Flt2RegOmitAWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float2RegOmitAWithRcConv);
		defineArgLayout(aIAL_Flt3RegOmitBWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float3RegOmitBWithRcConv);
		defineArgLayout(aIAL_Flt3RegOmitCWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float3RegOmitCWithRcConv);

		// Branch Instructions
		currentOpGroup = pushOpCodeGroupToDict(aPOC_BC);
		{
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional", "bc", aIAL_BC);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_B);
		{
			currentInstruction = currentOpGroup->pushInstruction("Branch", "b", aIAL_B);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_B_SpReg, 21, 10);
		{
			// Operation:: BCCTR
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional to Count Register ", "bcctr", aIAL_BCCTR, 528);
			// Operation:: BCLR
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional to Link Register ", "bclr", aIAL_BCLR, 16);
		}

		// Compare Instructions
		currentOpGroup = pushOpCodeGroupToDict(aPOC_CMPWI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Compare Word Immediate", "cmpwi", aIAL_CMPWI);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_CMPLWI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Compare Logical Word Immediate", "cmplwi", aIAL_CMPLWI);
		}

		// Integer Arithmetic Instructions
		currentOpGroup = pushOpCodeGroupToDict(aPOC_MULLI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Multiply Low Immediate", "mulli", aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_SUBFIC);
		{
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Immediate Carrying", "subfic", aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDIC);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Carrying", "addic", aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDIC_DOT);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Carrying and Record", "addic.", aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate", "addi", aIAL_IntADDI);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ADDIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Shifted", "addis", aIAL_IntADDI);
		}


		// Float Arithmetic Instructions
		currentOpGroup = pushOpCodeGroupToDict(aPOC_FLOAT_D_ARTH, 26, 5);
		{
			currentInstruction = currentOpGroup->pushInstruction("Floating Add" + opName_DoublePrecision, "fadd", aIAL_Flt3RegOmitCWithRC, 21);
			currentInstruction = currentOpGroup->pushInstruction("Floating Divide" + opName_DoublePrecision, "fdiv", aIAL_Flt3RegOmitCWithRC, 18);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply" + opName_DoublePrecision, "fmul", aIAL_Flt3RegOmitBWithRC, 25);
			currentInstruction = currentOpGroup->pushInstruction("Floating Subtract" + opName_DoublePrecision, "fsub", aIAL_Flt3RegOmitCWithRC, 20);

			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word", "fctiw", aIAL_Flt2RegOmitAWithRC, 14);
			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word with Round toward Zero", "fctiwz", aIAL_Flt2RegOmitAWithRC, 15);
			currentInstruction = currentOpGroup->pushInstruction("Floating Round to Single", "frsp", aIAL_Flt2RegOmitAWithRC, 12);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_FLOAT_S_ARTH, 26, 5);
		{
			currentInstruction = currentOpGroup->pushInstruction("Floating Add" + opName_SinglePrecision, "fadds", aIAL_Flt3RegOmitCWithRC, 21);
			currentInstruction = currentOpGroup->pushInstruction("Floating Divide" + opName_SinglePrecision, "fdivs", aIAL_Flt3RegOmitCWithRC, 18);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply" + opName_SinglePrecision, "fmuls", aIAL_Flt3RegOmitBWithRC, 25);
			currentInstruction = currentOpGroup->pushInstruction("Floating Subtract" + opName_SinglePrecision, "fsubs", aIAL_Flt3RegOmitCWithRC, 20);
		}


		// Load Instructions
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LWZ);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero", "lwz", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LWZU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_WithUpdateString, "lwzu", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LBZ);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero", "lbz", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LBZU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero" + opName_WithUpdateString, "lbzu", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LHZ);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero", "lhz", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LHZU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero" + opName_WithUpdateString, "lhzu", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFS);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single", "lfs", aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFSU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single" + opName_WithUpdateString, "lfsu", aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFD);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double", "lfd", aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_LFDU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double" + opName_WithUpdateString, "lfdu", aIAL_FltLoadStore);
		}
		

		// Store Instructions
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STW);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Word", "stw", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STWU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_WithUpdateString, "stwu", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STB);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Byte", "stb", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STBU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_WithUpdateString, "stbu", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STH);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word", "sth", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STHU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_WithUpdateString, "sthu", aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFS);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single", "stfs", aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFSU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Floating-Point Single" + opName_WithUpdateString, "stfsu", aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFD);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double", "stfd", aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_STFDU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Floating-Point Double" + opName_WithUpdateString, "stfdu", aIAL_FltLoadStore);
		}

		
		// Logical Integer Instructions
			
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ORI);
		{
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate", "ori", aIAL_IntORI);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ORIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate Shifted", "oris", aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_XORI);
		{
			currentInstruction = currentOpGroup->pushInstruction("XOR Immediate", "xori", aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_XORIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("XOR Immediate Shifted", "xoris", aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ANDI);
		{
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate" + opName_WithUpdateString, "andi.", aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(aPOC_ANDIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate Shifted" + opName_WithUpdateString, "andis.", aIAL_Int2RegWithUIMM);
		}

		// Op Code 31
		currentOpGroup = pushOpCodeGroupToDict(aPOC_31, 21, 10);
		{
			// Operation: ADD, ADDO
			currentInstruction = currentOpGroup->pushInstruction("Add", "add", aIAL_Int3RegWithRC, 266);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDC, ADDCO
			currentInstruction = currentOpGroup->pushInstruction("Add Carrying", "addc", aIAL_Int3RegWithRC, 10);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDE
			currentInstruction = currentOpGroup->pushInstruction("Add Extended", "adde", aIAL_Int3RegWithRC, 138);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: DIVW, DIVWO
			currentInstruction = currentOpGroup->pushInstruction("Divide Word", "divw", aIAL_Int3RegWithRC, 491);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: DIVWU, DIVWUO
			currentInstruction = currentOpGroup->pushInstruction("Divide Word Unsigned", "divwu", aIAL_Int3RegWithRC, 459);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: MULHW, MULHWO
			currentInstruction = currentOpGroup->pushInstruction("Multiply High Word", "mulhw", aIAL_Int3RegWithRC, 75);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: MULHWU, MULHWUO
			currentInstruction = currentOpGroup->pushInstruction("Multiply High Word Unsigned", "mulhwu", aIAL_Int3RegWithRC, 11);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: MULLW, MULLWO
			currentInstruction = currentOpGroup->pushInstruction("Multiply Low Word", "mullw", aIAL_Int3RegWithRC, 235);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: SUBF, SUBFO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From", "subf", aIAL_Int3RegWithRC, 40);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: SUBFC, SUBFCO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Carrying", "subfc", aIAL_Int3RegWithRC, 8);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: SUBFE, SUBFEO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Extended", "subfe", aIAL_Int3RegWithRC, 136);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: NEG, NEGO
			currentInstruction = currentOpGroup->pushInstruction("Negate", "neg", aIAL_Int2RegWithRC, 104);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: AND
			currentInstruction = currentOpGroup->pushInstruction("AND", "and", aIAL_Int3RegSASwapWithRC, 28);
			// Operation: ANDC
			currentInstruction = currentOpGroup->pushInstruction("AND" + opName_WithComplString, "andc", aIAL_Int3RegSASwapWithRC, 60);

			// Operation: OR
			currentInstruction = currentOpGroup->pushInstruction("OR", "or", aIAL_Int3RegSASwapWithRC, 444);
			// Operation: ORC
			currentInstruction = currentOpGroup->pushInstruction("OR" + opName_WithComplString, "orc", aIAL_Int3RegSASwapWithRC, 412);

			// Operation: EQV
			currentInstruction = currentOpGroup->pushInstruction("Equivalent", "eqv", aIAL_Int3RegSASwapWithRC, 284);

			// Operation: NOR
			currentInstruction = currentOpGroup->pushInstruction("NOR", "nor", aIAL_Int3RegSASwapWithRC, 124);

			// Operation: XOR
			currentInstruction = currentOpGroup->pushInstruction("XOR", "xor", aIAL_Int3RegSASwapWithRC, 316);

			// Operation: SLW
			currentInstruction = currentOpGroup->pushInstruction("Shift Left Word", "slw", aIAL_Int3RegSASwapWithRC, 24);
			// Operation: SRAW
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word", "sraw", aIAL_Int3RegSASwapWithRC, 792);
			// Operation: SRAWI
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word Immediate", "srawi", aIAL_Int2RegSASwapWithSHAndRC, 824);

			// Operation: CMPW
			currentInstruction = currentOpGroup->pushInstruction("Compare Word", "cmpw", aIAL_CMPW, 0);
			// Operation: CMPLW
			currentInstruction = currentOpGroup->pushInstruction("Compare Word Logical", "cmplw", aIAL_CMPW, 32);
		}
	}

	std::string convertOperationHexToString(unsigned long hexIn)
	{
		std::stringstream result;

		unsigned long opCode = extractInstructionArg(hexIn, 0, 6);
		if (instructionDictionary.find(opCode) != instructionDictionary.end())
		{
			asmPrOpCodeGroup* opCodeGroup = &instructionDictionary[opCode];
			unsigned short secondaryOpCode = (unsigned short)extractInstructionArg(hexIn, opCodeGroup->secondaryOpCodeStartBit, opCodeGroup->secondaryOpCodeLength);

			if (opCodeGroup->secondaryOpCodeToInstructions.find(secondaryOpCode) != opCodeGroup->secondaryOpCodeToInstructions.end())
			{
				asmInstruction* targetInstruction = &opCodeGroup->secondaryOpCodeToInstructions[secondaryOpCode];
				result << targetInstruction->getArgLayoutPtr()->conversionFunc(targetInstruction, hexIn);
			}
		}

		return result.str();
	}
}