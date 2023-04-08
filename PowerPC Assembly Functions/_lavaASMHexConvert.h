#ifndef LAVA_ASM_HEX_CONVERT_V1_H
#define LAVA_ASM_HEX_CONVERT_V1_H

#include <functional>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <map>

namespace lava
{
	// lavaASMHexConvert v1.1.0 - A utility for converting 32-bit IBM PowerPC Assembly Instruction Hex into readable code.
	// Conventions and Concessions:
	// - This library presently supports only a subset of the 32-bit PPC ASM instruction set. It offers no support for 64-bit
	//		exclusive instructions.
	// - To simplify parsing, some opcode 31 integer arithmetic operations which use bit 21 to encode the OE flag have instead
	//		had their definitions split into two explicit operations, one with the "o" suffix, and one without. The variants
	//		using the "o" suffix have had 0b1000000000 ORed into their recorded secondary opcode so that they still parse correctly.
	//


	enum asmPrimaryOpCodes
	{
		aPOC_NULL = -1,
		aPOC_31 = 31,
		// Arithmetic Instructions
		aPOC_MULLI = 7,
		aPOC_ADDIC = 12,
		aPOC_ADDIC_DOT = 13,
		aPOC_ADDI = 14,
		aPOC_ADDIS = 15,
		aPOC_SUBFIC = 8,
		aPOC_FLOAT_D_ARTH = 63,
		aPOC_FLOAT_S_ARTH = 59,
		// Load Instructions
		aPOC_LWZ = 32,
		aPOC_LWZU = 33,
		aPOC_LBZ = 34,
		aPOC_LBZU = 35,
		aPOC_LHZ = 40,
		aPOC_LHZU = 41,
		aPOC_LFS = 48,
		aPOC_LFSU = 49,
		aPOC_LFD = 50,
		aPOC_LFDU = 51,
		// Store Instructions
		aPOC_STW = 36,
		aPOC_STWU = 37,
		aPOC_STB = 38,
		aPOC_STBU = 39,
		aPOC_STH = 44,
		aPOC_STHU = 45,
		aPOC_STFS = 52,
		aPOC_STFSU = 53,
		aPOC_STFD = 54,
		aPOC_STFDU = 55,
		// Logical Integer Instructions
		aPOC_ORI = 24,
		aPOC_ORIS = 25,
		aPOC_ANDI = 28,
		aPOC_ANDIS = 29,
	};
	unsigned long extractInstructionArg(unsigned long hexIn, unsigned char startBitIndex, unsigned char length);
	unsigned long getInstructionOpCode(unsigned long hexIn);


	struct asmInstruction;
	// Default function, just returns mneumonic.
	std::string defaultAsmInstrToStrFunc(asmInstruction* instructionIn, unsigned long hexIn);

	enum asmInstructionArgLayout
	{
		aIAL_NULL = -1,
		aIAL_IntADDI = 0,
		aIAL_IntORI,
		aIAL_IntLogical,
		aIAL_IntLoadStore,
		aIAL_Int2RegWithSIMM,
		aIAL_Int2RegWithUIMM,
		aIAL_Int3RegWithRC,
		aIAL_Int3RegSASwapWithRC,
		aIAL_Int2RegSASwapWithSHAndRC,
		aIAL_FltLoadStore,
		aIAL_Flt2RegOmitAWithRC,
		aIAL_Flt3RegOmitBWithRC,
		aIAL_Flt3RegOmitCWithRC,
		aIAL_LAYOUT_COUNT,
	};
	struct argumentLayout
	{
		asmInstructionArgLayout layoutID = aIAL_NULL;
		std::vector<unsigned char> argumentStartBits{};
		std::string(*conversionFunc)(asmInstruction*, unsigned long hexIn) = defaultAsmInstrToStrFunc;
		unsigned char secondaryOpCodeArgIndex = UCHAR_MAX;
		
		argumentLayout() {};

		std::vector<unsigned long> splitHexIntoArguments(unsigned long instructionHexIn);
	};
	extern std::array<argumentLayout, aIAL_LAYOUT_COUNT> layoutDictionary;
	argumentLayout* defineArgLayout(asmInstructionArgLayout IDIn, std::vector<unsigned char> argStartsIn, 
		std::string(*convFuncIn)(asmInstruction*, unsigned long) = defaultAsmInstrToStrFunc, unsigned char secOpCodeArgIndex = UCHAR_MAX);

	
	struct asmInstruction
	{
		asmPrimaryOpCodes primaryOpCode = aPOC_NULL;
		std::string name = "";
		std::string mneumonic = "";
		unsigned long canonForm = ULONG_MAX;
		unsigned short secondaryOpCode = USHRT_MAX;
		asmInstructionArgLayout layoutID = aIAL_NULL;

		asmInstruction() {};
		asmInstruction(asmPrimaryOpCodes prOpIn, std::string nameIn, std::string mneumIn, unsigned short secOpIn, unsigned long canonIn) :
			primaryOpCode(prOpIn), name(nameIn), mneumonic(mneumIn), secondaryOpCode(secOpIn), canonForm(canonIn) {};

		argumentLayout* getArgLayoutPtr();
	};
	struct asmPrOpCodeGroup
	{
		asmPrimaryOpCodes primaryOpCode = aPOC_NULL;
		unsigned char secondaryOpCodeStartBit = UCHAR_MAX;
		unsigned char secondaryOpCodeLength = UCHAR_MAX;
		std::map<unsigned short, asmInstruction> secondaryOpCodeToInstructions{};

		asmPrOpCodeGroup() {};

		asmInstruction* pushInstruction(std::string nameIn, std::string mneumIn, asmInstructionArgLayout layoutIDIn, unsigned short secOpIn = USHRT_MAX);
		asmInstruction* pushOverflowVersionOfInstruction(asmInstruction* originalInstrIn);
	};

	extern std::map<unsigned short, asmPrOpCodeGroup> instructionDictionary;
	asmPrOpCodeGroup* pushOpCodeGroupToDict(asmPrimaryOpCodes opCodeIn, unsigned char secOpCodeStart = UCHAR_MAX, unsigned char secOpCodeLength = UCHAR_MAX);
	void buildInstructionDictionary();

	std::string convertOperationHexToString(unsigned long hexIn);
}

#endif