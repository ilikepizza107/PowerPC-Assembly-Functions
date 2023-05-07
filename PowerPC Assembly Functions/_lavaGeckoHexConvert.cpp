#include "stdafx.h"
#include "_lavaGeckoHexConvert.h"

namespace lava::gecko
{
	// Constants
	constexpr unsigned long signatureAddressMask = 0x1FFFFFF;
	constexpr unsigned long signatureAddressBase = 0x80000000;

	// Utility
	unsigned long getAddressFromCodeSignature(unsigned long codeSignatureIn)
	{
		return (signatureAddressMask & codeSignatureIn) | signatureAddressBase;
	}
	std::string convertPPCInstructionHex(unsigned long hexIn)
	{
		std::string result = "";

		result = lava::ppc::convertInstructionHexToString(hexIn);
		if (!result.empty())
		{
			std::size_t relativeCommentLocation = 0x20;
			std::size_t paddingLength = (result.size() > relativeCommentLocation) ? 0x00 : relativeCommentLocation - result.size();
			if (paddingLength > 0)
			{
				result += std::string(paddingLength, ' ');
			}
			result += "# 0x" + lava::numToHexStringWithPadding(hexIn, 8);
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

		currentCodeTypeGroup = pushPrTypeGroupToDict(geckoPrimaryCodeTypes::gPCT_Assembly);
		{
			currentCodeType = currentCodeTypeGroup->pushInstruction("Insert ASM", 2, geckoC2CodeConv);
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