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
#include "_CSSRosterChange.h"
#include "_ThemeChange.h"
#include "_DashAttackItemGrab.h"
#include "_TripRateModifier.h"
#include "_PlayerSlotColorChangers.h"
#include "_JumpsquatOverride.h"
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

		loadMenuOptionsTree(cmnuOptionsOutputFilePath, menuOptionsTree);
		buildCharacterIDLists();
		buildRosterLists();
		buildThemeLists();

		ChangelogOutput.write(
			"PowerPC Assembly Functions (Code Menu Building Utility " + lava::version + ")\n", ULONG_MAX, lava::outputSplitter::sOS_NULL);

		ChangelogOutput << "Building \"" << cmnuFileName << "\" for ";
		switch (BUILD_TYPE)
		{
			case NORMAL:
			{
				ChangelogOutput << "LegacyTE";
				break;
			}
			case PMEX:
			{
				ChangelogOutput << "Project M EX";
				break;
			}
			case PROJECT_PLUS:
			{
				if (PROJECT_PLUS_EX_BUILD == true)
				{
					ChangelogOutput << "Project+ EX";
				}
				else
				{
					ChangelogOutput << "Project+";
				}
				break;
			}
			default:
			{
				ChangelogOutput << "Unknown";
				break;
			}
		}
		if (BUILD_NETPLAY_FILES == true)
		{
			ChangelogOutput << " Netplay";
		}
		if (DOLPHIN_BUILD == true)
		{
			ChangelogOutput << " (Dolphin)";
		}
		else
		{
			ChangelogOutput << " (Offline)";
		}
		ChangelogOutput << "\n";
		if (DOLPHIN_BUILD == true)
		{
			ChangelogOutput << "Note: This code menu was configured for use with Dolphin only, and IS NOT COMPATIBLE with consoles!\n";
			ChangelogOutput << "\tAttempting to use this code menu on console can (and likely will) damage your system.\n";
		}

		if (TOURNAMENT_ADDITION_BUILD == true)
		{
			ChangelogOutput << "Note: Tournament Addition Flag is ON!\n";
		}
		if (IS_DEBUGGING == true)
		{
			ChangelogOutput << "Note: General Debug Flag is ON!\n";
		}
		if (EON_DEBUG_BUILD == true)
		{
			ChangelogOutput << "Note: Eon's Debug Flag is ON!\n";
		}

		// If we're building in netplay mode, we'll try to parse using the netplay-specific config file.
		bool parsedConfigXML = (BUILD_NETPLAY_FILES && lava::parseAndApplyConfigXML(netMenuConfigXMLFileName, ChangelogOutput));
		// If we don't parse the netplay config (either because we're building the offline menu or cuz it didn't exist)...
		if (!parsedConfigXML)
		{
			// try to parse the offline config file.
			parsedConfigXML = lava::parseAndApplyConfigXML(menuConfigXMLFileName, ChangelogOutput);
		}
		// And if we couldn't parse that either...
		if (!parsedConfigXML)
		{
			ChangelogOutput.write("[WARNING] Failed to parse config XML! Proceeding with default settings.\n", ULONG_MAX, lava::outputSplitter::sOS_CERR);
		}

		CodeStart(asmTextOutputFilePath);
		ChangelogOutput << "\n";

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
		
		cssRosterChange(); themeChange(); 

		//playerSlotColorChangers(CONFIG_BACKPLATE_COLOR_MODE);

		playerSlotColorChangersV3(CONFIG_PSCC_ENABLED);

		dashAttackItemGrab(CONFIG_DASH_ATTACK_ITEM_GRAB_ENABLED);

		jumpsquatOverride(CONFIG_JUMPSQUAT_OVERRIDE_ENABLED);

		// tripRateModifier();

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
				ChangelogOutput << "Note: Backed up \"" << cmnuBuildLocationFilePath << "\" and overwrote it with the newly built Code Menu.\n";
			}
		}
		else if (std::filesystem::is_directory(buildFolder + cmnuBuildLocationDirectory))
		{
			if (lava::offerCopy(cmnuOutputFilePath, cmnuBuildLocationFilePath, lava::CMNUCopyOverride))
			{
				ChangelogOutput << "Note: Copied newly built Code Menu to \"" << cmnuBuildLocationFilePath << "\".\n";
			}
		}

		// Initialize dictionaries and variables for ASM output.
		lava::ppc::buildInstructionDictionary();
		lava::gecko::buildGeckoCodeDictionary();
		if (!CONFIG_DISABLE_ASM_DISASSEMBLY && std::filesystem::is_regular_file(symbolMapInputFileName))
		{
			ChangelogOutput << "\nSymbol map file detected! Parsing \"" << symbolMapInputFileName << "\"... ";
			if (lava::ppc::parseMapFile(symbolMapInputFileName))
			{
				ChangelogOutput << "Success!\n";
			}
			else
			{
				ChangelogOutput.write("Failure!\n", ULONG_MAX, lava::outputSplitter::sOS_CERR);
			}
		}
		if (CONFIG_OUTPUT_ASM_INSTRUCTION_DICTIONARY)
		{
			lava::ppc::summarizeInstructionDictionary(outputFolder + "ASMDictionary.txt");
		}
		// Handle ASM output.
		ChangelogOutput << "\nWriting ASM file... ";
		if (MakeASM(asmTextOutputFilePath, asmOutputFilePath, CONFIG_DISABLE_ASM_DISASSEMBLY))
		{
			ChangelogOutput << "Success!\n";
			if (lava::placeASMInBuild(*ChangelogOutput.getChangelogPtr()))
			{
				if (lava::handleAutoGCTRMProcess(*ChangelogOutput.getChangelogPtr()) && BUILD_NETPLAY_FILES)
				{
					ChangelogOutput << "Note: The built GCTs are configured for use in Dolphin Netplay only, and ARE NOT COMPATIBLE with consoles!\n";
					ChangelogOutput << "Attempting to use them on console can (and likely will) damage your system.\n\n";
				}
			}
		}
		else
		{
			ChangelogOutput.write("Failure!\n", ULONG_MAX, lava::outputSplitter::stdOutStreamEnum::sOS_CERR);
		}
	}
	else
	{
		std::cerr << "[ERROR] The expected output folder (\"" << outputFolder << "\") couldn't be found or created in this folder. Ensure that the specified folder exists in the same folder as this program and try again.\n\n";
	}
	if (lava::CloseOnFinishBypass == INT_MAX || lava::CloseOnFinishBypass == 0)
	{
		std::cout << "\nPress any key to exit.\n";
		_getch();
	}
	return 0;
}
