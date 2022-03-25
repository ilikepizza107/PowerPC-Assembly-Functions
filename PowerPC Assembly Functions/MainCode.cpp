#include "stdafx.h"
#include "ReplayFix.h"
#include "Controls.h"
#include "StopStartAlts.h"
#include "Save States.h"
#include "Miscellaneous Code.h"
#include "StopDPadOnSSS (except Wiimote).h"
#include "DrawDI.h"
#include "Code Menu.h"
#include "IASA Overlay.h"
#include "Control Codes.h"
#include "Last Character Auto Select.h"
#include "Tag Based Costumes.h"
#include "Light Shield.h"
#include "IkeClimbers.h"
#include "AIDisplay.h"
#include "C++Injection.h"
#include "_AdditionalCode.h"
//#include "FPS Display.h"
using namespace std;

int main()
{
	std::cout << "PowerPC Assembly Functions (Code Menu Building Utility)\n";
	if (lava::folderExists(outputFolder))
	{
		initMenuFileStream();
		string OutputTextPath = asmTextFilePath;

		std::ofstream ppexOut;
		ppexOut.open(outputFolder + changelogFileName);
		ppexOut << "PowerPC Assembly Functions (Code Menu Building Utility)\n";
		ppexOut << "Current Menu Config: ";
		std::cout << "Current Menu Config: ";
		switch (BUILD_TYPE)
		{
			case NORMAL:
			{
				ppexOut << "LegacyTE";
				std::cout << "LegacyTE";
				break;
			}
			case PMEX:
			{
				ppexOut << "Project M EX";
				std::cout << "Project M EX";
				break;
			}
			case PROJECT_PLUS:
			{
				if (PROJECT_PLUS_EX_BUILD == true)
				{
					ppexOut << "Project+ EX";
					std::cout << "Project+ EX";
				}
				else
				{
					ppexOut << "Project+";
					std::cout << "Project+";
				}
				break;
			}
			default:
			{
				ppexOut << "Unknown";
				std::cout << "Unknown";
				break;
			}
		}
		if (DOLPHIN_BUILD == true)
		{
			ppexOut << " (Dolphin/Netplay)";
			std::cout << " (Dolphin/Netplay)";
		}
		ppexOut << "\n";
		std::cout << "\n";
		if (TOURNAMENT_ADDITION_BUILD == true)
		{
			ppexOut << "Note: Tournament Addition Flag is ON!\n";
			std::cout << "Note: Tournament Addition Flag is ON!\n";
		}
		if (IS_DEBUGGING == true)
		{
			ppexOut << "Note: General Debug Flag is ON!\n";
			std::cout << "Note: General Debug Flag is ON!\n";
		}
		if (EON_DEBUG_BUILD == true)
		{
			ppexOut << "Note: Eon's Debug Flag is ON!\n";
			std::cout << "Note: Eon's Debug Flag is ON!\n";
		}

		ppexOut << "\n";
		std::cout << "\n";

#if PROJECT_PLUS_EX_BUILD == true
		ppexOut << "Adding Characters to Code Menu from \"" << exCharInputFilename << "\"...\n";
		std::cout << "Adding Characters to Code Menu from \"" << exCharInputFilename << "\"...\n";
		// Builds a map from the predefined character and character ID lists.
		// Doing it this way ensures that paired values stay together, and handles sorting automatically when we insert new entries.
		std::map<std::string, u16> zippedIDMap;
		for (int i = 0; i < CHARACTER_LIST.size(); i++)
		{
			zippedIDMap.insert(std::make_pair(CHARACTER_LIST[i], CHARACTER_ID_LIST[i]));
		}

		// Read from character file
		std::ifstream ppexIn;
		ppexIn.open(exCharInputFilename);
		// Initiate changelog file
		
		if (ppexIn.is_open())
		{
			std::string currentLine = "";
			std::string manipStr = "";
			unsigned long validEntryCount = 0;
			while (std::getline(ppexIn, currentLine))
			{
				// Disregard the current line if it's empty, or is marked as a comment
				if (!currentLine.empty() && currentLine[0] != '#' && currentLine[0] != '/')
				{
					// Clean the string
					// Removes any space characters from outside of quotes. Note, quotes can be escaped with \\.
					manipStr = "";
					bool inQuote = 0;
					bool doEscapeChar = 0;
					for (std::size_t i = 0; i < currentLine.size(); i++)
					{
						if (currentLine[i] == '\"' && !doEscapeChar)
						{
							inQuote = !inQuote;
						}
						else if (currentLine[i] == '\\')
						{
							doEscapeChar = 1;
						}
						else if (inQuote || !std::isspace(currentLine[i]))
						{
							doEscapeChar = 0;
							manipStr += currentLine[i];
						}
					}

					// Determines the location of the delimiter, and ensures that there's something before and something after it.
					// Line is obviously invalid if that fails, so we skip it.
					std::size_t delimLoc = manipStr.find('=');
					if (delimLoc != std::string::npos && delimLoc > 0 && delimLoc < (manipStr.size() - 1))
					{
						// Store character name portion of string
						std::string characterNameIn = manipStr.substr(0, delimLoc);
						// Initialize var for character id portion of string
						u16 characterSlotIDIn = SHRT_MAX;
						// Handles hex input for character id
						characterSlotIDIn = lava::stringToNum(manipStr.substr(delimLoc + 1, std::string::npos), 1, SHRT_MAX);
						if (characterSlotIDIn != SHRT_MAX)
						{
							validEntryCount++;
							// Insert new entry into list.
							auto itr = zippedIDMap.insert(std::make_pair(characterNameIn, characterSlotIDIn));
							// If the entry was newly added to the list (ie. not overwriting existing data), announce it.
							if (itr.second)
							{
								std::cout << "[ADDED] " << itr.first->first << " (Slot ID = 0x" << lava::numToHexStringWithPadding(itr.first->second, 2) << ")\n";
								ppexOut << "[ADDED] " << itr.first->first << " (Slot ID = 0x" << lava::numToHexStringWithPadding(itr.first->second, 2) << ")\n";
							}
							// Otherwise, announce what was changed.
							else if (itr.first != zippedIDMap.end())
							{
								itr.first->second = characterSlotIDIn;
								std::cout << "[CHANGED] " << itr.first->first << " (Slot ID = 0x" << lava::numToHexStringWithPadding(itr.first->second, 2) << ")\n";
								ppexOut << "[CHANGED] " << itr.first->first << " (Slot ID = 0x" << lava::numToHexStringWithPadding(itr.first->second, 2) << ")\n";
							}
						}
						else
						{
							std::cerr << "[ERROR] Invalid Slot ID specified! The character \"" << characterNameIn << "\" will not be added to the Code Menu!\n";
							ppexOut << "[ERROR] Invalid Slot ID specified! The character \"" << characterNameIn << "\" will not be added to the Code Menu!\n";
						}
					}
				}
			}
			if (validEntryCount == 0)
			{
				std::cout << "[WARNING] \"" << exCharInputFilename << "\" was opened successfully, but no valid entries could be found.\n";
				ppexOut << "[WARNING] \"" << exCharInputFilename << "\" was opened successfully, but no valid entries could be found.\n";
			}

			// Write the newly edited list back into the list vectors
			CHARACTER_LIST.clear();
			CHARACTER_ID_LIST.clear();
			for (auto itr = zippedIDMap.begin(); itr != zippedIDMap.end(); itr++)
			{
				CHARACTER_LIST.push_back(itr->first);
				CHARACTER_ID_LIST.push_back(itr->second);
			}
		}
		else
		{
			std::cout << "[ERROR] Couldn't open \"" << exCharInputFilename << "\"! Ensure that the file is present in this folder and try again!\n";
			ppexOut << "[ERROR] Couldn't open \"" << exCharInputFilename << "\"! Ensure that the file is present in this folder and try again!\n";
		}
		// Print the results.
		std::cout << "\nFinal Character List:\n";
		ppexOut << "\nFinal Character List:\n";
		for (std::size_t i = 0; i != CHARACTER_LIST.size(); i++)
		{
			std::cout << "\t" << CHARACTER_LIST[i] << " (Slot ID = 0x" << lava::numToHexStringWithPadding(CHARACTER_ID_LIST[i], 2) << ")\n";
			ppexOut << "\t" << CHARACTER_LIST[i] << " (Slot ID = 0x" << lava::numToHexStringWithPadding(CHARACTER_ID_LIST[i], 2) << ")\n";
		}

		std::cout << "\n";
		ppexOut << "\n";

		// Close the changelog and character files.
		ppexIn.close();
#endif

		CodeStart(OutputTextPath);
		//place all ASM code here

		//ReplayFix();

		//NameIsFound();

		//MenuControlCodes();

		//StopStartAltFunctions();

		//StopPokemonTrainerSwitch();

		//StopDPadOnSSS();

		//ConvertButtons();

		//ItemSpawnControl();

		//ClearASLData();

		//SetTeamAttackTraining();

		//LXPGreenOverlayFix();

		CodeMenu(); tagBasedCostumes();

		//musicPercentCode();

		//DoubleFighterTest();

		//UCF();

		//CStickSlowFix();

		//FixStickyRAlts();

		//SelectLastCharacter();

		//FixTr4shTeamToggle();

		//cstickTiltTest();

		//FPSDisplay();

		//CStickTiltFix();

		//DBZModeTest();

		//slipperyTechs();

		//lightShield();

		//IkeClimbers();

		//fixStanimaTextBug();

		//AIDisplay();

		//loadCppCodes(); writeInjectionsRepeat();

		CodeEnd();

		std::cout << "\n";
		if (lava::offerCopyOverAndBackup(cmnuFilePath, cmnuFileAutoReplacePath))
		{
			ppexOut << "Note: Backed up \"" << cmnuFileAutoReplacePath << "\" and overwrote it with the newly built Code Menu.\n";
		}
		if (MakeASM(OutputTextPath, asmFilePath))
		{
			if (lava::offerCopyOverAndBackup(asmFilePath, asmFileAutoReplacePath))
			{
				ppexOut << "Note: Backed up \"" << asmFileAutoReplacePath << "\" and overwrote it with the newly built ASM.\n";
			}
			lava::handleAutoGCTRMProcess(ppexOut);
		}
	}
	else
	{
		std::cerr << "[ERROR] The expected output folder (\"" << outputFolder << "\") couldn't be found in this folder. Ensure that the specified folder exists and try again.\n\n";
	}
	std::cout << "Press any key to exit.\n";
	_getch();
	return 0;
}
