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
//#include "FPS Display.h"
using namespace std;

int main()
{
	string OutputTextPath = ".\\ASM.txt";

#ifdef RIDLEY
	// Builds a map from the predefined character and character ID lists.
	// Doing it this way ensures that paired values stay together, and handles sorting automatically when we insert new entries.
	std::map<std::string, u16> zippedIDMap;
	for (int i = 0; i < CHARACTER_LIST.size(); i++)
	{
		zippedIDMap.insert(std::make_pair(CHARACTER_LIST[i], CHARACTER_ID_LIST[i]));
	}

	// Read from character file
	std::ifstream ppexIn;
	ppexIn.open("EX_Characters.txt");
	// Initiate changelog file
	std::ofstream ppexOut;
	ppexOut.open("EX_Characters_Changelog.txt");
	if (ppexIn.is_open())
	{
		std::string currentLine = "";
		std::string manipStr = "";
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
					std::string newChar = manipStr.substr(0, delimLoc);
					// Initialize var for character id portion of string
					u16 newID;
					// Handles hex input for character id
					if (manipStr.find("0x", delimLoc + 1) == delimLoc + 1)
					{
						newID = std::stoul(manipStr.substr(delimLoc + 1, std::string::npos), nullptr, 16);
					}
					// Handles dec input for character id. If this fails the program should abort.
					else
					{
						newID = std::stoul(manipStr.substr(delimLoc + 1, std::string::npos));
					}
					// Insert new entry into list.
					auto itr = zippedIDMap.insert(std::make_pair(newChar, newID));
					// If the entry was newly added to the list (ie. not overwriting existing data), announce it.
					if (itr.second)
					{
						std::cout << "[ADDED] " << itr.first->first << " (ID = " << itr.first->second << ")\n";
						ppexOut << "[ADDED] " << itr.first->first << " (ID = " << itr.first->second << ")\n";
					}
					// Otherwise, announce what was changed.
					else if (itr.first != zippedIDMap.end())
					{
						itr.first->second = newID;
						std::cout << "[CHANGED] " << itr.first->first << " (ID = " << itr.first->second << ")\n";
						ppexOut << "[CHANGED] " << itr.first->first << " (ID = " << itr.first->second << ")\n";
					}
				}
			}
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
	// Print the results.
	std::cout << "Char List:\n";
	ppexOut << "Char List:\n";
	for (std::size_t i = 0; i != CHARACTER_LIST.size(); i++)
	{
		std::cout << "\t" << CHARACTER_LIST[i] << " (ID: " << CHARACTER_ID_LIST[i] << ")\n";
		ppexOut << "\t" << CHARACTER_LIST[i] << " (ID: " << CHARACTER_ID_LIST[i] << ")\n";
	}
	std::cout << "\n";

	// Close the changelog and character files.
	ppexIn.close();
	ppexOut.close();

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
}