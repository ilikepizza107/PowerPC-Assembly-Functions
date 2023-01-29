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
#include "_lavaOutputSplitter.h"
#include "_CSSRosterChange.h"
#include "_ThemeChange.h"
#include "_DashAttackItemGrab.h"
#include "_TripRateModifier.h"
//#include "FPS Display.h"
using namespace std;

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		for (unsigned long i = 1; i < argc && i < lava::argumentIDs::argumentCount; i++)
		{
			if (std::strcmp("-", argv[i]) != 0)
			{
				try
				{
					bool decisionVal = !(std::stoi(argv[i]) == 0);
					switch (i)
					{
						case lava::argumentIDs::aI_CMNU:
						{
							lava::CMNUCopyOverride = decisionVal;
							std::cout << "[C.ARG] Forcing CMNU decision to: ";
							break;
						}
						case lava::argumentIDs::aI_ASM:
						{
							lava::ASMCopyOverride = decisionVal;
							std::cout << "[C.ARG] Forcing ASM decision to: ";
							break;
						}
						case lava::argumentIDs::aI_GCT:
						{
							lava::GCTBuildOverride = decisionVal;
							std::cout << "[C.ARG] Forcing GCT decision to: ";
							break;
						}
						case lava::argumentIDs::aI_CLOSE:
						{
							lava::CloseOnFinishBypass = decisionVal;
							std::cout << "[C.ARG] Bypass push button to close?: ";
							break;
						}
						default:
						{
							break;
						}
					}
					if (decisionVal)
					{
						std::cout << "Yes\n";
					}
					else
					{
						std::cout << "No\n";
					}
				}
				catch (std::exception e)
				{
					std::cerr << "[ERROR] Invalid argument value (\"" << argv[i] << "\") provided. ";
					switch (i)
					{
						case lava::argumentIDs::aI_CMNU:
						{
							std::cerr << "CMNU";
							break;
						}
						case lava::argumentIDs::aI_ASM:
						{
							std::cerr << "ASM";
							break;
						}
						case lava::argumentIDs::aI_GCT:
						{
							std::cerr << "GCT";
							break;
						}
						case lava::argumentIDs::aI_CLOSE:
						{
							std::cerr << "Push button to close";
							break;
						}
						default:
						{
							break;
						}
					}
					std::cerr << " argument not processed!\n";
				}
			}
		}
	}

	std::cout << "PowerPC Assembly Functions (Code Menu Building Utility " << lava::version << ")\n";
	if (!std::filesystem::is_directory(outputFolder))
	{
		std::filesystem::create_directories(outputFolder);
	}
	if (std::filesystem::is_directory(outputFolder))
	{
		initMenuFileStream();
		string OutputTextPath = asmTextOutputFilePath;

		loadMenuOptionsTree(cmnuOptionsOutputFilePath, menuOptionsTree);
		applyCharacterListSettingFromMenuOptionsTree(menuOptionsTree);
		buildCharacterIDLists();
		buildRosterLists();
		buildThemeLists();

		std::shared_ptr<std::ofstream> soloLogFileOutput = std::make_shared<std::ofstream>(outputFolder + changelogFileName);
		*soloLogFileOutput << "PowerPC Assembly Functions (Code Menu Building Utility " << lava::version << ")\n";

		lava::outputSplitter logOutput;
		logOutput.setStandardOutputStream(lava::outputSplitter::sOS_COUT);
		logOutput.pushStream(lava::outputEntry(soloLogFileOutput, ULONG_MAX));

		logOutput << "Building \"" << cmnuFileName << "\" for ";
		switch (BUILD_TYPE)
		{
			case NORMAL:
			{
				logOutput << "LegacyTE";
				break;
			}
			case PMEX:
			{
				logOutput << "Project M EX";
				break;
			}
			case PROJECT_PLUS:
			{
				if (PROJECT_PLUS_EX_BUILD == true)
				{
					logOutput << "Project+ EX";
				}
				else
				{
					logOutput << "Project+";
				}
				break;
			}
			default:
			{
				logOutput << "Unknown";
				break;
			}
		}
		if (BUILD_NETPLAY_FILES == true)
		{
			logOutput << " Netplay";
		}
		if (DOLPHIN_BUILD == true)
		{
			logOutput << " (Dolphin)";
		}
		else
		{
			logOutput << " (Console)";
		}
		logOutput << "\n";
		if (DOLPHIN_BUILD == true)
		{
			logOutput << "Note: This code menu was configured for use with Dolphin only, and IS NOT COMPATIBLE with consoles!\n";
			logOutput << "\tAttempting to use this code menu on console can (and likely will) damage your system.\n";
		}

		if (TOURNAMENT_ADDITION_BUILD == true)
		{
			logOutput << "Note: Tournament Addition Flag is ON!\n";
		}
		if (IS_DEBUGGING == true)
		{
			logOutput << "Note: General Debug Flag is ON!\n";
		}
		if (EON_DEBUG_BUILD == true)
		{
			logOutput << "Note: Eon's Debug Flag is ON!\n";
		}

		logOutput << "\n";

		if (COLLECT_EXTERNAL_THEMES == true)
		{
			logOutput << "Adding Themes to Code Menu from \"" << themeInputFileName << "\"...\n";

			bool themeInputOpenedSuccesfully = 0;
			std::vector<menuTheme> tempThemeList = lava::collectThemesFromXML(themeInputFileName, themeInputOpenedSuccesfully);

			if (themeInputOpenedSuccesfully)
			{
				// If we actually retrieved any valid themes from the file, process them!
				if (!tempThemeList.empty())
				{
					// Create a map for quickly finding whether existing theme names are taken, and track their respective positions.
					std::map<std::string, std::size_t> themeNameToIndexMap{};
					// And a vector for storing the themes we've collected in order.
					std::vector<std::pair<std::string, menuTheme>> zippedThemeVec{};

					// Add all of our predefined themes to these containers before we process the newly collected ones. 
					for (std::size_t i = 0; i < THEME_LIST.size(); i++)
					{
						zippedThemeVec.push_back(std::make_pair(THEME_LIST[i], THEME_SPEC_LIST[i]));
						themeNameToIndexMap.insert(std::make_pair(THEME_LIST[i], i));
					}

					// For each newly collected theme...
					for (int i = 0; i < tempThemeList.size(); i++)
					{
						menuTheme* currTheme = &tempThemeList[i];

						// ... check to see if a theme of the same name already exists in our map.
						auto itr = themeNameToIndexMap.find(currTheme->name);

						// If one by that name doesn't already exist...
						if (itr == themeNameToIndexMap.end())
						{
							// ... add it to the map and vector so we can keep track of them going forward...
							themeNameToIndexMap.insert(std::make_pair(currTheme->name, i));
							zippedThemeVec.push_back(std::make_pair(currTheme->name, *currTheme));
							// ... and announce that a theme has been successfully collected.
							logOutput << "[ADDED]";
						}
						// Otherwise, if a theme by that name *does* already exist...
						else
						{
							// ... overwrite the theme currently associated with that name...
							zippedThemeVec[itr->second].second = *currTheme;
							// ... and announce that a theme has been changed.
							logOutput << "[CHANGED]";
						}
						// Describe the processed theme.
						logOutput << " \"" << currTheme->name << "\", Replacement Prefixes Are:\n";
						for (std::size_t u = 0; u < currTheme->prefixes.size(); u++)
						{
							logOutput << "\t\"" << themeConstants::filenames[u] << "\": \"" << currTheme->prefixes[u] << "\"\n";
						}
					}

					// Once we're done, clear out the old theme lists...
					THEME_LIST.clear();
					THEME_SPEC_LIST.clear();
					// ... and add re-populate them with the data we just processed.
					for (std::size_t i = 0; i < zippedThemeVec.size(); i++)
					{
						THEME_LIST.push_back(zippedThemeVec[i].first);
						THEME_SPEC_LIST.push_back(zippedThemeVec[i].second);
					}
				}
				else
				{
					logOutput << "[WARNING] \"" << themeInputFileName << "\" was opened successfully, but no valid theme entries could be found.\n";
				}
			}
			else
			{
				logOutput.setStandardOutputStream(lava::outputSplitter::sOS_CERR);
				logOutput << "[ERROR] Couldn't open \"" << themeInputFileName << "\"! Ensure that the file is present in this folder and try again!\n";
				logOutput.setStandardOutputStream(lava::outputSplitter::sOS_COUT);
			}
			//Print the results.
			logOutput << "\nFinal Theme List:\n";
			for (std::size_t i = 0; i < THEME_LIST.size(); i++)
			{
				logOutput << "\t\"" << THEME_LIST[i] << "\", Replacement Prefixes Are:\n";
				for (std::size_t u = 0; u < THEME_SPEC_LIST[i].prefixes.size(); u++)
				{
					logOutput << "\t\t\"" << themeConstants::filenames[u] << "\": \"" << THEME_SPEC_LIST[i].prefixes[u] << "\"\n";
				}
			}

			logOutput << "\n";
		}
		if (COLLECT_EXTERNAL_ROSTERS == true)
		{
			logOutput << "Adding Rosters to Code Menu from \"" << rosterInputFileName << "\"...\n";

			bool rosterInputOpenedSuccessfully = 0;
			std::vector<std::pair<std::string, std::string>> rosterNameFileNamePairs = lava::collectedRosterNamePathPairs(rosterInputFileName, rosterInputOpenedSuccessfully);
			if (rosterInputOpenedSuccessfully)
			{
				if (rosterNameFileNamePairs.size())
				{
					std::vector<std::pair<std::string, std::string>> zippedRosterVec{};
					std::map<std::string, std::size_t> rosterNameToIndexMap{};
					for (std::size_t i = 0; i < ROSTER_LIST.size(); i++)
					{
						zippedRosterVec.push_back(std::make_pair(ROSTER_LIST[i], ROSTER_FILENAME_LIST[i]));
						rosterNameToIndexMap.insert(std::make_pair(ROSTER_LIST[i], i));
					}
					for (int i = 0; i < rosterNameFileNamePairs.size(); i++)
					{
						std::pair<std::string, std::string>* currPair = &rosterNameFileNamePairs[i];
						if (currPair->second.size())
						{
							auto itr = rosterNameToIndexMap.find(currPair->first);
							if (itr == rosterNameToIndexMap.end())
							{
								zippedRosterVec.push_back(*currPair);
								rosterNameToIndexMap.insert(std::make_pair(currPair->first, zippedRosterVec.size() - 1));
								logOutput << "[ADDED] \"" << currPair->first << "\" (Filename: " << currPair->second << ")\n";
							}
							// Otherwise, announce what was changed.
							else
							{
								zippedRosterVec[itr->second].second = currPair->second;
								logOutput << "[CHANGED] \"" << itr->first << "\" (Filename: " << currPair->second << ")\n";
							}
						}
						else
						{
							logOutput.setStandardOutputStream(lava::outputSplitter::sOS_CERR);
							logOutput << "[ERROR] Invalid Filename specified! The roster \"" << currPair->first << "\" will not be added to the Code Menu!\n";
							logOutput.setStandardOutputStream(lava::outputSplitter::sOS_COUT);
						}
					}

					// Write the newly edited list back into the list vectors
					ROSTER_LIST.clear();
					ROSTER_FILENAME_LIST.clear();
					for (auto itr = zippedRosterVec.cbegin(); itr != zippedRosterVec.cend(); itr++)
					{
						ROSTER_LIST.push_back(itr->first);
						ROSTER_FILENAME_LIST.push_back(itr->second);
					}
				}
				else
				{
					logOutput << "[WARNING] \"" << rosterInputFileName << "\" was opened successfully, but no valid roster entries could be found.\n";
				}
			}
			else
			{
				logOutput.setStandardOutputStream(lava::outputSplitter::sOS_CERR);
				logOutput << "[ERROR] Couldn't open \"" << rosterInputFileName << "\"! Ensure that the file is present in this folder and try again!\n";
				logOutput.setStandardOutputStream(lava::outputSplitter::sOS_COUT);
			}
			//Print the results.
			logOutput << "\nFinal Roster List:\n";
			for (std::size_t i = 0; i < ROSTER_LIST.size(); i++)
			{
				logOutput << "\t\"" << ROSTER_LIST[i] << "\" (Filename: " << ROSTER_FILENAME_LIST[i] << ")\n";
			}

			logOutput << "\n";
		}
		if (COLLECT_EXTERNAL_EX_CHARACTERS == true)
		{
			logOutput << "Adding Characters to Code Menu from \"" << exCharInputFileName << "\"...\n";

			bool exCharInputOpenedSuccessfully = 0;
			std::vector<std::pair<std::string, u16>> nameIDPairs = lava::collectNameSlotIDPairs(exCharInputFileName, exCharInputOpenedSuccessfully);
			if (exCharInputOpenedSuccessfully)
			{
				if (nameIDPairs.size())
				{
					// Builds a map from the predefined character and character ID lists.
					// Doing it this way ensures that paired values stay together, and handles sorting automatically when we insert new entries.
					std::map<std::string, u16> zippedIDMap;
					for (int i = 0; i < CHARACTER_LIST.size(); i++)
					{
						zippedIDMap.insert(std::make_pair(CHARACTER_LIST[i], CHARACTER_ID_LIST[i]));
					}
					for (int i = 0; i < nameIDPairs.size(); i++)
					{
						std::pair<std::string, u16>* currPair = &nameIDPairs[i];
						if (currPair->second != SHRT_MAX)
						{
							auto itr = zippedIDMap.insert(*currPair);
							// If the entry was newly added to the list (ie. not overwriting existing data), announce it.
							if (itr.second)
							{
								logOutput << "[ADDED] \"" << itr.first->first << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(itr.first->second, 2) << ")\n";
							}
							// Otherwise, announce what was changed.
							else if (itr.first != zippedIDMap.end())
							{
								itr.first->second = currPair->second;
								logOutput << "[CHANGED] \"" << itr.first->first << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(itr.first->second, 2) << ")\n";
							}
						}
						else
						{
							logOutput.setStandardOutputStream(lava::outputSplitter::sOS_CERR);
							logOutput << "[ERROR] Invalid Slot ID specified! The character \"" << currPair->first << "\" will not be added to the Code Menu!\n";
							logOutput.setStandardOutputStream(lava::outputSplitter::sOS_COUT);
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
				else
				{
					logOutput << "[WARNING] \"" << exCharInputFileName << "\" was opened successfully, but no valid character entries could be found.\n";
				}
			}
			else
			{
				logOutput.setStandardOutputStream(lava::outputSplitter::sOS_CERR);
				logOutput << "[ERROR] Couldn't open \"" << exCharInputFileName << "\"! Ensure that the file is present in this folder and try again!\n";
				logOutput.setStandardOutputStream(lava::outputSplitter::sOS_COUT);
			}
			//Print the results.
			logOutput << "\nFinal Character List:\n";
			for (std::size_t i = 0; i < CHARACTER_LIST.size(); i++)
			{
				logOutput << "\t\"" << CHARACTER_LIST[i] << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(CHARACTER_ID_LIST[i], 2) << ")\n";
			}

			logOutput << "\n";
		}
		

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

		CodeMenu(); tagBasedCostumes(); cssRosterChange(); themeChange(); dashAttackItemGrab(); tripRateModifier();

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

		if (std::filesystem::is_regular_file(cmnuBuildLocationFilePath))
		{
			if (lava::offerCopyOverAndBackup(cmnuOutputFilePath, cmnuBuildLocationFilePath, lava::CMNUCopyOverride))
			{
				logOutput << "Note: Backed up \"" << cmnuBuildLocationFilePath << "\" and overwrote it with the newly built Code Menu.\n";
			}
		}
		else if (std::filesystem::is_directory(buildFolder + cmnuBuildLocationDirectory))
		{
			if (lava::offerCopy(cmnuOutputFilePath, cmnuBuildLocationFilePath, lava::CMNUCopyOverride))
			{
				logOutput << "Note: Copied newly built Code Menu to \"" << cmnuBuildLocationFilePath << "\".\n";
			}
		}
		if (MakeASM(OutputTextPath, asmOutputFilePath))
		{
			if (std::filesystem::is_regular_file(asmBuildLocationFilePath))
			{
				if (lava::offerCopyOverAndBackup(asmOutputFilePath, asmBuildLocationFilePath, lava::ASMCopyOverride))
				{
					logOutput << "Note: Backed up \"" << asmBuildLocationFilePath << "\" and overwrote it with the newly built ASM.\n";
				}
			}
			else if (std::filesystem::is_directory(buildFolder + asmBuildLocationDirectory))
			{
				if (lava::offerCopy(asmOutputFilePath, asmBuildLocationFilePath, lava::ASMCopyOverride))
				{
					logOutput << "Note: Copied newly built ASM to \"" << asmBuildLocationFilePath << "\".\n";
				}
			}
			if (lava::handleAutoGCTRMProcess(*soloLogFileOutput, lava::GCTBuildOverride) && BUILD_NETPLAY_FILES)
			{
				logOutput << "Note: The built GCTs are configured for use in Dolphin Netplay only, and ARE NOT COMPATIBLE with consoles!\n";
				logOutput << "Attempting to use them on console can (and likely will) damage your system.\n\n";
			}
		}
	}
	else
	{
		std::cerr << "[ERROR] The expected output folder (\"" << outputFolder << "\") couldn't be found or created in this folder. Ensure that the specified folder exists in the same folder as this program and try again.\n\n";
	}
	if (lava::CloseOnFinishBypass == INT_MAX || lava::CloseOnFinishBypass == 0)
	{
		std::cout << "Press any key to exit.\n";
		_getch();
	}
	return 0;
}
