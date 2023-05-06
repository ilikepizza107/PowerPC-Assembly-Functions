#include "stdafx.h"
#include "_AdditionalCode_Util.h"

namespace lava
{
	decConvStream::decConvStream() { buf << std::setfill('0'); };
	hexConvStream::hexConvStream() { buf << std::hex << std::uppercase << std::internal << std::setfill('0'); };
	fltConvStream::fltConvStream() { buf << std::fixed << std::showpoint << std::uppercase << std::internal << std::setfill('0'); };
	int stringToNum(const std::string& stringIn, bool allowNeg, int defaultVal)
	{
		int result = defaultVal;
		std::string manipStr = stringIn;
		int base = (manipStr.find("0x") == 0) ? 16 : 10;
		char* res = nullptr;
		result = std::strtoul(manipStr.c_str(), &res, base);
		if (res != (manipStr.c_str() + manipStr.size()))
		{
			result = defaultVal;
		}
		if (result < 0 && !allowNeg)
		{
			result = defaultVal;
		}
		return result;
	}
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
}
