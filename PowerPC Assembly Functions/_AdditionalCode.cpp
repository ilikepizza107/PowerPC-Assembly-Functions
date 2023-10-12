#include "stdafx.h"
#include "_AdditionalCode.h"

namespace lava
{
	int CMNUCopyOverride = INT_MAX;
	int ASMCopyOverride = INT_MAX;
	int GCTBuildOverride = INT_MAX;
	int CloseOnFinishBypass = INT_MAX;

	bool copyFile(std::filesystem::path sourceFile, std::filesystem::path targetFile, bool overwriteExistingFile)
	{
		// Record result
		bool result = 0;
		// If the incoming paths don't point to the same file...
		if (sourceFile != targetFile)
		{
			result = std::filesystem::copy_file(sourceFile, targetFile, (overwriteExistingFile) ? 
				std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::skip_existing);
		}
		return result;
	}
	bool backupFile(std::string fileToBackup, std::string backupSuffix, bool overwriteExistingBackup)
	{
		return copyFile(fileToBackup, fileToBackup + backupSuffix, overwriteExistingBackup);
	}

	bool yesNoDecision(char yesKey, char noKey)
	{
		char keyIn = ' ';
		yesKey = std::tolower(yesKey);
		noKey = std::tolower(noKey);
		while (keyIn != yesKey && keyIn != noKey)
		{
			keyIn = _getch();
			keyIn = std::tolower(keyIn);
		}
		return (keyIn == yesKey);
	}
	bool offerCopyOverAndBackup(std::string fileToCopy, std::string fileToOverwrite, int decisionOverride)
	{
		bool backupSucceeded = 0;
		bool copySucceeded = 0;

		if (std::filesystem::is_regular_file(fileToCopy) && std::filesystem::is_regular_file(fileToOverwrite))
		{
			std::cout << "\nDetected \"" << fileToOverwrite << "\".\n" <<
				"Would you like to copy \"" << fileToCopy << "\" over it? " <<
				"A backup will be made of the existing file.\n";
			std::cout << "[Press 'Y' for Yes, 'N' for No]\n";
			if ((decisionOverride == INT_MAX && yesNoDecision('y', 'n')) || (decisionOverride != INT_MAX && decisionOverride != 0))
			{
				std::cout << "Making backup... ";
				if (lava::backupFile(fileToOverwrite, ".bak", 1))
				{
					backupSucceeded = 1;
					std::cout << "Successfully backed up file to \"" << fileToOverwrite << ".bak\"!\n";
					if (lava::backupFile(fileToOverwrite, ".orig", 0))
					{
						std::cout << "Note: Did extra backup of file's original state to \"" << fileToOverwrite << ".orig\"!\n";
					}

					std::cout << "Copying over \"" << fileToCopy << "\"... ";
					if (lava::copyFile(fileToCopy, fileToOverwrite, 1))
					{
						copySucceeded = 1;
						std::cout << "Success!\n";
					}
					else
					{
						std::cerr << "Failure! Please ensure that the destination file is able to be written to!\n";
					}
				}
				else
				{
					std::cerr << "Backup failed! Please ensure that \"" << fileToOverwrite << ".bak\" is able to be written to!\n";
				}
			}
			else
			{
				std::cout << "Skipping copy.\n";
			}
		}

		return backupSucceeded && copySucceeded;
	}
	bool offerCopy(std::string fileToCopy, std::string fileToOverwrite, int decisionOverride)
	{
		bool copySucceeded = 0;

		if (std::filesystem::is_regular_file(fileToCopy) && !std::filesystem::is_regular_file(fileToOverwrite))
		{
			std::cout << "\nCouldn't detect \"" << fileToOverwrite << "\".\n" << "Would you like to copy \"" << fileToCopy << "\" to that location?\n";
			std::cout << "[Press 'Y' for Yes, 'N' for No]\n";
			if ((decisionOverride == INT_MAX && yesNoDecision('y', 'n')) || (decisionOverride != INT_MAX && decisionOverride != 0))
			{
				std::cout << "Copying over \"" << fileToCopy << "\"... ";
				if (lava::copyFile(fileToCopy, fileToOverwrite, 1))
				{
					copySucceeded = 1;
					std::cout << "Success!\n";
				}
				else
				{
					std::cerr << "Failure! Please ensure that the destination file is able to be written to!\n";
				}
			}
			else
			{
				std::cout << "Skipping copy.\n";
			}
			std::cout << "\n";
		}

		return copySucceeded;
	}
	bool handleAutoGCTRMProcess(std::ostream& logOutput, int decisionOverride)
	{
		bool result = 0;

		if (std::filesystem::is_regular_file(GCTRMExePath) && std::filesystem::is_regular_file(mainGCTTextFile) && std::filesystem::is_regular_file(boostGCTTextFile))
		{
			std::cout << "\nDetected \"" << GCTRMExePath << "\".\nWould you like to build \"" << mainGCTFile << "\" and \"" << boostGCTFile << "\"? Backups will be made of any existing files.\n";

			bool mainGCTBackupNeeded = std::filesystem::is_regular_file(mainGCTFile);
			// If no backup is needed, we can consider the backup resolved. If one is, we cannot.
			bool mainGCTBackupResolved = !mainGCTBackupNeeded;
			// Same as above.
			bool boostGCTBackupNeeded = std::filesystem::is_regular_file(boostGCTFile);
			bool boostGCTBackupResolved = !boostGCTBackupNeeded;

			std::cout << "[Press 'Y' for Yes, 'N' for No]\n";
			if ((decisionOverride == INT_MAX && yesNoDecision('y', 'n')) || (decisionOverride != INT_MAX && decisionOverride != 0))
			{
				if (mainGCTBackupNeeded || boostGCTBackupNeeded)
				{
					std::cout << "Backing up files... ";
					if (mainGCTBackupNeeded)
					{
						mainGCTBackupResolved = lava::backupFile(mainGCTFile, ".bak", 1);
					}
					if (boostGCTBackupNeeded)
					{
						boostGCTBackupResolved = lava::backupFile(boostGCTFile, ".bak", 1);
					}
				}
				if (mainGCTBackupResolved && boostGCTBackupResolved)
				{
					std::cout << "Success! Running GCTRM:\n";
					result = 1;
					std::string commandFull = "\"" + GCTRMCommandBase + "\"" + mainGCTTextFile + "\"\"";
					std::cout << "\n" << commandFull << "\n";
					system(commandFull.c_str());
					if (mainGCTBackupNeeded)
					{
						logOutput << "Note: Backed up and rebuilt \"" << mainGCTFile << "\".\n";
					}
					else
					{
						logOutput << "Note: Built \"" << mainGCTFile << "\".\n";
					}

					commandFull = "\"" + GCTRMCommandBase + "\"" + boostGCTTextFile + "\"\"";
					std::cout << "\n" << commandFull << "\n";
					system(commandFull.c_str());
					if (boostGCTBackupNeeded)
					{
						logOutput << "Note: Backed up and rebuilt \"" << boostGCTFile << "\".\n";
					}
					else
					{
						logOutput << "Note: Built \"" << boostGCTFile << "\".";
					}
					std::cout << "\n";
				}
				else
				{
					std::cerr << "Something went wrong while backing up the files. Skipping GCTRM.\n";
				}
			}
			else
			{
				std::cout << "Skipping GCTRM.\n";
			}
		}
		return result;
	}

	void WriteByteVec(const unsigned char* Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite, bool appendNullTerminator)
	{
		if (Address != ULONG_MAX)
		{
			SetRegister(addressReg, Address); // Load destination address into register
		}

		unsigned long fullWordCount = numToWrite / 4; // Get the number of 4 byte words we can make from the given vec.
		unsigned long currWord = 0;
		unsigned long offsetIntoBytes = 0;

		// For each word we can make...
		for (unsigned long i = 0; i < fullWordCount; i++)
		{
			// ... grab it, then store it in our manip register.
			currWord = lava::bytesToFundamental<unsigned long>(Bytes + offsetIntoBytes);
			SetRegister(manipReg, currWord);
			// Then write a command which stores the packed word into the desired location.
			STW(manipReg, addressReg, offsetIntoBytes);
			offsetIntoBytes += 0x04;
		}
		// For the remaining few bytes...
		for (offsetIntoBytes; offsetIntoBytes < numToWrite; offsetIntoBytes++)
		{
			// ... just write them one by one, no other choice really.
			SetRegister(manipReg, Bytes[offsetIntoBytes]);
			STB(manipReg, addressReg, offsetIntoBytes);
		}
		if (appendNullTerminator)
		{
			SetRegister(manipReg, 0x00);
			STB(manipReg, addressReg, offsetIntoBytes);
		}
	}
	void WriteByteVec(std::vector<unsigned char> Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite, bool appendNullTerminator)
	{
		WriteByteVec((const unsigned char*)Bytes.data(), Address, addressReg, manipReg, std::min<std::size_t>(Bytes.size(), numToWrite), appendNullTerminator);
	}
	void WriteByteVec(std::string Bytes, u32 Address, unsigned char addressReg, unsigned char manipReg, std::size_t numToWrite, bool appendNullTerminator)
	{
		WriteByteVec((const unsigned char*)Bytes.data(), Address, addressReg, manipReg, std::min<std::size_t>(Bytes.size(), numToWrite), appendNullTerminator);
	}


	// ==================== Menu Config Parsing and Constants ====================

	// Config XML Parsing Constants
	namespace configXMLConstants
	{
		// Debug
		const std::string _debugOptionsTag = "debugOptions";
		const std::string _implicitOptimizationsTag = "allowImplicitOptimizations";
		const std::string _allowBLAFuncCalls = "allowBLAFuncCalls";
		const std::string _deleteASMTxtTag = "deleteASMTxt";
		const std::string _asmDictTag = "makeASMDictionary";
		const std::string _disableDisassemmblerTag = "disableDisassembler";
		const std::string _enableASMHexCommentsTag = "enableASMHexComments";

		// General
		const std::string menuConfigTag = "codeMenuConfig";
		const std::string enabledTag = "enabled";
		const std::string nameTag = "name";
		const std::string textTag = "text";
		const std::string filenameTag = "filename";
		const std::string codeModeTag = "codeMode";

		// Menu Properties
		const std::string menuPropsTag = "menuProperties";
		const std::string baseFolderTag = "buildBaseFolder";
		const std::string menuTitleTag = "menuTitle";
		const std::string menuCommentsTag = "menuComments";
		const std::string commentTag = "comment";
		const std::string deleteOrigCommentsTag = "deleteControlsComments";

		// EX Characters
		const std::string characterListTag = "characterList";
		const std::string baseCharListTag = "baseListVersion";
		const std::string characterTag = "character";
		const std::string slotIDTag = "slotID";

		// Code Settings
		const std::string codeSettingsTag = "codeSettings";

		// EX Rosters
		const std::string roseterDeclsTag = "rosterChanger";
		const std::string rosterTag = "roster";

		// Themes
		const std::string themeDeclsTag = "themeChanger";
		const std::string themeTag = "menuTheme";
		const std::string themeFileTag = "themeFile";
		const std::string prefixTag = "replacementPrefix";

		// Dash Attack Item Grab
		const std::string dashAttackItemGrabTag = "dashAttackItemGrab";

		// Colors
		const std::string slotColorDeclsTag = "slotColorChanger";

		// Jumpsquat Override
		const std::string jumpsquatOverrideTag = "jumpsquatModifier";
	}

	void fixIndentationOfChildNodes(pugi::xml_node& targetNode)
	{
		// Indentation string is gonna be for the children of the current node, so we include a tab to start.
		std::string indentationString = "\n\t";
		// Then for each level up we can go through the parents of this node...
		for (pugi::xml_node_iterator currNode = targetNode.parent(); !currNode->parent().empty(); currNode = currNode->parent())
		{
			// ... add an additional tab!
			indentationString += "\t";
		}

		// Lastly, to apply the fixed indentation string, iterate through every child node...
		for (auto currChild : targetNode.children())
		{
			// ... and if we run into a pcData node...
			if (currChild.type() == pugi::xml_node_type::node_pcdata)
			{
				// ... grab a copy of it's string.
				std::string pcStr = currChild.value();
				// Then, iterate backwards through the string to find the first non-space char...
				auto charItr = pcStr.rbegin();
				for (charItr; charItr != pcStr.rend(); charItr++)
				{
					if (!std::isspace(*charItr)) break;
				}
				// ... and if we wind up finding one...
				if (charItr != pcStr.rend())
				{
					// ... then chop off any whitespace characters, and instead append our indentation string.
					pcStr = std::string(pcStr.begin(), charItr.base()) + indentationString;
				}
				// Otherwise...
				else
				{
					// ... simply overwrite the string with our indentation.
					pcStr = indentationString;
				}
				// And write the result out to the pcData node!
				currChild.set_value(pcStr.c_str());
			}
		}
	}

	bool addOrApplyDebugOptionInNode(pugi::xml_node& debugOptionsNode, const std::string& debugSettingString, bool& destinationBool)
	{
		bool result = 0;

		if (debugOptionsNode)
		{
			pugi::xml_node childNode = debugOptionsNode.child(debugSettingString.c_str());
			if (!childNode)
			{
				result = 1;
				debugOptionsNode.append_child(debugSettingString.c_str()).append_attribute(configXMLConstants::enabledTag.c_str()).set_value(destinationBool);
			}
			else
			{
				destinationBool = childNode.attribute(configXMLConstants::enabledTag.c_str()).as_bool(0);
			}
		}

		return result;
	}
	bool handleDebugOptions(pugi::xml_node& documentRoot)
	{
		bool result = 0;

		pugi::xml_node debugOptionsNode = documentRoot.child(configXMLConstants::_debugOptionsTag.c_str());
		if (debugOptionsNode)
		{
			result |= addOrApplyDebugOptionInNode(debugOptionsNode, configXMLConstants::_asmDictTag, CONFIG_OUTPUT_ASM_INSTRUCTION_DICTIONARY);
			result |= addOrApplyDebugOptionInNode(debugOptionsNode, configXMLConstants::_implicitOptimizationsTag, CONFIG_ALLOW_IMPLICIT_OPTIMIZATIONS);
			result |= addOrApplyDebugOptionInNode(debugOptionsNode, configXMLConstants::_allowBLAFuncCalls, CONFIG_ALLOW_BLA_FUNCTION_CALLS);
			result |= addOrApplyDebugOptionInNode(debugOptionsNode, configXMLConstants::_deleteASMTxtTag, CONFIG_DELETE_ASM_TXT_FILE);
			result |= addOrApplyDebugOptionInNode(debugOptionsNode, configXMLConstants::_disableDisassemmblerTag, CONFIG_DISABLE_ASM_DISASSEMBLY);
			result |= addOrApplyDebugOptionInNode(debugOptionsNode, configXMLConstants::_enableASMHexCommentsTag, CONFIG_ENABLE_ASM_HEX_COMMENTS);
		}
		if (result)
		{
			fixIndentationOfChildNodes(debugOptionsNode);
		}

		return result;
	}

	// Returns a string to be used as padding to replace the newline and indentation from replaced plaintext nodes. 
	std::string getNodeIndentationPCDataReplacement(const pugi::xml_node_iterator& nodeItr)
	{
		// Indentation is for the children of the current node, so we include a tab to start.
		std::string result = "\n\t";
		// Then for each level up we can go through the parents of this node...
		for (pugi::xml_node_iterator currNode = nodeItr->parent(); !currNode->parent().empty(); currNode = currNode->parent())
		{
			// ... add an additional tab!
			result += "\t";
		}
		return result;
	}

	// EX Character Handling
	// Adds any collected entries to the destination vector, and returns the number of entries collected in this call.
	std::size_t collectEXCharactersFromPlaintext(std::istream& streamIn, std::vector<std::pair<std::string, u16>>& destinationVector)
	{
		std::size_t originalCount = destinationVector.size();

		if (streamIn.good())
		{
			std::string currentLine = "";
			std::string manipStr = "";
			while (std::getline(streamIn, currentLine))
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
						// Insert new entry into list.
						destinationVector.push_back(std::make_pair(characterNameIn, characterSlotIDIn));
					}
				}
			}
		}
		
		return destinationVector.size() - originalCount;
	}
	std::vector<std::pair<std::string, u16>> collectEXCharactersFromXML(pugi::xml_node_iterator& characterDeclNodeItr, bool& collectedPlaintextEntry)
	{
		std::vector<std::pair<std::string, u16>> result{};

		// Collect any entries contained in the node.
		for (pugi::xml_node_iterator characterItr = characterDeclNodeItr->begin(); characterItr != characterDeclNodeItr->end(); characterItr++)
		{
			// If we're looking at a proper character node, we can just parse its XML contents as normal.
			if (characterItr->name() == configXMLConstants::characterTag)
			{
				std::pair<std::string, u16> tempPair("", USHRT_MAX);
				tempPair.first = characterItr->attribute(configXMLConstants::nameTag.c_str()).as_string("");
				tempPair.second = characterItr->attribute(configXMLConstants::slotIDTag.c_str()).as_int(USHRT_MAX);

				if (!tempPair.first.empty() && (tempPair.second != USHRT_MAX))
				{
					result.push_back(tempPair);
				}
			}
			// The only other thing we care about are plaintext nodes, since those *could* contain old-school entries.
			else if (characterItr->type() == pugi::node_pcdata)
			{
				// For these, populate a stringstream with the node's text, 
				// and attempt to pull any entries from the text. This'll return the number of nodes, so if it's greater than 0...
				if (collectEXCharactersFromPlaintext(std::stringstream(characterItr->value()), result) > 0)
				{
					// ... then flag that we've collected a plaintext entry...
					collectedPlaintextEntry = 1;
					// ... and delete the plaintext entries!
					characterItr->set_value("");
					// Note: this removes those plaintext entries from the tree, they'll be re-generated later so that they use proper XML syntax!
				}
			}
		}

		return result;
	}
	void addCollectedEXCharactersToMenuLists(const std::vector<std::pair<std::string, u16>>& nameIDPairs, lava::outputSplitter& logOutput)
	{
		// If there are entries to include:
		if (nameIDPairs.size())
		{
			// Builds a map from the predefined character and character ID lists.
			// Doing it this way ensures that paired values stay together, and handles sorting automatically when we insert new entries.
			std::map<std::string, u16> zippedIDMap{};
			zipVectorsToMap(CHARACTER_LIST, CHARACTER_ID_LIST, zippedIDMap);
			std::set<u16> uniqueIDList{};
			uniqueIDList.insert(CHARACTER_ID_LIST.cbegin(), CHARACTER_ID_LIST.cend());

			for (int i = 0; i < nameIDPairs.size(); i++)
			{
				const std::pair<std::string, u16>* currPair = &nameIDPairs[i];
				
				// If this entry has an invalid ID...
				if (currPair->second == SHRT_MAX)
				{
					// ... report the error...
					logOutput.write("[ERROR] Invalid Slot ID specified! The character \"" + currPair->first + "\" will not be added to the Code Menu!\n",
						ULONG_MAX, lava::outputSplitter::sOS_CERR);
					// ... and skip to next entry!
					continue;
				}

				const std::map<std::string, u16>::const_iterator zipEndItr = zippedIDMap.cend();
				std::map<std::string, u16>::iterator entryWithSameName = zippedIDMap.find(currPair->first);
				std::map<std::string, u16>::iterator entryWithSameID = zippedIDMap.end();
				// If the incoming ID isn't unique, then populate entryWithSameID!
				if (uniqueIDList.find(currPair->second) != uniqueIDList.end())
				{
					// Find the entry already using that ID...
					for (auto zipMapItr = zippedIDMap.begin(); zipMapItr != zipEndItr; zipMapItr++)
					{
						if (zipMapItr->second == currPair->second)
						{
							// ... and write its iterator to entryWithSameID!
							entryWithSameID = zipMapItr;
							break;
						}
					}
				}

				// If both values are unique in this map...
				if ((entryWithSameName == zipEndItr) && (entryWithSameID == zipEndItr))
				{
					// ... we can simply add the entry to the map, and the ID into the set!
					auto insertItr = zippedIDMap.insert(*currPair);
					uniqueIDList.insert(insertItr.first->second);
					// Then report the addition!
					logOutput << "[ADDED] \"" << insertItr.first->first << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(insertItr.first->second, 2) << ")\n";
				}
				// Otherwise, if only name is unique (so we need to change an existing entry's name)...
				else if (entryWithSameName == zipEndItr)
				{
					// ... then we can record the old name, erase the old entry from the map...
					std::string oldName = entryWithSameID->first;
					zippedIDMap.erase(entryWithSameID);
					// ... and insert our new entry!
					auto insertItr = zippedIDMap.insert(*currPair);
					logOutput << "[CHANGED] \"" << oldName << "\" (Slot ID: 0x" << lava::numToHexStringWithPadding(insertItr.first->second, 2) << "): ";
					logOutput << "Name = \"" << insertItr.first->first << "\"\n";
				}
				// Otherwise, if only ID is unique (so we need to change an existing entry's ID)...
				else if (entryWithSameID == zipEndItr)
				{
					// ... then we can record the old ID, erase the old ID from the set...
					u16 oldID = entryWithSameName->second;
					uniqueIDList.erase(entryWithSameName->second);
					// ... change the ID of the existing entry...
					entryWithSameName->second = currPair->second;
					// ... and add the new ID to the set!
					uniqueIDList.insert(entryWithSameName->second);
					logOutput << "[CHANGED] \"" << entryWithSameName->first << "\" (Slot ID: 0x" << lava::numToHexStringWithPadding(oldID, 2) << "): ";
					logOutput << "Slot ID = 0x" << lava::numToHexStringWithPadding(entryWithSameName->second, 2) << "\n";
				}
				// Otherwise, if neither are unique...
				else
				{
					// ... then we can't do the addition at all! Report the error:
					logOutput << "[ERROR] Unable to process entry \"" <<
						currPair->first << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(currPair->second, 2) << ")!\n";
					logOutput << "\tName is used by: \"" <<
						entryWithSameName->first << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(entryWithSameName->second, 2) << ")!\n";
					logOutput << "\tSlot ID is used by: \"" <<
						entryWithSameID->first << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(entryWithSameID->second, 2) << ")!\n";
				}
			}

			// Write the newly edited list back into the list vectors
			CHARACTER_LIST.clear();
			CHARACTER_ID_LIST.clear();
			unzipMapToVectors(zippedIDMap, CHARACTER_LIST, CHARACTER_ID_LIST);
		}
		else
		{
			logOutput << "[WARNING] EX Character Declaration block parsed, but no valid entries were found!\n";
		}
	}
	void regenEXCharacterDeclsInXML(pugi::xml_node_iterator& characterDeclNodeItr, const std::vector<std::pair<std::string, u16>>& nameIDPairs)
	{
		// Remove all the proper character nodes from the XML...
		while (characterDeclNodeItr->remove_child(configXMLConstants::characterTag.c_str())) {}

		// And for each nameID pair...
		for (std::size_t i = 0; i < nameIDPairs.size(); i++)
		{
			// ... generate a proper character node for it.
			pugi::xml_node tempCharNode = characterDeclNodeItr->append_child(configXMLConstants::characterTag.c_str());
			tempCharNode.append_attribute(configXMLConstants::nameTag.c_str()) = nameIDPairs[i].first.c_str();
			tempCharNode.append_attribute(configXMLConstants::slotIDTag.c_str()) = ("0x" + lava::numToHexStringWithPadding(nameIDPairs[i].second, 2)).c_str();
		}
	}

	// EX Roster Handling
	// Adds any collected entries to the destination vector, and returns the number of entries collected in this call.
	std::size_t collectEXRostersFromPlaintext(std::istream& streamIn, std::vector<std::pair<std::string, std::string>>& destinationVector)
	{
		std::size_t originalCount = destinationVector.size();

		if (streamIn.good())
		{
			std::string currentLine = "";
			std::string manipStr = "";
			while (std::getline(streamIn, currentLine))
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
						// Store roster name portion of string
						std::string rosterNameIn = manipStr.substr(0, delimLoc);
						// Store roster filename portion of string
						std::string fileNameIn = manipStr.substr(delimLoc + 1, std::string::npos);
						// Insert new entry into list.
						destinationVector.push_back(std::make_pair(rosterNameIn, fileNameIn));
					}
				}
			}
		}

		return destinationVector.size() - originalCount;
	}
	std::vector<std::pair<std::string, std::string>> collectEXRostersFromXML(const pugi::xml_node_iterator& rosterDeclNodeItr, bool& collectedPlaintextEntry)
	{
		std::vector<std::pair<std::string, std::string>> result{};

		// Collect proper node entries.
		for (pugi::xml_node_iterator rosterItr = rosterDeclNodeItr->begin(); rosterItr != rosterDeclNodeItr->end(); rosterItr++)
		{
			// If we're looking at a proper roster node, we can just parse its XML contents as normal.
			if (rosterItr->name() == configXMLConstants::rosterTag)
			{
				std::pair<std::string, std::string> tempPair("", "");
				tempPair.first = rosterItr->attribute(configXMLConstants::nameTag.c_str()).as_string("");
				tempPair.second = rosterItr->attribute(configXMLConstants::filenameTag.c_str()).as_string("");

				if (!tempPair.first.empty() && !tempPair.second.empty())
				{
					result.push_back(tempPair);
				}
			}
			// The only other thing we care about are plaintext nodes, since those *could* contain old-school entries.
			else if (rosterItr->type() == pugi::node_pcdata)
			{
				// For these, populate a stringstream with the node's text, 
				// and attempt to pull any entries from the text. This'll return the number of nodes, so if it's greater than 0...
				if (collectEXRostersFromPlaintext(std::stringstream(rosterItr->value()), result) > 0)
				{
					// ... then flag that we've collected a plaintext entry...
					collectedPlaintextEntry = 1;
					// ... and delete the plaintext entries!
					rosterItr->set_value("");
					// Note: this removes those plaintext entries from the tree, they'll be re-generated later so that they use proper XML syntax!
				}
			}
		}

		return result;
	}
	void addCollectedEXRostersToMenuLists(const std::vector<std::pair<std::string, std::string>>& tempRosterList, lava::outputSplitter& logOutput)
	{
		// If we actually retrieved any valid rosters from the file, process them!
		if (!tempRosterList.empty())
		{
			// For each newly collected roster...
			for (int i = 0; i < tempRosterList.size(); i++)
			{
				const std::pair<std::string, std::string>* currPair = &tempRosterList[i];

				// ... check to see if a roster of the same name already exists in our lists.
				auto itr = std::find(ROSTER_LIST.begin(), ROSTER_LIST.end(), currPair->first);

				// If one by that name doesn't already exist...
				if (itr == ROSTER_LIST.end())
				{
					// Add it to our list...
					ROSTER_LIST.push_back(currPair->first);
					ROSTER_FILENAME_LIST.push_back(currPair->second);
					// ... and announce that a roster has been successfully collected.
					logOutput << "[ADDED]";
				}
				// Otherwise, if one by that name *does* already exist...
				else
				{
					// ... overwrite the roster currently associated with that name...
					ROSTER_FILENAME_LIST[itr - ROSTER_LIST.begin()] = currPair->second;
					// ... and announce that the roster has been changed.
					logOutput << "[CHANGED]";
				}
				logOutput << "\"" << currPair->first << "\" (Filename: " << currPair->second << ")\n";
			}
		}
		// Otherwise, note that nothing was found.
		else
		{
			logOutput << "[WARNING] Roster Declaration block parsed, but no valid entries were found!\n";
		}
	}
	void regenRosterDeclsInXML(pugi::xml_node_iterator& rosterDeclNodeItr, const std::vector<std::pair<std::string, std::string>>& tempRosterList)
	{
		// Remove all the proper roster nodes from the XML.
		while (rosterDeclNodeItr->remove_child(configXMLConstants::rosterTag.c_str())) {}

		// And for each roster string pair...
		for (std::size_t i = 0; i < tempRosterList.size(); i++)
		{
			// ... generate a proper roster node for it.
			pugi::xml_node tempCharNode = rosterDeclNodeItr->append_child(configXMLConstants::rosterTag.c_str());
			tempCharNode.append_attribute(configXMLConstants::nameTag.c_str()) = tempRosterList[i].first.c_str();
			tempCharNode.append_attribute(configXMLConstants::filenameTag.c_str()) = tempRosterList[i].second.c_str();
		}
	}

	// Theme Handling
	std::vector<menuTheme> collectThemesFromXML(const pugi::xml_node_iterator& themeDeclNodeItr)
	{
		std::vector<menuTheme> result{};

		for (pugi::xml_node_iterator themeItr = themeDeclNodeItr->begin(); themeItr != themeDeclNodeItr->end(); themeItr++)
		{
			if (themeItr->name() == configXMLConstants::themeTag)
			{
				menuTheme tempTheme;
				tempTheme.name = themeItr->attribute(configXMLConstants::nameTag.c_str()).as_string(tempTheme.name);
				// If the entry has no name, skip to next node.
				if (tempTheme.name.empty()) continue;

				for (pugi::xml_node_iterator themeFileItr = themeItr->begin(); themeFileItr != themeItr->end(); themeFileItr++)
				{
					if (themeFileItr->name() == configXMLConstants::themeFileTag)
					{
						std::size_t filenameIndex = SIZE_MAX;

						for (pugi::xml_attribute_iterator themeFileAttrItr = themeFileItr->attributes_begin(); themeFileAttrItr != themeFileItr->attributes_end(); themeFileAttrItr++)
						{
							if (themeFileAttrItr->name() == configXMLConstants::nameTag)
							{
								auto filenameItr = std::find(themeConstants::filenames.begin(), themeConstants::filenames.end(), themeFileAttrItr->as_string());
								if (filenameItr != themeConstants::filenames.end())
								{
									filenameIndex = filenameItr - themeConstants::filenames.begin();
								}
							}
							else if (filenameIndex != SIZE_MAX && themeFileAttrItr->name() == configXMLConstants::prefixTag)
							{
								tempTheme.prefixes[filenameIndex] = themeFileAttrItr->as_string().substr(0, themeConstants::prefixLength);
								THEME_FILE_GOT_UNIQUE_PREFIX[filenameIndex] |= themeConstants::filenames[filenameIndex].find(tempTheme.prefixes[filenameIndex]) != 0x00;
							}
						}
					}
				}
				result.push_back(tempTheme);
			}
		}

		return result;
	}
	void addCollectedThemesToMenuLists(const std::vector<menuTheme>& tempThemeList, lava::outputSplitter& logOutput)
	{
		// If we actually retrieved any valid themes from the file, process them!
		if (!tempThemeList.empty())
		{
			// For each newly collected theme...
			for (int i = 0; i < tempThemeList.size(); i++)
			{
				const menuTheme* currTheme = &tempThemeList[i];

				// ... check to see if a theme of the same name already exists in our map.
				auto itr = std::find(THEME_LIST.begin(), THEME_LIST.end(), currTheme->name);

				// If one by that name doesn't already exist...
				if (itr == THEME_LIST.end())
				{
					// Add it to our list...
					THEME_LIST.push_back(currTheme->name);
					THEME_SPEC_LIST.push_back(*currTheme);
					// ... and announce that a theme has been successfully collected.
					logOutput << "[ADDED]";
				}
				// Otherwise, if a theme by that name *does* already exist...
				else
				{
					// ... overwrite the theme currently associated with that name...
					THEME_SPEC_LIST[itr - THEME_LIST.begin()] = *currTheme;
					// ... and announce that a theme has been changed.
					logOutput << "[CHANGED]";
				}
				// Describe the processed theme.
				logOutput << " \"" << currTheme->name << "\"\n";
			}
		}
		// Otherwise, note that nothing was found.
		else
		{
			logOutput << "[WARNING] Theme Declaration block parsed, but no valid entries were found!\n";
		}
	}

	// Generic Code Handling
	// Reads in the requested code version from the from the incoming node, and (if it's less than the mode count passed in) stores it in the storage var.
	// Returns the applied mode value, or UCHAR_MAX (0xFF) if either no mode was specified or the requested value was invalid.
	unsigned char setCodeModeFromXML(const pugi::xml_node_iterator& codeNodeItr, unsigned char& modeStorageVariable, unsigned char modeCount, lava::outputSplitter& logOutput)
	{
		unsigned char result = UCHAR_MAX;

		// Determine the result value:
		// If a mode attribute exists for the code's node...
		pugi::xml_attribute modeAttrObj = codeNodeItr->attribute(configXMLConstants::codeModeTag.c_str());
		if (modeAttrObj)
		{
			// ... try to interpret its value as a number.
			result = lava::stringToNum<unsigned char>(modeAttrObj.as_string(), 0, UCHAR_MAX, 0);
			// If the result is valid...
			if (result < modeCount)
			{
				// ... note the success and store the value!
				modeStorageVariable = result;
				logOutput << "[SUCCESS] Mode is now: " << +modeStorageVariable << "!\n";
			}
			// Otherwise...
			else
			{
				// ... leave the value as is and note the invalid specification.
				logOutput << "[WARNING] Invalid mode specified (" << +result << ")! Mode is still: " << +modeStorageVariable << "!\n";
			}
		}
		// Otherwise...
		else
		{
			// ... leave the value as is and note the lack of a mode specification.
			logOutput << "[WARNING] No mode specified! Mode is still: " << +modeStorageVariable << "!\n";
		}

		return result;
	}
	// Reads in whether or not the code is enabled or disabled by the incoming node, and sets the storage var accordingly.
	// Always returns the end value of the storage variable.
	bool setCodeEnabledFromXML(const pugi::xml_node_iterator& codeNodeItr, bool& enabledStorageVariable, lava::outputSplitter& logOutput)
	{
		// If an "enabled" attribute exists for the code's node...
		pugi::xml_attribute modeAttrObj = codeNodeItr->attribute(configXMLConstants::enabledTag.c_str());
		if (modeAttrObj)
		{
			// ... interpret its value as a bool, and note the success.
			enabledStorageVariable = modeAttrObj.as_bool(enabledStorageVariable);
			logOutput << "[SUCCESS] Option is now " << ((enabledStorageVariable) ? "included" : "excluded") << "!\n";
		}
		// Otherwise...
		else
		{
			// ... we leave the value as is and note the lack of a mode specification.
			logOutput << "[WARNING] No value specified! Option is still " << ((enabledStorageVariable) ? "included" : "excluded") << "!\n";
		}

		return enabledStorageVariable;
	}

	// Core Functions
	bool declNodeIsEnabled(const pugi::xml_node_iterator& declNodeItr)
	{
		// Note, we default to enabled if no value is specified!
		bool result = 1;

		// If an "enabled" attribute is present in this node...
		pugi::xml_attribute enabledAttr = declNodeItr->attribute(configXMLConstants::enabledTag.c_str());
		if (enabledAttr)
		{
			// ... attempt to read it as a bool, store its value in the result.
			result = enabledAttr.as_bool(1);
		}

		return result;
	}
	bool parseAndApplyConfigXML(std::string configFilePath, lava::outputSplitter& logOutput)
	{
		// If the config file doesn't exist, we can exit.
		if (!std::filesystem::is_regular_file(configFilePath)) return 0;

		// Otherwise, create our configDoc object and attempt to populate it from the file...
		pugi::xml_document configDoc;
		pugi::xml_parse_result res = configDoc.load_file(configFilePath.c_str(), pugi::parse_default | pugi::parse_comments | pugi::parse_ws_pcdata);
		// ... and return 0 if the document doesn't successfully parse.
		if (res.status != pugi::xml_parse_status::status_ok) return 0;

		// Attempt to grab the config root node from the document...
		pugi::xml_node configRoot = configDoc.child(configXMLConstants::menuConfigTag.c_str());
		// ... and return 0 if we fail to find one.
		if (!configRoot) return 0;

		bool doRebuild = 0;

		// Check for the debug options node, apply any settings found in it, and add any settings not present.
		doRebuild = handleDebugOptions(configRoot);

		// If we've successfully reached our config root node, we can begin iterating through its child nodes!
		for (pugi::xml_node_iterator declNodeItr = configRoot.begin(); declNodeItr != configRoot.end(); declNodeItr++)
		{
			// If a menu properties block exists...
			if (declNodeItr->name() == configXMLConstants::menuPropsTag)
			{
				// ... apply the contained values.
				logOutput << "\nApplying Menu Properties from \"" << configFilePath << "\"...\n";

				// Used for pulling values from potential nodes!
				pugi::xml_node foundNode{};
				std::string bufferStr("");

				// Check if a base folder declaration node exists in the properties block.
				foundNode = declNodeItr->child(configXMLConstants::baseFolderTag.c_str());
				// If one was actually found, and its value isn't empty...
				bufferStr = foundNode.attribute(configXMLConstants::nameTag.c_str()).as_string("");
				if (!bufferStr.empty())
				{
					// ... attempt to use the retrieved value to set MAIN_FOLDER.
					logOutput << "Build Base Folder argument detected, applying settings...\n";
					if (setMAIN_FOLDER(bufferStr))
					{
						logOutput << "[SUCCESS] Build Base Folder is now \"" << MAIN_FOLDER << "\"!\n";
					}
					else
					{
						logOutput << "[WARNING] Invalid Folder specified, using default value (\"" << MAIN_FOLDER << "\")!\n";
					}
				}

				// Check if a menu title declaration node exists in the properties block.
				foundNode = declNodeItr->child(configXMLConstants::menuTitleTag.c_str());
				// If one was actually found, and its value isn't empty...
				bufferStr = foundNode.attribute(configXMLConstants::textTag.c_str()).as_string("");
				if (!bufferStr.empty())
				{
					// ... use the value to overwrite MENU_NAME.
					MENU_NAME = bufferStr;
					CUSTOM_NAME_SUPPLIED = 1;
					logOutput << "Menu Title argument detected, applying settings...\n";
					logOutput << "[SUCCESS] Menu title is now \"" << MENU_NAME << "\"!\n";
				}

				// Check if a menu comments declaration block exists in the properties block.
				foundNode = declNodeItr->child(configXMLConstants::menuCommentsTag.c_str());
				// If the node is present, and it actually has comment declarations in it...
				if (foundNode)
				{
					logOutput << "Menu header comments block detected! Parsing contents...\n";
					CONFIG_DELETE_CONTROLS_COMMENTS = foundNode.attribute(configXMLConstants::deleteOrigCommentsTag.c_str()).as_bool(0);
					if (CONFIG_DELETE_CONTROLS_COMMENTS)
					{
						logOutput << "[NOTE] Menu Controls comment block will be omitted!\n";
					}
					if (foundNode.child(configXMLConstants::commentTag.c_str()))
					{
						logOutput << "Header Comments detected! Collecting comments...\n";
						for (pugi::xml_node_iterator commentItr = foundNode.begin(); commentItr != foundNode.end(); commentItr++)
						{
							if (commentItr->name() == configXMLConstants::commentTag)
							{
								pugi::xml_attribute tempAttr = commentItr->attribute(configXMLConstants::textTag.c_str());
								if (tempAttr)
								{
									CONFIG_INCOMING_COMMENTS.push_back(tempAttr.as_string());
									logOutput << "\t[ADDED] \"" << tempAttr.as_string() << "\"\n";
								}
							}
						}
					}
				}
			}

			// If looking at the character list block...
			if (declNodeItr->name() == configXMLConstants::characterListTag)
			{
				logOutput << "\nParsing Character List block from \"" << configFilePath << "\"...\n";

				// Check if a character list version argument was given...
				unsigned long requestedCharListVersion =
					lava::stringToNum(declNodeItr->attribute(configXMLConstants::baseCharListTag.c_str()).as_string(), 0, ULONG_MAX);
				if (requestedCharListVersion != ULONG_MAX)
				{
					// ... and attempt to apply it if so.
					logOutput << "Base Character List argument detected, applying settings...\n";
					if (applyCharacterListVersion(requestedCharListVersion))
					{
						logOutput << "[SUCCESS] Base Character list changed to \"" << characterListVersionNames[characterListVersion] << "\"!\n";
					}
					else
					{
						logOutput << "[WARNING] Invalid list requested! Using \"" << characterListVersionNames[characterListVersion] << "\" list instead!\n";
					}
					logOutput << "\n";
				}

				// If we're set to additionally collect externally defined EX Characters...
				if (COLLECT_EXTERNAL_EX_CHARACTERS)
				{
					// ... collect character entries from the XML, then add them to the menu.
					logOutput << "Adding EX Characters to Character List...\n";
					bool collectedPlaintextEntry = 0;
					// Populate our entry list...
					std::vector<std::pair<std::string, u16>> nameIDPairs = collectEXCharactersFromXML(declNodeItr, collectedPlaintextEntry);
					// ... and if that list doesn't end up empty...
					if (!nameIDPairs.empty())
					{
						// ... then we'll add those to the menu lists proper.
						addCollectedEXCharactersToMenuLists(nameIDPairs, logOutput);
						// Additionally, if we pulled any entries from plaintext...
						if (collectedPlaintextEntry)
						{
							// ... then we need to promote them to properly formatted entries, so regen the entries...
							regenEXCharacterDeclsInXML(declNodeItr, nameIDPairs);
							// ... and fix the indentation on them!
							fixIndentationOfChildNodes(*declNodeItr);
							// Additionally, flag that we need to do a rebuild later.
							doRebuild = 1;
						}
					}
				}

				//Do final character list summary.
				logOutput << "\nFinal Character List (Base List = \"" << characterListVersionNames[characterListVersion] << "\")\n";
				for (std::size_t i = 0; i < CHARACTER_LIST.size(); i++)
				{
					logOutput << "\t\"" << CHARACTER_LIST[i] << "\" (Slot ID = 0x" << lava::numToHexStringWithPadding(CHARACTER_ID_LIST[i], 2) << ")\n";
				}
			}

			// If we've reached the code configuration block...
			if (declNodeItr->name() == configXMLConstants::codeSettingsTag)
			{
				for (pugi::xml_node_iterator codeNodeItr = declNodeItr->begin(); codeNodeItr != declNodeItr->end(); codeNodeItr++)
				{
					// If we're looking at the EX Rosters block...
					if (COLLECT_EXTERNAL_ROSTERS && declNodeIsEnabled(codeNodeItr) && codeNodeItr->name() == configXMLConstants::roseterDeclsTag)
					{
						logOutput << "\nAdding Rosters to Code Menu from \"" << configFilePath << "\"...\n";

						// ... collect roster entries from the XML, then add them to the menu.
						bool collectedPlaintextEntry = 0;
						// Populate our entry list...
						std::vector<std::pair<std::string, std::string>> tempRosterList = collectEXRostersFromXML(codeNodeItr, collectedPlaintextEntry);
						// ... and if that list doesn't end up empty...
						if (!tempRosterList.empty())
						{
							// ... then we'll add those to the menu lists proper.
							addCollectedEXRostersToMenuLists(tempRosterList, logOutput);
							// Additionally, if we pulled any entries from plaintext...
							if (collectedPlaintextEntry)
							{
								// ... then we need to promote them to properly formatted entries, so regen the entries...
								regenRosterDeclsInXML(codeNodeItr, tempRosterList);
								// ... and fix the indentation on them!
								fixIndentationOfChildNodes(*codeNodeItr);
								// Additionally, flag that we need to do a rebuild later.
								doRebuild = 1;
							}
						}

						//Do final roster list summary.
						logOutput << "\nFinal Roster List:\n";
						for (std::size_t i = 0; i < ROSTER_LIST.size(); i++)
						{
							logOutput << "\t\"" << ROSTER_LIST[i] << "\" (Filename: " << ROSTER_FILENAME_LIST[i] << ")\n";
						}
					}
					// If we're looking at the Themes block...
					else if (COLLECT_EXTERNAL_THEMES && declNodeIsEnabled(codeNodeItr) && codeNodeItr->name() == configXMLConstants::themeDeclsTag)
					{
						logOutput << "\nAdding Themes to Code Menu from \"" << configFilePath << "\"...\n";

						// ... collect theme entries from the XML, then add them to the menu.
						std::vector<menuTheme> tempThemeList = collectThemesFromXML(codeNodeItr);
						addCollectedThemesToMenuLists(tempThemeList, logOutput);

						// Do final theme list summary.
						logOutput << "\nFinal Theme List:\n";
						for (std::size_t i = 0; i < THEME_LIST.size(); i++)
						{
							logOutput << "\t\"" << THEME_LIST[i] << "\", Replacement Prefixes Are:\n";
							for (std::size_t u = 0; u < THEME_SPEC_LIST[i].prefixes.size(); u++)
							{
								logOutput << "\t\t\"" << themeConstants::filenames[u] << "\": \"" << THEME_SPEC_LIST[i].prefixes[u] << "\"\n";
							}
						}
					}
					// If we're looking at the Item Grab block...
					else if (codeNodeItr->name() == configXMLConstants::dashAttackItemGrabTag)
					{
						logOutput << "\nSetting Dash Attack Item Grab Toggle status...\n";
						// ... handle enabling/disabling it.
						setCodeEnabledFromXML(codeNodeItr, CONFIG_DASH_ATTACK_ITEM_GRAB_ENABLED, logOutput);
					}
					// If we're looking at the Jumpsquat Override block...
					else if (codeNodeItr->name() == configXMLConstants::jumpsquatOverrideTag)
					{
						logOutput << "\nSetting Jumpsquat Modifier status...\n";
						// ... handle enabling/disabling it.
						setCodeEnabledFromXML(codeNodeItr, CONFIG_JUMPSQUAT_OVERRIDE_ENABLED, logOutput);
					}
					// If we're looking at the Slot Colors block...
					else if (codeNodeItr->name() == configXMLConstants::slotColorDeclsTag)
					{
						logOutput << "\nSetting Player Slot Color Changer mode... \n";
						// ... handle setting its mode. 
						setCodeModeFromXML(codeNodeItr, CONFIG_BACKPLATE_COLOR_MODE, backplateColorConstants::playerSlotColorLevel::pSCL__COUNT, logOutput);
					}
				}
			}
		}

		if (doRebuild)
		{
			configDoc.save_file(configFilePath.c_str());
		}
		
		return 1;
	}
}