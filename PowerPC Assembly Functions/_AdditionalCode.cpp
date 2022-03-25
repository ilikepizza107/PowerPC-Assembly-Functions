#include "stdafx.h"
#include "_AdditionalCode.h"

namespace lava
{
	int stringToNum(const std::string& stringIn, bool allowNeg, int defaultVal)
	{
		int result = defaultVal;
		std::string manipStr = stringIn;
		int base = (manipStr.find("0x") == 0) ? 16 : 10;
		char* res = nullptr;
		result = std::strtoul(manipStr.c_str(), &res, base);
		if (res != (manipStr.c_str() + manipStr.size()))
		{
			result = defaultVal;
		}
		if (result < 0 && !allowNeg)
		{
			result = defaultVal;
		}
		return result;
	}
	std::string numToHexStringWithPadding(std::size_t numIn, std::size_t paddingLength)
	{
		std::stringstream convBuff;
		convBuff << std::hex << numIn;
		std::string result = convBuff.str();
		for (int i = 0; i < result.size(); i++)
		{
			result[i] = std::toupper(result[i]);
		}
		if (result.size() < paddingLength)
		{
			result = std::string(paddingLength - result.size(), '0') + result;
		}
		return result;
	}
	std::string numToDecStringWithPadding(std::size_t numIn, std::size_t paddingLength)
	{
		std::string result = std::to_string(numIn);
		if (result.size() < paddingLength)
		{
			result = std::string(paddingLength - result.size(), '0') + result;
		}
		return result;
	}

	bool fileExists(std::string filepathIn)
	{
		std::ifstream result(filepathIn);
		return result.is_open();
	}
	bool folderExists(std::string folderpathIn)
	{
		// Record result
		bool result = 0;
		// Ensure that the provided folder path ends with a slash
		if (folderpathIn.back() != '/' && folderpathIn.back() != '\\')
		{
			folderpathIn.push_back('/');
		}
		// Define location of test file to be opened within specified path
		std::string testFileLoc = folderpathIn + "__test__";
		// Attempt to open file at the testFileLoc.
		std::ofstream test(testFileLoc, std::ios_base::out | std::ios_base::ate);
		// If the file opens...
		if (test.is_open())
		{
			// ... we know the directory exists, so our result is true.
			result = 1;
			// We opened at the end of the file (using ::ate), so if our position is 0, the file is empty.
			if (test.tellp() == 0)
			{
				// We assume that the file was created by this function and didn't previously exist, so we delete it.
				test.close();
				remove(testFileLoc.c_str());
			}
			// If the position isn't 0 then we know for sure the file wasn't created by this function, so we'll leave it.
			else
			{
				test.close();
			}
		}
		return result;
	}
	bool copyFile(std::string sourceFile, std::string targetFile, bool overwriteExistingFile)
	{
		// Record result
		bool result = 0;
		if (sourceFile != targetFile)
		{
			// Initialize in and out streams
			std::ifstream sourceFileStream;
			std::ofstream targetFileStream;
			// Open and test input stream
			sourceFileStream.open(sourceFile, std::ios_base::in | std::ios_base::binary);
			if (sourceFileStream.is_open())
			{
				if (overwriteExistingFile || !fileExists(targetFile))
				{
					// If successful, open and test output stream
					targetFileStream.open(targetFile, std::ios_base::out | std::ios_base::binary);
					if (targetFileStream.is_open())
					{
						// If both streams are open and valid, copy over the file's contents and record the success in result
						targetFileStream << sourceFileStream.rdbuf();
						result = 1;
					}
					targetFileStream.close();
				}
			}
			sourceFileStream.close();
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
	bool offerCopyOverAndBackup(std::string fileToCopy, std::string fileToOverwrite)
	{
		bool backupSucceeded = 0;
		bool copySucceeded = 0;

		if (lava::fileExists(fileToCopy) && lava::fileExists(fileToOverwrite))
		{
			std::cout << "Detected \"" << fileToOverwrite << "\".\n" <<
				"Would you like to copy \"" << fileToCopy << "\" over it? " <<
				"A backup will be made of the existing file.\n";
			std::cout << "[Press 'Y' for Yes, 'N' for No]\n";
			if (yesNoDecision('y', 'n'))
			{
				std::cout << "Making backup... ";
				if (lava::backupFile(fileToOverwrite, ".bak", 1))
				{
					backupSucceeded = 1;
					std::cout << "Success!\nCopying over \"" << fileToCopy << "\"... ";
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
					std::cerr << "Backup failed! Please ensure that " << fileToOverwrite << ".bak is able to be written to!\n";
				}
			}
			else
			{
				std::cout << "Skipping copy.\n";
			}
			std::cout << "\n";
		}

		return backupSucceeded && copySucceeded;
	}
	bool handleAutoGCTRMProcess(std::ostream& logOutput)
	{
		bool result = 0;

		if (lava::fileExists(GCTRMExePath) && lava::fileExists(mainGCTTextFile) && lava::fileExists(boostGCTTextFile))
		{
			std::cout << "Detected \"" << GCTRMExePath << "\".\nWould you like to build \"" << mainGCTFile << "\" and \"" << boostGCTFile << "\"? ";
			std::cout << "Backups will be made of the existing files.\n";
			std::cout << "[Press 'Y' for Yes, 'N' for No]\n";
			if (yesNoDecision('y', 'n'))
			{
				std::cout << "Backing up files... ";
				if (lava::backupFile(mainGCTFile, ".bak", 1) && lava::backupFile(boostGCTFile, ".bak", 1))
				{
					std::cout << "Success! Running GCTRM.\n";
					result = 1;
					std::string commandFull = "\"" + GCTRMCommandBase + "\"" + mainGCTTextFile + "\"\"";
					std::cout << "\n" << commandFull << "\n";
					system(commandFull.c_str());
					commandFull = "\"" + GCTRMCommandBase + "\"" + boostGCTTextFile + "\"\"";
					std::cout << "\n" << commandFull << "\n";
					system(commandFull.c_str());
					logOutput << "Note: Backed up and rebuilt \"" << mainGCTFile << "\".\n";
					logOutput << "Note: Backed up and rebuilt \"" << boostGCTFile << "\".\n";
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
}