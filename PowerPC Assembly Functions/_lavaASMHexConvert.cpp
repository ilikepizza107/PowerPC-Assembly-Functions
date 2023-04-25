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
	std::string getMaskFromMBMESH(unsigned char MBIn, unsigned char MEIn, unsigned char SHIn)
	{
		std::stringstream result;

		bool invertMask = MBIn > MEIn;
		unsigned long maskComp1 = (unsigned long long(1) << (32 - MBIn)) - 1;
		unsigned long maskComp2 = ~((unsigned long long(1) << (31 - MEIn)) - 1);
		unsigned long finalMask = (!invertMask) ? maskComp1 & maskComp2 : maskComp1 | maskComp2;
		finalMask = (finalMask >> SHIn) | (finalMask << (32 - SHIn));
		result << std::hex << finalMask << std::dec;
		std::string maskStr = result.str();
		maskStr = std::string(8 - maskStr.size(), '0') + maskStr;

		return maskStr;
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

				result << " " << (unsigned long)BO << ", " << (unsigned long)BI << ", ";
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
				if (argumentsIn[6] != 0)
				{
					result << "l";
				}
			}
			else
			{
				result << instructionIn->mnemonic;
				if (argumentsIn[6] != 0)
				{
					result << "l";
				}

				result << " " << (int)BO << ", " << (int)BI << ", " << (int)BH;
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
				if (argumentsIn[6] != 0)
				{
					result << "l";
				}
			}
			else
			{
				result << instructionIn->mnemonic;
				if (argumentsIn[6] != 0)
				{
					result << "l";
				}

				result << " " << (int)BO << ", " << (int)BI << ", " << (int)BH;
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
			result << ", " << unsignedImmArgToSignedString(argumentsIn[5], 16, 1);
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
				result << " r" << argumentsIn[2];
				result << ", r" << argumentsIn[1];
				result << ", " << std::hex << "0x" << argumentsIn[3];
			}
		}

		return result.str();
	}
	std::string integerLogicalIMMConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 4)
		{
			result << instructionIn->mnemonic;
			result << " r" << argumentsIn[2];
			result << ", r" << argumentsIn[1] << ", ";
			result << std::hex << "0x" << argumentsIn[3];
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
	std::string integer2RegSASwapWithRc(asmInstruction* instructionIn, unsigned long hexIn)
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
	std::string lswiConv(asmInstruction* instructionIn, unsigned long hexIn)
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
			result << ", " << argumentsIn[3];
		}

		return result.str();
	}
	std::string rlwnmConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 7)
		{
			unsigned char MB = argumentsIn[4];
			unsigned char ME = argumentsIn[5];

			result << instructionIn->mnemonic;
			if (argumentsIn[6])
			{
				result << '.';
			}
			result << " r" << argumentsIn[2];
			result << ", r" << argumentsIn[1];
			result << ", r" << argumentsIn[3];
			result << ", " << argumentsIn[4];
			result << ", " << argumentsIn[5];
			result << "    # (Mask: 0x" << getMaskFromMBMESH(MB, ME, 0) << ")";
		}

		return result.str();
	}
	std::string rlwinmConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 7)
		{
			unsigned char SH = argumentsIn[3];
			unsigned char MB = argumentsIn[4];
			unsigned char ME = argumentsIn[5];

			result << instructionIn->mnemonic;
			if (argumentsIn[6])
			{
				result << '.';
			}
			result << " r" << argumentsIn[2];
			result << ", r" << argumentsIn[1];
			result << ", " << argumentsIn[3];
			result << ", " << argumentsIn[4];
			result << ", " << argumentsIn[5];
			result << "    # (Mask: 0x" << getMaskFromMBMESH(MB, ME, SH) << ")";
		}

		return result.str();
	}
	std::string floatCompareConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 7)
		{
			result << instructionIn->mnemonic;
			result << " cr" << argumentsIn[1];
			result << ", f" << argumentsIn[3];
			result << ", f" << argumentsIn[4];
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
	std::string floatLoadStoreIndexedConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mnemonic;
			result << " f" << argumentsIn[1] << ", ";
			result << " r" << argumentsIn[2] << ", ";
			result << " r" << argumentsIn[3];
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
	std::string float3RegOmitACWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
	std::string float4RegBCSwapWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
			result << ", f" << argumentsIn[3];
		}

		return result.str();
	}
	std::string moveToFromSPRegConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 5)
		{
			int shiftedSpReg = argumentsIn[2] >> 5;
			switch (shiftedSpReg)
			{
			case 1:
			{
				result << instructionIn->mnemonic.substr(0, 2) << "xer";
				break;
			}
			case 8:
			{
				result << instructionIn->mnemonic.substr(0, 2) << "lr";
				break;
			}
			case 9:
			{
				result << instructionIn->mnemonic.substr(0, 2) << "ctr";
				break;
			}
			default:
			{
				result << instructionIn->mnemonic << " " << argumentsIn[2] << ",";
				break;
			}
			}
			result << " r" << argumentsIn[1];
		}

		return result.str();
	}
	std::string conditionRegLogicalsConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mnemonic;
			result << " " << argumentsIn[1];
			result << ", " << argumentsIn[2];
			result << ", " << argumentsIn[3];
		}

		return result.str();
	}
	std::string conditionRegMoveFieldConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 8)
		{
			result << instructionIn->mnemonic;
			result << " cr" << argumentsIn[1];
			result << ", cr" << argumentsIn[3];
		}

		return result.str();
	}
	std::string pairedSingleCompareConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 7)
		{
			result << instructionIn->mnemonic;
			result << " cr" << argumentsIn[1];
			result << ", f" << argumentsIn[3];
			result << ", f" << argumentsIn[4];
		}

		return result.str();
	}
	std::string pairedSingleQLoadStoreConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mnemonic;
			result << " f" << argumentsIn[1] << ", ";
			result << unsignedImmArgToSignedString(argumentsIn[5], 16, 1);
			result << "(r" << argumentsIn[2] << "), ";
			result << argumentsIn[3] << ", ";
			result << argumentsIn[4];
		}

		return result.str();
	}
	std::string pairedSingleQLoadStoreIndexedConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 8)
		{
			result << instructionIn->mnemonic;
			result << " f" << argumentsIn[1] << ", ";
			result << "r" << argumentsIn[2] << ", ";
			result << "r" << argumentsIn[3] << ", ";
			result << argumentsIn[4] << ", ";
			result << argumentsIn[5];
		}

		return result.str();
	}
	std::string pairedSingle3RegWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
			result << " f" << argumentsIn[1];
			result << ", f" << argumentsIn[2];
			result << ", f" << argumentsIn[3];
		}

		return result.str();
	}
	std::string pairedSingle3RegOmitAWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
			result << " f" << argumentsIn[1];
			result << ", f" << argumentsIn[3];
		}

		return result.str();
	}
	std::string pairedSingle4RegWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
			result << ", f" << argumentsIn[3];
		}

		return result.str();
	}
	std::string pairedSingle4RegOmitBWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
	std::string pairedSingle4RegOmitCWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
	std::string pairedSingle4RegOmitACWithRcConv(asmInstruction* instructionIn, unsigned long hexIn)
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
	std::string dataCache3RegOmitDConv(asmInstruction* instructionIn, unsigned long hexIn)
	{
		std::stringstream result;

		std::vector<unsigned long> argumentsIn = instructionIn->getArgLayoutPtr()->splitHexIntoArguments(hexIn);
		if (argumentsIn.size() >= 6)
		{
			result << instructionIn->mnemonic;
			result << " r" << argumentsIn[2];
			result << ", r" << argumentsIn[3];
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

	// asmPrOpCodeGroup
	unsigned long instructionCount = 0x00;
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

			if (!secondaryOpCodeStartsAndLengths.empty())
			{
				unsigned char expectedSecOpCodeEnd = secondaryOpCodeStartsAndLengths[0].first + secondaryOpCodeStartsAndLengths[0].second;
				result->canonForm |= result->secondaryOpCode << (32 - expectedSecOpCodeEnd);
			}

			instructionCount++;
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
			result->isUnofficialInstr = 1;
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
			if (secOpCodeStart != UCHAR_MAX && secOpCodeLength != UCHAR_MAX)
			{
				result->secondaryOpCodeStartsAndLengths.push_back({ secOpCodeStart, secOpCodeLength });
			}
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
		defineArgLayout(asmInstructionArgLayout::aIAL_IntLogicalIMM, { 0, 6, 11, 16 }, integerLogicalIMMConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_IntLoadStore, { 0, 6, 11, 16 }, integerLoadStoreConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegWithSIMM, { 0, 6, 11, 16 }, integer2RegWithSIMMConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegWithRC, { 0, 6, 11, 16, 21, 31 }, integer2RegWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int3RegWithRC, { 0, 6, 11, 16, 21, 31 }, integer3RegWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegSASwapWithRC, { 0, 6, 11, 16, 21, 31 }, integer2RegSASwapWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, { 0, 6, 11, 16, 21, 31 }, integer3RegSASwapWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_Int2RegSASwapWithSHAndRC, { 0, 6, 11, 16, 21, 31 }, integer2RegSASwapWithSHAndRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_LSWI, {0, 6, 11, 16, 21, 31}, lswiConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_RLWNM, { 0, 6, 11, 16, 21, 26, 31 }, rlwnmConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_RLWINM, { 0, 6, 11, 16, 21, 26, 31 }, rlwinmConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_FltCompare, { 0, 6, 9, 11, 16, 21, 31 }, floatCompareConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_FltLoadStore, { 0, 6, 11, 16 }, floatLoadStoreConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, { 0, 6, 11, 16, 21, 31 }, floatLoadStoreIndexedConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float2RegOmitAWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt3RegOmitBWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float3RegOmitBWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float3RegOmitCWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt3RegOmitACWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float3RegOmitACWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, { 0, 6, 11, 16, 21, 26, 31 }, float4RegBCSwapWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_MoveToFromSPReg, { 0, 6, 11, 21, 31 }, moveToFromSPRegConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_ConditionRegLogicals, { 0, 6, 11, 16, 21, 31 }, conditionRegLogicalsConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_ConditionRegMoveField, { 0, 6, 9, 11, 14, 16, 21, 31 }, conditionRegMoveFieldConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingleCompare, { 0, 6, 9, 11, 16, 21, 31 }, pairedSingleCompareConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingleQLoadStore, { 0, 6, 11, 16, 17, 20 }, pairedSingleQLoadStoreConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingleQLoadStoreIdx, { 0, 6, 11, 16, 21, 22, 25, 31 }, pairedSingleQLoadStoreIndexedConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingle3Reg, { 0, 6, 11, 16, 21, 31 }, pairedSingle3RegWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingle3RegOmitA, { 0, 6, 11, 16, 21, 31 }, pairedSingle3RegOmitAWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingle4Reg, { 0, 6, 11, 16, 21, 26, 31 }, pairedSingle4RegWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingle4RegOmitB, { 0, 6, 11, 16, 21, 26, 31 }, pairedSingle4RegOmitBWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingle4RegOmitC, { 0, 6, 11, 16, 21, 26, 31 }, pairedSingle4RegOmitCWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_PairedSingle4RegOmitAC, { 0, 6, 11, 16, 21, 26, 31 }, pairedSingle4RegOmitACWithRcConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_DataCache3RegOmitD, {0, 6, 11, 16, 21, 31}, dataCache3RegOmitDConv);
		defineArgLayout(asmInstructionArgLayout::aIAL_MemSync3Reg, { 0, 6, 11, 16, 21, 31 }, integer3RegWithRc);
		defineArgLayout(asmInstructionArgLayout::aIAL_MemSyncNoReg, {0, 6, 11, 16, 21, 31}, defaultAsmInstrToStrFunc);

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
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional to Count Register", "bcctr", asmInstructionArgLayout::aIAL_BCCTR, 528);
			// Operation:: BCLR
			currentInstruction = currentOpGroup->pushInstruction("Branch Conditional to Link Register", "bclr", asmInstructionArgLayout::aIAL_BCLR, 16);

			// Operation:: CRAND, CRANDC
			currentInstruction = currentOpGroup->pushInstruction("Condition Register AND", "crand", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 257);
			currentInstruction = currentOpGroup->pushInstruction("Condition Register AND" + opName_WithComplString, "crandc", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 129);
			// Operation:: CREQV
			currentInstruction = currentOpGroup->pushInstruction("Condition Register Equivalent", "creqv", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 289);
			// Operation:: CRNAND
			currentInstruction = currentOpGroup->pushInstruction("Condition Register NAND", "crnand", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 225);
			// Operation:: CNOR
			currentInstruction = currentOpGroup->pushInstruction("Condition Register NOR", "crnor", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 33);
			// Operation:: CROR, CRORC
			currentInstruction = currentOpGroup->pushInstruction("Condition Register OR", "cror", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 499);
			currentInstruction = currentOpGroup->pushInstruction("Condition Register OR" + opName_WithComplString, "crorc", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 417);
			// Operation:: CRXOR
			currentInstruction = currentOpGroup->pushInstruction("Condition Register XOR", "crxor", asmInstructionArgLayout::aIAL_ConditionRegLogicals, 193);
			
			// Operation: MCRF
			currentInstruction = currentOpGroup->pushInstruction("Move Condition Register Field", "mcrf", asmInstructionArgLayout::aIAL_ConditionRegMoveField, 0);

			// Operation: ISYNC
			currentInstruction = currentOpGroup->pushInstruction("Instruction Synchronize", "isync", asmInstructionArgLayout::aIAL_MemSyncNoReg, 150);
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
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply-Add" + opName_DoublePrecision, "fmadd", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 29);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply-Subtract" + opName_DoublePrecision, "fmsub", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 28);
			currentInstruction = currentOpGroup->pushInstruction("Floating Negative Multiply-Add" + opName_DoublePrecision, "fnmadd", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 31);
			currentInstruction = currentOpGroup->pushInstruction("Floating Negative Multiply-Subtract" + opName_DoublePrecision, "fnmsub", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 30);
			currentInstruction = currentOpGroup->pushInstruction("Floating Select", "fsel", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 23);
			currentInstruction = currentOpGroup->pushInstruction("Floating Reciprocal Square Root Estimate", "frsqrte", asmInstructionArgLayout::aIAL_Flt3RegOmitACWithRC, 26);
			

			// Instructions Which Use Extended Length SecOp
			currentOpGroup->secondaryOpCodeStartsAndLengths.push_back({21, 10});
			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word", "fctiw", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 14);
			currentInstruction = currentOpGroup->pushInstruction("Floating Convert to Integer Word with Round toward Zero", "fctiwz", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 15);
			currentInstruction = currentOpGroup->pushInstruction("Floating Round to Single", "frsp", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 12);
			currentInstruction = currentOpGroup->pushInstruction("Floating Move Register" + opName_DoublePrecision, "fmr", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 72);
			currentInstruction = currentOpGroup->pushInstruction("Floating Negate", "fneg", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 40);
			currentInstruction = currentOpGroup->pushInstruction("Floating Absolute Value", "fabs", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 264);
			currentInstruction = currentOpGroup->pushInstruction("Floating Negative Absolute Value", "fnabs", asmInstructionArgLayout::aIAL_Flt2RegOmitAWithRC, 136);
			currentInstruction = currentOpGroup->pushInstruction("Floating Compare Unordered", "fcmpu", asmInstructionArgLayout::aIAL_FltCompare, 0);
			currentInstruction = currentOpGroup->pushInstruction("Floating Compare Ordered", "fcmpo", asmInstructionArgLayout::aIAL_FltCompare, 32);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_FLOAT_S_ARTH, 26, 5);
		{
			currentInstruction = currentOpGroup->pushInstruction("Floating Add" + opName_SinglePrecision, "fadds", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 21);
			currentInstruction = currentOpGroup->pushInstruction("Floating Divide" + opName_SinglePrecision, "fdivs", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 18);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply" + opName_SinglePrecision, "fmuls", asmInstructionArgLayout::aIAL_Flt3RegOmitBWithRC, 25);
			currentInstruction = currentOpGroup->pushInstruction("Floating Subtract" + opName_SinglePrecision, "fsubs", asmInstructionArgLayout::aIAL_Flt3RegOmitCWithRC, 20);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply-Add" + opName_SinglePrecision, "fmadds", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 29);
			currentInstruction = currentOpGroup->pushInstruction("Floating Multiply-Subtract" + opName_SinglePrecision, "fmsubs", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 28);
			currentInstruction = currentOpGroup->pushInstruction("Floating Negative Multiply-Add" + opName_SinglePrecision, "fnmadds", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 31);
			currentInstruction = currentOpGroup->pushInstruction("Floating Negative Multiply-Subtract" + opName_SinglePrecision, "fnmsubs", asmInstructionArgLayout::aIAL_Flt4RegBCSwapWithRC, 30);
			currentInstruction = currentOpGroup->pushInstruction("Floating Reciprocal Estimate Single", "fres", asmInstructionArgLayout::aIAL_Flt3RegOmitACWithRC, 24);
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
			currentInstruction = currentOpGroup->pushInstruction("Store Multiple Word", "stmw", asmInstructionArgLayout::aIAL_IntLoadStore);
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
			currentInstruction = currentOpGroup->pushInstruction("OR Immediate Shifted", "oris", asmInstructionArgLayout::aIAL_IntLogicalIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_XORI);
		{
			currentInstruction = currentOpGroup->pushInstruction("XOR Immediate", "xori", asmInstructionArgLayout::aIAL_IntLogicalIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_XORIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("XOR Immediate Shifted", "xoris", asmInstructionArgLayout::aIAL_IntLogicalIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ANDI);
		{
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate" + opName_WithUpdateString, "andi.", asmInstructionArgLayout::aIAL_IntLogicalIMM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_ANDIS);
		{
			currentInstruction = currentOpGroup->pushInstruction("AND Immediate Shifted" + opName_WithUpdateString, "andis.", asmInstructionArgLayout::aIAL_IntLogicalIMM);
		}

		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_RLWINM);
		{
			currentInstruction = currentOpGroup->pushInstruction("Rotate Left Word Immediate then AND with Mask", "rlwinm", asmInstructionArgLayout::aIAL_RLWINM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_RLWIMI);
		{
			currentInstruction = currentOpGroup->pushInstruction("Rotate Left Word Immediate then Mask Insert", "rlwimi", asmInstructionArgLayout::aIAL_RLWINM);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_RLWNM);
		{
			currentInstruction = currentOpGroup->pushInstruction("Rotate Left Word then AND with Mask", "rlwnm", asmInstructionArgLayout::aIAL_RLWNM);
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
			// Operation: ADDE, ADDEO
			currentInstruction = currentOpGroup->pushInstruction("Add Extended", "adde", asmInstructionArgLayout::aIAL_Int3RegWithRC, 138);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDME, ADDMEO
			currentInstruction = currentOpGroup->pushInstruction("Add to Minus One Extended", "addme", asmInstructionArgLayout::aIAL_Int2RegWithRC, 234);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: ADDZE, ADDZEO
			currentInstruction = currentOpGroup->pushInstruction("Add to Zero Extended", "addze", asmInstructionArgLayout::aIAL_Int2RegWithRC, 202);
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
			// Operation: SUBFME, SUBFMEO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Minus One Extended", "subfme", asmInstructionArgLayout::aIAL_Int2RegWithRC, 232);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: SUBFZE, SUBFZEO
			currentInstruction = currentOpGroup->pushInstruction("Subtract From Zero Extended", "subfze", asmInstructionArgLayout::aIAL_Int2RegWithRC, 200);
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
			// Operation: LHBRX
			currentInstruction = currentOpGroup->pushInstruction("Load Half Word Byte-Reverse" + opName_IndexedString, "lhbrx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 790);
			// Operation: LWZX, LWZUX
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_IndexedString, "lwzx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 23);
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Zero" + opName_WithUpdateIndexedString, "lwzux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 55);
			// Operation: LWBRX
			currentInstruction = currentOpGroup->pushInstruction("Load Word Byte-Reverse" + opName_IndexedString, "lwbrx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 534);
			// Operation: LFDX, LFDUX
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double" + opName_IndexedString, "lfdx", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 599);
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Double" + opName_WithUpdateIndexedString, "lfdux", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 631);
			// Operation: LFSX, LFSUX
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single" + opName_IndexedString, "lfsx", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 535);
			currentInstruction = currentOpGroup->pushInstruction("Load Floating-Point Single" + opName_WithUpdateIndexedString, "lfsux", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 567);


			// Operation: STBX, STBUX
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_IndexedString, "stbx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 215);
			currentInstruction = currentOpGroup->pushInstruction("Store Byte" + opName_WithUpdateIndexedString, "stbux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 247);
			// Operation: STHX, STHUX
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_IndexedString, "sthx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 407);
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word" + opName_WithUpdateIndexedString, "sthux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 439);
			// Operation: STHBRX
			currentInstruction = currentOpGroup->pushInstruction("Store Half Word Byte-Reverse" + opName_WithUpdateIndexedString, "sthbrx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 918);
			// Operation: STWX, STWUX
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_IndexedString, "stwx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 151);
			currentInstruction = currentOpGroup->pushInstruction("Store Word" + opName_WithUpdateIndexedString, "stwux", asmInstructionArgLayout::aIAL_Int3RegWithRC, 183);
			// Operation: STWBRX
			currentInstruction = currentOpGroup->pushInstruction("Store Word Byte-Reverse" + opName_WithUpdateIndexedString, "stwbrx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 662);
			// Operation: STFDX, STFDUX
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double" + opName_IndexedString, "stfdx", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 727);
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Double" + opName_WithUpdateIndexedString, "stfdux", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 759);
			// Operation: STFSX, STFSUX
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single" + opName_IndexedString, "stfsx", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 663);
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point Single" + opName_WithUpdateIndexedString, "stfsux", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 695);
			// Operation: STFIWX
			currentInstruction = currentOpGroup->pushInstruction("Store Floating-Point as Integer Word Indexed", "stfiwx", asmInstructionArgLayout::aIAL_FltLoadStoreIndexed, 983);


			// Operation: NEG, NEGO
			currentInstruction = currentOpGroup->pushInstruction("Negate", "neg", asmInstructionArgLayout::aIAL_Int2RegWithRC, 104);
			currentInstruction = currentOpGroup->pushOverflowVersionOfInstruction(currentInstruction);
			// Operation: AND, ANDC
			currentInstruction = currentOpGroup->pushInstruction("AND", "and", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 28);
			currentInstruction = currentOpGroup->pushInstruction("AND" + opName_WithComplString, "andc", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 60);
			// Operation: OR, ORC
			currentInstruction = currentOpGroup->pushInstruction("OR", "or", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 444);
			currentInstruction = currentOpGroup->pushInstruction("OR" + opName_WithComplString, "orc", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 412);
			// Operation: EQV
			currentInstruction = currentOpGroup->pushInstruction("Equivalent", "eqv", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 284);
			// Operation: NOR
			currentInstruction = currentOpGroup->pushInstruction("NOR", "nor", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 124);
			// Operation: XOR
			currentInstruction = currentOpGroup->pushInstruction("XOR", "xor", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 316);
			// Operation: NAND
			currentInstruction = currentOpGroup->pushInstruction("NAND", "nand", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 476);

			// Operation: SLW
			currentInstruction = currentOpGroup->pushInstruction("Shift Left Word", "slw", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 24);
			// Operation: SRW
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Word", "srw", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 536);
			// Operation: SRAW
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word", "sraw", asmInstructionArgLayout::aIAL_Int3RegSASwapWithRC, 792);
			// Operation: SRAWI
			currentInstruction = currentOpGroup->pushInstruction("Shift Right Algebraic Word Immediate", "srawi", asmInstructionArgLayout::aIAL_Int2RegSASwapWithSHAndRC, 824);

			// Operation: CNTLZW
			currentInstruction = currentOpGroup->pushInstruction("Count Leading Zeros Word", "cntlzw", asmInstructionArgLayout::aIAL_Int2RegSASwapWithRC, 26);
			// Operation: EXTSB
			currentInstruction = currentOpGroup->pushInstruction("Extend Sign Byte", "extsb", asmInstructionArgLayout::aIAL_Int2RegSASwapWithRC, 954);
			// Operation: EXTSH
			currentInstruction = currentOpGroup->pushInstruction("Extend Sign Half Word", "extsh", asmInstructionArgLayout::aIAL_Int2RegSASwapWithRC, 922);

			// Operation: LSWI, LSWX
			currentInstruction = currentOpGroup->pushInstruction("Load String Word Immediate", "lswi", asmInstructionArgLayout::aIAL_LSWI, 597);
			currentInstruction = currentOpGroup->pushInstruction("Load String Word Immediate" + opName_IndexedString, "lswx", asmInstructionArgLayout::aIAL_Int3RegWithRC, 533);

			// Operation: CMPW, CMPLW
			currentInstruction = currentOpGroup->pushInstruction("Compare Word", "cmpw", asmInstructionArgLayout::aIAL_CMPW, 0);
			currentInstruction = currentOpGroup->pushInstruction("Compare Word Logical", "cmplw", asmInstructionArgLayout::aIAL_CMPW, 32);

			// Operation: MFSPR, MTSPR
			currentInstruction = currentOpGroup->pushInstruction("Move from Special-Purpose Register", "mfspr", asmInstructionArgLayout::aIAL_MoveToFromSPReg, 339);
			currentInstruction = currentOpGroup->pushInstruction("Move to Special-Purpose Register", "mtspr", asmInstructionArgLayout::aIAL_MoveToFromSPReg, 467);

			// Operation: DCBF, DCBI
			currentInstruction = currentOpGroup->pushInstruction("Data Cache Block Flush", "dcbf", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 86);
			currentInstruction = currentOpGroup->pushInstruction("Data Cache Block Invalidate", "dcbi", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 470);
			// Operation: DCBST, DCBT, DCBTST, DCBZ
			currentInstruction = currentOpGroup->pushInstruction("Data Cache Block Store", "dcbst", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 54);
			currentInstruction = currentOpGroup->pushInstruction("Data Cache Block Touch", "dcbt", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 278);
			currentInstruction = currentOpGroup->pushInstruction("Data Cache Block Touch for Store", "dcbtst", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 246);
			currentInstruction = currentOpGroup->pushInstruction("Data Cache Block Clear to Zero", "dcbz", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 1014);
			// Operation:: ICBI
			currentInstruction = currentOpGroup->pushInstruction("Instruction Cache Block Invalidate", "icbi", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 982);

			// Operation: EIEIO, SYNC
			currentInstruction = currentOpGroup->pushInstruction("Enforce In-Order Execution of I/O", "eieio", asmInstructionArgLayout::aIAL_MemSyncNoReg, 854);
			currentInstruction = currentOpGroup->pushInstruction("Synchronize", "sync", asmInstructionArgLayout::aIAL_MemSyncNoReg, 598);
			// Operation: LWARX, STWCX.
			currentInstruction = currentOpGroup->pushInstruction("Load Word and Reserve Indexed", "lwarx", asmInstructionArgLayout::aIAL_MemSync3Reg, 20);
			currentInstruction = currentOpGroup->pushInstruction("Store Word Conditional Indexed", "stwcx.", asmInstructionArgLayout::aIAL_MemSync3Reg, 150);
		}


		// Paired Single Instructions
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_PS_GENERAL, 21, 10);
		{
			// Compare Instructions
			currentInstruction = currentOpGroup->pushInstruction("Paired Singles Compare Ordered High", "ps_cmpo0", asmInstructionArgLayout::aIAL_PairedSingleCompare, 32);
			currentInstruction = currentOpGroup->pushInstruction("Paired Singles Compare Ordered Low", "ps_cmpo1", asmInstructionArgLayout::aIAL_PairedSingleCompare, 96);
			currentInstruction = currentOpGroup->pushInstruction("Paired Singles Compare Unordered High", "ps_cmpu0", asmInstructionArgLayout::aIAL_PairedSingleCompare, 0);
			currentInstruction = currentOpGroup->pushInstruction("Paired Singles Compare Unordered Low", "ps_cmpu1", asmInstructionArgLayout::aIAL_PairedSingleCompare, 64);
			// Manip Instructions
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Negate", "ps_neg", asmInstructionArgLayout::aIAL_PairedSingle3RegOmitA, 40);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Move Register", "ps_mr", asmInstructionArgLayout::aIAL_PairedSingle3RegOmitA, 72);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Absolute Value", "ps_abs", asmInstructionArgLayout::aIAL_PairedSingle3RegOmitA, 264);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Negative Absolute Value", "ps_nabs", asmInstructionArgLayout::aIAL_PairedSingle3RegOmitA, 136);
			// Merge Instructions
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Merge High", "ps_merge00", asmInstructionArgLayout::aIAL_PairedSingle3Reg, 528);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Merge Direct", "ps_merge01", asmInstructionArgLayout::aIAL_PairedSingle3Reg, 560);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Merge Swapped", "ps_merge10", asmInstructionArgLayout::aIAL_PairedSingle3Reg, 592);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Merge Low", "ps_merge11", asmInstructionArgLayout::aIAL_PairedSingle3Reg, 624);
			// Data Cache Instructions
			currentInstruction = currentOpGroup->pushInstruction("Data Cache Block Set to Zero Locked", "dcbz_l", asmInstructionArgLayout::aIAL_DataCache3RegOmitD, 1014);

			// Math Instructions (Sec Op Starts at bit 26)
			currentOpGroup->secondaryOpCodeStartsAndLengths.push_back({26, 5});
			// Full 4 Reg
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Vector Sum High", "ps_sum0", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 10);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Vector Sum Low", "ps_sum1", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 11);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Multiply-Add Scalar High", "ps_madds0", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 14);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Multiply-Add Scalar Low", "ps_madds1", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 15);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Select", "ps_sel", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 23);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Multiply-Add", "ps_madd", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 29);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Multiply-Subtract", "ps_msub", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 28);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Negative Multiply-Add", "ps_nmadd", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 31);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Negative Multiply-Subtract", "ps_nmsub", asmInstructionArgLayout::aIAL_PairedSingle4Reg, 30);
			// 4 Reg Omit B
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Multiply", "ps_mul", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitB, 25);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Multiply Scalar High", "ps_muls0", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitB, 12);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Multiply Scalar Low", "ps_muls1", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitB, 13);
			// 4 Reg Omit C
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Add", "ps_add", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitC, 21);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Subtract", "ps_sub", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitC, 20);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Divide", "ps_div", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitC, 18);
			// 4 Reg Omit AC
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Reciprocal Estimate", "ps_res", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitAC, 24);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Reciprocal Square Root Estimate", "ps_rsqrte", asmInstructionArgLayout::aIAL_PairedSingle4RegOmitAC, 26);


			// Indexed LoadStore Instructions (Sec Op Starts at bit 25)
			currentOpGroup->secondaryOpCodeStartsAndLengths.push_back({25, 6});
			// PSQ_LX, PSQ_LUX
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Load" + opName_IndexedString, "psq_lx", asmInstructionArgLayout::aIAL_PairedSingleQLoadStoreIdx, 6);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Load" + opName_WithUpdateIndexedString, "psq_lux", asmInstructionArgLayout::aIAL_PairedSingleQLoadStoreIdx, 38);
			// PSQ_STX, PSQ_STUX
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Store" + opName_IndexedString, "psq_stx", asmInstructionArgLayout::aIAL_PairedSingleQLoadStoreIdx, 7);
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Store" + opName_WithUpdateIndexedString, "psq_stux", asmInstructionArgLayout::aIAL_PairedSingleQLoadStoreIdx, 39);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_PSQ_L);
		{
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Load", "psq_l", asmInstructionArgLayout::aIAL_PairedSingleQLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_PSQ_LU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Load" + opName_WithUpdateString, "psq_lu", asmInstructionArgLayout::aIAL_PairedSingleQLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_PSQ_ST);
		{
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Store", "psq_st", asmInstructionArgLayout::aIAL_PairedSingleQLoadStore);
		}
		currentOpGroup = pushOpCodeGroupToDict(asmPrimaryOpCodes::aPOC_PSQ_STU);
		{
			currentInstruction = currentOpGroup->pushInstruction("Paired Single Quantized Store" + opName_WithUpdateString, "psq_stu", asmInstructionArgLayout::aIAL_PairedSingleQLoadStore);
		}
		return;
	}

	std::string convertInstructionHexToString(unsigned long hexIn)
	{
		std::stringstream result;

		unsigned long opCode = extractInstructionArg(hexIn, 0, 6);
		if (instructionDictionary.find(opCode) != instructionDictionary.end())
		{
			asmPrOpCodeGroup* opCodeGroup = &instructionDictionary[opCode];
			asmInstruction* targetInstruction = nullptr;

			if (opCodeGroup->secondaryOpCodeStartsAndLengths.empty())
			{
				targetInstruction = &opCodeGroup->secondaryOpCodeToInstructions.begin()->second;
			}
			else
			{
				unsigned short secondaryOpCode = USHRT_MAX;
				std::pair<unsigned char, unsigned char>* currPair = nullptr;
				for (unsigned long i = 0; targetInstruction == nullptr && i < opCodeGroup->secondaryOpCodeStartsAndLengths.size(); i++)
				{
					currPair = &opCodeGroup->secondaryOpCodeStartsAndLengths[i];
					secondaryOpCode = (unsigned short)extractInstructionArg(hexIn, currPair->first, currPair->second);
					if (opCodeGroup->secondaryOpCodeToInstructions.find(secondaryOpCode) != opCodeGroup->secondaryOpCodeToInstructions.end())
					{
						targetInstruction = &opCodeGroup->secondaryOpCodeToInstructions[secondaryOpCode];
					}
				}
			}

			if (targetInstruction != nullptr)
			{
				result << targetInstruction->getArgLayoutPtr()->conversionFunc(targetInstruction, hexIn);
			}
		}

		return result.str();
	}
	bool summarizeInstructionDictionary(std::ostream& output)
	{
		bool result = 0;

		if (output.good())
		{
			output << "PowerPC Assembly Instruction Dictionary:\n";

			for (auto i = instructionDictionary.cbegin(); i != instructionDictionary.end(); i++)
			{
				for (auto u = i->second.secondaryOpCodeToInstructions.cbegin(); u != i->second.secondaryOpCodeToInstructions.end(); u++)
				{
					if (u->second.isUnofficialInstr)
					{
						output << "[Note: Unofficial] ";
					}
					output << "[" << i->first;
					if (u->second.secondaryOpCode != USHRT_MAX)
					{
						output << ", " << u->first;
					}
					output << "] " << u->second.mnemonic << " (" << u->second.name << ") [0x" << std::hex << u->second.canonForm << std::dec << "]";
					output << " {Ex: " << convertInstructionHexToString(u->second.canonForm) << "}\n";
				}
			}

			result = output.good();
		}

		return result;
	}
	bool summarizeInstructionDictionary(std::string outputFilepath)
	{
		bool result = 0;

		std::ofstream output(outputFilepath, std::ios_base::out);
		if (output.is_open())
		{
			result = summarizeInstructionDictionary(output);
		}

		return result;
	}
}