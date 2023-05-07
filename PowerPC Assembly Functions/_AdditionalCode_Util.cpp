#include "stdafx.h"
#include "_AdditionalCode_Util.h"

namespace lava
{
	decConvStream::decConvStream() { buf << std::setfill('0'); };
	hexConvStream::hexConvStream() { buf << std::hex << std::uppercase << std::internal << std::setfill('0'); };
	fltConvStream::fltConvStream() { buf << std::fixed << std::showpoint << std::uppercase << std::internal << std::setfill('0'); };
	std::string doubleToStringWithPadding(double dblIn, unsigned char paddingLength, unsigned long precisionIn)
	{
		static fltConvStream conv;
		conv.buf.str("");
		conv.buf.precision(precisionIn);
		conv.buf << std::setw(paddingLength) << dblIn;
		return conv.buf.str();
	}
	std::string floatToStringWithPadding(float fltIn, unsigned char paddingLength, unsigned long precisionIn)
	{
		static fltConvStream conv;
		conv.buf.str("");
		conv.buf.precision(precisionIn);
		conv.buf << std::setw(paddingLength) << fltIn;
		return conv.buf.str();
	}
	bool readNCharsFromStream(std::string& destination, std::istream& source, std::size_t numToRead, bool resetStreamPos)
	{
		bool result = 0;

		if (source.good())
		{
			std::size_t originalPos = source.tellg();
			destination.resize(numToRead);
			source.read(&destination[0], numToRead);
			result = source.gcount() == numToRead;

			if (resetStreamPos)
			{
				source.seekg(originalPos);
				source.clear();
			}
		}

		return result;
	}
}
