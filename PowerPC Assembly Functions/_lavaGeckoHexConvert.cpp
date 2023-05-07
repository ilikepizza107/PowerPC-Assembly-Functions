#include "stdafx.h"
#include "_lavaGeckoHexConvert.h"

namespace lava::gecko
{
	std::size_t parseGeckoCode(std::ostream& output, std::istream& codeStreamIn, std::size_t streamStartPos, std::size_t expectedLength)
	{
		std::size_t result = SIZE_MAX;

		if (output.good() && codeStreamIn.good())
		{
			unsigned char codeTypeByte = codeStreamIn.peek();
			bool bapoBit = codeTypeByte & 0b00010000;
			unsigned char primaryCodeType = (codeTypeByte >> 4) & 0b1110;
			unsigned char secondaryCodeType = (codeTypeByte) & 0b1110;
		}

		return result;
	}
}