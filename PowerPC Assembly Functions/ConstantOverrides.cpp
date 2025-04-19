#include "stdafx.h"
#include "ConstantOverrides.h"

bool operator<(const ConstantPair& lhs, const ConstantPair& rhs)
{
	return lhs.address < rhs.address;
}