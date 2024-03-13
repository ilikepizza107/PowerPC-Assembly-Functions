#ifndef XML_PROCESSING_H_V1
#define XML_PROCESSING_H_V1

#include "stdafx.h"
#include "Code Menu.h"
#include "pugi/pugixml.hpp"
#include "_lavaBytes.h"
#include <conio.h>
#include <filesystem>

class Page;
class Line;

namespace xml
{
	// ==================== Menu Config Parsing and Constants =====================

	// Parses the specified Configuration XML and applies any changes it requests to the menu.
	bool parseAndApplyConfigXML(std::string configFilePath, lava::outputSplitter& logOutput);

	// ============================================================================

	// =======================  Addon Parsing and Constants =======================

	// Enumeration of the xml-definable line fields.
	enum lineFields
	{
		lf_ValDefault = 0,
		lf_ValMin,
		lf_ValMax,
		lf_Speed,
		lf_Options,
		lc__COUNT
	};
	typedef std::array<bool, lineFields::lc__COUNT> fieldChangeArr;

	// A bundle which holds the details for an addon line!
	struct addonLine
	{
		// Fullname used for display and logging purposes.
		std::string lineName = "";
		// Shortname used for linking.
		lava::shortNameType shortName = "";
		int INDEX = INT_MAX;
		std::shared_ptr<Line> linePtr = nullptr;
		fieldChangeArr populated{};

	private:
		void buildIntegerLine(const pugi::xml_node& sourceNode);
		void buildFloatLine(const pugi::xml_node& sourceNode);
		void buildToggleLine(const pugi::xml_node& sourceNode);
		void buildSelectionLine(const pugi::xml_node& sourceNode);
		void buildCommentLine(const pugi::xml_node& sourceNode);

	public:
		addonLine() {};
		addonLine(const pugi::xml_node& sourceNode);
		bool populate(const pugi::xml_node& sourceNode);
	};
	struct addonPage
	{
		// Fullname used for display and logging purposes.
		std::string pageName = "";
		// Shortname used for linking.
		lava::shortNameType shortName = "";
		// Collected lines in order.
		std::vector<std::shared_ptr<addonLine>> lines{};
		// Map of line shortnames to their structs.
		std::map<lava::shortNameType, std::shared_ptr<addonLine>> lineMap{};

		bool populate(const pugi::xml_node& sourceNode);
	};
	struct addon
	{
		// Fullname used for display and logging purposes.
		std::string addonName = "";
		// Shortname used for linking.
		lava::shortNameType shortName = "";
		// Input Folder.
		std::filesystem::path inputDirPath = "";
		// Maps page shortnames to their structs!
		std::map<lava::shortNameType, addonPage> pages{};

		addon() {};
		addon(std::string inputDirPathIn);
		bool populate(std::string inputDirPathIn);
	};
	extern std::vector<addon> collectedAddons;

	void applyCollectedAddons();

	// ============================================================================


	// ==================== Menu Options Parsing and Constants ====================

	// Incoming Configuration XML Variables (See "Code Menu.cpp" for defaults, and "_AdditionalCode.cpp" for relevant Config Parsing code!)
	extern std::vector<std::string> CONFIG_INCOMING_COMMENTS;
	extern bool CONFIG_DELETE_CONTROLS_COMMENTS;
	extern bool CONFIG_PSCC_ENABLED;
	extern bool CONFIG_DASH_ATTACK_ITEM_GRAB_ENABLED;
	extern bool CONFIG_JUMPSQUAT_OVERRIDE_ENABLED;

	void applyLineSettingsFromMenuOptionsTree(Page& mainPageIn, const pugi::xml_document& xmlDocumentIn, lava::outputSplitter& logOutput);
	bool applyLineSettingsFromMenuOptionsTree(Page& mainPageIn, std::string xmlPathIn, lava::outputSplitter& logOutput);
	bool buildMenuOptionsTreeFromMenu(Page& mainPageIn, std::string xmlPathOut);
}

#endif

