#include "_lavaASMHexConvert.h"

namespace lava
{
	constexpr unsigned long overflowSecondaryOpcodeFlag = 0b1000000000;
	const std::string opName_WithOverflowString = " w/ Overflow";
	const std::string opName_WithUpdateString = " w/ Update";
	const std::string opName_IndexedString = " Indexed";
	const std::string opName_WithUpdateIndexedString = opName_WithUpdateString + opName_IndexedString;
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
	std::string parseBOAndBIToBranchMnem(unsigned char BOIn, unsigned char BIIn, std::string suffixIn)
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

				// If we've written to result / if a mnemonic was found.
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
	std::array<argumentLayout, (int)asmInstructionArgLayout::aIAL_LAYOUT_COUNT> layoutDictionary{};
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
		argumentLayout* targetLayout = &layoutDictionary[(int)IDIn];
		targetLayout->layoutID = IDIn;
		targetLayout->argumentStartBits = argStartsIn;
		targetLayout->conversionFunc = convFuncIn;
		return targetLayout;
	}

	// Instruction to String Conversion Predicates
	std::string defaultAsmInstrToStrFunc(asmInstruction* instructionIn, unsigned long hexIn)
	{
		return instructionIn->mnemonic;
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

			std::string simpleMnem = parseBOAndBIToBranchMnem(BO, BI, "");
			if (!simpleMnem.empty())
			{
				result << simpleMnem;
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
				result << instructionIn->mnemonic;
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

			std::string simpleMnem = (BH == 0) ? parseBOAndBIToBranchMnem(BO, BI, "lr") : "";
			if (!simpleMnem.empty())
			{
				result << simpleMnem;
			}
			else
			{
				result << instructionIn->mnemonic;
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

			std::string simpleMnem = (BH == 0) ? parseBOAndBIToBranchMnem(BO, BI, "ctr") : "";
			if (!simpleMnem.empty())
			{
				result << simpleMnem;
			}
			else
			{
				result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic << " ";
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
			result << instructionIn->mnemonic << " ";
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
			result << instructionIn->mnemonic << " ";
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
			// Activate "li"/"lis" mnemonic if rA == 0
			if (argumentsIn[2] == 0)
			{
				if (instructionIn->mnemonic.back() == 's')
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
					if (instructionIn->mnemonic.back() == 's')
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
					result << instructionIn->mnemonic;
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
				result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			if (instructionIn->mnemonic == "or" && argumentsIn[1] == argumentsIn[3])
			{
				result << "mr r" << argumentsIn[2] << ", r" << argumentsIn[1];
			}
			else
			{
				result << instructionIn->mnemonic;
				if (argumentsIn[5])
				{
					result << '.';
				}
				result << " r" << argumentsIn[2];
				result << ", r" << argumentsIn[1];
				result << ", r" << argumentsIn[3];
			}
		}

		return result.str();
	}
	std::string integer2RegSASwapWithSHAndRc(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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
			result << instructionIn->mnemonic;
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

		if (layoutID != asmInstructionArgLayout::aIAL_NULL)
		{
			result = &layoutDictionary[(int)layoutID];
		}

		return result;
	}
	bool asmInstruction::isRightInstruction(unsigned long hexIn)
	{
		return (hexIn & canonForm) == hexIn;
	}

	// asmPrOpCodeGroup
	asmInstruction* asmPrOpCodeGroup::pushInstruction(std::string nameIn, std::string mnemIn, asmInstructionArgLayout layoutIDIn, unsigned short secOpIn)
	{
		asmInstruction* result = nullptr;

		if (secondaryOpCodeToInstructions.find(secOpIn) == secondaryOpCodeToInstructions.end())
		{
			result = &secondaryOpCodeToInstructions[secOpIn];
			result->primaryOpCode = this->primaryOpCode;
			result->name = nameIn;
			result->mnemonic = mnemIn;
			result->secondaryOpCode = secOpIn;
			result->canonForm = (int)result->primaryOpCode << (32 - 6);
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
			result = pushInstruction(originalInstrIn->name + opName_WithOverflowString, originalInstrIn->mnemonic + "o",
				originalInstrIn->layoutID, originalInstrIn->secondaryOpCode | overflowSecondaryOpcodeFlag);
		}

		return result;
	}

	// instructionDictionary
	std::map<unsigned short, asmPrOpCodeGroup> instructionDictionary{};
	asmPrOpCodeGroup* pushOpCodeGroupToDict(asmPrimaryOpCodes opCodeIn, unsigned char secOpCodeStart, unsigned char secOpCodeLength)
	{
		asmPrOpCodeGroup* result = nullptr;

		if (instructionDictionary.find((int)opCodeIn) == instructionDictionary.end())
		{
			result = &instructionDictionary[(int)opCodeIn];
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
		defineArgLayout(asmInstructionArgLayout::aIAL_B, { 0, 6, 30, 31 }, bConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_BC, { 0, 6, 11, 16, 30, 31 }, bcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_BCLR, { 0, 6, 11, 16, 19, 21, 31 }, bclrConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_BCCTR, { 0, 6, 11, 16, 19, 21, 31 }, bcctrConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_CMPW, { 0, 6, 9, 10, 11, 16, 21, 30 }, cmpwConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_CMPWI, { 0, 6, 9, 10, 11, 16 }, cmpwiConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_CMPLWI, { 0, 6, 9, 10, 11, 16 }, cmplwiConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_IntADDI, { 0, 6, 11, 16 }, integerAddImmConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_IntORI, { 0, 6, 11, 16 }, integerORImmConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_IntLogical, { 0, 6, 11, 16 }, integer2RegWithUIMMConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_IntLoadStore, { 0, 6, 11, 16 }, integerLoadStoreConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegWithSIMM, { 0, 6, 11, 16 }, integer2RegWithSIMMConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegWithUIMM, { 0, 6, 11, 16 }, integer2RegWithUIMMConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegWithRC, {0, 6, 11, 16, 21, 31}, integer2RegWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int3RegWithRC, { 0, 6, 11, 16, 21, 31 }, integer3RegWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, { 0, 6, 11, 16, 21, 31 }, integer3RegSASwapWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegSASwapWithSHAndRC, { 0, 6, 11, 16, 21, 31 }, integer2RegSASwapWithSHAndRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_FltLoadStore, { 0, 6, 11, 16 }, floatLoadStoreConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float2RegOmitAWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt3RegOmitBWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float3RegOmitBWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float3RegOmitCWithRcConv);

		// Branch Instructions
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_BC);
		{
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional", "bc", asmInstructionArgLayout::aIAL_BC);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_B);
		{
			currentInstruction = currentOpGroup->pushInstruction("Branch", "b", asmInstructionArgLayout::aIAL_B);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_B_SpReg, 21, 10);
		{
			// Operation:: BCCTR
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional to Count Register ", "bcctr", asmInstructionArgLayout::aIAL_BCCTR, 528);
			// Operation:: BCLR
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional to Link Register ", "bclr", asmInstructionArgLayout::aIAL_BCLR, 16);
		}

		// Compare Instructions
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_CMPWI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Compare Word Immediate", "cmpwi", asmInstructionArgLayout::aIAL_CMPWI);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_CMPLWI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Compare Logical Word Immediate", "cmplwi", asmInstructionArgLayout::aIAL_CMPLWI);
		}

		// Integer Arithmetic Instructions
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_MULLI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Multiply Low Immediate", "mulli", asmInstructionArgLayout::aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_SUBFIC);
		{
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Immediate Carrying", "subfic", asmInstructionArgLayout::aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ADDIC);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Carrying", "addic", asmInstructionArgLayout::aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ADDIC_DOT);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Carrying and Record", "addic.", asmInstructionArgLayout::aIAL_Int2RegWithSIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ADDI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate", "addi", asmInstructionArgLayout::aIAL_IntADDI);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ADDIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("Add Immediate Shifted", "addis", asmInstructionArgLayout::aIAL_IntADDI);
		}


		// Float Arithmetic Instructions
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_FLOAT_D_ARTH, 26, 5);
		{
			currentInstruction = currentOpGroup->pushInstruction("Floating Add" + opName_DoublePrecision, "fadd", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 21);
			currentInstruction = currentOpGroup->pushInstruction("Floating Divide" + opName_DoublePrecision, "fdiv", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 18);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply" + opName_DoublePrecision, "fmul", asmInstructionArgLayout::aIAL_Flt3RegOmitBWithRC, 25);
			currentInstruction = currentOpGroup->pushInstruction("Floating Subtract" + opName_DoublePrecision, "fsub", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 20);

			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word", "fctiw", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 14);
			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word with Round toward Zero", "fctiwz", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 15);
			currentInstruction = currentOpGroup->pushInstruction("Floating Round to Single", "frsp", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 12);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_FLOAT_S_ARTH, 26, 5);
		{
			currentInstruction = currentOpGroup->pushInstruction("Floating Add" + opName_SinglePrecision, "fadds", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 21);
			currentInstruction = currentOpGroup->pushInstruction("Floating Divide" + opName_SinglePrecision, "fdivs", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 18);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply" + opName_SinglePrecision, "fmuls", asmInstructionArgLayout::aIAL_Flt3RegOmitBWithRC, 25);
			currentInstruction = currentOpGroup->pushInstruction("Floating Subtract" + opName_SinglePrecision, "fsubs", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 20);
		}


		// Load Instructions
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LWZ);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero", "lwz", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LWZU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_WithUpdateString, "lwzu", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LBZ);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero", "lbz", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LBZU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero" + opName_WithUpdateString, "lbzu", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LHZ);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero", "lhz", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LHZU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero" + opName_WithUpdateString, "lhzu", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LHA);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word Algebraic", "lha", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LHAU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word Algebraic" + opName_WithUpdateString, "lhau", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LMW);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Multiple Word", "lmw", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LFS);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single", "lfs", asmInstructionArgLayout::aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LFSU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single" + opName_WithUpdateString, "lfsu", asmInstructionArgLayout::aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LFD);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double", "lfd", asmInstructionArgLayout::aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_LFDU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double" + opName_WithUpdateString, "lfdu", asmInstructionArgLayout::aIAL_FltLoadStore);
		}
		

		// Store Instructions
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STW);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Word", "stw", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STWU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_WithUpdateString, "stwu", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STB);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Byte", "stb", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STBU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_WithUpdateString, "stbu", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STH);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word", "sth", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STHU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_WithUpdateString, "sthu", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STMW);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Multiple Word", "lmw", asmInstructionArgLayout::aIAL_IntLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STFS);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single", "stfs", asmInstructionArgLayout::aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STFSU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Floating-Point Single" + opName_WithUpdateString, "stfsu", asmInstructionArgLayout::aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STFD);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double", "stfd", asmInstructionArgLayout::aIAL_FltLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_STFDU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Store Half Floating-Point Double" + opName_WithUpdateString, "stfdu", asmInstructionArgLayout::aIAL_FltLoadStore);
		}

		
		// Logical Integer Instructions
			
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ORI);
		{
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate", "ori", asmInstructionArgLayout::aIAL_IntORI);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ORIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate Shifted", "oris", asmInstructionArgLayout::aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_XORI);
		{
			currentInstruction = currentOpGroup->pushInstruction("XOR Immediate", "xori", asmInstructionArgLayout::aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_XORIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("XOR Immediate Shifted", "xoris", asmInstructionArgLayout::aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ANDI);
		{
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate" + opName_WithUpdateString, "andi.", asmInstructionArgLayout::aIAL_Int2RegWithUIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ANDIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate Shifted" + opName_WithUpdateString, "andis.", asmInstructionArgLayout::aIAL_Int2RegWithUIMM);
		}

		// Op Code 31
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_31, 21, 10);
		{
			// Operation: ADD, ADDO
			currentInstruction = currentOpGroup->pushInstruction("Add", "add", asmInstructionArgLayout::aIAL_Int3RegWithRC, 266);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDC, ADDCO
			currentInstruction = currentOpGroup->pushInstruction("Add Carrying", "addc", asmInstructionArgLayout::aIAL_Int3RegWithRC, 10);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDE
			currentInstruction = currentOpGroup->pushInstruction("Add Extended", "adde", asmInstructionArgLayout::aIAL_Int3RegWithRC, 138);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: DIVW, DIVWO
			currentInstruction = currentOpGroup->pushInstruction("Divide Word", "divw", asmInstructionArgLayout::aIAL_Int3RegWithRC, 491);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: DIVWU, DIVWUO
			currentInstruction = currentOpGroup->pushInstruction("Divide Word Unsigned", "divwu", asmInstructionArgLayout::aIAL_Int3RegWithRC, 459);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: MULHW, MULHWO
			currentInstruction = currentOpGroup->pushInstruction("Multiply High Word", "mulhw", asmInstructionArgLayout::aIAL_Int3RegWithRC, 75);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: MULHWU, MULHWUO
			currentInstruction = currentOpGroup->pushInstruction("Multiply High Word Unsigned", "mulhwu", asmInstructionArgLayout::aIAL_Int3RegWithRC, 11);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: MULLW, MULLWO
			currentInstruction = currentOpGroup->pushInstruction("Multiply Low Word", "mullw", asmInstructionArgLayout::aIAL_Int3RegWithRC, 235);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: SUBF, SUBFO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From", "subf", asmInstructionArgLayout::aIAL_Int3RegWithRC, 40);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: SUBFC, SUBFCO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Carrying", "subfc", asmInstructionArgLayout::aIAL_Int3RegWithRC, 8);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: SUBFE, SUBFEO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Extended", "subfe", asmInstructionArgLayout::aIAL_Int3RegWithRC, 136);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: LBZX, LBZUX
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero" + opName_IndexedString, "lbzx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 87);
			currentInstruction = currentOpGroup->pushInstruction("Load Byte and Zero" + opName_WithUpdateIndexedString, "lbzux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 119);
			// Operation: LHZX, LHZUX
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero" + opName_IndexedString, "lhzx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 279);
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word and Zero" + opName_WithUpdateIndexedString, "lhzux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 311);
			// Operation: LHAX, LHAUX
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word Algebraic" + opName_IndexedString, "lhax", asmInstructionArgLayout::aIAL_Int3RegWithRC, 343);
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word Algebraic" + opName_WithUpdateIndexedString, "lhaux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 375);
			// Operation: LWZX, LWZUX
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_IndexedString, "lwzx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 23);
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_WithUpdateIndexedString, "lwzux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 55);

			// Operation: STBX, STBUX
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_IndexedString, "stbx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 215);
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_WithUpdateIndexedString, "stbux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 247);
			// Operation: STHX, STHUX
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_IndexedString, "sthx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 407);
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_WithUpdateIndexedString, "sthux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 439);
			// Operation: STWX, STWUX
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_IndexedString, "stwx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 151);
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_WithUpdateIndexedString, "stwux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 183);

			// Operation: NEG, NEGO
			currentInstruction = currentOpGroup->pushInstruction("Negate", "neg", asmInstructionArgLayout::aIAL_Int2RegWithRC, 104);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);

			// Operation: AND
			currentInstruction = currentOpGroup->pushInstruction("AND", "and", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 28);
			// Operation: ANDC
			currentInstruction = currentOpGroup->pushInstruction("AND" + opName_WithComplString, "andc", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 60);

			// Operation: OR
			currentInstruction = currentOpGroup->pushInstruction("OR", "or", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 444);
			// Operation: ORC
			currentInstruction = currentOpGroup->pushInstruction("OR" + opName_WithComplString, "orc", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 412);

			// Operation: EQV
			currentInstruction = currentOpGroup->pushInstruction("Equivalent", "eqv", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 284);

			// Operation: NOR
			currentInstruction = currentOpGroup->pushInstruction("NOR", "nor", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 124);

			// Operation: XOR
			currentInstruction = currentOpGroup->pushInstruction("XOR", "xor", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 316);

			// Operation: SLW
			currentInstruction = currentOpGroup->pushInstruction("Shift Left Word", "slw", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 24);
			// Operation: SRAW
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word", "sraw", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 792);
			// Operation: SRAWI
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word Immediate", "srawi", asmInstructionArgLayout::aIAL_Int2RegSASwapWithSHAndRC, 824);


			// Operation: CMPW
			currentInstruction = currentOpGroup->pushInstruction("Compare Word", "cmpw", asmInstructionArgLayout::aIAL_CMPW, 0);
			// Operation: CMPLW
			currentInstruction = currentOpGroup->pushInstruction("Compare Word Logical", "cmplw", asmInstructionArgLayout::aIAL_CMPW, 32);

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