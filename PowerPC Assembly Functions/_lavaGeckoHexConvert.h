#ifndef GECKO_HEX_CONVERT_V1_H
#define GECKO_HEX_CONVERT_V1_H

#include "stdafx.h"
#include "_lavaASMHexConvert.h"

namespace lava::gecko
{

	std::size_t parseGeckoCode(std::ostream& output, std::istream& codeStreamIn, std::size_t streamStartPos, std::size_t expectedLength);

}

#endif