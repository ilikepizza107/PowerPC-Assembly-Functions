#ifndef CONSTANT_OVERRIDES_H
#define CONSTANT_OVERRIDES_H

struct ConstantPair {
	enum dataSize
	{
		ds_WORD = 0,
		ds_HALF_A,
		ds_HALF_B,
		ds_BYTE_A,
		ds_BYTE_B,
		ds_BYTE_C,
		ds_BYTE_D,
		ds__COUNT,
	};

	int address;
	int* index;
	dataSize writeSize;

	ConstantPair(int address, int& index, dataSize writeSizeIn = ds_WORD) : address(address), index(&index), writeSize(writeSizeIn) {}
};

bool operator<(const ConstantPair& lhs, const ConstantPair& rhs);

#endif