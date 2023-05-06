#ifndef ADDITIONAL_CODE_UTIL_H
#define ADDITIONAL_CODE_UTIL_H

#include "stdafx.h"
#include <iomanip>
#include <type_traits>
#include <sstream>

namespace lava
{
	// General Utility
	struct decConvStream
	{
		std::stringstream buf;
		decConvStream();
	};
	struct hexConvStream
	{
		std::stringstream buf;
		hexConvStream();
	};
	struct fltConvStream
	{
		std::stringstream buf;
		fltConvStream();
	};
	int stringToNum(const std::string& stringIn, bool allowNeg = 1, int defaultVal = INT_MAX);
	template <typename numType>
	std::string numToHexStringWithPadding(numType numIn, unsigned char paddingLength)
	{
		static_assert(std::is_integral<numType>::value, "Type must be an integer primitve.");

		static hexConvStream conv;
		conv.buf.str("");
		conv.buf << std::setw(paddingLength) << numIn;
		return conv.buf.str();
	}
	template <typename numType>
	std::string numToDecStringWithPadding(numType numIn, unsigned char paddingLength)
	{
		static_assert(std::is_integral<numType>::value, "Template type must be a valid integer primitve.");

		static decConvStream conv;
		conv.buf.str("");
		conv.buf << std::setw(paddingLength) << numIn;
		return conv.buf.str();
	}
	std::string doubleToStringWithPadding(double dblIn, unsigned char paddingLength, unsigned long precisionIn = 2);
	std::string floatToStringWithPadding(float fltIn, unsigned char paddingLength, unsigned long precisionIn = 2);
}

#endif
