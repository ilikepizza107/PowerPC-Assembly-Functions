#include "stdafx.h"
#include "_lavaGeckoHexConvert.h"

namespace lava::gecko
{
	// Constants
	constexpr unsigned long signatureBaPoMask = 0x10000000;
	constexpr unsigned long signatureAddressMask = 0x1FFFFFF;
	constexpr unsigned long signatureAddressBase = 0x80000000;

	// Utility
	unsigned long getAddressFromCodeSignature(unsigned long codeSignatureIn)
	{
		return (signatureAddressMask & codeSignatureIn) | signatureAddressBase;
	}
	void appendCommentToString(std::string& destination, std::string commentStr, unsigned long relativeCommentLoc = 0x20)
	{
		std::size_t paddingLength = (destination.size() > relativeCommentLoc) ? 0x00 : relativeCommentLoc - destination.size();
		if (paddingLength > 0)
		{
			destination += std::string(paddingLength, ' ');
		}
		destination += "# " + commentStr;
	}

	std::string convertPPCInstructionHex(unsigned long hexIn)
	{
		std::string result = "";

		result = lava::ppc::convertInstructionHexToString(hexIn);
		if (!result.empty())
		{
			appendCommentToString(result, "0x" + lava::numToHexStringWithPadding(hexIn, 8));
		}

		return result;
	}
	std::string convertPPCInstructionHex(std::string hexIn)
	{
		std::string result = "";

		unsigned long integerConversion = lava::stringToNum<unsigned long>(hexIn, 0, ULONG_MAX, 1);
		result = convertPPCInstructionHex(integerConversion);

		return result;
	}

	// Conversion Predicates
	std::size_t defaultGeckoConversionConv(geckoCodeType* codeTypeIn, std::istream& codeStreamIn, std::ostream& outputStreamIn)
	{
		return 0;
	}
	std::size_t gecko06CodeConv(geckoCodeType* codeTypeIn, std::istream& codeStreamIn, std::ostream& outputStreamIn)
	{
		std::size_t result = SIZE_MAX;

		if (codeStreamIn.good() && outputStreamIn.good())
		{
			std::streampos initialPos = codeStreamIn.tellg();

			std::string signatureWord("");
			std::string immWord("");

			lava::readNCharsFromStream(signatureWord, codeStreamIn, 8, 0);
			lava::readNCharsFromStream(immWord, codeStreamIn, 8, 0);

			unsigned long signatureNum = lava::stringToNum<unsigned long>(signatureWord, 0, ULONG_MAX, 1);
			unsigned long immNum = lava::stringToNum<unsigned long>(immWord, 0, ULONG_MAX, 1);
			std::vector<unsigned char> bytesToWrite(lava::padLengthTo<unsigned long>(immNum, 0x08, 1), UCHAR_MAX);

			std::string byteInStr("");
			unsigned char byteIn = CHAR_MAX;
			bool allPrintChars = 1;
			bool lastByteWasZero = 0;
			bool isNullTerminated = 0;
			unsigned long terminatorCount = 0x00;
			for (unsigned long i = 0; i < bytesToWrite.size(); i++)
			{
				lava::readNCharsFromStream(byteInStr, codeStreamIn, 2, 0);
				byteIn = lava::stringToNum<unsigned char>(byteInStr, 0, UCHAR_MAX, 1);
				bytesToWrite[i] = byteIn;
				if (i < immNum)
				{
					// We need to allow null terminators between strings, so we have to allow 0x00.
					if (byteIn == 0x00)
					{
						if (allPrintChars && !lastByteWasZero)
						{
							terminatorCount++;
						}
					}
					else if (!std::isprint(byteIn))
					{
						allPrintChars = 0;
					}
				}
				lastByteWasZero = byteIn == 0x00;
			}
			isNullTerminated = allPrintChars && (bytesToWrite[immNum - 1] == 0x00);

			// Initialize Strings For Output
			std::string outputString("");
			std::stringstream commentString("");

			// Output First Line
			outputString = "* " + signatureWord + " " + immWord;
			commentString << codeTypeIn->name << " (" << std::to_string(immNum) << " characters";
			if (allPrintChars && isNullTerminated && terminatorCount > 1)
			{
				commentString << ", " << terminatorCount << " strings";
			}
			commentString << ") @ $(";
			if (!(signatureBaPoMask & signatureNum))
			{
				commentString << "ba + ";
			}
			else
			{
				commentString << "po + ";
			}
			commentString << "0x" << lava::numToHexStringWithPadding(signatureAddressMask & signatureNum, 7) << "):";
			appendCommentToString(outputString, commentString.str());
			outputStreamIn << outputString << "\n";

			// Loop through the rest of the output
			std::size_t cursor = 0;
			std::size_t numFullLines = bytesToWrite.size() / 8;
			bool stringCurrentlyOpen = 0;
			for (unsigned long i = 0; i < numFullLines; i++)
			{
				outputString = "* " + lava::numToHexStringWithPadding(lava::bytesToFundamental<unsigned long>(bytesToWrite.data() + cursor), 8);
				outputString += " " + lava::numToHexStringWithPadding(lava::bytesToFundamental<unsigned long>(bytesToWrite.data() + cursor + 4), 8);
				commentString.str("");
				commentString << "\t";
				// String Comment Output
				if (allPrintChars)
				{
					unsigned char currCharacter = UCHAR_MAX;
					if (stringCurrentlyOpen)
					{
						commentString << "...";
					}
					for (unsigned long u = 0; (u < 8) && ((u + cursor) < immNum); u++)
					{
						currCharacter = bytesToWrite[u + cursor];
						if ((stringCurrentlyOpen && currCharacter == 0x00) || (!stringCurrentlyOpen && currCharacter != 0x00))
						{
							commentString << "\"";
							stringCurrentlyOpen = !stringCurrentlyOpen;
						}
						if (currCharacter != 0x00)
						{
							commentString << currCharacter;
						}
					}
					if (stringCurrentlyOpen)
					{
						if ((i + 1) < numFullLines)
						{
							if (bytesToWrite[cursor + 8] == 0x00)
							{
								commentString << "\"";
								stringCurrentlyOpen = 0;
							}
							else
							{
								commentString << "...";
							}
						}
						else
						{
							commentString << "\" (Note: Not Null-Terminated!)";
						}
					}
				}
				// Raw Hex Output
				else
				{
					commentString << "0x" << lava::numToHexStringWithPadding(bytesToWrite[cursor], 2);
					for (unsigned long u = 1; (u < 8) && ((u + cursor) < immNum); u++)
					{
						commentString << ", 0x" << lava::numToHexStringWithPadding(bytesToWrite[u + cursor], 2);
					}
				}
				appendCommentToString(outputString, commentString.str());
				outputStreamIn << outputString << "\n";
				cursor += 8;
			}

			result = codeStreamIn.tellg() - initialPos;
		}

		return result;
	}
	std::size_t geckoCompareCodeConv(geckoCodeType* codeTypeIn, std::istream& codeStreamIn, std::ostream& outputStreamIn)
	{
		std::size_t result = SIZE_MAX;

		if (codeStreamIn.good() && outputStreamIn.good())
		{
			std::streampos initialPos = codeStreamIn.tellg();

			std::string signatureWord("");
			std::string immWord("");

			lava::readNCharsFromStream(signatureWord, codeStreamIn, 8, 0);
			lava::readNCharsFromStream(immWord, codeStreamIn, 8, 0);

			unsigned long signatureNum = lava::stringToNum<unsigned long>(signatureWord, 0, ULONG_MAX, 1);
			unsigned long immNum = lava::stringToNum<unsigned long>(immWord, 0, ULONG_MAX, 1);

			std::string outputStr = "* " + signatureWord + " " + immWord;

			std::stringstream commentStr("");
			commentStr << codeTypeIn->name;
			if (signatureNum & 1)
			{
				commentStr << " (With EndIf)";
				signatureNum &= ~1;
			}
			commentStr << ": If Val @ $(";
			if (signatureNum & signatureBaPoMask)
			{
				commentStr << "po +";
			}
			else
			{
				commentStr << "ba +";
			}
			commentStr << " 0x" << lava::numToHexStringWithPadding(signatureNum & signatureAddressMask, 7);
			commentStr << ") ";

			switch (codeTypeIn->secondaryCodeType % 8)
			{
			case 0: { commentStr << "=="; break; }
			case 2: { commentStr << "!="; break; }
			case 4: { commentStr << ">"; break; }
			case 6: { commentStr << "<"; break; }
			default: {break; }
			}

			if (codeTypeIn->secondaryCodeType <= 6)
			{
				commentStr << " 0x" << lava::numToHexStringWithPadding(immNum, 8);
			}
			else
			{
				commentStr << " (0x" << lava::numToHexStringWithPadding(immNum >> 0x10, 4) << " & 0x" << lava::numToHexStringWithPadding(immNum & 0xFFFF, 4) << ")";
			}

			appendCommentToString(outputStr, commentStr.str());
			outputStreamIn << outputStr << "\n";

			result = codeStreamIn.tellg() - initialPos;
		}

		return result;
	}
	std::size_t gecko42CodeConv(geckoCodeType* codeTypeIn, std::istream& codeStreamIn, std::ostream& outputStreamIn)
	{
		std::size_t result = SIZE_MAX;

		if (codeStreamIn.good() && outputStreamIn.good())
		{
			std::streampos initialPos = codeStreamIn.tellg();

			std::string signatureWord("");
			std::string addrWord("");

			lava::readNCharsFromStream(signatureWord, codeStreamIn, 8, 0);
			lava::readNCharsFromStream(addrWord, codeStreamIn, 8, 0);

			unsigned long signatureNum = lava::stringToNum<unsigned long>(signatureWord, 0, ULONG_MAX, 1);
			unsigned long addrNum = lava::stringToNum<unsigned long>(addrWord, 0, ULONG_MAX, 1);

			std::string outputStr = "* " + signatureWord + " " + addrWord;
			std::stringstream commentStr("");
			commentStr << codeTypeIn->name << ": ba ";
			if (signatureNum & 0x00100000)
			{
				commentStr << "+";
			}
			commentStr << "= ";
			if (signatureNum & 0x00010000)
			{
				if (signatureNum & signatureBaPoMask)
				{
					commentStr << "po + ";
				}
				else
				{
					commentStr << "ba + ";
				}
			}
			if (signatureNum & 0x00001000)
			{
				commentStr << "gr" + lava::numToDecStringWithPadding(signatureNum & 0xF, 0) << " + ";
			}
			commentStr << "0x" + addrWord;
			appendCommentToString(outputStr, commentStr.str());

			outputStreamIn << outputStr << "\n";

			result = codeStreamIn.tellg() - initialPos;
		}

		return result;
	}
	std::size_t geckoC2CodeConv(geckoCodeType* codeTypeIn, std::istream& codeStreamIn, std::ostream& outputStreamIn)
	{
		std::size_t result = SIZE_MAX;

		if (codeStreamIn.good() && outputStreamIn.good())
		{
			std::streampos initialPos = codeStreamIn.tellg();

			std::string signatureWord("");
			std::string lengthWord("");

			lava::readNCharsFromStream(signatureWord, codeStreamIn, 8, 0);
			lava::readNCharsFromStream(lengthWord, codeStreamIn, 8, 0);

			unsigned long signatureNum = lava::stringToNum<unsigned long>(signatureWord, 0, ULONG_MAX, 1);
			unsigned long lengthNum = lava::stringToNum<unsigned long>(lengthWord, 0, ULONG_MAX, 1);
			
			outputStreamIn << "HOOK @ $" << lava::numToHexStringWithPadding(getAddressFromCodeSignature(signatureNum), 8) << "\n";
			outputStreamIn << "{\n";

			std::string hexWord("");
			std::string conversion("");
			for (unsigned long i = 0; i < lengthNum * 2; i++)
			{
				lava::readNCharsFromStream(hexWord, codeStreamIn, 8, 0);
				conversion = convertPPCInstructionHex(hexWord);
				if (!conversion.empty())
				{
					outputStreamIn << "\t" << conversion << "\n";
				}
			}

			outputStreamIn << "}\n";

			result = codeStreamIn.tellg() - initialPos;
		}

		return result;
	}
	std::size_t geckoE0CodeConv(geckoCodeType* codeTypeIn, std::istream& codeStreamIn, std::ostream& outputStreamIn)
	{
		std::size_t result = SIZE_MAX;

		if (codeStreamIn.good() && outputStreamIn.good())
		{
			std::streampos initialPos = codeStreamIn.tellg();

			std::string signatureWord("");
			std::string immWord("");

			lava::readNCharsFromStream(signatureWord, codeStreamIn, 8, 0);
			lava::readNCharsFromStream(immWord, codeStreamIn, 8, 0);

			unsigned long signatureNum = lava::stringToNum<unsigned long>(signatureWord, 0, ULONG_MAX, 1);
			unsigned long immNum = lava::stringToNum<unsigned long>(immWord, 0, ULONG_MAX, 1);

			std::string outputStr = "* " + signatureWord + " " + immWord;
			std::stringstream commentStr("");
			commentStr << codeTypeIn->name << ": ";
			if (immNum >> 0x10)
			{
				commentStr << "ba = 0x" << lava::numToHexStringWithPadding(immNum >> 0x10, 4) << "0000";
			}
			else
			{
				commentStr << "ba unchanged";
			}
			commentStr << ", ";
			if (immNum & 0xFFFF)
			{
				commentStr << "po = 0x" << lava::numToHexStringWithPadding(immNum & 0xFFFF, 4) << "0000";
			}
			else
			{
				commentStr << "po unchanged";
			}

			appendCommentToString(outputStr, commentStr.str());
			outputStreamIn << outputStr << "\n";

			result = codeStreamIn.tellg() - initialPos;
		}

		return result;
	}

	// Code Type Group
	geckoCodeType* geckoPrTypeGroup::pushInstruction(std::string nameIn, unsigned char secOpIn, std::size_t(*conversionFuncIn)(geckoCodeType*, std::istream&, std::ostream&))
	{
		geckoCodeType* result = nullptr;

		if (secondaryCodeTypesToCodes.find(secOpIn) == secondaryCodeTypesToCodes.end())
		{
			result = &secondaryCodeTypesToCodes[secOpIn];
			result->name = nameIn;
			result->primaryCodeType = primaryCodeType;
			result->secondaryCodeType = secOpIn;
			result->conversionFunc = conversionFuncIn;
		}

		return result;
	}

	// Dictionary
	std::map<unsigned char, geckoPrTypeGroup> geckoCodeDictionary{};
	geckoPrTypeGroup* pushPrTypeGroupToDict(geckoPrimaryCodeTypes codeTypeIn)
	{
		geckoPrTypeGroup* result = nullptr;

		if (geckoCodeDictionary.find((unsigned char)codeTypeIn) == geckoCodeDictionary.end())
		{
			result = &geckoCodeDictionary[(unsigned char)codeTypeIn];
			result->primaryCodeType = codeTypeIn;
		}

		return result;
	}
	void buildGeckoCodeDictionary()
	{
		geckoPrTypeGroup* currentCodeTypeGroup = nullptr;
		geckoCodeType* currentCodeType = nullptr;

		currentCodeTypeGroup = pushPrTypeGroupToDict(geckoPrimaryCodeTypes::gPCT_RAMWrite);
		{
			currentCodeType = currentCodeTypeGroup->pushInstruction("String Write", 0x6, gecko06CodeConv);
		}
		currentCodeTypeGroup = pushPrTypeGroupToDict(geckoPrimaryCodeTypes::gPCT_If);
		{
			currentCodeType = currentCodeTypeGroup->pushInstruction("32-Bit If Equal", 0x0, geckoCompareCodeConv);
			currentCodeType = currentCodeTypeGroup->pushInstruction("32-Bit If Not Equal", 0x2, geckoCompareCodeConv);
			currentCodeType = currentCodeTypeGroup->pushInstruction("32-Bit If Greater", 0x4, geckoCompareCodeConv);
			currentCodeType = currentCodeTypeGroup->pushInstruction("32-Bit If Lesser", 0x6, geckoCompareCodeConv);
			currentCodeType = currentCodeTypeGroup->pushInstruction("16-Bit If Equal", 0x8, geckoCompareCodeConv);
			currentCodeType = currentCodeTypeGroup->pushInstruction("16-Bit If Not Equal", 0xA, geckoCompareCodeConv);
			currentCodeType = currentCodeTypeGroup->pushInstruction("16-Bit If Greater", 0xC, geckoCompareCodeConv);
			currentCodeType = currentCodeTypeGroup->pushInstruction("16-Bit If Lesser", 0xE, geckoCompareCodeConv);
		}
		currentCodeTypeGroup = pushPrTypeGroupToDict(geckoPrimaryCodeTypes::gPCT_BaseAddr);
		{
			currentCodeType = currentCodeTypeGroup->pushInstruction("Set Base Address", 2, gecko42CodeConv);
		}
		currentCodeTypeGroup = pushPrTypeGroupToDict(geckoPrimaryCodeTypes::gPCT_Assembly);
		{
			currentCodeType = currentCodeTypeGroup->pushInstruction("Insert ASM", 2, geckoC2CodeConv);
		}
		currentCodeTypeGroup = pushPrTypeGroupToDict(geckoPrimaryCodeTypes::gPCT_Misc);
		{
			currentCodeType = currentCodeTypeGroup->pushInstruction("Full Terminator", 0, geckoE0CodeConv);
		}
	}

	geckoCodeType* findRelevantGeckoCodeType(unsigned char primaryType, unsigned char secondaryType)
	{
		geckoCodeType* result = nullptr;

		auto codeGroupSearchRes = geckoCodeDictionary.find(primaryType);
		if (codeGroupSearchRes != geckoCodeDictionary.end())
		{
			geckoPrTypeGroup* targetGroup = &codeGroupSearchRes->second;
			if (targetGroup->secondaryCodeTypesToCodes.find(secondaryType) != targetGroup->secondaryCodeTypesToCodes.end())
			{
				result = &targetGroup->secondaryCodeTypesToCodes[secondaryType];
			}
		}

		return result;
	}
	std::size_t parseGeckoCode(std::ostream& output, std::istream& codeStreamIn, std::size_t expectedLength)
	{
		std::size_t result = SIZE_MAX;

		if (output.good() && codeStreamIn.good())
		{
			result = 0;

			unsigned char currCodeType = UCHAR_MAX;
			unsigned char currCodePrType = UCHAR_MAX;
			unsigned char currCodeScType = UCHAR_MAX;

			std::string codeTypeStr("");
			geckoCodeType* targetedGeckoCodeType = nullptr;
			while (result < expectedLength)
			{
				lava::readNCharsFromStream(codeTypeStr, codeStreamIn, 2, 1);

				currCodeType = lava::stringToNum<unsigned long>(codeTypeStr, 1, UCHAR_MAX, 1);
				currCodePrType = (currCodeType & 0b11100000) >> 4;	// First hex digit (minus bit 4) is primary code type.
				currCodeScType = (currCodeType & 0b00001110);		// Second hex digit (minus bit 8) is secondary code type.

				targetedGeckoCodeType = findRelevantGeckoCodeType(currCodePrType, currCodeScType);
				if (targetedGeckoCodeType != nullptr)
				{
					result += targetedGeckoCodeType->conversionFunc(targetedGeckoCodeType, codeStreamIn, output);
				}
				else
				{
					bool startingNewLine = 1;
					std::string dumpStr = "";
					dumpStr.reserve(8);
					while (result < expectedLength)
					{
						if (startingNewLine)
						{
							output << "*";
						}
						lava::readNCharsFromStream(dumpStr, codeStreamIn, 0x8, 0);
						output << " " << dumpStr;
						if (!startingNewLine)
						{
							output << "\n";
						}
						startingNewLine = !startingNewLine;
						result += 0x8;
					}
				}
			}
		}

		return result;
	}
}